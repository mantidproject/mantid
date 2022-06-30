// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include <QWidget>
#include <string>

// forward declaration
class QHelpEngine;
class QString;
class QUrl;
class QWidget;
class pqHelpWindow;

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON MantidHelpWindow : public API::MantidHelpInterface {
  Q_OBJECT

public:
  static bool helpWindowExists() { return g_helpWindow != nullptr; }
  static bool doesHelpPageExist(const QUrl &url);

  MantidHelpWindow(QWidget *parent = nullptr, const Qt::WindowFlags &flags = nullptr);
  ~MantidHelpWindow() override;

  void showPage(const std::string &url = std::string()) override;
  void showPage(const QString &url) override;
  void showPage(const QUrl &url) override;
  void showAlgorithm(const std::string &name = std::string(), const int version = -1) override;
  void showAlgorithm(const QString &name, const int version = -1) override;
  void showConcept(const std::string &name) override;
  void showConcept(const QString &name) override;
  void showFitFunction(const std::string &name = std::string()) override;
  void showFitFunction(const QString &name) override;
  void showCustomInterface(const std::string &name = std::string(), const std::string &area = std::string(),
                           const std::string &section = std::string()) override;
  void showCustomInterface(const QString &name, const QString &area = QString(),
                           const QString &section = QString()) override;

  /// Perform any clean up on main window shutdown
  void shutdown() override;

private:
  void showHelp(const QString &url);
  void openWebpage(const QUrl &url);

  static std::string findCollectionFile(const QString &binDirectory);
  static std::string findCacheFile(const std::string &collectionFile);

  /// The full path of the collection file.
  std::string m_collectionFile;
  /** The full path of the cache file. If it is not
      determined this is an empty string. */
  std::string m_cacheFile;
  /// The window that renders the help information
  static pqHelpWindow *g_helpWindow;

  void determineFileLocs();

public slots:
  void warning(const QString &msg);
};

} // namespace MantidWidgets
} // namespace MantidQt
