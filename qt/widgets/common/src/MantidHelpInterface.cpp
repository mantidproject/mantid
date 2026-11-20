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

MantidHelpInterface::MantidHelpInterface() = default;

MantidHelpInterface::~MantidHelpInterface() = default;

void MantidHelpInterface::showPage([[maybe_unused]] const std::string &url) {}

void MantidHelpInterface::showPage([[maybe_unused]] const QString &url) {}

void MantidHelpInterface::showPage([[maybe_unused]] const QUrl &url) {}

void MantidHelpInterface::showConcept([[maybe_unused]] const std::string &page) {}

void MantidHelpInterface::showConcept([[maybe_unused]] const QString &page) {}

void MantidHelpInterface::showAlgorithm([[maybe_unused]] const std::string &name, [[maybe_unused]] const int version) {}

void MantidHelpInterface::showAlgorithm([[maybe_unused]] const QString &name, [[maybe_unused]] const int version) {}

void MantidHelpInterface::showFitFunction([[maybe_unused]] const std::string &name) {}

void MantidHelpInterface::showFitFunction([[maybe_unused]] const QString &name) {}

void MantidHelpInterface::showCustomInterface([[maybe_unused]] const std::string &name,
                                              [[maybe_unused]] const std::string &area,
                                              [[maybe_unused]] const std::string &section) {}

void MantidHelpInterface::showCustomInterface([[maybe_unused]] const QString &name,
                                              [[maybe_unused]] const QString &area,
                                              [[maybe_unused]] const QString &section) {}

void MantidHelpInterface::shutdown() {}
