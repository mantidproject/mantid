//=============================
// MantidApplciation definitions
//==============================
#include "MantidApplication.h"
#include "MantidQtAPI/MantidDialog.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/UsageService.h"

#include <QMessageBox>
#include <QPushButton>

#include <iostream>

namespace {
/// static logger
Mantid::Kernel::Logger g_log("MantidApplication");
}

/// Constructor
MantidApplication::MantidApplication(int &argc, char **argv)
    : QApplication(argc, argv) {
  try {
    Mantid::Kernel::UsageService::Instance().setApplication("mantidplot");
  } catch (std::runtime_error &rexc) {
    g_log.error() << "Failed to initialize the Mantid usage service. This "
                     "is probably a sign that this Mantid is not fully or "
                     "correctly set up. "
                     "Error details: " +
                         std::string(rexc.what()) << '\n';
  }
}

bool MantidApplication::notify(QObject *receiver, QEvent *event) {
  bool res = false;
  try {
    res = QApplication::notify(receiver, event);
  } catch (std::exception &e) {
    if (MantidQt::API::MantidDialog::handle(receiver, e))
      return true; // stops event propagation

    // Restore possible override cursor
    while (QApplication::overrideCursor()) {
      QApplication::restoreOverrideCursor();
    }

    g_log.fatal() << "Unexpected exception: " << e.what() << "\n";
    QMessageBox ask;
    QAbstractButton *terminateButton =
        ask.addButton(tr("Terminate"), QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    ask.setText("Sorry, MantidPlot has caught an unexpected exception:\n\n" +
                QString::fromStdString(e.what()) +
                "\n\nWould you like to terminate MantidPlot or try to continue "
                "working?\nIf you choose to continue it is advisable to save "
                "your data and restart the application.");
    ask.setIcon(QMessageBox::Critical);
    ask.exec();
    if (ask.clickedButton() == terminateButton) {
      g_log.fatal("Terminated by user.");
      quit();
    } else
      g_log.fatal("Continue working.");
  } catch (...) {

    g_log.fatal() << "Unknown exception\n";
    QMessageBox ask;
    QAbstractButton *terminateButton =
        ask.addButton(tr("Terminate"), QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    ask.setText("Sorry, MantidPlot has caught an unexpected exception"
                "\n\nWould you like to terminate MantidPlot or try to continue "
                "working?\nIf you choose to continue it is advisable to save "
                "your data and restart the application.");
    ask.setIcon(QMessageBox::Critical);
    ask.exec();
    if (ask.clickedButton() == terminateButton) {
      g_log.fatal("Terminated by user.");
      quit();
    } else
      g_log.fatal("Continue working.");
  }
  return res;
}

//=============================================================================
