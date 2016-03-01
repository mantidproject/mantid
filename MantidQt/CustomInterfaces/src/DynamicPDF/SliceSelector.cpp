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

// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(SliceSelector)

/// Constructor
// SliceSelector::SliceSelector(QWidget *parent) : UserSubWindow{parent},
// m_loadedWorkspace{nullptr}, m_BackgroundRemover{nullptr} {
SliceSelector::SliceSelector(QWidget *parent) :
  UserSubWindow{parent},
  m_loadedWorkspace(),
  m_selectedWorkspaceIndex{0} {

}

SliceSelector::~SliceSelector() { m_selectedWorkspaceIndex = 0; }

/// Initialize the ui form and connect SIGNALS to SLOTS
void SliceSelector::initLayout() {
  m_uiForm.setupUi(this);
  connect(m_uiForm.dataSelector, SIGNAL(dataReady(const QString &)), this,
          SLOT(loadSlices(const QString &)));
  connect(m_uiForm.pushHelp, SIGNAL(clicked()), this, SLOT(showHelp()));
  connect(m_uiForm.spinboxSliceSelector, SIGNAL(valueChanged(int)), this,
          SLOT(updateSelectedSlice(int)));
  connect(m_uiForm.sliderSelectSlice, SIGNAL(valueChanged(int)), this,
          SLOT(updateSelectedSlice(int)));
  connect(m_uiForm.pushLaunchBackgroundRemover, SIGNAL(clicked()), this,
          SLOT(launchBackgroundRemover()));
}

/// Load file or workspace containing energy slices
void SliceSelector::loadSlices(const QString &workspaceName) {
  m_loadedWorkspace =
      boost::make_shared<WorkspaceRecord>(workspaceName.toStdString());
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

  /// initialize the slider next to the 2D view
  m_uiForm.sliderSelectSlice->setMinimum(0);
  m_uiForm.sliderSelectSlice->setMaximum(maximumWorkspaceIndex);
  m_uiForm.spinboxSliceSelector->setValue(0);

  /// initialize the 2D view of the slices;
  m_uiForm.slices2DPlot->setWorkspace(m_loadedWorkspace->m_ws);
  m_uiForm.slices2DPlot->updateDisplay();

  /// initialize the preview plot
  updatePlotSelectedSlice();
}

///
void SliceSelector::updatePlotSelectedSlice() {
  m_uiForm.previewPlotSelectedSlice->clear();
  m_uiForm.previewPlotSelectedSlice->addSpectrum(
      QString::fromStdString(m_loadedWorkspace->m_label),
      m_loadedWorkspace->m_ws, m_selectedWorkspaceIndex);
}

/// Update all widgets in the form with the new selected index
void SliceSelector::updateSelectedSlice(const int &newSelectedIndex) {
  m_selectedWorkspaceIndex = static_cast<size_t>(newSelectedIndex);
  /// Check  pointer m_loadedWorkspace because the user may attemp to manipulate
  /// the widgets before (s)he loads any data
  if (m_loadedWorkspace) {
    m_loadedWorkspace->updateMetadata(m_selectedWorkspaceIndex);
    m_uiForm.labelSliceEnergy->setText(
        QString::fromStdString(m_loadedWorkspace->m_label));
    m_uiForm.spinboxSliceSelector->setValue(newSelectedIndex);
    m_uiForm.sliderSelectSlice->setValue(newSelectedIndex);
    updatePlotSelectedSlice();
  }
}

/// Initialize and/or update the dialog to remove the multiphonon background
void SliceSelector::launchBackgroundRemover() {
  /// parent of BackgroundRemover is this main window
  // if (!m_BackgroundRemover){
  //  m_BackgroundRemover = boost::make_shared<BackgroundRemover>(this);
  //}
  // m_BackgroundRemover->refreshSlice(m_loadedWorkspace,
  // m_selectedWorkspaceIndex);
  g_log.error("Not implemented...yet");
}

/// Opens the Qt help page for the interface
void SliceSelector::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("Dynamic PDF Calculator"));
}
}
}
}
