#include "SimpleGuiApp.h"
#include "MpMainWindow.h"

int main(int argc, char** argv)
{
  SimpleGuiApp app(argc, argv);
  mpMainWindow window;
  window.show();
  return app.exec();
}
