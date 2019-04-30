// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDHELPWINDOW_H
#define MANTIDHELPWINDOW_H

#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include <QWidget>
#include <string>

// forward declaration
class QHelpEngine;
class QString;
class QWidget;
class pqHelpWindow;

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON MantidHelpWindow
    : public API::MantidHelpInterface {
  Q_OBJECT

public:
  static bool helpWindowExists() { return g_helpWindow != nullptr; }

  MantidHelpWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = nullptr);
  ~MantidHelpWindow() override;

  void showPage(const std::string &url = std::string()) override;
  void showPage(const QString &url) override;
  void showPage(const QUrl &url) override;
  void showWikiPage(const std::string &page = std::string()) override;
  void showWikiPage(const QString &page) override;
  void showAlgorithm(const std::string &name = std::string(),
                     const int version = -1) override;
  void showAlgorithm(const QString &name, const int version = -1) override;
  void showConcept(const std::string &name) override;
  void showConcept(const QString &name) override;
  void showFitFunction(const std::string &name = std::string()) override;
  void showFitFunction(const QString &name) override;
  void showCustomInterface(const std::string &name = std::string(),
                           const std::string &section = std::string()) override;
  void showCustomInterface(const QString &name,
                           const QString &section = QString()) override;

private:
  void showHelp(const QString &url);
  void openWebpage(const QUrl &url);

  /// The full path of the collection file.
  std::string m_collectionFile;
  /** The full path of the cache file. If it is not
      determined this is an empty string. */
  std::string m_cacheFile;
  /// The window that renders the help information
  static pqHelpWindow *g_helpWindow;

  /// Whether this is the very first startup of the helpwindow.
  bool m_firstRun;

  void findCollectionFile(std::string &binDir);
  void determineFileLocs();

public slots:
  /// Perform any clean up on main window shutdown
  void shutdown() override;
  void warning(QString msg);
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDHELPWINDOW_H
