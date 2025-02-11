// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/PythonHelpWindow.h"
#include "MantidKernel/RegistrationHelper.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/PythonHelpBridge.h"
#include <QString>
#include <string>

namespace MantidQt {
namespace MantidWidgets {

#ifndef DOCS_QTHELP
REGISTER_HELPWINDOW(PythonHelpWindow)
#endif

PythonHelpWindow::PythonHelpWindow() : MantidQt::API::MantidHelpInterface() {}

void PythonHelpWindow::showPage(const std::string &url) {
  PythonHelpBridge bridge;
  bridge.showHelpPage(url.empty() ? "index.html" : url);
}

void PythonHelpWindow::showPage(const QString &url) {
  PythonHelpBridge bridge;
  std::string page = url.isEmpty() ? "index.html" : url.toStdString();
  bridge.showHelpPage(page);
}

void PythonHelpWindow::showPage(const QUrl &url) {
  PythonHelpBridge bridge;
  std::string page = url.isEmpty() ? "index.html" : url.toString().toStdString();
  bridge.showHelpPage(page);
}

void PythonHelpWindow::showAlgorithm(const std::string &name, const int version) {
  QString page = "algorithms/" + QString::fromStdString(name);
  if (!name.empty() && version > 0)
    page += "-v" + QString::number(version) + ".html";
  else if (!name.empty())
    page += ".html";
  else
    page = "algorithms/index.html";
  showPage(page);
}

void PythonHelpWindow::showAlgorithm(const QString &name, const int version) {
  std::string n = name.toStdString();
  showAlgorithm(n, version);
}

void PythonHelpWindow::showConcept(const std::string &name) {
  QString page = name.empty() ? "concepts/index.html" : "concepts/" + QString::fromStdString(name) + ".html";
  showPage(page);
}

void PythonHelpWindow::showConcept(const QString &name) {
  std::string n = name.toStdString();
  showConcept(n);
}

void PythonHelpWindow::showFitFunction(const std::string &name) {
  QString page = name.empty() ? "fitting/fitfunctions/index.html"
                              : "fitting/fitfunctions/" + QString::fromStdString(name) + ".html";
  showPage(page);
}

void PythonHelpWindow::showFitFunction(const QString &name) {
  std::string n = name.toStdString();
  showFitFunction(n);
}

void PythonHelpWindow::showCustomInterface(const std::string &name, const std::string &area,
                                           const std::string &section) {
  QString areaPath = area.empty() ? "" : QString::fromStdString(area) + "/";
  QString page = "interfaces/" + areaPath + (name.empty() ? "index.html" : QString::fromStdString(name) + ".html");
  if (!section.empty())
    page += "#" + QString::fromStdString(section);
  showPage(page);
}

void PythonHelpWindow::showCustomInterface(const QString &name, const QString &area, const QString &section) {
  std::string n = name.toStdString();
  std::string a = area.toStdString();
  std::string s = section.toStdString();
  showCustomInterface(n, a, s);
}

void PythonHelpWindow::shutdown() {
  // no-op for python help window
}

} // namespace MantidWidgets
} // namespace MantidQt
