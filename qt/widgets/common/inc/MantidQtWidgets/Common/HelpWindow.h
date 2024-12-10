// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>
#include <QWidget>
#include <string>

// Forward declarations
class QUrl;

namespace MantidQt {
namespace API {

/**
 * HelpWindow provides a centralized interface for displaying
 * help pages, algorithm details, and concepts. It supports both
 * the C++ HelpWindow and launching a Python-based HelpWindow.
 */
class EXPORT_OPT_MANTIDQT_COMMON HelpWindow : public QWidget {
  Q_OBJECT

public:
  explicit HelpWindow(QWidget *parent = nullptr);

  /// Show a page by URL
  static void showPage(const std::string &url = std::string());
  static void showPage(const QString &url);
  static void showPage(const QUrl &url);

  /// Show help for an algorithm
  static void showAlgorithm(const std::string &name = std::string(), const int version = -1);
  static void showAlgorithm(const QString &name, const int version = -1);

  /// Show help for a concept
  static void showConcept(const std::string &name = std::string());
  static void showConcept(const QString &name);

  /// Show help for a fit function
  static void showFitFunction(const std::string &name = std::string());
  static void showFitFunction(const QString &name);

  /// Show help for a custom interface
  static void showCustomInterface(const QString &name, const QString &area = QString(),
                                  const QString &section = QString());
  static void showCustomInterface(const std::string &name = std::string(), const std::string &area = std::string(),
                                  const std::string &section = std::string());

  /// Launch the Python-based Help Window
  static void launchPythonHelpWindow();

  /// Launch a specific Python help page
  void launchPythonHelp();

  /// Get the current page URL
  QString currentPageUrl() const;

private:
  QString currentUrl; // Stores the current URL
};

} // namespace API
} // namespace MantidQt
