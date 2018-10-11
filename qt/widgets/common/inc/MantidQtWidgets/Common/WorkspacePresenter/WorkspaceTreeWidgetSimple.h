// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGETSIMPLE_H
#define MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGETSIMPLE_H

#include "MantidQtWidgets/Common/DllOption.h"
#include <MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h>

#include <MantidAPI/MatrixWorkspace_fwd.h>

#include <QMenu>
#include <QWidget>

class QTreeWidgetItem;
class QSignalMapper;

namespace MantidQt {
namespace MantidWidgets {
class MantidDisplayBase;
class MantidTreeWidget;

/**
\class  WorkspaceTreeWidgetSimple
\brief  WorkspaceTreeWidget implementation for the Workbench - required for some
function overides
\author Elliot Oram
\date   16-01-2018
\version 1.0
*/
class EXPORT_OPT_MANTIDQT_COMMON WorkspaceTreeWidgetSimple
    : public WorkspaceTreeWidget {
  Q_OBJECT
public:
  explicit WorkspaceTreeWidgetSimple(bool showButtonToolbar, QWidget *parent = nullptr);
  ~WorkspaceTreeWidgetSimple();

  // Context Menu Handlers
  void popupContextMenu() override;

signals:
  void plotSpectrumClicked(const QStringList &workspaceName);
  void overplotSpectrumClicked(const QStringList &workspaceName);
  void plotSpectrumWithErrorsClicked(const QStringList &workspaceName);
  void overplotSpectrumWithErrorsClicked(const QStringList &workspaceName);
  void plotColorfillClicked(const QStringList &workspaceName);

private slots:
  void onPlotSpectrumClicked();
  void onOverplotSpectrumClicked();
  void onPlotSpectrumWithErrorsClicked();
  void onOverplotSpectrumWithErrorsClicked();
  void onPlotColorfillClicked();

private:
  QAction *m_plotSpectrum, *m_overplotSpectrum, *m_plotSpectrumWithErrs,
      *m_overplotSpectrumWithErrs, *m_plotColorfill;
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQT_MANTIDWIDGETS_WORKSPACETREEWIDGETSIMPLE_H
