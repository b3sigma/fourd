#pragma once

namespace fd {

  class ConsoleInterface {
    public:
      static bool s_consoleActive;

      typedef void (* OnCommandCallback)(const char* command);

      static bool Init(OnCommandCallback callback);
      static bool Render();
      static void Shutdown();

      static void SetFocus(bool shouldBeFocused);
      static void DropNextKeyInput();
      static void DropPreviousKeyInput();
  };


}; //namespace fd 