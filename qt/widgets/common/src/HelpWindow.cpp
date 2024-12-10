// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"

#include <QPushButton>
#define PY_SSIZE_T_CLEAN
#undef slots
#include <Python.h>

#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <boost/python.hpp>

namespace MantidQt::API {
namespace {
/// Static logger
Mantid::Kernel::Logger g_log("HelpWindow");
} // namespace

using std::string;

void HelpWindow::showPage(const std::string &url) { showPage(QString::fromStdString(url)); }

void HelpWindow::showPage(const QString &url) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showPage(url);
  } else {
    QUrl validUrl = QUrl::fromUserInput(url);
    if (!validUrl.isValid()) {
      g_log.error() << "Invalid URL: " << url.toStdString() << "\n";
      return;
    }
    MantidDesktopServices::openUrl(validUrl);
    g_log.debug() << "Opening URL in default browser: " << validUrl.toString().toStdString() << "\n";
  }
}

void HelpWindow::showPage(const QUrl &url) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showPage(url);
  } else {
    if (!url.isValid()) {
      g_log.error() << "Invalid QUrl: " << url.toString().toStdString() << "\n";
      return;
    }
    MantidDesktopServices::openUrl(url);
    g_log.debug() << "Opening URL in default browser: " << url.toString().toStdString() << "\n";
  }
}

void HelpWindow::showAlgorithm(const std::string &name, const int version) {
  showAlgorithm(QString::fromStdString(name), version);
}

void HelpWindow::showAlgorithm(const QString &name, const int version) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showAlgorithm(name, version);
  } else {
    QString baseUrl = "https://docs.mantidproject.org/algorithms/";
    QString url = baseUrl + name + "-v" + QString::number(version) + ".html";
    QUrl validUrl = QUrl::fromUserInput(url);
    if (!validUrl.isValid()) {
      g_log.error() << "Invalid algorithm URL: " << url.toStdString() << "\n";
      return;
    }
    MantidDesktopServices::openUrl(validUrl);
    g_log.debug() << "Opening algorithm help URL: " << validUrl.toString().toStdString() << "\n";
  }
}

void HelpWindow::showConcept(const std::string &name) { showConcept(QString::fromStdString(name)); }

void HelpWindow::showConcept(const QString &name) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showConcept(name);
  } else {
    QString baseUrl = "https://docs.mantidproject.org/concepts/";
    QString url = baseUrl + name + ".html";
    QUrl validUrl = QUrl::fromUserInput(url);
    if (!validUrl.isValid()) {
      g_log.error() << "Invalid concept URL: " << url.toStdString() << "\n";
      return;
    }
    MantidDesktopServices::openUrl(validUrl);
    g_log.debug() << "Opening concept help URL: " << validUrl.toString().toStdString() << "\n";
  }
}

void HelpWindow::showFitFunction(const std::string &name) { showFitFunction(QString::fromStdString(name)); }

void HelpWindow::showFitFunction(const QString &name) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showFitFunction(name);
  } else {
    QString baseUrl = "https://docs.mantidproject.org/functions/";
    QString url = baseUrl + name + ".html";
    QUrl validUrl = QUrl::fromUserInput(url);
    if (!validUrl.isValid()) {
      g_log.error() << "Invalid function URL: " << url.toStdString() << "\n";
      return;
    }
    MantidDesktopServices::openUrl(validUrl);
    g_log.debug() << "Opening fit function help URL: " << validUrl.toString().toStdString() << "\n";
  }
}

void HelpWindow::showCustomInterface(const std::string &name, const std::string &area, const std::string &section) {
  showCustomInterface(QString::fromStdString(name), QString::fromStdString(area), QString::fromStdString(section));
}

void HelpWindow::showCustomInterface(const QString &name, const QString &area, const QString &section) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showCustomInterface(name, area, section);
  } else {
    QString baseUrl = "https://docs.mantidproject.org/interfaces/";
    if (!area.isEmpty()) {
      baseUrl += area + "/";
    }
    QString url = baseUrl + name + ".html";
    QUrl validUrl = QUrl::fromUserInput(url);
    if (!validUrl.isValid()) {
      g_log.error() << "Invalid custom interface URL: " << url.toStdString() << "\n";
      return;
    }
    MantidDesktopServices::openUrl(validUrl);
    g_log.debug() << "Opening custom interface help URL: " << validUrl.toString().toStdString() << "\n";
  }
}

void HelpWindow::launchPythonHelpWindow() {
  try {
    // Initialize Python environment
    Py_Initialize();
    boost::python::object main = boost::python::import("__main__");
    boost::python::object globals = main.attr("__dict__");

    // Update the Python script to include the correct module path
    const std::string pythonScript = R"(
import sys
sys.path.append('mantid/qt/python/mantidqt/mantidqt/widgets/helpwindow/')
from helpwindowpresenter import HelpWindowPresenter
presenter = HelpWindowPresenter()
presenter.show_help_window()
)";
    boost::python::exec(pythonScript.c_str(), globals, globals);
  } catch (boost::python::error_already_set &) {
    PyErr_Print();
    g_log.error("Error launching Python Help Window.");
  }
}

void HelpWindow::launchPythonHelp() {
  try {
    boost::python::object mainNamespace = boost::python::import("__main__").attr("__dict__");
    boost::python::exec("from helpwindowpresenter import HelpPresenter", mainNamespace);

    std::string currentPage = currentUrl.toStdString();
    boost::python::exec(("presenter = HelpPresenter.instance(); presenter.show_page('" + currentPage + "')").c_str(),
                        mainNamespace);
  } catch (const boost::python::error_already_set &) {
    PyErr_Print();
    g_log.error("Failed to launch Python HelpWindow.");
  }
}

HelpWindow::HelpWindow(QWidget *parent) : QWidget(parent), currentUrl("about:blank") {
  auto *layout = new QVBoxLayout(this);

  QPushButton *pythonHelpButton = new QPushButton("Python Implementation", this);
  connect(pythonHelpButton, &QPushButton::clicked, this, &HelpWindow::launchPythonHelp);
  layout->addWidget(pythonHelpButton);
}

QString HelpWindow::currentPageUrl() const {
  return this->currentUrl; // Provide the current URL
}

} // namespace MantidQt::API
