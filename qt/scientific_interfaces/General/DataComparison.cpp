// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "DataComparison.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/LegacyQwt/QwtWorkspaceSpectrumData.h"

namespace {
Mantid::Kernel::Logger g_log("DataComparison");
}

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(DataComparison)
}
} // namespace MantidQt

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;

//----------------------
// Public member functions
//----------------------
/// Constructor
DataComparison::DataComparison(QWidget *parent)
    : UserSubWindow(parent), WorkspaceObserver(), m_plot(new QwtPlot(parent)),
      m_zoomTool(nullptr), m_panTool(nullptr), m_magnifyTool(nullptr),
      m_diffWorkspaceNames(qMakePair(QString(), QString())) {
  observeAfterReplace();
  observeRename();
  observePreDelete();
}

/// Set up the dialog layout
void DataComparison::initLayout() {
  m_uiForm.setupUi(this);

  m_zoomTool =
      new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
                        QwtPicker::DragSelection | QwtPicker::CornerToCorner,
                        QwtPicker::AlwaysOff, m_plot->canvas());
  m_zoomTool->setEnabled(false);

  m_panTool = new QwtPlotPanner(m_plot->canvas());
  m_panTool->setEnabled(false);

  m_magnifyTool = new QwtPlotMagnifier(m_plot->canvas());
  m_magnifyTool->setEnabled(false);

  // Add the plot to the UI
  m_plot->setCanvasBackground(Qt::white);
  m_uiForm.loPlot->addWidget(m_plot);

  // Connect push buttons
  connect(m_uiForm.pbAddData, SIGNAL(clicked()), this, SLOT(addData()));

  connect(m_uiForm.pbRemoveSelectedData, SIGNAL(clicked()), this,
          SLOT(removeSelectedData()));
  connect(m_uiForm.pbRemoveAllData, SIGNAL(clicked()), this,
          SLOT(removeAllData()));

  connect(m_uiForm.pbDiffSelected, SIGNAL(clicked()), this,
          SLOT(diffSelected()));
  connect(m_uiForm.pbClearDiff, SIGNAL(clicked()), this, SLOT(clearDiff()));

  connect(m_uiForm.pbPan, SIGNAL(toggled(bool)), this, SLOT(togglePan(bool)));
  connect(m_uiForm.pbZoom, SIGNAL(toggled(bool)), this, SLOT(toggleZoom(bool)));
  connect(m_uiForm.pbResetView, SIGNAL(clicked()), this, SLOT(resetView()));

  // Replot spectra when the workspace index is changed
  connect(m_uiForm.sbSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(workspaceIndexChanged()));

  // Add headers to data table
  QStringList headerLabels;
  headerLabels << "Colour"
               << "Workspace"
               << "Offset"
               << "Spec.";
  m_uiForm.twCurrentData->setColumnCount(headerLabels.size());
  m_uiForm.twCurrentData->setHorizontalHeaderLabels(headerLabels);

  // Select entire rows when a cell is selected
  m_uiForm.twCurrentData->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Fit columns
  m_uiForm.twCurrentData->resizeColumnsToContents();
}

/**
 * Adds the data currently selected by the data selector to the plot.
 */
void DataComparison::addData() {
  const QString dataName = m_uiForm.dsData->getCurrentDataName();

  // Do nothing if the data is not found
  if (!AnalysisDataService::Instance().doesExist(dataName.toStdString()))
    return;

  // Get the workspace
  Workspace_const_sptr ws =
      AnalysisDataService::Instance().retrieveWS<Workspace>(
          dataName.toStdString());
  WorkspaceGroup_const_sptr wsGroup =
      boost::dynamic_pointer_cast<const WorkspaceGroup>(ws);

  m_uiForm.twCurrentData->blockSignals(true);

  // If this is a WorkspaceGroup then add all items
  if (wsGroup != nullptr) {
    size_t numWs = wsGroup->size();
    for (size_t wsIdx = 0; wsIdx < numWs; wsIdx++) {
      addDataItem(wsGroup->getItem(wsIdx));
    }
  }
  // Otherwise just add the single workspace
  else {
    addDataItem(ws);
  }

  m_uiForm.twCurrentData->blockSignals(false);

  // Fit columns
  m_uiForm.twCurrentData->resizeColumnsToContents();

  // Replot the workspaces
  plotWorkspaces();
}

/**
 * Adds a MatrixWorkspace by name to the data table.
 *
 * @param ws Pointer to workspace to add.
 */
void DataComparison::addDataItem(Workspace_const_sptr ws) {
  // Check that the workspace is the correct type
  MatrixWorkspace_const_sptr matrixWs =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
  if (!matrixWs) {
    g_log.error() << "Workspace " << ws->getName() << "is of incorrect type!\n";
    return;
  }

  // Check that the workspace does not already exist in the comparison
  if (containsWorkspace(matrixWs)) {
    g_log.information() << "Workspace " << matrixWs->getName()
                        << " already shown in comparison.\n";
    return;
  }

  std::string wsName = matrixWs->getName();

  // Append a new row to the data table
  int currentRows = m_uiForm.twCurrentData->rowCount();
  m_uiForm.twCurrentData->insertRow(currentRows);

  // Insert the colour selector
  QComboBox *colourCombo = new QComboBox();
  // Add colours
  colourCombo->addItem("Black", QVariant(Qt::black));
  colourCombo->addItem("Red", QVariant(Qt::red));
  colourCombo->addItem("Green", QVariant(Qt::green));
  colourCombo->addItem("Blue", QVariant(Qt::blue));
  colourCombo->addItem("Cyan", QVariant(Qt::cyan));
  colourCombo->addItem("Magenta", QVariant(Qt::magenta));
  colourCombo->addItem("Yellow", QVariant(Qt::yellow));
  colourCombo->addItem("Light Gray", QVariant(Qt::lightGray));
  colourCombo->addItem("Gray", QVariant(Qt::gray));
  colourCombo->addItem("Dark Red", QVariant(Qt::darkRed));
  colourCombo->addItem("Dark Green", QVariant(Qt::darkGreen));
  colourCombo->addItem("Dark Blue", QVariant(Qt::darkBlue));
  colourCombo->addItem("Dark Cyan", QVariant(Qt::darkCyan));
  colourCombo->addItem("Dark Magenta", QVariant(Qt::darkMagenta));
  colourCombo->addItem("Dark Yellow", QVariant(Qt::darkYellow));
  colourCombo->addItem("Dark Gray", QVariant(Qt::darkGray));
  // Set the initial colour
  colourCombo->setCurrentIndex(getInitialColourIndex());
  // Update plots when colour changed
  connect(colourCombo, SIGNAL(currentIndexChanged(int)), this,
          SLOT(plotWorkspaces()));
  // Add widget to table
  m_uiForm.twCurrentData->setCellWidget(currentRows, COLOUR, colourCombo);

  // Insert the workspace name
  QTableWidgetItem *wsNameItem = new QTableWidgetItem(tr(wsName.c_str()));
  wsNameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_uiForm.twCurrentData->setItem(currentRows, WORKSPACE_NAME, wsNameItem);

  // Insert the spectra offset
  QSpinBox *offsetSpin = new QSpinBox();
  offsetSpin->setMinimum(0);
  offsetSpin->setMaximum(INT_MAX);
  connect(offsetSpin, SIGNAL(valueChanged(int)), this,
          SLOT(spectrumIndexChanged()));
  m_uiForm.twCurrentData->setCellWidget(currentRows, SPEC_OFFSET, offsetSpin);

  // Insert the current displayed spectra
  QTableWidgetItem *currentSpecItem = new QTableWidgetItem(tr("n/a"));
  currentSpecItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_uiForm.twCurrentData->setItem(currentRows, CURRENT_SPEC, currentSpecItem);
}

/**
 * Determines if a given workspace is currently shown in the UI.
 *
 * @param ws Pointer to the workspace
 */
bool DataComparison::containsWorkspace(MatrixWorkspace_const_sptr ws) {
  QString testWsName = QString::fromStdString(ws->getName());

  int numRows = m_uiForm.twCurrentData->rowCount();
  for (int row = 0; row < numRows; row++) {
    QString workspaceName =
        m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();
    if (workspaceName == testWsName)
      return true;
  }

  return false;
}

/**
 * Gets a colour as an index for the combo box for a new workspace.
 * Looks for the lowest unused index, if all colours are used then returns 0.
 *
 * @return An index to set for the conbo box
 */
int DataComparison::getInitialColourIndex() {
  int numRows = m_uiForm.twCurrentData->rowCount();

  // Just use the first colour if this is the first row
  if (numRows <= 1)
    return 0;

  // Build a list of used colours
  QList<int> usedColours;
  for (int row = 0; row < numRows - 1; row++) {
    QComboBox *colourSelector = dynamic_cast<QComboBox *>(
        m_uiForm.twCurrentData->cellWidget(row, COLOUR));
    int index = colourSelector->currentIndex();
    usedColours << index;
  }

  // Find the smallest unused colour
  int numColours =
      dynamic_cast<QComboBox *>(m_uiForm.twCurrentData->cellWidget(0, COLOUR))
          ->count();
  for (int i = 0; i < numColours; i++) {
    if (!usedColours.contains(i))
      return i;
  }

  return 0;
}

/**
 * Removes the data currently selected in the table from the plot.
 */
void DataComparison::removeSelectedData() {
  QList<QTableWidgetItem *> selectedItems =
      m_uiForm.twCurrentData->selectedItems();

  while (!selectedItems.isEmpty()) {
    // Get the row number of the item
    int row = selectedItems[0]->row();

    // Get workspace name
    QString workspaceName =
        m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();

    if (m_diffWorkspaceNames.first == workspaceName ||
        m_diffWorkspaceNames.second == workspaceName) {
      clearDiff();
    }

    // Remove from data tabel
    m_uiForm.twCurrentData->removeRow(row);

    // Detach the old curve from the plot if it exists
    if (m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(nullptr);

    selectedItems = m_uiForm.twCurrentData->selectedItems();
  }

  // Replot the workspaces
  updatePlot();
}

/**
 * Removed all loaded data from the plot.
 */
void DataComparison::removeAllData() {
  clearDiff();

  int numRows = m_uiForm.twCurrentData->rowCount();
  for (int row = 0; row < numRows; row++) {
    // Get workspace name
    QString workspaceName =
        m_uiForm.twCurrentData->item(0, WORKSPACE_NAME)->text();

    // Remove from data tabel
    m_uiForm.twCurrentData->removeRow(0);

    // Detach the old curve from the plot if it exists
    if (m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(nullptr);
  }

  // Replot the workspaces
  workspaceIndexChanged();
}

/**
 * Replots the currently loaded workspaces.
 */
void DataComparison::plotWorkspaces() {
  int globalWsIndex = m_uiForm.sbSpectrum->value();
  int maxGlobalWsIndex = 0;

  int numRows = m_uiForm.twCurrentData->rowCount();
  for (int row = 0; row < numRows; row++) {
    // Get workspace
    QString workspaceName =
        m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();
    MatrixWorkspace_const_sptr workspace =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            workspaceName.toStdString());
    int numSpec = static_cast<int>(workspace->getNumberHistograms());

    // Calculate spectrum number
    QSpinBox *specOffsetSpin = dynamic_cast<QSpinBox *>(
        m_uiForm.twCurrentData->cellWidget(row, SPEC_OFFSET));
    int specOffset = specOffsetSpin->value();
    int wsIndex = globalWsIndex - specOffset;
    g_log.debug() << "Workspace index for workspace "
                  << workspaceName.toStdString() << " is " << wsIndex
                  << ", with offset " << specOffset << '\n';

    // See if this workspace extends the reach of the global spectrum selector
    int maxGlobalWsIndexForWs = numSpec + specOffset - 1;
    if (maxGlobalWsIndexForWs > maxGlobalWsIndex)
      maxGlobalWsIndex = maxGlobalWsIndexForWs;

    // Check the workspace index is in range
    if (wsIndex >= numSpec || wsIndex < 0) {
      g_log.debug() << "Workspace " << workspaceName.toStdString()
                    << ", workspace index out of range.\n";
      ;

      // Give "n/a" in current spectrum display
      m_uiForm.twCurrentData->item(row, CURRENT_SPEC)->setText(tr("n/a"));

      // Detech the curve from the plot
      if (m_curves.contains(workspaceName))
        m_curves[workspaceName]->attach(nullptr);

      continue;
    }

    // Update current spectrum display
    m_uiForm.twCurrentData->item(row, CURRENT_SPEC)
        ->setText(tr(std::to_string(wsIndex).c_str()));

    // Create the curve data
    const bool logScale(false), distribution(false);
    QwtWorkspaceSpectrumData wsData(*workspace, static_cast<int>(wsIndex),
                                    logScale, distribution);

    // Detach the old curve from the plot if it exists
    if (m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(nullptr);

    QComboBox *colourSelector = dynamic_cast<QComboBox *>(
        m_uiForm.twCurrentData->cellWidget(row, COLOUR));
    QColor curveColour =
        colourSelector->itemData(colourSelector->currentIndex())
            .value<QColor>();

    // Create a new curve and attach it to the plot
    auto curve = boost::make_shared<QwtPlotCurve>();
    curve->setData(wsData);
    curve->setPen(curveColour);
    curve->attach(m_plot);
    m_curves[workspaceName] = curve;
  }

  // Plot the diff
  plotDiffWorkspace();

  // Update the plot
  m_plot->replot();

  // Set the max value for global spectrum spin box
  m_uiForm.sbSpectrum->setMaximum(maxGlobalWsIndex);
  m_uiForm.sbSpectrum->setSuffix(" / " + QString::number(maxGlobalWsIndex));
}

/**
 * Normalises the workspace index offsets in the data table to zero.
 */
void DataComparison::normaliseSpectraOffsets() {
  m_uiForm.twCurrentData->blockSignals(true);

  int numRows = m_uiForm.twCurrentData->rowCount();
  int lowestOffset = INT_MAX;

  // Find the lowest offset in the data table
  for (int row = 0; row < numRows; row++) {
    QSpinBox *specOffsetSpin = dynamic_cast<QSpinBox *>(
        m_uiForm.twCurrentData->cellWidget(row, SPEC_OFFSET));
    int specOffset = specOffsetSpin->value();
    if (specOffset < lowestOffset)
      lowestOffset = specOffset;
  }

  // Subtract the lowest offset from all offsets to ensure at least one offset
  // is zero
  for (int row = 0; row < numRows; row++) {
    QSpinBox *specOffsetSpin = dynamic_cast<QSpinBox *>(
        m_uiForm.twCurrentData->cellWidget(row, SPEC_OFFSET));
    int specOffset = specOffsetSpin->value();
    specOffset -= lowestOffset;
    specOffsetSpin->setValue(specOffset);
  }

  m_uiForm.twCurrentData->blockSignals(false);
}

/**
 * Handles updating the plot, i.e. normalising offsets and replotting spectra.
 */
void DataComparison::updatePlot() {
  normaliseSpectraOffsets();
  plotWorkspaces();
}

/**
 * Handles a workspace index or offset being modified.
 */
void DataComparison::workspaceIndexChanged() {
  normaliseSpectraOffsets();
  plotWorkspaces();

  bool maintainZoom = m_uiForm.cbMaintainZoom->isChecked();
  if (!maintainZoom)
    resetView();
}

/**
 * Handles creating a diff of two workspaces and plotting it.
 */
void DataComparison::plotDiffWorkspace() {
  // Detach old curve
  if (m_diffCurve != nullptr)
    m_diffCurve->attach(nullptr);

  // Do nothing if there are not two workspaces
  if (m_diffWorkspaceNames.first.isEmpty() ||
      m_diffWorkspaceNames.second.isEmpty())
    return;

  // Get pointers to the workspaces to be diffed
  MatrixWorkspace_sptr ws1 =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_diffWorkspaceNames.first.toStdString());
  MatrixWorkspace_sptr ws2 =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_diffWorkspaceNames.second.toStdString());

  int ws1Spec = 0;
  int ws2Spec = 0;

  // Get the current spectrum for each workspace
  int numRows = m_uiForm.twCurrentData->rowCount();
  for (int row = 0; row < numRows; row++) {
    QString workspaceName =
        m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();
    QString currentSpecName =
        m_uiForm.twCurrentData->item(row, CURRENT_SPEC)->text();
    bool ok = false;
    bool found = false;

    if (workspaceName == m_diffWorkspaceNames.first) {
      ws1Spec = currentSpecName.toInt(&ok);
      found = true;
    }
    if (workspaceName == m_diffWorkspaceNames.second) {
      ws2Spec = currentSpecName.toInt(&ok);
      found = true;
    }

    // Check that the spectra are not out of range
    if (!ok && found) {
      // Set info message
      QString infoMessage = workspaceName + ": Index out of range.";
      m_uiForm.lbDiffInfo->setText(infoMessage);
      return;
    }
  }

  // Extract the current spectrum for both workspaces
  IAlgorithm_sptr extractWs1Alg =
      AlgorithmManager::Instance().create("ExtractSingleSpectrum");
  extractWs1Alg->setChild(true);
  extractWs1Alg->initialize();
  extractWs1Alg->setProperty("InputWorkspace", ws1);
  extractWs1Alg->setProperty("OutputWorkspace", "__ws1_spec");
  extractWs1Alg->setProperty("WorkspaceIndex", ws1Spec);
  extractWs1Alg->execute();
  MatrixWorkspace_sptr ws1SpecWs =
      extractWs1Alg->getProperty("OutputWorkspace");

  IAlgorithm_sptr extractWs2Alg =
      AlgorithmManager::Instance().create("ExtractSingleSpectrum");
  extractWs2Alg->setChild(true);
  extractWs2Alg->initialize();
  extractWs2Alg->setProperty("InputWorkspace", ws2);
  extractWs2Alg->setProperty("OutputWorkspace", "__ws2_spec");
  extractWs2Alg->setProperty("WorkspaceIndex", ws2Spec);
  extractWs2Alg->execute();
  MatrixWorkspace_sptr ws2SpecWs =
      extractWs2Alg->getProperty("OutputWorkspace");

  // Rebin the second workspace to the first
  // (needed for identical binning for Minus algorithm)
  IAlgorithm_sptr rebinAlg =
      AlgorithmManager::Instance().create("RebinToWorkspace");
  rebinAlg->setChild(true);
  rebinAlg->initialize();
  rebinAlg->setProperty("WorkspaceToRebin", ws2SpecWs);
  rebinAlg->setProperty("WorkspaceToMatch", ws1SpecWs);
  rebinAlg->setProperty("OutputWorkspace", "__ws2_spec_rebin");
  rebinAlg->execute();
  MatrixWorkspace_sptr rebinnedWs2SpecWs =
      rebinAlg->getProperty("OutputWorkspace");

  // Subtract the two extracted spectra
  IAlgorithm_sptr minusAlg = AlgorithmManager::Instance().create("Minus");
  minusAlg->setChild(true);
  minusAlg->initialize();
  minusAlg->setProperty("LHSWorkspace", ws1SpecWs);
  minusAlg->setProperty("RHSWorkspace", rebinnedWs2SpecWs);
  minusAlg->setProperty("OutputWorkspace", "__diff");
  minusAlg->execute();
  MatrixWorkspace_sptr diffWorkspace = minusAlg->getProperty("OutputWorkspace");

  // Create curve and add to plot
  QwtWorkspaceSpectrumData wsData(*diffWorkspace, 0, false, false);
  auto curve = boost::make_shared<QwtPlotCurve>();
  curve->setData(wsData);
  curve->setPen(QColor(Qt::green));
  curve->attach(m_plot);
  m_diffCurve = curve;

  // Set info message
  QString infoMessage =
      m_diffWorkspaceNames.first + "(" + QString::number(ws1Spec) + ") - " +
      m_diffWorkspaceNames.second + "(" + QString::number(ws2Spec) + ")";
  m_uiForm.lbDiffInfo->setText(infoMessage);
}

/**
 * Configures a diff of the two currently selected workspaces in the table to be
 *plotted
 * when plotWorkspaces is called.
 *
 * Does nothing if there are not 2 workspaces selected.
 */
void DataComparison::diffSelected() {
  QList<QTableWidgetItem *> selectedItems =
      m_uiForm.twCurrentData->selectedItems();
  QList<int> selectedRows;

  // Generate a list of selected row numbers
  for (auto it = selectedItems.begin(); it != selectedItems.end(); ++it) {
    int row = (*it)->row();
    if (!selectedRows.contains(row))
      selectedRows << (*it)->row();
  }

  // Check there is the correct number of selected items
  if (selectedRows.size() != 2) {
    g_log.error()
        << "Need to have exactly 2 workspaces selected for diff (have "
        << selectedRows.size() << ")\n";
    return;
  }

  // Record the workspace names
  m_diffWorkspaceNames = qMakePair(
      m_uiForm.twCurrentData->item(selectedRows[0], WORKSPACE_NAME)->text(),
      m_uiForm.twCurrentData->item(selectedRows[1], WORKSPACE_NAME)->text());

  // Update the plot
  plotWorkspaces();
}

/**
 * Removes the configured diff.
 */
void DataComparison::clearDiff() {
  // Clear the info message
  m_uiForm.lbDiffInfo->setText("No current diff.");

  // Remove the recorded diff workspace names
  m_diffWorkspaceNames = qMakePair(QString(), QString());

  // Update the plot
  plotWorkspaces();
}

/**
 * Toggles the pan plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void DataComparison::togglePan(bool enabled) {
  // First disbale the zoom tool
  if (enabled && m_uiForm.pbZoom->isChecked())
    m_uiForm.pbZoom->setChecked(false);

  g_log.debug() << "Pan tool enabled: " << enabled << '\n';

  m_panTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}

/**
 * Toggles the zoom plot tool.
 *
 * @param enabled If the tool should be enabled
 */
void DataComparison::toggleZoom(bool enabled) {
  // First disbale the pan tool
  if (enabled && m_uiForm.pbPan->isChecked())
    m_uiForm.pbPan->setChecked(false);

  g_log.debug() << "Zoom tool enabled: " << enabled << '\n';

  m_zoomTool->setEnabled(enabled);
  m_magnifyTool->setEnabled(enabled);
}

/**
 * Rests the zoom level to fit all curves on the plot.
 */
void DataComparison::resetView() {
  g_log.debug("Reset plot view");

  // Auto scale the axis
  m_plot->setAxisAutoScale(QwtPlot::xBottom);
  m_plot->setAxisAutoScale(QwtPlot::yLeft);

  // Set this as the default zoom level
  m_zoomTool->setZoomBase(true);
}

/**
 * Handles removing a workspace when it is deleted from ADS.
 *
 * @param wsName Name of the workspace being deleted
 * @param ws Pointer to the workspace
 */
void DataComparison::preDeleteHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  UNUSED_ARG(ws);
  QString oldWsName = QString::fromStdString(wsName);

  // Find the row in the data table for the workspace
  int numRows = m_uiForm.twCurrentData->rowCount();
  for (int row = 0; row < numRows; row++) {
    // Remove the row
    QString workspaceName =
        m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();
    if (workspaceName == oldWsName) {
      m_uiForm.twCurrentData->removeRow(row);
      break;
    }
  }

  // Detach the old curve from the plot if it exists
  if (m_curves.contains(oldWsName))
    m_curves[oldWsName]->attach(nullptr);

  // Update the plot
  plotWorkspaces();
}

/**
 * Handle a workspace being renamed.
 *
 * @param oldName Old name for the workspace
 * @param newName New name for the workspace
 */
void DataComparison::renameHandle(const std::string &oldName,
                                  const std::string &newName) {
  QString oldWsName = QString::fromStdString(oldName);

  // Find the row in the data table for the workspace
  int numRows = m_uiForm.twCurrentData->rowCount();
  for (int row = 0; row < numRows; row++) {
    // Rename the workspace in the data table
    QString workspaceName =
        m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();
    if (workspaceName == oldWsName) {
      m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)
          ->setText(QString::fromStdString(newName));
      break;
    }
  }

  // Detach the old curve from the plot if it exists
  if (m_curves.contains(oldWsName))
    m_curves[oldWsName]->attach(nullptr);

  // Update the plot
  plotWorkspaces();
}

/**
 * Handle replotting after a workspace has been changed.
 *
 * @param wsName Name of changed workspace
 * @param ws Pointer to changed workspace
 */
void DataComparison::afterReplaceHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  UNUSED_ARG(wsName);
  UNUSED_ARG(ws);

  // Update the plot
  plotWorkspaces();
}
