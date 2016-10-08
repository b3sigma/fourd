#pragma once

namespace fd {

  class ConsoleInterface {
    public:

      typedef void (* OnCommandCallback)(const char* command);

      static bool Init(OnCommandCallback callback);
      static bool Render();
      static void Shutdown();
  };


}; //namespace fd 