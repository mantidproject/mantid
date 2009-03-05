//---------------------------
// Includes
//--------------------------
#include "MantidQtCustomDialogs/LOQScriptInputDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include <QFileInfo>
#include <QDir>



//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(LOQScriptInputDialog);
}
}

// Just to save writing this everywhere 
using namespace MantidQt::CustomDialogs;

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
LOQScriptInputDialog::LOQScriptInputDialog(MantidQt::API::AlgorithmDialog* parent) :
  AlgorithmDialog(parent)
{
}

/**
 * Set up the dialog
 */
void LOQScriptInputDialog::initLayout()
{
  m_uiForm.setupUi(this);
  
  connect( m_uiForm.browseButton, SIGNAL(clicked()), this, SLOT(browseClicked()) );

  setOldLineEditInput("SampleWorkspace", m_uiForm.sampleBox);
  setOldLineEditInput("EmptyCanWorkspace", m_uiForm.emptycanBox);
  setOldLineEditInput("TransmissionSampleWorkspace", m_uiForm.transSampleBox);
  setOldLineEditInput("TransmissionDirectWorkspace", m_uiForm.transDirectBox);
  setOldLineEditInput("TransmissionEmptyCanWorkspace", m_uiForm.transEmptyBox);
  
  setOldLineEditInput("Radius_min", m_uiForm.radMinBox);
  setOldLineEditInput("Radius_max", m_uiForm.radMaxBox);

  setOldLineEditInput("Wavelength_min", m_uiForm.wavMinBox);
  setOldLineEditInput("Wavelength_max", m_uiForm.wavMaxBox);
  setOldLineEditInput("Wavelength_delta", m_uiForm.wavBinBox);

  setOldLineEditInput("Q_min", m_uiForm.momMinBox);
  setOldLineEditInput("Q_max", m_uiForm.momMaxBox);
  setOldLineEditInput("Q_delta", m_uiForm.momBinBox);

  setOldLineEditInput("Beam_Centre_X", m_uiForm.beamXBox);
  setOldLineEditInput("Beam_Centre_Y", m_uiForm.beamYBox);
  
  //Efficiency correction
  setOldLineEditInput("EfficiencyCorrectionFile", m_uiForm.effFileBox);

  m_uiForm.sampleBox->setFocus();
  
}

/**
 * Retrieve the input from the dialog
 */
void LOQScriptInputDialog::parseInput()
{
  // All elements within the dialog are public memebers of the LOQScriptInputDialog class
  // and have the names given to them in Qt Designer
  
  //Simply access each widget and use addPropertyValueToMap( propName, m_uiForm->propValue)    
  //method of the AlgorithmDialog base class to add the input value for this property

  addPropertyValueToMap("SampleWorkspace", m_uiForm.sampleBox->text());
  addPropertyValueToMap("EmptyCanWorkspace", m_uiForm.emptycanBox->text());
  addPropertyValueToMap("TransmissionSampleWorkspace", m_uiForm.transSampleBox->text());
  addPropertyValueToMap("TransmissionDirectWorkspace", m_uiForm.transDirectBox->text());
  addPropertyValueToMap("TransmissionEmptyCanWorkspace", m_uiForm.transEmptyBox->text());
  
  addPropertyValueToMap("Radius_min", m_uiForm.radMinBox->text());
  addPropertyValueToMap("Radius_max", m_uiForm.radMaxBox->text());

  addPropertyValueToMap("Wavelength_min", m_uiForm.wavMinBox->text());
  addPropertyValueToMap("Wavelength_max", m_uiForm.wavMaxBox->text());
  addPropertyValueToMap("Wavelength_delta", m_uiForm.wavBinBox->text());

  addPropertyValueToMap("Q_min", m_uiForm.momMinBox->text());
  addPropertyValueToMap("Q_max", m_uiForm.momMaxBox->text());
  addPropertyValueToMap("Q_delta", m_uiForm.momBinBox->text());

  addPropertyValueToMap("Beam_Centre_X", m_uiForm.beamXBox->text());
  addPropertyValueToMap("Beam_Centre_Y", m_uiForm.beamYBox->text());
  
  //Efficiency correction
  addPropertyValueToMap("EfficiencyCorrectionFile", m_uiForm.effFileBox->text());

}

/**
  * A slot for the browse button "clicked" signal
  */
void LOQScriptInputDialog::browseClicked()
{
  if( !m_uiForm.effFileBox->text().isEmpty() )
  {
    QString dir = QFileInfo(m_uiForm.effFileBox->text()).absoluteDir().path();
    MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(dir);
  }  

  QString filepath = this->openLoadFileDialog("EfficiencyCorrectionFile");
  if( !filepath.isEmpty() ) m_uiForm.effFileBox->setText(filepath);
}

