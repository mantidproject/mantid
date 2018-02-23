//=============================
// MantidApplciation definitions
//==============================
#include "MantidApplication.h"
#include "MantidQtWidgets/Common/MantidDialog.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ErrorReporter.h"
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

void MantidApplication::errorHandling(bool continueWork, int share,
                                      QString name, QString email) {
  if (share == 0) {
    Mantid::Kernel::ErrorReporter errorReporter(
        "mantidplot", Mantid::Kernel::UsageService::Instance().getUpTime(), "",
        true, name.toStdString(), email.toStdString());
    errorReporter.sendErrorReport();
  } else if (share == 1) {
    Mantid::Kernel::ErrorReporter errorReporter(
        "mantidplot", Mantid::Kernel::UsageService::Instance().getUpTime(), "",
        false, name.toStdString(), email.toStdString());
    errorReporter.sendErrorReport();
  }

  if (!continueWork) {
    g_log.fatal("Terminated by user.");
    quit();
  }
  g_log.fatal("Continue working.");
}

bool MantidApplication::notify(QObject *receiver, QEvent *event) {
  bool res = false;
  bool error = false;
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
    error = true;
  } catch (...) {

    g_log.fatal() << "Unknown exception\n";
    error = true;
  }

  if (error) {
    QString pythonCode("from ErrorReporter.errorreport import "
                       "CrashReportPage\npage = "
                       "CrashReportPage()\npage.show()");

    emit runAsPythonScript(pythonCode);
  }

  return res;
}

//=============================================================================
