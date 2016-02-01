#include <iostream>
#include <iomanip>
#include "MantidQtCustomInterfaces/DynamicPDF/SliceSelector.h"
#include "MantidQtCustomInterfaces/DynamicPDF/BackgroundRemover.h"

#include "MantidQtAPI/HelpWindow.h"

namespace
{
  Mantid::Kernel::Logger g_log("DynamicPDF");
}


namespace MantidQt
{
namespace CustomInterfaces
{
namespace DynamicPDF
{

  WorkspaceRecord::WorkspaceRecord(const std::string &workspaceName):name(workspaceName){
    ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
  }

  void WorkspaceRecord::updateMetadata(const size_t &newIndex){
    energy = ws->getAxis(1)->getValue(newIndex);
    std::stringstream stream = "Energy = " << std::fixed << std::setprecision(2) << energy << " meV";
    std::string label = stream.str();
  }

  //Add this class to the list of specialised dialogs in this namespace
  DECLARE_SUBWINDOW(SliceSelector)

  ///Constructor
  SliceSelector::SliceSelector(QWidget *parent) : UserSubWindow(parent), m_loadedWorkspace(NULL), m_BackgroundRemover(NULL)
  {
  }

  /// Initialize the ui form and connect SIGNALS to SLOTS
  void SliceSelector::initLayout()
  {
    m_uiForm.setupUi(this);
    connect(m_uiForm.dataSelector, SIGNAL(dataReady(const QString&)),
            this, SLOT(loadedSlices(const QString&)));
    connect(m_uiForm.pushHelp, SIGNAL(clicked()), this, SLOT(showHelp()));
    connect(m_uiForm.spinboxSliceSelector, SIGNAL(valueChanged(int)),
            this, SLOT(updateSelectedSlice(int)));
    connect(m_uiForm.pushLaunchBackgroundRemover, SIGNAL(clicked()),
            this, SLOT(launchBackgroundRemover()));
  }

  ///
  void SliceSelector::loadedSlices(const QString &workspaceName){
      m_loadedWorkspace = boost::make_shared<WorkspaceRecord>(workspaceName);
      m_selectedWorkspaceIndex = 0;
      m_loadedWorkspace->updateMetadata(m_selectedWorkspaceIndex);
      size_t maximumWorkspaceIndex = m_loadedWorkspace->ws->getNumberHistograms() - 1;

      /// initialize the label displaying the energy
      m_uiForm.labelSliceEnergy->setText(QString(m_loadedWorkspace->label));

      /// initialize the spin box that selects the energy slice
      m_uiForm.spinBoxSliceSelector->setMinimum(0);
      m_uiForm.spinBoxSliceSelector->setMaximum(maximumWorkspaceIndex);
      m_uiForm.spinBoxSliceSelector->setValue(m_selectedWorkspaceIndex);

      /// initialize the preview plot
      updatePlotSelectedSlice();
  }

  ///
  void SliceSelector::updatePlotSelectedSlice(){
    m_uiForm.previewPlotSelectedSlice->clear();
    m_uiForm.previewPlotSelectedSlice->addSpectrum(m_loadedWorkspace->label,
                                                   m_loadedWorkspace->ws,
                                                   m_selectedWorkspaceIndex);
  }

  /// Update all widgets in the form with the new selected index
  void SliceSelector::updateSelectedSlice(const int &newSelectedIndex){
    m_selectedWorkspaceIndex = newSelectedIndex;
    m_loadedWorkspace->updateMetadata(m_selectedWorkspaceIndex);
    m_uiForm.labelSliceEnergy->setText(m_loadedWorkspace->label);
    updatePlotSelectedSlice();
  }

  /// Initialize and/or update the dialog to remove the multiphonon background
  void SliceSelector::launchBackgroundRemover(){
    /// parent of BackgroundRemover is this main window
    if (!m_BackgroundRemover){
      m_BackgroundRemover = boost::make_shared<BackgroundRemover>(this);
    }
    m_BackgroundRemover->refreshSlice(m_loadedWorkspace, m_selectedWorkspaceIndex);
  }

  /// Qt-help page
  void SliceSelector::showHelp()
  {
    MantidQt::API::HelpWindow::showCustomInterface(NULL, QString("Dynamic PDF Calculator"));
  }

}
}
}





