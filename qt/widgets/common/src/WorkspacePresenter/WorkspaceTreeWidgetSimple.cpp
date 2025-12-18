// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidQtWidgets/Common/MantidTreeModel.h"
#include "MantidQtWidgets/Common/MantidTreeWidget.h"
#include "MantidQtWidgets/Common/MantidTreeWidgetItem.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <QMenu>
#include <QSignalMapper>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
bool hasSingleValue(const MatrixWorkspace &ws) {
  return (ws.getNumberHistograms() == 1 && ws.blocksize() == 1 && ws.getNumDims() == 0);
}
bool hasMultipleBins(const MatrixWorkspace &ws) {
  try {
    return (ws.blocksize() > 1);
  } catch (...) {
    const size_t numHist = ws.getNumberHistograms();
    for (size_t i = 0; i < numHist; ++i) {
      if (ws.y(i).size() > 1) {
        return true;
      }
    }
  }
  return false;
}
} // namespace

namespace MantidQt::MantidWidgets {

WorkspaceTreeWidgetSimple::WorkspaceTreeWidgetSimple(bool viewOnly, QWidget *parent)
    : WorkspaceTreeWidget(new MantidTreeModel(), viewOnly, parent), m_plotSpectrum(new QAction("Spectrum...", this)),
      m_plotBin(new QAction("Bin", this)), m_overplotSpectrum(new QAction("Overplot spectrum...", this)),
      m_plotSpectrumWithErrs(new QAction("Spectrum with errors...", this)),
      m_overplotSpectrumWithErrs(new QAction("Overplot spectrum with errors...", this)),
      m_plotColorfill(new QAction("Colorfill", this)), m_sampleLogs(new QAction("Show Sample Logs", this)),
      m_sliceViewer(new QAction("Show Slice Viewer", this)), m_showInstrument(new QAction("Show Instrument", this)),
      m_showData(new QAction("Show Data", this)), m_showAlgorithmHistory(new QAction("Show History", this)),
      m_showDetectors(new QAction("Show Detectors", this)), m_plotAdvanced(new QAction("Advanced...", this)),
      m_plotSurface(new QAction("Surface", this)), m_plotWireframe(new QAction("Wireframe", this)),
      m_plotContour(new QAction("Contour", this)), m_plotMDHisto1D(new QAction("Plot 1D MDHistogram...", this)),
      m_overplotMDHisto1D(new QAction("Overplot 1D MDHistogram...", this)),
      m_plotMDHisto1DWithErrs(new QAction("Plot 1D MDHistogram with errors...", this)),
      m_overplotMDHisto1DWithErrs(new QAction("Overplot 1D MDHistogram with errors...", this)),
      m_sampleMaterial(new QAction("Show Sample Material", this)),
      m_sampleShape(new QAction("Show Sample Shape", this)), m_superplot(new QAction("Superplot...", this)),
      m_superplotWithErrs(new QAction("Superplot with errors...", this)),
      m_superplotBins(new QAction("Superplot bins...", this)),
      m_superplotBinsWithErrs(new QAction("Superplot bins with errors...", this)),
      m_showNewInstrumentView(new QAction("(Experimental) Show Instrument", this)), m_separator(new QAction()) {

  // Replace the double click action on the MantidTreeWidget
  m_tree->m_doubleClickAction = [&](const QString &wsName) { emit workspaceDoubleClicked(wsName); };

  connect(m_plotSpectrum, SIGNAL(triggered()), this, SLOT(onPlotSpectrumClicked()));
  // connect event m_plotMDHisto1D to signal slot onPlotMDHistoWorkspaceClicked
  connect(m_plotMDHisto1D, SIGNAL(triggered()), this, SLOT(onPlotMDHistoWorkspaceClicked()));
  connect(m_overplotMDHisto1D, SIGNAL(triggered()), this, SLOT(onOverPlotMDHistoWorkspaceClicked()));
  connect(m_plotMDHisto1DWithErrs, SIGNAL(triggered()), this, SLOT(onPlotMDHistoWorkspaceWithErrorsClicked()));
  connect(m_overplotMDHisto1DWithErrs, SIGNAL(triggered()), this, SLOT(onOverPlotMDHistoWorkspaceWithErrorsClicked()));

  connect(m_plotBin, SIGNAL(triggered()), this, SLOT(onPlotBinClicked()));
  connect(m_overplotSpectrum, SIGNAL(triggered()), this, SLOT(onOverplotSpectrumClicked()));
  connect(m_plotSpectrumWithErrs, SIGNAL(triggered()), this, SLOT(onPlotSpectrumWithErrorsClicked()));
  connect(m_overplotSpectrumWithErrs, SIGNAL(triggered()), this, SLOT(onOverplotSpectrumWithErrorsClicked()));
  connect(m_plotColorfill, SIGNAL(triggered()), this, SLOT(onPlotColorfillClicked()));
  connect(m_sampleLogs, SIGNAL(triggered()), this, SLOT(onSampleLogsClicked()));
  connect(m_sliceViewer, SIGNAL(triggered()), this, SLOT(onSliceViewerClicked()));
  connect(m_showInstrument, SIGNAL(triggered()), this, SLOT(onShowInstrumentClicked()));
  connect(m_showData, SIGNAL(triggered()), this, SLOT(onShowDataClicked()));
  connect(m_tree, SIGNAL(itemSelectionChanged()), this, SIGNAL(treeSelectionChanged()));
  connect(m_showAlgorithmHistory, SIGNAL(triggered()), this, SLOT(onShowAlgorithmHistoryClicked()));
  connect(m_showDetectors, SIGNAL(triggered()), this, SLOT(onShowDetectorsClicked()));
  connect(m_plotAdvanced, SIGNAL(triggered()), this, SLOT(onPlotAdvancedClicked()));
  connect(m_plotSurface, SIGNAL(triggered()), this, SLOT(onPlotSurfaceClicked()));
  connect(m_plotWireframe, SIGNAL(triggered()), this, SLOT(onPlotWireframeClicked()));
  connect(m_plotContour, SIGNAL(triggered()), this, SLOT(onPlotContourClicked()));
  connect(m_sampleMaterial, SIGNAL(triggered()), this, SLOT(onSampleMaterialClicked()));
  connect(m_sampleShape, SIGNAL(triggered()), this, SLOT(onSampleShapeClicked()));
  connect(m_superplot, SIGNAL(triggered()), this, SLOT(onSuperplotClicked()));
  connect(m_superplotWithErrs, SIGNAL(triggered()), this, SLOT(onSuperplotWithErrsClicked()));
  connect(m_superplotBins, SIGNAL(triggered()), this, SLOT(onSuperplotBinsClicked()));
  connect(m_superplotBinsWithErrs, SIGNAL(triggered()), this, SLOT(onSuperplotBinsWithErrsClicked()));
  connect(m_showNewInstrumentView, SIGNAL(triggered()), this, SLOT(onShowNewInstrumentViewClicked()));
  m_separator->setSeparator(true);
}

WorkspaceTreeWidgetSimple::~WorkspaceTreeWidgetSimple() = default;

void WorkspaceTreeWidgetSimple::setOverplotDisabled(bool disabled) {
  m_overplotSpectrum->setDisabled(disabled);
  m_overplotSpectrumWithErrs->setDisabled(disabled);
}

void WorkspaceTreeWidgetSimple::popupContextMenu() {
  emit contextMenuAboutToShow();
  QTreeWidgetItem *treeItem = m_tree->itemAt(m_menuPosition);
  selectedWsName = "";
  if (treeItem)
    selectedWsName = treeItem->text(0);
  else
    m_tree->selectionModel()->clear();

  QMenu *menu(nullptr);
  if (selectedWsName.isEmpty()) {
    // If no workspace is here then have load items
    menu = m_loadMenu;
  } else {
    auto selectedWorksapceNames = getSelectedWorkspaceNamesAsQList();
    menu = createWorkspaceContextMenu(selectedWorksapceNames);
  }

  // Show the menu at the cursor's current position
  menu->popup(QCursor::pos());
}

void WorkspaceTreeWidgetSimple::onPlotSpectrumClicked() {
  emit plotSpectrumClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotBinClicked() { emit plotBinClicked(getSelectedWorkspaceNamesAsQList()); }

void WorkspaceTreeWidgetSimple::onOverplotSpectrumClicked() {
  emit overplotSpectrumClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotSpectrumWithErrorsClicked() {
  emit plotSpectrumWithErrorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onOverplotSpectrumWithErrorsClicked() {
  emit overplotSpectrumWithErrorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotColorfillClicked() {
  emit plotColorfillClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onSampleLogsClicked() { emit sampleLogsClicked(getSelectedWorkspaceNamesAsQList()); }

void WorkspaceTreeWidgetSimple::onSliceViewerClicked() { emit sliceViewerClicked(getSelectedWorkspaceNamesAsQList()); }

void WorkspaceTreeWidgetSimple::onShowInstrumentClicked() {
  emit showInstrumentClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onShowDataClicked() { emit showDataClicked(getSelectedWorkspaceNamesAsQList()); }

void WorkspaceTreeWidgetSimple::onShowAlgorithmHistoryClicked() {
  emit showAlgorithmHistoryClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onShowDetectorsClicked() {
  emit showDetectorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotAdvancedClicked() {
  emit plotAdvancedClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotSurfaceClicked() { emit plotSurfaceClicked(getSelectedWorkspaceNamesAsQList()); }

void WorkspaceTreeWidgetSimple::onPlotWireframeClicked() {
  emit plotWireframeClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotContourClicked() { emit plotContourClicked(getSelectedWorkspaceNamesAsQList()); }

// Define signal
void WorkspaceTreeWidgetSimple::onPlotMDHistoWorkspaceClicked() {
  emit plotMDHistoClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onOverPlotMDHistoWorkspaceClicked() {
  emit overplotMDHistoClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotMDHistoWorkspaceWithErrorsClicked() {
  emit plotMDHistoWithErrorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onOverPlotMDHistoWorkspaceWithErrorsClicked() {
  emit overplotMDHistoWithErrorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onSampleMaterialClicked() {
  emit sampleMaterialClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onSampleShapeClicked() { emit sampleShapeClicked(getSelectedWorkspaceNamesAsQList()); }

void WorkspaceTreeWidgetSimple::onSuperplotClicked() { emit superplotClicked(getSelectedWorkspaceNamesAsQList()); }

void WorkspaceTreeWidgetSimple::onSuperplotWithErrsClicked() {
  emit superplotWithErrsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onSuperplotBinsClicked() {
  emit superplotBinsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onSuperplotBinsWithErrsClicked() {
  emit superplotBinsWithErrsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onShowNewInstrumentViewClicked() {
  emit showNewInstrumentViewClicked(getSelectedWorkspaceNamesAsQList());
}

/**
 * Create a new QMenu object filled with appropriate items for the given workspace
 * The created object has this as its parent and WA_DeleteOnClose set
 */
QMenu *WorkspaceTreeWidgetSimple::createWorkspaceContextMenu(QStringList &selectedWorkspaces) {
  auto menu = new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose, true);
  menu->setObjectName("WorkspaceContextMenu");

  std::vector<std::vector<QAction *>> actionVecs;
  std::vector<std::vector<QAction *>> plotMenuActionVecs;
  std::vector<std::vector<QAction *>> plotMenu3DActionVecs;
  for (const auto &workspaceName : selectedWorkspaces) {
    Workspace_sptr workspace;
    try {
      workspace = AnalysisDataService::Instance().retrieve(workspaceName.toStdString());
    } catch (Exception::NotFoundError &) {
      continue;
    }
    if (auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(workspace)) {
      auto [actions, plotActions] = createMatrixWorkspaceActions(*matrixWS);
      actionVecs.push_back(actions);
      plotMenuActionVecs.push_back(plotActions.plotActions);
      plotMenu3DActionVecs.push_back(plotActions.plot3DActions);
    } else if (auto tableWS = std::dynamic_pointer_cast<ITableWorkspace>(workspace)) {
      actionVecs.push_back(createTableWorkspaceActions(*tableWS));
      plotMenuActionVecs.push_back(std::vector<QAction *>{});
      plotMenu3DActionVecs.push_back(std::vector<QAction *>{});
    } else if (auto mdWS = std::dynamic_pointer_cast<IMDWorkspace>(workspace)) {
      auto [actions, plotActions] = createMDWorkspaceActions(*mdWS);
      actionVecs.push_back(actions);
      plotMenuActionVecs.push_back(plotActions.plotActions);
      plotMenu3DActionVecs.push_back(plotActions.plot3DActions);
    } else if (auto wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)) {
      auto [actions, plotActions] = createWorkspaceGroupActions(*wsGroup);
      actionVecs.push_back(actions);
      plotMenuActionVecs.push_back(plotActions.plotActions);
      plotMenu3DActionVecs.push_back(plotActions.plot3DActions);
    }
  }

  std::vector<QAction *> combinedActions = intersectionOfActions(actionVecs);
  std::vector<QAction *> combinedPlotMenuActions = intersectionOfActions(plotMenuActionVecs);
  std::vector<QAction *> combined3DPlotMenuActions = intersectionOfActions(plotMenu3DActionVecs);

  if (!combinedPlotMenuActions.empty()) {
    auto *plotSubMenu = new QMenu("Plot", menu);
    for (const auto &action : combinedPlotMenuActions) {
      plotSubMenu->addAction(action);
    }
    if (!combined3DPlotMenuActions.empty()) {
      auto *plot3DMenu = new QMenu("3D", menu);
      for (const auto &action : combined3DPlotMenuActions) {
        plot3DMenu->addAction(action);
      }
      plotSubMenu->addMenu(plot3DMenu);
    }
    menu->addMenu(plotSubMenu);
  }

  for (const auto &action : combinedActions) {
    menu->addAction(action);
  }

  // Add for all types
  addGeneralWorkspaceActions(menu);

  return menu;
}

std::vector<QAction *>
WorkspaceTreeWidgetSimple::intersectionOfActions(std::vector<std::vector<QAction *>> actionVecs) {
  std::sort(actionVecs.begin(), actionVecs.end(), [](const auto &a, const auto &b) { return a.size() > b.size(); });
  std::vector<QAction *> combinedActions = actionVecs.front();

  std::erase_if(combinedActions, [&actionVecs](QAction *action) {
    return (
        !std::all_of(actionVecs.cbegin() + 1, actionVecs.cend(), [&action](const std::vector<QAction *> &actionVec) {
          const auto it = std::find(actionVec.cbegin(), actionVec.cend(), action);
          return it != actionVec.cend();
        }));
  });

  return combinedActions;
}

std::tuple<std::vector<QAction *>, plotMenuActions>
WorkspaceTreeWidgetSimple::createMatrixWorkspaceActions(const Mantid::API::MatrixWorkspace &workspace) {
  std::vector<QAction *> actions;
  // Show just data for a single value.
  if (hasSingleValue(workspace)) {
    actions.push_back(m_showData);
    return std::make_tuple(actions, plotMenuActions());
  }

  auto plotActions = createMatrixWorkspacePlotMenu(hasMultipleBins(workspace));
  actions.push_back(m_separator);
  actions.push_back(m_showData);
  actions.push_back(m_showAlgorithmHistory);
  actions.push_back(m_showInstrument);
  // TODO Should do this in the main function over all workspaces if the action is present (same with other setEnabled)
  m_showInstrument->setEnabled(workspace.getInstrument() && !workspace.getInstrument()->getName().empty() &&
                               workspace.getAxis(1)->isSpectra());
  actions.push_back(m_sampleLogs);
  actions.push_back(m_sliceViewer);
  actions.push_back(m_showDetectors);
  if (m_tree->selectedItems().size() == 1) {
    actions.push_back(m_sampleMaterial);
    actions.push_back(m_sampleShape);
  }
  actions.push_back(m_showNewInstrumentView);
  m_showNewInstrumentView->setEnabled(workspace.getInstrument() && !workspace.getInstrument()->getName().empty() &&
                                      workspace.getAxis(1)->isSpectra() &&
                                      !workspace.detectorInfo().detectorIDs().empty());
  return std::make_tuple(actions, plotActions);
}

std::vector<QAction *>
WorkspaceTreeWidgetSimple::createTableWorkspaceActions(const Mantid::API::ITableWorkspace &workspace) {
  std::vector<QAction *> actions;
  actions.push_back(m_showData);
  actions.push_back(m_showAlgorithmHistory);
  if (dynamic_cast<const IPeaksWorkspace *>(&workspace)) {
    actions.push_back(m_showDetectors);
  }
  return actions;
}

std::tuple<std::vector<QAction *>, plotMenuActions>
WorkspaceTreeWidgetSimple::createMDWorkspaceActions(const Mantid::API::IMDWorkspace &workspace) {
  std::vector<QAction *> actions;
  plotMenuActions plotMenu;
  actions.push_back(m_showAlgorithmHistory);
  actions.push_back(m_sampleLogs);

  // launch slice viewer or plot spectrum conditionally
  bool addSliceViewer = false;
  bool add1DPlot = false;

  if (workspace.isMDHistoWorkspace()) {
    // if the number of non-integral  if the number of non-integrated
    // dimensions is 1.
    auto num_dims = workspace.getNumNonIntegratedDims();
    if (num_dims == 1) {
      // number of non-integral dimension is 1: show menu item to plot
      // spectrum
      add1DPlot = true;
    } else if (num_dims > 1) {
      // number of non-integral dimension is larger than 1: show menu item
      // to launch slice view
      addSliceViewer = true;
    }
  } else if (workspace.getNumDims() > 1) {
    addSliceViewer = true;
  }

  if (addSliceViewer) {
    actions.push_back(m_sliceViewer);
  } else if (add1DPlot) {
    plotMenu.plotActions.push_back(m_plotMDHisto1D);
    plotMenu.plotActions.push_back(m_overplotMDHisto1D);
    plotMenu.plotActions.push_back(m_plotMDHisto1DWithErrs);
    plotMenu.plotActions.push_back(m_overplotMDHisto1DWithErrs);
  }
  return std::make_tuple(actions, plotMenu);
}

std::tuple<std::vector<QAction *>, plotMenuActions>
WorkspaceTreeWidgetSimple::createWorkspaceGroupActions(const Mantid::API::WorkspaceGroup &workspace) {
  std::vector<QAction *> actions;
  plotMenuActions plotMenu;
  auto workspaces = workspace.getAllItems();
  bool containsMatrixWorkspace{false};
  bool containsPeaksWorkspace{false};
  for (const auto &ws : workspaces) {
    if (std::dynamic_pointer_cast<MatrixWorkspace>(ws)) {
      containsMatrixWorkspace = true;
      break;
    } else if (std::dynamic_pointer_cast<IPeaksWorkspace>(ws)) {
      containsPeaksWorkspace = true;
    }
  }

  // Add plotting options if the group contains at least one matrix
  // workspace.
  if (containsMatrixWorkspace) {
    plotMenu = createMatrixWorkspacePlotMenu(true);
    actions.push_back(m_separator);
    actions.push_back(m_showDetectors);
  }

  if (containsPeaksWorkspace) {
    actions.push_back(m_showData);
    actions.push_back(m_showDetectors);
  }
  return std::make_tuple(actions, plotMenu);
}

void WorkspaceTreeWidgetSimple::addGeneralWorkspaceActions(QMenu *menu) const {
  menu->addSeparator();
  menu->addAction(m_rename);
  menu->addAction(m_saveNexus);
  menu->addSeparator();
  menu->addAction(m_delete);
}

plotMenuActions WorkspaceTreeWidgetSimple::createMatrixWorkspacePlotMenu(bool hasMultipleBins) {
  plotMenuActions plotMenu;
  if (hasMultipleBins) {
    plotMenu.plotActions.push_back(m_plotSpectrum);
    plotMenu.plotActions.push_back(m_overplotSpectrum);
    plotMenu.plotActions.push_back(m_plotSpectrumWithErrs);
    plotMenu.plotActions.push_back(m_overplotSpectrumWithErrs);
    plotMenu.plotActions.push_back(m_plotAdvanced);
    plotMenu.plotActions.push_back(m_superplot);
    plotMenu.plotActions.push_back(m_superplotWithErrs);
    plotMenu.plotActions.push_back(m_separator);
    plotMenu.plotActions.push_back(m_plotColorfill);
    // 3D
    plotMenu.plot3DActions.push_back(m_plotSurface);
    plotMenu.plot3DActions.push_back(m_plotWireframe);
    plotMenu.plot3DActions.push_back(m_plotContour);

  } else {
    plotMenu.plotActions.push_back(m_superplotBins);
    plotMenu.plotActions.push_back(m_superplotBinsWithErrs);
    plotMenu.plotActions.push_back(m_separator);
    plotMenu.plotActions.push_back(m_plotColorfill);
  }

  return plotMenu;
}

} // namespace MantidQt::MantidWidgets
