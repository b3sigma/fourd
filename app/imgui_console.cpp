// Pulled from ImGui example ExampleAppConsole
#include <ctype.h>      // toupper, isprint
#include <stdio.h>      // vsnprintf, sscanf

#include <GL/glew.h>
#include "../imgui/imgui.h"
#include <GLFW/glfw3.h>

#include "imgui_console.h"

namespace fd {

#ifndef IM_ARRAYSIZE
#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR)/sizeof(*_ARR)))
#endif 

static int ImStrnicmp(const char* str1, const char* str2, int count)
{
    int d = 0;
    while (count > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; count--; }
    return d;
}

static int ImStricmp(const char* str1, const char* str2)
{
    int d;
    while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; }
    return d;
}

static size_t ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
    buf[buf_size-1] = 0;
    return (w == -1) ? buf_size : (size_t)w;
}

static size_t ImFormatStringV(char* buf, size_t buf_size, const char* fmt, va_list args)
{
    int w = vsnprintf(buf, buf_size, fmt, args);
    buf[buf_size-1] = 0;
    return (w == -1) ? buf_size : (size_t)w;
}
static inline bool  ImCharIsSpace(int c) { return c == ' ' || c == '\t' || c == 0x3000; }

struct Console
{
    char                  InputBuf[256];
    ImVector<char*>       Items;
    bool                  ScrollToBottom;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.size()-1 browsing history.
    ImVec2                StartSize;
    bool                  TakeFocus;
    ConsoleInterface::OnCommandCallback     Callback;
    ImVector<const char*> Commands;

    Console(float sizeX, float sizeY, ConsoleInterface::OnCommandCallback callback,
            bool takeFocus)
        : TakeFocus(takeFocus)
        , StartSize(sizeX, sizeY)
    {
        ClearLog();
        HistoryPos = -1;
        Commands.push_back("#HELP");
        Commands.push_back("#HISTORY");
        Commands.push_back("#CLEAR");
        Commands.push_back("#CLOSE");
        Callback = callback;
    }
    ~Console()
    {
        ClearLog();
        for (size_t i = 0; i < Items.size(); i++) 
            free(History[i]); 
    }

    void    ClearLog()
    {
        for (size_t i = 0; i < Items.size(); i++) 
            free(Items[i]); 
        Items.clear();
        ScrollToBottom = true;
    }

    void    AddLog(const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        ImFormatStringV(buf, IM_ARRAYSIZE(buf), fmt, args);
        va_end(args);
        Items.push_back(strdup(buf));
        ScrollToBottom = true;
    }

    void    Run(const char* title, bool* opened)
    {
        // ImGui::SetNextWindowSize(StartSize, ImGuiSetCond_FirstUseEver);
        ImGui::SetNextWindowSize(StartSize, ImGuiSetCond_Once);
        if (!ImGui::Begin(title, opened))
        {
            ImGui::End();
            return;
        }

        // ImGui::TextWrapped("This example implements a console with basic coloring, completion and history. A more elaborate implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
        ImGui::TextWrapped("Enter '#HELP' for help, press TAB to use text completion.");

        // TODO: display items starting from the bottom

        // if (ImGui::SmallButton("Add Dummy Text")) { AddLog("%d some text", Items.size()); AddLog("some more text"); AddLog("display very important message here!"); } ImGui::SameLine(); 
        // if (ImGui::SmallButton("Add Dummy Error")) AddLog("[error] something went wrong"); ImGui::SameLine(); 
        if (ImGui::SmallButton("Clear")) ClearLog();
        //static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

        ImGui::Separator();


        // Command-line
        if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
        {
            char* input_end = InputBuf+strlen(InputBuf);
            while (input_end > InputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
            if (InputBuf[0])
                ExecCommand(InputBuf);
            strcpy(InputBuf, "");
        }

        if(TakeFocus) {
            // Demonstrate keeping auto focus on the input box
            if (ImGui::IsItemHovered()
                    || (ImGui::IsRootWindowOrAnyChildFocused() 
                        && !ImGui::IsAnyItemActive() 
                        && !ImGui::IsMouseClicked(0))) {
                ImGui::SetKeyboardFocusHere(-1); // Auto focus
            }
        }

        ImGui::Separator();

        static ImGuiTextFilter filter;
        // ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
        // filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        // ImGui::PopStyleVar();
        // ImGui::Separator();

        // Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient. You can seek and display only the lines that are visible - CalcListClipping() is a helper to compute this information.
        // If your items are of variable size you may want to implement code similar to what CalcListClipping() does. Or split your data into fixed height items to allow random-seeking into your list.
        ImGui::BeginChild("ScrollingRegion", ImVec2(0,-ImGui::GetItemsLineHeightWithSpacing()));
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
        for (size_t i = 0; i < Items.size(); i++)
        {
            const char* item = Items[i];
            if (!filter.PassFilter(item))
                continue;
            ImVec4 col(1,1,1,1); // A better implement may store a type per-item. For the sample let's just parse the text.
            if (strstr(item, "[error]")) col = ImVec4(1.0f,0.4f,0.4f,1.0f);
            else if (strncmp(item, "# ", 2) == 0) col = ImVec4(1.0f,0.8f,0.6f,1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(item);
            ImGui::PopStyleColor();
        }
        if (ScrollToBottom)
            ImGui::SetScrollPosHere();
        ScrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
        // ImGui::Separator();


        ImGui::End();
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = (int)History.size()-1; i >= 0; i--)
            if (ImStricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(strdup(command_line));

        // Process command
        if (ImStricmp(command_line, "#CLEAR") == 0)
        {
            ClearLog();
        }
        else if (ImStricmp(command_line, "#HELP") == 0)
        {
            AddLog("Commands:");
            for (size_t i = 0; i < Commands.size(); i++)
                AddLog("- %s", Commands[i]);
        }
        else if (ImStricmp(command_line, "#HISTORY") == 0)
        {
            for (size_t i = History.size() >= 10 ? History.size() - 10 : 0; i < History.size(); i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else if (ImStricmp(command_line, "#CLOSE") == 0)
        {
            AddLog("Yeah someone should write that.");
        }
        else
        {
            // AddLog("Ex-cute: '%s'\n", command_line);
            Callback(command_line);
        }
    }

    static int TextEditCallbackStub(ImGuiTextEditCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
    {
        Console* console = (Console*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiTextEditCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (ImCharIsSpace(c) || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                for (size_t i = 0; i < Commands.size(); i++)
                    if (ImStrnicmp(Commands[i], word_start, (int)(word_end-word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.size() == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", word_end-word_start, word_start);
                }
                else if (candidates.size() == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
                    data->DeleteChars((int)(word_start-data->Buf), (int)(word_end-word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLOSE"
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end-word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (size_t i = 0; i < candidates.size(); i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = (int)(History.size() - 1);
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= (int)History.size())
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    ImFormatString(data->Buf, data->BufSize, "%s", (HistoryPos >= 0) ? History[HistoryPos] : "");
                    data->BufDirty = true;
                    data->CursorPos = data->SelectionStart = data->SelectionEnd = (int)strlen(data->Buf);
                }
            }
        }
        return 0;
    }
};


static Console* g_console = NULL;
//static FILE* g_stdout = NULL; // ugh to capture python printf?

bool ConsoleInterface::Init(OnCommandCallback callback) {
  static bool takeFocus = true;
  g_console = new Console(320.0f, 300.0f, callback, takeFocus);
  return true;
}

bool ConsoleInterface::Render() {
  if(!g_console) return false;

  // TODO: move flag to Console class
  static bool opened = true; //false; 
  g_console->Run("Console", &opened);

  return true;
}

void ConsoleInterface::Shutdown() {
  delete g_console;
  g_console = NULL;
}

} //namespace fd
