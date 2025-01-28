// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/MantidHelpInterface.h"
#include <QPointer>
#include <QWidget>
#include <string>

// forward declaration
class QHelpEngine;
class QString;
class QWidget;
class pqHelpWindow;

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON MantidHelpWindow : public API::MantidHelpInterface {
  Q_OBJECT

public:
  static bool helpWindowExists()

      MantidHelpWindow(const Qt::WindowFlags &flags = Qt::WindowFlags());

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

private:
  void showHelp(const QString &url);
  void openWebpage(const QUrl &url);

  /// The full path of the collection file.
  std::string m_collectionFile;
  /// The window that renders the help information
  static QPointer<pqHelpWindow> g_helpWindow;

  /// Whether this is the very first startup of the helpwindow.
  bool m_firstRun;

  void findCollectionFile(const std::string &binDir);
  void determineFileLocs();

public slots:
  /// Perform any clean up on main window shutdown
  void shutdown() override;
  void warning(const QString &msg);
};

} // namespace MantidWidgets
} // namespace MantidQt
