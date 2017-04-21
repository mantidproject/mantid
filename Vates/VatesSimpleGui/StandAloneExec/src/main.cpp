#include "SimpleGuiApp.h"
#include "VsgMainWindow.h"

#include <QMessageBox>

int main(int argc, char **argv) {
  SimpleGuiApp app(argc, argv);
  try {
    VsgMainWindow window;
    window.show();
    return app.exec();
  } catch (std::exception &e) {
    QMessageBox::critical(nullptr, "VatesSimpleGui - Error",
                          QString("An unhandled exception has been caught. "
                                  "VatesSimpleGui will have to close. "
                                  "Details:\n\n") +
                              e.what());
  } catch (...) {
    QMessageBox::critical(nullptr, "VatesSimpleGui - Error",
                          "An unhandled exception has been caught. "
                          "VatesSimpleGui will have to close.");
  }
}
