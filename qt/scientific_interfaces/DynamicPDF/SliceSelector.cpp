// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <iomanip>
// includes for workspace handling
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"
// includes for interface
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "SliceSelector.h"
#include <qwt_plot_spectrogram.h>
// Mantid headers from other projects
#include "MantidKernel/UsageService.h"

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/**
 * @brief Constructor
 * @param workspaceName retrieve the workspace with the Analysis Data Service
 */
WorkspaceRecord::WorkspaceRecord(const std::string &workspaceName)
    : m_name{workspaceName}, m_energy{0.0}, m_label() {
  m_ws = Mantid::API::AnalysisDataService::Instance()
             .retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
}

/**
 * @brief Destructor. Reset the pointer to the workspace
 */
WorkspaceRecord::~WorkspaceRecord() { m_ws.reset(); }

void WorkspaceRecord::updateMetadata(const size_t &newIndex) {
  m_energy = m_ws->getAxis(1)->getValue(newIndex);
  std::stringstream energyLabelStream;
  energyLabelStream << std::fixed;
  energyLabelStream.precision(2);
  energyLabelStream << "Energy = " << m_energy;
  energyLabelStream << " meV";
  m_label = energyLabelStream.str();
}

/**
 * @brief Find out minimum and maximum energies in the loaded workspace
 * @return (minimum, maximum) as std::pair
 */
std::pair<double, double> WorkspaceRecord::getErange() {
  auto minimum = m_ws->getAxis(1)->getMin();
  auto maximum = m_ws->getAxis(1)->getMax();
  return std::pair<double, double>(minimum, maximum);
}

/*        **********************
 *        **  Public Members  **
 *        **********************/

/**
 * @brief Constructor
 */
SliceSelector::SliceSelector(QWidget *parent)
    : QMainWindow(parent), m_pickerLine{nullptr},
      m_loadedWorkspace(), m_selectedWorkspaceIndex{0} {
  this->observePreDelete(true); // Subscribe to notifications
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      "Feature", "DynamicPDF->SliceSelector", false);
  this->initLayout();
}

SliceSelector::~SliceSelector() {
  this->observePreDelete(false); // Cancel subscription to notifications
  delete m_pickerLine;
}

/*              *************************
 *              **  Protected Members  **
 *              *************************/

/**
 * @brief Actions when slices workspace is deleted
 */
void SliceSelector::preDeleteHandle(
    const std::string &workspaceName,
    const boost::shared_ptr<Mantid::API::Workspace> workspace) {
  UNUSED_ARG(workspaceName);
  Mantid::API::MatrixWorkspace_sptr ws =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace);
  if (!ws || (ws != m_loadedWorkspace->m_ws)) {
    return;
  }

  // Clean the 2D view
  m_pickerLine->setVisible(false);
  // Clean the 1D view (automatically taken care by the underlying PreviewPlot
  // objet)
  // Clean the rest of the widgets
  m_uiForm.labelSliceEnergy->setText(QString::fromStdString("Energy = NAN"));
  // Clean the data structure
  m_selectedWorkspaceIndex = 0;
  this->tearConnections(); // prevent unwanted signaling from the spin box
  m_uiForm.spinboxSliceSelector->setValue(0);
  m_loadedWorkspace.reset();
}

/*        *********************
 *        **  Private Slots  **
 *        *********************/

/**
 * @brief Load file or workspace, then initialize the widgets
 * @param workspaceName
 */
void SliceSelector::loadSlices(const QString &workspaceName) {
  m_loadedWorkspace =
      Mantid::Kernel::make_unique<WorkspaceRecord>(workspaceName.toStdString());
  /// don't process if workspace is not valid
  if (!this->isWorkspaceValid()) {
    return;
  }
  m_selectedWorkspaceIndex = 0;
  m_loadedWorkspace->updateMetadata(m_selectedWorkspaceIndex);
  int maximumWorkspaceIndex =
      static_cast<int>(m_loadedWorkspace->m_ws->getNumberHistograms()) - 1;

  /// initialize the label displaying the energy
  m_uiForm.labelSliceEnergy->setText(
      QString::fromStdString(m_loadedWorkspace->m_label));

  /// initialize the spin box that selects the energy slice
  m_uiForm.spinboxSliceSelector->setMinimum(0);
  m_uiForm.spinboxSliceSelector->setMaximum(maximumWorkspaceIndex);
  m_uiForm.spinboxSliceSelector->setValue(0);
  m_uiForm.spinboxSliceSelector->setSingleStep(1);

  /// show the slice picker
  this->initPickerLine();

  /// initialize the 2D view of the slices;
  m_uiForm.slices2DPlot->setWorkspace(m_loadedWorkspace->m_ws);
  m_uiForm.slices2DPlot->updateDisplay();

  /// initialize the 1D PreviewPlot widget
  updatePreviewPlotSelectedSlice();

  this->setupConnections();
  emit signalSlicesLoaded(workspaceName);
}

/**
 * @brief Refresh the slice showing in the 1D plot
 */
void SliceSelector::updatePreviewPlotSelectedSlice() {
  m_uiForm.previewPlotSelectedSlice->clear();
  m_uiForm.previewPlotSelectedSlice->addSpectrum(
      QString::fromStdString(m_loadedWorkspace->m_label),
      m_loadedWorkspace->m_ws, m_selectedWorkspaceIndex);
}

/**
 * @brief Update all widgets in the form with the new selected index
 * @param newSelectedIndex
 */
void SliceSelector::updateSelectedSlice(const int &newSelectedIndex) {
  m_selectedWorkspaceIndex = static_cast<size_t>(newSelectedIndex);
  /// Check  pointer m_loadedWorkspace because the user may attemp to manipulate
  /// the widgets before (s)he loads any data
  if (m_loadedWorkspace) {
    m_loadedWorkspace->updateMetadata(m_selectedWorkspaceIndex);
    m_uiForm.labelSliceEnergy->setText(
        QString::fromStdString(m_loadedWorkspace->m_label));
    m_uiForm.spinboxSliceSelector->setValue(newSelectedIndex);
    this->updatePickerLine();
    this->updatePreviewPlotSelectedSlice();
  }
}

/**
 * @brief Update widgets when pickerLine is manually changed. Do not update if
 * the pickerLine moved so little that it did not positioned over a different
 * slice.
 * @param newEnergySelected the new energy retrieved from the pickerLine
 */
void SliceSelector::newIndexFromPickedEnergy(const double &newEnergySelected) {
  auto axis = m_loadedWorkspace->m_ws->getAxis(1);
  auto newSelectedIndex = axis->indexOfValue(newEnergySelected);
  if (m_selectedWorkspaceIndex != newSelectedIndex) {
    updateSelectedSlice(static_cast<int>(newSelectedIndex));
  }
}

/**
 * @brief Update the position of the picker line as a response to changes in the
 * SliceSelector, unless the energy being pointed to corresponds to the current
 * index.
 */
void SliceSelector::updatePickerLine() {
  auto energyBeingPointedTo = m_pickerLine->getMinimum();
  auto axis = m_loadedWorkspace->m_ws->getAxis(1);
  auto indexBeingPointedTo = axis->indexOfValue(energyBeingPointedTo);
  if (m_selectedWorkspaceIndex != indexBeingPointedTo) {
    m_pickerLine->setMinimum(m_loadedWorkspace->m_energy);
  }
}

/**
 * @brief Public broadcast of the slice that user selected for fitting
 */
void SliceSelector::selectSliceForFitting() {
  if (m_loadedWorkspace) {
    emit this->signalSliceForFittingSelected(m_selectedWorkspaceIndex);
  }
}

/**
 * @brief Opens the Qt help page for the interface
 */
void SliceSelector::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Dynamic PDF Background Remover"));
}

/*        ***********************
 *        **  Private Members  **
 *        ***********************/

/**
 * @brief Initialize UI form, spawn picker line, connect SIGNALS/SLOTS
 */
void SliceSelector::initLayout() {
  m_uiForm.setupUi(this);
  this->spawnPickerLine();
  // user wants help
  connect(m_uiForm.buttonpushHelp, SIGNAL(clicked()), this, SLOT(showHelp()));
  // user wants to fit the selected slice with the Background remover
  connect(m_uiForm.pushButtonFit, SIGNAL(clicked()), this,
          SLOT(selectSliceForFitting()));
  // user has loaded slices from a workspace or file
  connect(m_uiForm.dataSelector, SIGNAL(dataReady(const QString &)), this,
          SLOT(loadSlices(const QString &)));
  this->setupConnections();
}

/**
 * @brief Establish signals/connections between widgets components
 */
void SliceSelector::setupConnections() {
  // user is selecting a slice with the spin box
  connect(m_uiForm.spinboxSliceSelector, SIGNAL(valueChanged(int)), this,
          SLOT(updateSelectedSlice(int)));
  // user is selecting a slice with the picker line
  connect(m_pickerLine, SIGNAL(minValueChanged(double)), this,
          SLOT(newIndexFromPickedEnergy(double)));
}

/**
 * @brief disconnect the signals/connections established
 *  in setupConnections
 */
void SliceSelector::tearConnections() {
  // user is selecting a slice with the spin box
  disconnect(m_uiForm.spinboxSliceSelector, SIGNAL(valueChanged(int)), this,
             SLOT(updateSelectedSlice(int)));
  // user is selecting a slice with the picker line
  disconnect(m_pickerLine, SIGNAL(minValueChanged(double)), this,
             SLOT(newIndexFromPickedEnergy(double)));
}

/**
 * @brief Allocate the slice selector in the 2D view. No workspace loading is
 * necessary.
 */
void SliceSelector::spawnPickerLine() {
  auto qwtplot = m_uiForm.slices2DPlot->getPlot2D();
  bool isVisible{false};
  m_pickerLine = new MantidWidgets::RangeSelector(
      qwtplot, MantidWidgets::RangeSelector::YSINGLE, isVisible);
  m_pickerLine->setColour(QColor(Qt::black));
}

/**
 * @brief Initialize the picker line with default options after workspace is
 * loaded.
 */
void SliceSelector::initPickerLine() {
  auto eRange = m_loadedWorkspace->getErange();
  m_pickerLine->setRange(eRange);
  m_pickerLine->setMinimum(eRange.first);
  m_pickerLine->setMaximum(eRange.second);
  m_pickerLine->setVisible(true);
}

/**
 * @brief Check for correct units and workspace type
 */
bool SliceSelector::isWorkspaceValid() {
  /// check the pointer to the workspace is not empty
  if (!m_loadedWorkspace->m_ws) {
    auto title = this->objectName();
    auto error =
        QString::fromStdString("Workspace must be of type MatrixWorkspace");
    QMessageBox::warning(this, title, error);
    return false;
  }
  /// check the units of the "X-axis" is momentum transfer
  auto axis = m_loadedWorkspace->m_ws->getAxis(0);
  if (axis->unit()->unitID() != "MomentumTransfer") {
    auto title = this->objectName();
    auto error =
        QString::fromStdString("X-axis units must be momentum transfer");
    QMessageBox::warning(this, title, error);
    return false;
  }
  /// check the units of the "vertical" dimension is energy transfer
  axis = m_loadedWorkspace->m_ws->getAxis(1);
  if (axis->unit()->unitID() != "DeltaE") {
    auto title = this->objectName();
    auto error =
        QString::fromStdString("Y-axis units must be energy transfer (meV)");
    QMessageBox::warning(this, title, error);
    return false;
  }
  return true;
}
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
