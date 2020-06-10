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
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <QMenu>
#include <QSignalMapper>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace MantidQt {
namespace MantidWidgets {

WorkspaceTreeWidgetSimple::WorkspaceTreeWidgetSimple(bool viewOnly,
                                                     QWidget *parent)
    : WorkspaceTreeWidget(new MantidTreeModel(), viewOnly, parent),
      m_plotSpectrum(new QAction("Spectrum...", this)),
      m_plotBin(new QAction("Bin", this)),
      m_overplotSpectrum(new QAction("Overplot spectrum...", this)),
      m_plotSpectrumWithErrs(new QAction("Spectrum with errors...", this)),
      m_overplotSpectrumWithErrs(
          new QAction("Overplot spectrum with errors...", this)),
      m_plotColorfill(new QAction("Colorfill", this)),
      m_sampleLogs(new QAction("Show Sample Logs", this)),
      m_sliceViewer(new QAction("Show Slice Viewer", this)),
      m_showInstrument(new QAction("Show Instrument", this)),
      m_showData(new QAction("Show Data", this)),
      m_showAlgorithmHistory(new QAction("Show History", this)),
      m_showDetectors(new QAction("Show Detectors", this)),
      m_plotAdvanced(new QAction("Advanced...", this)),
      m_plotSurface(new QAction("Surface", this)),
      m_plotWireframe(new QAction("Wireframe", this)),
      m_plotContour(new QAction("Contour", this)) {

  // Replace the double click action on the MantidTreeWidget
  m_tree->m_doubleClickAction = [&](const QString &wsName) {
    emit workspaceDoubleClicked(wsName);
  };

  connect(m_plotSpectrum, SIGNAL(triggered()), this,
          SLOT(onPlotSpectrumClicked()));
  connect(m_plotBin, SIGNAL(triggered()), this, SLOT(onPlotBinClicked()));
  connect(m_overplotSpectrum, SIGNAL(triggered()), this,
          SLOT(onOverplotSpectrumClicked()));
  connect(m_plotSpectrumWithErrs, SIGNAL(triggered()), this,
          SLOT(onPlotSpectrumWithErrorsClicked()));
  connect(m_overplotSpectrumWithErrs, SIGNAL(triggered()), this,
          SLOT(onOverplotSpectrumWithErrorsClicked()));
  connect(m_plotColorfill, SIGNAL(triggered()), this,
          SLOT(onPlotColorfillClicked()));
  connect(m_sampleLogs, SIGNAL(triggered()), this, SLOT(onSampleLogsClicked()));
  connect(m_sliceViewer, SIGNAL(triggered()), this,
          SLOT(onSliceViewerClicked()));
  connect(m_showInstrument, SIGNAL(triggered()), this,
          SLOT(onShowInstrumentClicked()));
  connect(m_showData, SIGNAL(triggered()), this, SLOT(onShowDataClicked()));
  connect(m_tree, SIGNAL(itemSelectionChanged()), this,
          SIGNAL(treeSelectionChanged()));
  connect(m_showAlgorithmHistory, SIGNAL(triggered()), this,
          SLOT(onShowAlgorithmHistoryClicked()));
  connect(m_showDetectors, SIGNAL(triggered()), this,
          SLOT(onShowDetectorsClicked()));
  connect(m_plotAdvanced, SIGNAL(triggered()), this,
          SLOT(onPlotAdvancedClicked()));
  connect(m_plotSurface, SIGNAL(triggered()), this,
          SLOT(onPlotSurfaceClicked()));
  connect(m_plotWireframe, SIGNAL(triggered()), this,
          SLOT(onPlotWireframeClicked()));
  connect(m_plotContour, SIGNAL(triggered()), this,
          SLOT(onPlotContourClicked()));
}

WorkspaceTreeWidgetSimple::~WorkspaceTreeWidgetSimple() {}

void WorkspaceTreeWidgetSimple::popupContextMenu() {
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
      workspace = AnalysisDataService::Instance().retrieve(
          selectedWsName.toStdString());
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
      } else {
        plotSubMenu->addAction(m_plotBin);
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

      menu->addMenu(plotSubMenu);
      menu->addSeparator();
      menu->addAction(m_showData);
      menu->addAction(m_showAlgorithmHistory);
      menu->addAction(m_showInstrument);
      m_showInstrument->setEnabled(
          matrixWS->getInstrument() &&
          !matrixWS->getInstrument()->getName().empty() &&
          matrixWS->getAxis(1)->isSpectra());
      menu->addAction(m_sampleLogs);
      menu->addAction(m_sliceViewer);
      menu->addAction(m_showDetectors);
    } else if (std::dynamic_pointer_cast<ITableWorkspace>(workspace)) {
      menu->addAction(m_showData);
      menu->addAction(m_showAlgorithmHistory);
      if (std::dynamic_pointer_cast<IPeaksWorkspace>(workspace)) {
        menu->addAction(m_showDetectors);
      }
    } else if (std::dynamic_pointer_cast<IMDWorkspace>(workspace)) {
      menu->addAction(m_showAlgorithmHistory);
      menu->addAction(m_sampleLogs);
      menu->addAction(m_sliceViewer);
    } else if (auto wsGroup =
                   std::dynamic_pointer_cast<WorkspaceGroup>(workspace)) {
      auto workspaces = wsGroup->getAllItems();
      bool containsMatrixWorkspace{false};
      bool containsPeaksWorkspace{false};

      for (auto ws : workspaces) {
        if (auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws)) {
          containsMatrixWorkspace = true;
          break;
        } else if (auto peaksWS =
                       std::dynamic_pointer_cast<IPeaksWorkspace>(ws)) {
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

        plotSubMenu->addSeparator();
        plotSubMenu->addAction(m_plotColorfill);
        menu->addMenu(plotSubMenu);

        menu->addSeparator();
      }

      if (containsMatrixWorkspace || containsPeaksWorkspace) {
        menu->addAction(m_showDetectors);
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

void WorkspaceTreeWidgetSimple::onPlotBinClicked() {
  emit plotBinClicked(getSelectedWorkspaceNamesAsQList());
}

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

void WorkspaceTreeWidgetSimple::onSampleLogsClicked() {
  emit sampleLogsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onSliceViewerClicked() {
  emit sliceViewerClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onShowInstrumentClicked() {
  emit showInstrumentClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onShowDataClicked() {
  emit showDataClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onShowAlgorithmHistoryClicked() {
  emit showAlgorithmHistoryClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onShowDetectorsClicked() {
  emit showDetectorsClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotAdvancedClicked() {
  emit plotAdvancedClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotSurfaceClicked() {
  emit plotSurfaceClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotWireframeClicked() {
  emit plotWireframeClicked(getSelectedWorkspaceNamesAsQList());
}

void WorkspaceTreeWidgetSimple::onPlotContourClicked() {
  emit plotContourClicked(getSelectedWorkspaceNamesAsQList());
}

} // namespace MantidWidgets
} // namespace MantidQt
