// Most of this code is ganked from imgui impl example
// which is probably the point of it?

#include <memory>

#include <GL/glew.h>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include "imgui_wrapper.h"

#if defined(_MSC_VER) && defined(WIN32)
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include "shader.h"

namespace fd {

GLFWwindow* ImGuiWrapper::s_glfwWindow = NULL;
GLFWmousebuttonfun ImGuiWrapper::s_chainedMouseButtonCallback = NULL;
GLFWkeyfun ImGuiWrapper::s_chainedKeyCallback = NULL;
Shader* ImGuiWrapper::s_UIRender = NULL;
Shader* ImGuiWrapper::s_UIRenderVR = NULL;

static GLuint       g_FontTexture = 0;
static int          g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int          g_AttribLocationPosition = 0, g_AttribLocationUV = 0, g_AttribLocationColor = 0;
static size_t       g_VboSize = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0;


void ImGuiMouseButtonCallback(
    GLFWwindow* window, int button, int action, int mods) {
  ImGuiIO& io = ImGui::GetIO();
  // hmm, the imgui example used an intermediate structure instead of writing
  // to io.MouseDown directly. Let's try it this way and see if it works.
  if(button >= 0 && button <= (sizeof(io.MouseDown) / sizeof(io.MouseDown[0]))) {
    io.MouseDown[button] = (action == GLFW_PRESS);
  }

  if(ImGuiWrapper::s_chainedMouseButtonCallback) {
    ImGuiWrapper::s_chainedMouseButtonCallback(window, button, action, mods);
  }
}

void ImGuiScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
  ImGuiIO& io = ImGui::GetIO();
  io.MouseWheel += (float)yOffset;
}

void ImGuiCharCallback(GLFWwindow* window, unsigned int c) {
  ImGuiIO& io = ImGui::GetIO();
  if (c > 0 && c < 0x10000)
    io.AddInputCharacter((unsigned short)c);
}

void ImGuiKeyCallback(
    GLFWwindow* window, int key, int scancode, int action, int mods) {
  ImGuiIO& io = ImGui::GetIO();
  if (action == GLFW_PRESS)
    io.KeysDown[key] = true;
  if (action == GLFW_RELEASE)
    io.KeysDown[key] = false;

  (void)mods; // Modifiers are not reliable across systems
  io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
  io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
  io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];

  if(ImGuiWrapper::s_chainedKeyCallback) {
    ImGuiWrapper::s_chainedKeyCallback(window, key, scancode, action, mods);
  }
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void ImGuiRenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count) {
  if (cmd_lists_count == 0)
    return;

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
  GLint last_program, last_texture;
  glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

  bool lastBlendEnabled = (glIsEnabled(GL_BLEND) == GL_TRUE);
  GLint lastBlendEquation;
  glGetIntegerv(GL_BLEND_EQUATION, &lastBlendEquation);
  GLint lastBlendFuncSrc;
  GLint lastBlendFuncDst;
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &lastBlendFuncSrc);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &lastBlendFuncDst);
  bool lastFaceCullEnabled = (glIsEnabled(GL_CULL_FACE) == GL_TRUE);
  bool lastDepthTestEnabled = (glIsEnabled(GL_DEPTH_TEST) == GL_TRUE);
  bool lastScissorTestEnabled = (glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE);

  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glActiveTexture(GL_TEXTURE0);

  // Setup orthographic projection matrix
  const float width = ImGui::GetIO().DisplaySize.x;
  const float height = ImGui::GetIO().DisplaySize.y;
  const float ortho_projection[4][4] =
  {
    { 2.0f / width, 0.0f, 0.0f, 0.0f },
    { 0.0f, 2.0f / -height, 0.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f, 0.0f },
    { -1.0f, 1.0f, 0.0f, 1.0f },
  };
  glUseProgram(ImGuiWrapper::s_UIRender->getProgramId());
  glUniform1i(g_AttribLocationTex, 0);
  glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

  // Grow our buffer according to what we need
  size_t total_vtx_count = 0;
  for (int n = 0; n < cmd_lists_count; n++)
    total_vtx_count += cmd_lists[n]->vtx_buffer.size();
  glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
  size_t needed_vtx_size = total_vtx_count * sizeof(ImDrawVert);
  if (g_VboSize < needed_vtx_size)
  {
    g_VboSize = needed_vtx_size + 5000 * sizeof(ImDrawVert);  // Grow buffer
    glBufferData(GL_ARRAY_BUFFER, g_VboSize, NULL, GL_STREAM_DRAW);
  }

  // Copy and convert all vertices into a single contiguous buffer
  unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  if (!buffer_data)
    return;
  for (int n = 0; n < cmd_lists_count; n++)
  {
    const ImDrawList* cmd_list = cmd_lists[n];
    memcpy(buffer_data, &cmd_list->vtx_buffer[0], cmd_list->vtx_buffer.size() * sizeof(ImDrawVert));
    buffer_data += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(g_VaoHandle);

  int cmd_offset = 0;
  for (int n = 0; n < cmd_lists_count; n++)
  {
    const ImDrawList* cmd_list = cmd_lists[n];
    int vtx_offset = cmd_offset;
    const ImDrawCmd* pcmd_end = cmd_list->commands.end();
    for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
    {
      if (pcmd->user_callback)
      {
        pcmd->user_callback(cmd_list, pcmd);
      }
      else
      {
        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);
        glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
        glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
      }
      vtx_offset += pcmd->vtx_count;
    }
    cmd_offset = vtx_offset;
  }

  // Restore modified state
  glBindVertexArray(0);
  glUseProgram(last_program);
  glBindTexture(GL_TEXTURE_2D, last_texture);

  if(!lastBlendEnabled) {
    glDisable(GL_BLEND);
  }
  glBlendEquation(lastBlendEquation);
  glBlendFunc(lastBlendFuncSrc, lastBlendFuncDst);
  if(lastFaceCullEnabled) {
    glEnable(GL_CULL_FACE);
  }
  if(lastDepthTestEnabled) {
    glEnable(GL_DEPTH_TEST);
  }
  if(!lastScissorTestEnabled) {
    glDisable(GL_SCISSOR_TEST);
  }
}

const char* ImGuiGetClipboardText() {
  return glfwGetClipboardString(ImGuiWrapper::s_glfwWindow);
}

static void ImGuiSetClipboardText(const char* text) {
  glfwSetClipboardString(ImGuiWrapper::s_glfwWindow, text);
}

  
bool ImGuiWrapper::Init(GLFWwindow* glfwWindow,
    GLFWkeyfun keyCallback, GLFWmousebuttonfun mouseButtonCallback) {
  s_glfwWindow = glfwWindow;

  ImGuiIO& io = ImGui::GetIO();
  // dunno, just ripped these out of the imgui example
  io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
  io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
  io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
  io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
  io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
  io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

  io.RenderDrawListsFn = ImGuiRenderDrawLists;
  io.SetClipboardTextFn = ImGuiSetClipboardText;
  io.GetClipboardTextFn = ImGuiGetClipboardText;

#ifdef WIN32
  io.ImeWindowHandle = glfwGetWin32Window(s_glfwWindow);
#endif

  s_chainedMouseButtonCallback = mouseButtonCallback;
  glfwSetMouseButtonCallback(s_glfwWindow, ImGuiMouseButtonCallback);

  glfwSetCharCallback(s_glfwWindow, ImGuiCharCallback);
  glfwSetScrollCallback(s_glfwWindow, ImGuiScrollCallback);

  s_chainedKeyCallback = keyCallback;
  glfwSetKeyCallback(s_glfwWindow, ImGuiKeyCallback);

  InitOpenGL();

  return true;
}

bool ImGuiWrapper::InitOpenGL() {
  ImGuiIO& io = ImGui::GetIO();

  std::unique_ptr<Shader> shader(new Shader());
  if(!shader->LoadFromFile(
      "Imgui", "data\\uivImgui.glsl", "data\\uifImgui.glsl")) {
    return false;
  }
  s_UIRender = shader.release();

  std::unique_ptr<Shader> shaderVR(new Shader());
  shaderVR->AddDynamicMeshCommonSubShaders();
  if(!shaderVR->LoadFromFile(
      "ImguiVR", "data\\uivImguiVR.glsl", "data\\uifImguiVR.glsl")) {
    return false;
  }
  s_UIRenderVR = shaderVR.release();

  // wholesale ganked from imgui example to start
  g_AttribLocationTex = glGetUniformLocation(s_UIRender->getProgramId(), "texDiffuse0");
  g_AttribLocationProjMtx = glGetUniformLocation(s_UIRender->getProgramId(), "projectionMatrix");
  g_AttribLocationPosition = glGetAttribLocation(s_UIRender->getProgramId(), "vertPosition");
  g_AttribLocationUV = glGetAttribLocation(s_UIRender->getProgramId(), "vertCoord");
  g_AttribLocationColor = glGetAttribLocation(s_UIRender->getProgramId(), "vertColor");

  glGenBuffers(1, &g_VboHandle);

  glGenVertexArrays(1, &g_VaoHandle);
  glBindVertexArray(g_VaoHandle);
  glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
  glEnableVertexAttribArray(g_AttribLocationPosition);
  glEnableVertexAttribArray(g_AttribLocationUV);
  glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
  glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
  glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
  glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

  glGenTextures(1, &g_FontTexture);
  glBindTexture(GL_TEXTURE_2D, g_FontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

  // Cleanup (don't clear the input data if you want to append new fonts later)
  io.Fonts->ClearInputData();
  io.Fonts->ClearTexData();

  return false;
}

void ImGuiWrapper::Shutdown() {
  if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
  if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
  g_VaoHandle = 0;
  g_VboHandle = 0;

  delete s_UIRender;
  s_UIRender = NULL;
  delete s_UIRenderVR;
  s_UIRenderVR = NULL;

  if (g_FontTexture)
  {
    glDeleteTextures(1, &g_FontTexture);
    ImGui::GetIO().Fonts->TexID = 0;
    g_FontTexture = 0;
  }
  ImGui::Shutdown();

}

void ImGuiWrapper::NewFrame(float deltaTime) {
  ImGuiIO& io = ImGui::GetIO();

  int w, h;
  int display_w, display_h;
  glfwGetWindowSize(s_glfwWindow, &w, &h);
  glfwGetFramebufferSize(s_glfwWindow, &display_w, &display_h);
  io.DisplaySize = ImVec2((float)display_w, (float)display_h);

  io.DeltaTime = deltaTime;

  // Setup inputs
  // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
  if (glfwGetWindowAttrib(s_glfwWindow, GLFW_FOCUSED))
  {
    double mouse_x, mouse_y;
    glfwGetCursorPos(s_glfwWindow, &mouse_x, &mouse_y);
    mouse_x *= (float)display_w / w;                        // Convert mouse coordinates to pixels
    mouse_y *= (float)display_h / h;
    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
  }
  else
  {
    io.MousePos = ImVec2(-1, -1);
  }

  glfwSetInputMode(s_glfwWindow, GLFW_CURSOR,
      io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

  ImGui::NewFrame();

  // this might need to be broken out into another var
  io.MouseWheel = 0.0f;
}

void ImGuiWrapper::Render() {
  
  static bool opened = true;
  if(opened) {
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Another Window", &opened);
    ImGui::Text("Hello");
    ImGui::End();
  }

  static bool showTestWindow = true;
  if(showTestWindow) {
    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
    ImGui::ShowTestWindow(&showTestWindow);
  }

  ImGui::Render();
}
  
}; // namespace fd