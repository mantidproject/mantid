#include "SimpleGuiApp.h"
#include "MpMainWindow.h"

#include <QMessageBox>

int main(int argc, char** argv)
{
  SimpleGuiApp app(argc, argv);
  try
  {
    mpMainWindow window;
    window.show();
    return app.exec();
  }
  catch(std::exception& e)
  {
    QMessageBox::critical(0, "VatesSimpleGui - Error",
                          QString("An unhandled exception has been caught. "\
                                  "VatesSimpleGui will have to close. "\
                                  "Details:\n\n")+e.what());
  }
  catch(...)
  {
    QMessageBox::critical(0, "VatesSimpleGui - Error",
                          "An unhandled exception has been caught. "\
                          "VatesSimpleGui will have to close.");
  }
}
