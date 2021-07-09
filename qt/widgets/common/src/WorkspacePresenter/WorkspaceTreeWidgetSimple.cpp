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
bool singleValued(const MatrixWorkspace &ws) { return (ws.getNumberHistograms() == 1 && ws.blocksize() == 1); }
} // namespace

namespace MantidQt {
namespace MantidWidgets {

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
      m_sampleMaterial(new QAction("Show Sample Material", this)), m_superplot(new QAction("Superplot...", this)),
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
  connect(m_superplot, SIGNAL(triggered()), this, SLOT(onSuperplotClicked()));
  connect(m_superplotWithErrs, SIGNAL(triggered()), this, SLOT(onSuperplotWithErrsClicked()));
  connect(m_superplotBins, SIGNAL(triggered()), this, SLOT(onSuperplotBinsClicked()));
  connect(m_superplotBinsWithErrs, SIGNAL(triggered()), this, SLOT(onSuperplotBinsWithErrsClicked()));
}

WorkspaceTreeWidgetSimple::~WorkspaceTreeWidgetSimple() {}

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

  // If no workspace is here then have load items
  if (selectedWsName.isEmpty())
    menu = m_loadMenu;
  else {
    menu = new QMenu(this);
    menu->setObjectName("WorkspaceContextMenu");

    // plot submenu first for MatrixWorkspace.
    // Check is defensive just in case the workspace has disappeared
    Workspace_sptr workspace;
    try {
      workspace = AnalysisDataService::Instance().retrieve(selectedWsName.toStdString());
    } catch (Exception::NotFoundError &) {
      return;
    }
    if (auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(workspace)) {
      QMenu *plotSubMenu(new QMenu("Plot", menu));
      // Don't plot 1D spectra if only one X value
      bool multipleBins = false;
      try {
        multipleBins = (matrixWS->blocksize() > 1);
      } catch (...) {
        const size_t numHist = matrixWS->getNumberHistograms();
        for (size_t i = 0; i < numHist; ++i) {
          if (matrixWS->y(i).size() > 1) {
            multipleBins = true;
            break;
          }
        }
      }
      if (multipleBins) {
        plotSubMenu->addAction(m_plotSpectrum);
        plotSubMenu->addAction(m_overplotSpectrum);
        plotSubMenu->addAction(m_plotSpectrumWithErrs);
        plotSubMenu->addAction(m_overplotSpectrumWithErrs);
        plotSubMenu->addAction(m_plotAdvanced);
        plotSubMenu->addAction(m_superplot);
        plotSubMenu->addAction(m_superplotWithErrs);
      } else {
        plotSubMenu->addAction(m_plotBin);
        plotSubMenu->addAction(m_superplotBins);
        plotSubMenu->addAction(m_superplotBinsWithErrs);
      }
      plotSubMenu->addSeparator();
      plotSubMenu->addAction(m_plotColorfill);

      if (multipleBins) {
        QMenu *plot3DSubMenu(new QMenu("3D", menu));
        plot3DSubMenu->addAction(m_plotSurface);
        plot3DSubMenu->addAction(m_plotWireframe);
        plot3DSubMenu->addAction(m_plotContour);

        plotSubMenu->addMenu(plot3DSubMenu);
      }
      if (!singleValued(*matrixWS)) {
        // regular matrix workspace
        menu->addMenu(plotSubMenu);
        menu->addSeparator();
        menu->addAction(m_showData);
        menu->addAction(m_showAlgorithmHistory);
        menu->addAction(m_showInstrument);
        m_showInstrument->setEnabled(matrixWS->getInstrument() && !matrixWS->getInstrument()->getName().empty() &&
                                     matrixWS->getAxis(1)->isSpectra());
        menu->addAction(m_sampleLogs);
        menu->addAction(m_sliceViewer);
        menu->addAction(m_showDetectors);
      } else {
        menu->addAction(m_showData);
      }

    } else if (std::dynamic_pointer_cast<ITableWorkspace>(workspace)) {
      menu->addAction(m_showData);
      menu->addAction(m_showAlgorithmHistory);
      if (std::dynamic_pointer_cast<IPeaksWorkspace>(workspace)) {
        menu->addAction(m_showDetectors);
      }
    } else if (auto md_ws = std::dynamic_pointer_cast<IMDWorkspace>(workspace)) {
      menu->addAction(m_showAlgorithmHistory);
      menu->addAction(m_sampleLogs);

      // launch slice viewer or plot spectrum conditionally
      bool add_slice_viewer = false;
      bool add_1d_plot = false;

      if (md_ws->isMDHistoWorkspace()) {
        // if the number of non-integral  if the number of non-integrated
        // dimensions is 1.
        auto num_dims = md_ws->getNumNonIntegratedDims();
        if (num_dims == 1) {
          // number of non-integral dimension is 1: show menu item to plot
          // spectrum
          add_1d_plot = true;
        } else if (num_dims > 1) {
          // number of non-integral dimension is larger than 1: show menu item
          // to launch slice view
          add_slice_viewer = true;
        }
      } else if (md_ws->getNumDims() > 1) {
        add_slice_viewer = true;
      }

      if (add_slice_viewer) {
        menu->addAction(m_sliceViewer);
      } else if (add_1d_plot) {
        QMenu *plotSubMenu(new QMenu("Plot", menu));
        plotSubMenu->addAction(m_plotMDHisto1D);
        plotSubMenu->addAction(m_overplotMDHisto1D);
        plotSubMenu->addAction(m_plotMDHisto1DWithErrs);
        plotSubMenu->addAction(m_overplotMDHisto1DWithErrs);
        menu->addMenu(plotSubMenu);
      }

    } else if (auto wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)) {
      auto workspaces = wsGroup->getAllItems();
      bool containsMatrixWorkspace{false};
      bool containsPeaksWorkspace{false};

      for (auto ws : workspaces) {
        if (auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws)) {
          containsMatrixWorkspace = true;
          break;
        } else if (auto peaksWS = std::dynamic_pointer_cast<IPeaksWorkspace>(ws)) {
          containsPeaksWorkspace = true;
        }
      }

      // Add plotting options if the group contains at least one matrix
      // workspace.
      if (containsMatrixWorkspace) {
        QMenu *plotSubMenu(new QMenu("Plot", menu));

        plotSubMenu->addAction(m_plotSpectrum);
        plotSubMenu->addAction(m_overplotSpectrum);
        plotSubMenu->addAction(m_plotSpectrumWithErrs);
        plotSubMenu->addAction(m_overplotSpectrumWithErrs);
        plotSubMenu->addAction(m_plotAdvanced);
        plotSubMenu->addAction(m_superplot);
        plotSubMenu->addAction(m_superplotWithErrs);

        plotSubMenu->addSeparator();
        plotSubMenu->addAction(m_plotColorfill);
        menu->addMenu(plotSubMenu);

        menu->addSeparator();
      }

      if (containsMatrixWorkspace || containsPeaksWorkspace) {
        menu->addAction(m_showDetectors);
      }
    }

    // Only show sample material action if we have a single workspace
    // selected.
    if (m_tree->selectedItems().size() == 1) {
      // SetSampleMaterial algorithm requires that the workspace
      // inherits from ExperimentInfo, so check that it does
      // before adding the action to the context menu.
      if (auto experimentInfoWS = std::dynamic_pointer_cast<ExperimentInfo>(workspace)) {
        menu->addAction(m_sampleMaterial);
      }
    }

    menu->addSeparator();
    menu->addAction(m_rename);
    menu->addAction(m_saveNexus);

    menu->addSeparator();
    menu->addAction(m_delete);
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

} // namespace MantidWidgets
} // namespace MantidQt
