#include "imgui_tweak.h"

#include "../imgui/imgui.h"

#include "../common/tweak.h"
#include "../common/tweak_registrar.h"

namespace fd {

static void ShowHelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip(desc);
}

void TweakWindow::RenderWindow(float frameTime, const Vec2f& offset) {
  static bool opened = true;

  ImVec2 startPos(100.0f, 300.0f);
  startPos.x += offset.x();
  startPos.y += offset.y();
  ImGui::SetNextWindowPos(startPos);
  if (!ImGui::Begin("tweaks", &opened, ImVec2(0, 0), 0.3f, 0
                    // | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                    )) {
    ImGui::End();
    return;
  }

  ImGui::Text("Tweaks availabe:                                         ");

  for(auto tweakEntry : *TweakRegistrar::_nameToTweak) {
    TweakVariable* tweak = tweakEntry.second;
    ImGui::DragFloat(tweakEntry.first.c_str(), tweak->AsFloatPtr(), 0.005f);
    ImGui::SameLine(); 
    ShowHelpMarker("Click and drag to edit value.\nHold SHIFT/ALT for faster/slower edit.\nDouble-click or CTRL+click to input text.");
  }

  ImGui::End();
}

} //namespace fd
