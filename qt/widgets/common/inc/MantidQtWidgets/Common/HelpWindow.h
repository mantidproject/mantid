// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>
#include <string>

// forward declarations
class QUrl;

namespace MantidQt {
namespace API {

class EXPORT_OPT_MANTIDQT_COMMON HelpWindow {
public:
  static void showPage(const std::string &url = std::string());
  static void showPage(const QString &url);
  static void showPage(const QUrl &url);
  static void showAlgorithm(const std::string &name = std::string(), const int version = -1);
  static void showAlgorithm(const QString &name, const int version = -1);
  static void showConcept(const std::string &name = std::string());
  static void showConcept(const QString &name);
  static void showFitFunction(const std::string &name = std::string());
  static void showCustomInterface(const QString &name, const QString &area = QString(),
                                  const QString &section = QString());
  static void showCustomInterface(const std::string &name = std::string(), const std::string &area = std::string(),
                                  const std::string &section = std::string());
};
} // namespace API
} // namespace MantidQt
