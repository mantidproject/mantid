// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidget.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

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
  explicit WorkspaceTreeWidgetSimple(bool viewOnly = false,
                                     QWidget *parent = nullptr);
  ~WorkspaceTreeWidgetSimple();

  // Context Menu Handlers
  void popupContextMenu() override;
  void setOverplotDisabled(bool disabled);

signals:
  void plotSpectrumClicked(const QStringList &workspaceNames);
  void plotBinClicked(const QStringList &workspaceNames);
  void overplotSpectrumClicked(const QStringList &workspaceNames);
  void plotSpectrumWithErrorsClicked(const QStringList &workspaceNames);
  void overplotSpectrumWithErrorsClicked(const QStringList &workspaceNames);
  void plotColorfillClicked(const QStringList &workspaceNames);
  void sampleLogsClicked(const QStringList &workspaceName);
  void sliceViewerClicked(const QStringList &workspaceName);
  void showInstrumentClicked(const QStringList &workspaceNames);
  void showDataClicked(const QStringList &workspaceNames);
  void showAlgorithmHistoryClicked(const QStringList &workspaceNames);
  void showDetectorsClicked(const QStringList &workspaceNames);
  void plotAdvancedClicked(const QStringList &workspaceNames);
  void plotSurfaceClicked(const QStringList &workspaceNames);
  void plotWireframeClicked(const QStringList &workspaceNames);
  void plotContourClicked(const QStringList &workspaceNames);
  void contextMenuAboutToShow(void);

  void workspaceDoubleClicked(const QString &workspaceName);
  void treeSelectionChanged();

  // Signal when plot MDHistogram clicked
  void plotMDHistoClicked(const QStringList &workspaceNames);
  void overplotMDHistoClicked(const QStringList &workspaceNames);
  void plotMDHistoWithErrorsClicked(const QStringList &workspaceNames);
  void overplotMDHistoWithErrorsClicked(const QStringList &workspaceNames);

private slots:
  void onPlotSpectrumClicked();
  void onPlotBinClicked();
  void onOverplotSpectrumClicked();
  void onPlotSpectrumWithErrorsClicked();
  void onOverplotSpectrumWithErrorsClicked();
  void onPlotColorfillClicked();
  void onSampleLogsClicked();
  void onSliceViewerClicked();
  void onShowInstrumentClicked();
  void onShowDataClicked();
  void onShowAlgorithmHistoryClicked();
  void onShowDetectorsClicked();
  void onPlotAdvancedClicked();
  void onPlotSurfaceClicked();
  void onPlotWireframeClicked();
  void onPlotContourClicked();
  void onPlotMDHistoWorkspaceClicked(); // Linked to plotMDHistoClicked
  void onOverPlotMDHistoWorkspaceClicked();
  void onPlotMDHistoWorkspaceWithErrorsClicked();
  void onOverPlotMDHistoWorkspaceWithErrorsClicked();

private:
  QAction *m_plotSpectrum, *m_plotBin, *m_overplotSpectrum,
      *m_plotSpectrumWithErrs, *m_overplotSpectrumWithErrs, *m_plotColorfill,
      *m_sampleLogs, *m_sliceViewer, *m_showInstrument, *m_showData,
      *m_showAlgorithmHistory, *m_showDetectors, *m_plotAdvanced,
      *m_plotSurface, *m_plotWireframe, *m_plotContour, *m_plotMDHisto1D,
      *m_overplotMDHisto1D, *m_plotMDHisto1DWithErrs,
      *m_overplotMDHisto1DWithErrs;
};
} // namespace MantidWidgets
} // namespace MantidQt
