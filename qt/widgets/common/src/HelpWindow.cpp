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

#include <QUrl>
#include <QWidget>
#include <boost/lexical_cast.hpp>

namespace MantidQt::API {
namespace {
/// static logger
Mantid::Kernel::Logger g_log("HelpWindow");

} // namespace

using std::string;

void HelpWindow::showPage(const std::string &url) { showPage(QString(url.c_str())); }

void HelpWindow::showPage(const QString &url) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showPage(url);
  } else {
    g_log.error() << "Failed to launch help for page " << url.toStdString() << "\n";
  }
}

void HelpWindow::showPage(const QUrl &url) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showPage(url);
  } else {
    g_log.error() << "Failed to launch help for page " << url.toString().toStdString() << "\n";
  }
}

void HelpWindow::showAlgorithm(const std::string &name, const int version) {
  showAlgorithm(QString(name.c_str()), version);
}

void HelpWindow::showAlgorithm(const QString &name, const int version) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showAlgorithm(name, version);
  } else {
    // Open online help
    QString baseUrl = "https://docs.mantidproject.org/algorithms/";
    QString url = baseUrl + name + "-v" + QString::number(version) + ".html";
    MantidDesktopServices::openUrl(QUrl(url));
    g_log.debug("Opening online help page:\n" + url.toStdString());
  }
}

void HelpWindow::showConcept(const std::string &name) { showConcept(QString(name.c_str())); }

void HelpWindow::showConcept(const QString &name) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showConcept(name);
  } else {
    g_log.error() << "Failed to launch help for concept " << name.toStdString() << "\n";
  }
}

void HelpWindow::showFitFunction(const std::string &name) {
  InterfaceManager interfaceManager;
  MantidHelpInterface *gui = interfaceManager.createHelpWindow();
  if (gui) {
    gui->showFitFunction(name);
  } else {
    g_log.error() << "Failed to launch help for fit function " << name << "\n";
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
    // Open online help
    QString baseUrl = "https://docs.mantidproject.org/interfaces/";
    if (!area.toStdString().empty()) {
      baseUrl += area + "/";
    }
    QString url = baseUrl + name + ".html";
    MantidDesktopServices::openUrl(QUrl(url));
    g_log.debug("Opening online help page:\n" + url.toStdString());
  }
}

} // namespace MantidQt::API
