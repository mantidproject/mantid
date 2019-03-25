// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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


namespace {
/// static logger
Mantid::Kernel::Logger g_log("MantidApplication");
} // namespace

/// Constructor
MantidApplication::MantidApplication(int &argc, char **argv)
    : QApplication(argc, argv) {
  try {
    Mantid::Kernel::UsageService::Instance().setApplicationName("mantidplot");
  } catch (std::runtime_error &rexc) {
    g_log.error() << "Failed to initialize the Mantid usage service. This "
                     "is probably a sign that this Mantid is not fully or "
                     "correctly set up. "
                     "Error details: " +
                         std::string(rexc.what())
                  << '\n';
  }
}

void MantidApplication::errorHandling(bool continueWork, int share,
                                      QString name, QString email,
                                      QString textbox) {
  if (share == 0) {
    Mantid::Kernel::ErrorReporter errorReporter(
        "mantidplot", Mantid::Kernel::UsageService::Instance().getUpTime(), "",
        true, name.toStdString(), email.toStdString(), textbox.toStdString());
    errorReporter.sendErrorReport();
  } else if (share == 1) {
    Mantid::Kernel::ErrorReporter errorReporter(
        "mantidplot", Mantid::Kernel::UsageService::Instance().getUpTime(), "",
        false, name.toStdString(), email.toStdString(), textbox.toStdString());
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
  bool known_exception = false;
  std::string unexpected_exception;
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
    known_exception = true;
    unexpected_exception = e.what();
  } catch (...) {

    g_log.fatal() << "Unknown exception\n";
    error = true;
  }

  if (error && Mantid::Kernel::UsageService::Instance().isEnabled()) {
    QString pythonCode(
        "from ErrorReporter.error_report_presenter import "
        "ErrorReporterPresenter"
        "\nfrom ErrorReporter.errorreport import CrashReportPage"
        "\npage = CrashReportPage(show_continue_terminate=True)"
        "\npresenter = ErrorReporterPresenter(page, '', 'mantidplot')"
        "\npresenter.show_view()");

    emit runAsPythonScript(pythonCode);
  } else if (error) {
    QMessageBox ask;
    QAbstractButton *terminateButton =
        ask.addButton(tr("Terminate"), QMessageBox::ActionRole);
    ask.addButton(tr("Continue"), QMessageBox::ActionRole);
    if (known_exception) {
      ask.setText(
          "Sorry, MantidPlot has caught an unexpected exception:\n\n" +
          QString::fromStdString(unexpected_exception) +
          "\n\nWould you like to terminate MantidPlot or try to continue "
          "working?\nIf you choose to continue it is advisable to save "
          "your data and restart the application.");
    } else {
      ask.setText(
          "Sorry, MantidPlot has caught an unknown exception"
          "\n\nWould you like to terminate MantidPlot or try to continue "
          "working?\nIf you choose to continue it is advisable to save "
          "your data and restart the application.");
    }
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
