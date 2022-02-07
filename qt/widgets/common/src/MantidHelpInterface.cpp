// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include <QString>
#include <QUrl>

using namespace MantidQt::API;
using std::string;

MantidHelpInterface::MantidHelpInterface() {}

MantidHelpInterface::~MantidHelpInterface() {}

void MantidHelpInterface::showPage(const std::string &url) { UNUSED_ARG(url); }

void MantidHelpInterface::showPage(const QString &url) { UNUSED_ARG(url); }

void MantidHelpInterface::showPage(const QUrl &url) { UNUSED_ARG(url); }

void MantidHelpInterface::showWikiPage(const std::string &page) { UNUSED_ARG(page); }

void MantidHelpInterface::showWikiPage(const QString &page) { UNUSED_ARG(page); }

void MantidHelpInterface::showConcept(const std::string &page) { UNUSED_ARG(page); }

void MantidHelpInterface::showConcept(const QString &page) { UNUSED_ARG(page); }

void MantidHelpInterface::showAlgorithm(const std::string &name, const int version) {
  UNUSED_ARG(name);
  UNUSED_ARG(version);
}

void MantidHelpInterface::showAlgorithm(const QString &name, const int version) {
  UNUSED_ARG(name);
  UNUSED_ARG(version);
}

void MantidHelpInterface::showFitFunction(const std::string &name) { UNUSED_ARG(name); }

void MantidHelpInterface::showFitFunction(const QString &name) { UNUSED_ARG(name); }

void MantidHelpInterface::showCustomInterface(const std::string &name, const std::string &area,
                                              const std::string &section) {
  UNUSED_ARG(name);
  UNUSED_ARG(area);
  UNUSED_ARG(section);
}

void MantidHelpInterface::showCustomInterface(const QString &name, const QString &area, const QString &section) {
  UNUSED_ARG(name);
  UNUSED_ARG(area);
  UNUSED_ARG(section);
}

void MantidHelpInterface::shutdown() {}
