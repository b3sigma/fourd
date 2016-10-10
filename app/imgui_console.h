#pragma once

namespace fd {

  // this separation is already tiresome
  // odds of ever actually breaking this out to some other thing?
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
      static void AddConsoleCommand(const char* command);
  };


}; //namespace fd 