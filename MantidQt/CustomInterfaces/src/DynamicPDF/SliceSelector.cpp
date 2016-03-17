#include <iostream>
#include <iomanip>
// includes for workspace handling
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
// includes for interface
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/DynamicPDF/SliceSelector.h"
#include <qwt_plot_spectrogram.h>

//#include "MantidQtCustomInterfaces/DynamicPDF/BackgroundRemover.h"

namespace {
Mantid::Kernel::Logger g_log("DynamicPDF");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

WorkspaceRecord::WorkspaceRecord(const std::string &workspaceName) :
  m_name{workspaceName},
  m_energy{0.0},
  m_label() {
  m_ws = Mantid::API::AnalysisDataService::Instance()
             .retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
}

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
std::pair<double,double> WorkspaceRecord::getErange(){
  auto minimum = m_ws->getAxis(1)->getMin();
  auto maximum = m_ws->getAxis(1)->getMax();
  return std::pair<double,double>(minimum, maximum);
}

// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(SliceSelector)

/// Constructor
// SliceSelector::SliceSelector(QWidget *parent) : UserSubWindow{parent},
// m_loadedWorkspace{nullptr}, m_BackgroundRemover{nullptr} {
SliceSelector::SliceSelector(QWidget *parent) :
  UserSubWindow{parent},
  m_pickerLine{nullptr},
  m_loadedWorkspace(),
  m_selectedWorkspaceIndex{0} {

}

SliceSelector::~SliceSelector() {
  delete m_pickerLine;
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
      boost::make_shared<WorkspaceRecord>(workspaceName.toStdString());
  /// don't process if workspace is not valid
  if(!this->isWorkspaceValid()){
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
void SliceSelector::newIndexFromPickedEnergy(const double &newEnergySelected){
  auto axis = m_loadedWorkspace->m_ws->getAxis(1);
  auto newSelectedIndex = axis->indexOfValue(newEnergySelected);
  if(m_selectedWorkspaceIndex != newSelectedIndex){
    updateSelectedSlice(static_cast<int>(newSelectedIndex));
  }
}

/**
 * @brief Update the position of the picker line as a response to changes in the
 * SliceSelector, unless the energy being pointed to corresponds to the current index.
 */
void SliceSelector::updatePickerLine(){
  auto energyBeingPointedTo = m_pickerLine->getMinimum();
  auto axis = m_loadedWorkspace->m_ws->getAxis(1);
  auto indexBeingPointedTo = axis->indexOfValue(energyBeingPointedTo);
  if(m_selectedWorkspaceIndex != indexBeingPointedTo){
    m_pickerLine->setMinimum(m_loadedWorkspace->m_energy);
  }
}

/**
 * @brief Initialize and/or update the dialog to remove the multiphonon background
 */
void SliceSelector::launchBackgroundRemover() {
  /// parent of BackgroundRemover is this main window
  // if (!m_BackgroundRemover){
  //  m_BackgroundRemover = boost::make_shared<BackgroundRemover>(this);
  //}
  // m_BackgroundRemover->refreshSlice(m_loadedWorkspace,
  // m_selectedWorkspaceIndex);
  auto title = QString::fromStdString(this->name());
  auto error = QString::fromStdString("Not so fast, cowboy! (not implemented)");
  QMessageBox::warning(this, title, error);
}

/**
 * @brief Opens the Qt help page for the interface
 */
void SliceSelector::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("DynamicPDFSliceSelector"));
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
  connect(m_uiForm.dataSelector, SIGNAL(dataReady(const QString &)), this,
          SLOT(loadSlices(const QString &)));
  connect(m_uiForm.buttonpushHelp, SIGNAL(clicked()), this, SLOT(showHelp()));
  connect(m_uiForm.spinboxSliceSelector, SIGNAL(valueChanged(int)), this,
          SLOT(updateSelectedSlice(int)));
  connect(m_uiForm.pushLaunchBackgroundRemover, SIGNAL(clicked()), this,
          SLOT(launchBackgroundRemover()));
  connect(m_pickerLine, SIGNAL(minValueChanged(double)), this,
          SLOT(newIndexFromPickedEnergy(double)));
}

/**
 * @brief Allocate the slice selector in the 2D view. No workspace loading is neccessary.
 */
void SliceSelector::spawnPickerLine(){
  auto qwtplot = m_uiForm.slices2DPlot->getPlot2D();
  bool isVisible{false};
  m_pickerLine = new MantidWidgets::RangeSelector(
      qwtplot, MantidWidgets::RangeSelector::YSINGLE, isVisible);
  //    Mantid::Kernel::make_unique<MantidWidgets::RangeSelector>(
  //        qwtplot, MantidWidgets::RangeSelector::YSINGLE, isVisible);
  m_pickerLine->setColour(QColor(Qt::black));
}

/**
 * @brief Initialize the picker line with default options after workspace is loaded.
 */
void SliceSelector::initPickerLine(){
  auto eRange = m_loadedWorkspace->getErange();
  m_pickerLine->setRange(eRange);
  m_pickerLine->setMinimum(eRange.first);
  m_pickerLine->setMaximum(eRange.second);

  m_pickerLine->setVisible(true);
}


/**
 * @brief Check for correct units and workspace type
 */
bool SliceSelector::isWorkspaceValid(){
  /// check the pointer to the workspace is not empty
  if(!m_loadedWorkspace->m_ws){
    auto title = QString::fromStdString(this->name());
    auto error = QString::fromStdString("Workspace must be of type MatrixWorkspace");
    QMessageBox::warning(this, title, error);
    return false;
  }
  /// check the units of the "X-axis" is momentum transfer
  auto axis = m_loadedWorkspace->m_ws->getAxis(0);
  if (axis->unit()->unitID() != "MomentumTransfer") {
    auto title = QString::fromStdString(this->name());
    auto error = QString::fromStdString("X-axis units must be momentum transfer");
    QMessageBox::warning(this, title, error);
    return false;
  }
  /// check the units of the "vertical" dimension is energy transfer
  axis = m_loadedWorkspace->m_ws->getAxis(1);
  if (axis->unit()->unitID() != "DeltaE") {
    auto title = QString::fromStdString(this->name());
    auto error = QString::fromStdString("Y-axis units must be energy transfer (meV)");
    QMessageBox::warning(this, title, error);
    return false;
  }
  return true;
}




}
}
}
