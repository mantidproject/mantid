// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/WorkspacePresenter/WorkspaceTreeWidgetSimple.h"
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
      m_superplotBinsWithErrs(new QAction("Superplot bins with errors...", this)) {

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
    // Check is defensive just in case the workspace has disappeared
    Workspace_sptr workspace;
    try {
      workspace = AnalysisDataService::Instance().retrieve(selectedWsName.toStdString());
    } catch (Exception::NotFoundError &) {
      return;
    }
    menu = createWorkspaceContextMenu(*workspace);
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

/**
 * Create a new QMenu object filled with appropriate items for the given workspace
 * The created object has this as its parent and WA_DeleteOnClose set
 */
QMenu *WorkspaceTreeWidgetSimple::createWorkspaceContextMenu(const Mantid::API::Workspace &workspace) {
  auto menu = new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose, true);
  menu->setObjectName("WorkspaceContextMenu");

  if (auto matrixWS = dynamic_cast<const MatrixWorkspace *>(&workspace))
    addMatrixWorkspaceActions(menu, *matrixWS);
  else if (auto tableWS = dynamic_cast<const ITableWorkspace *>(&workspace))
    addTableWorkspaceActions(menu, *tableWS);
  else if (auto mdWS = dynamic_cast<const IMDWorkspace *>(&workspace))
    addMDWorkspaceActions(menu, *mdWS);
  else if (auto wsGroup = dynamic_cast<const WorkspaceGroup *>(&workspace))
    addWorkspaceGroupActions(menu, *wsGroup);

  // Add for all types
  addGeneralWorkspaceActions(menu);

  return menu;
}

void WorkspaceTreeWidgetSimple::addMatrixWorkspaceActions(QMenu *menu, const Mantid::API::MatrixWorkspace &workspace) {
  // Show just data for a single value.
  if (hasSingleValue(workspace)) {
    menu->addAction(m_showData);
    return;
  }

  menu->addMenu(createMatrixWorkspacePlotMenu(menu, hasMultipleBins(workspace)));
  menu->addSeparator();
  menu->addAction(m_showData);
  menu->addAction(m_showAlgorithmHistory);
  menu->addAction(m_showInstrument);
  m_showInstrument->setEnabled(workspace.getInstrument() && !workspace.getInstrument()->getName().empty() &&
                               workspace.getAxis(1)->isSpectra());
  menu->addAction(m_sampleLogs);
  menu->addAction(m_sliceViewer);
  menu->addAction(m_showDetectors);
  if (m_tree->selectedItems().size() == 1) {
    menu->addAction(m_sampleMaterial);
    menu->addAction(m_sampleShape);
  }
}

void WorkspaceTreeWidgetSimple::addTableWorkspaceActions(QMenu *menu, const Mantid::API::ITableWorkspace &workspace) {
  menu->addAction(m_showData);
  menu->addAction(m_showAlgorithmHistory);
  if (dynamic_cast<const IPeaksWorkspace *>(&workspace)) {
    menu->addAction(m_showDetectors);
  }
}

void WorkspaceTreeWidgetSimple::addMDWorkspaceActions(QMenu *menu, const Mantid::API::IMDWorkspace &workspace) {
  menu->addAction(m_showAlgorithmHistory);
  menu->addAction(m_sampleLogs);

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
    menu->addAction(m_sliceViewer);
  } else if (add1DPlot) {
    auto *plotSubMenu = new QMenu("Plot", menu);
    plotSubMenu->addAction(m_plotMDHisto1D);
    plotSubMenu->addAction(m_overplotMDHisto1D);
    plotSubMenu->addAction(m_plotMDHisto1DWithErrs);
    plotSubMenu->addAction(m_overplotMDHisto1DWithErrs);
    menu->addMenu(plotSubMenu);
  }
}

void WorkspaceTreeWidgetSimple::addWorkspaceGroupActions(QMenu *menu, const Mantid::API::WorkspaceGroup &workspace) {
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
    menu->addMenu(createMatrixWorkspacePlotMenu(menu, true));
    menu->addSeparator();
  }

  if (containsMatrixWorkspace || containsPeaksWorkspace) {
    menu->addAction(m_showDetectors);
  }
}

void WorkspaceTreeWidgetSimple::addGeneralWorkspaceActions(QMenu *menu) const {
  menu->addSeparator();
  menu->addAction(m_rename);
  menu->addAction(m_saveNexus);
  menu->addSeparator();
  menu->addAction(m_delete);
}

QMenu *WorkspaceTreeWidgetSimple::createMatrixWorkspacePlotMenu(QWidget *parent, bool hasMultipleBins) {
  auto *plotSubMenu = new QMenu("Plot", parent);
  if (hasMultipleBins) {
    plotSubMenu->addAction(m_plotSpectrum);
    plotSubMenu->addAction(m_overplotSpectrum);
    plotSubMenu->addAction(m_plotSpectrumWithErrs);
    plotSubMenu->addAction(m_overplotSpectrumWithErrs);
    plotSubMenu->addAction(m_plotAdvanced);
    plotSubMenu->addAction(m_superplot);
    plotSubMenu->addAction(m_superplotWithErrs);
    plotSubMenu->addSeparator();
    plotSubMenu->addAction(m_plotColorfill);
    // 3D
    auto *plot3DSubMenu = new QMenu("3D", plotSubMenu);
    plot3DSubMenu->addAction(m_plotSurface);
    plot3DSubMenu->addAction(m_plotWireframe);
    plot3DSubMenu->addAction(m_plotContour);
    plotSubMenu->addMenu(plot3DSubMenu);

  } else {
    plotSubMenu->addAction(m_plotBin);
    plotSubMenu->addAction(m_superplotBins);
    plotSubMenu->addAction(m_superplotBinsWithErrs);
    plotSubMenu->addSeparator();
    plotSubMenu->addAction(m_plotColorfill);
  }

  return plotSubMenu;
}

} // namespace MantidQt::MantidWidgets
