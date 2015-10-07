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
  DECLARE_DIALOG(LOQScriptInputDialog)
}
}

// Just to save writing this everywhere 
using namespace MantidQt::CustomDialogs;

//---------------------------------------
// Public member functions
//---------------------------------------

/// Constructor
LOQScriptInputDialog::LOQScriptInputDialog(QWidget* parent) :
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

  fillLineEdit("SampleWorkspace", m_uiForm.sampleBox);
  fillLineEdit("EmptyCanWorkspace", m_uiForm.emptycanBox);
  fillLineEdit("TransmissionSampleWorkspace", m_uiForm.transSampleBox);
  fillLineEdit("TransmissionDirectWorkspace", m_uiForm.transDirectBox);
  fillLineEdit("TransmissionEmptyCanWorkspace", m_uiForm.transEmptyBox);
  
  fillLineEdit("Radius_min", m_uiForm.radMinBox);
  fillLineEdit("Radius_max", m_uiForm.radMaxBox);

  fillLineEdit("Wavelength_min", m_uiForm.wavMinBox);
  fillLineEdit("Wavelength_max", m_uiForm.wavMaxBox);
  fillLineEdit("Wavelength_delta", m_uiForm.wavBinBox);

  fillLineEdit("Q_min", m_uiForm.momMinBox);
  fillLineEdit("Q_max", m_uiForm.momMaxBox);
  fillLineEdit("Q_delta", m_uiForm.momBinBox);

  fillLineEdit("Beam_Centre_X", m_uiForm.beamXBox);
  fillLineEdit("Beam_Centre_Y", m_uiForm.beamYBox);
  
  //Efficiency correction
  fillLineEdit("EfficiencyCorrectionFile", m_uiForm.effFileBox);

  m_uiForm.sampleBox->setFocus();
  
}

/**
 * Retrieve the input from the dialog
 */
void LOQScriptInputDialog::parseInput()
{
  // All elements within the dialog are public memebers of the LOQScriptInputDialog class
  // and have the names given to them in Qt Designer
  
  //Simply access each widget and use storePropertyValue( propName, m_uiForm->propValue)    
  //method of the AlgorithmDialog base class to add the input value for this property

  storePropertyValue("SampleWorkspace", m_uiForm.sampleBox->text());
  storePropertyValue("EmptyCanWorkspace", m_uiForm.emptycanBox->text());
  storePropertyValue("TransmissionSampleWorkspace", m_uiForm.transSampleBox->text());
  storePropertyValue("TransmissionDirectWorkspace", m_uiForm.transDirectBox->text());
  storePropertyValue("TransmissionEmptyCanWorkspace", m_uiForm.transEmptyBox->text());
  
  storePropertyValue("Radius_min", m_uiForm.radMinBox->text());
  storePropertyValue("Radius_max", m_uiForm.radMaxBox->text());

  storePropertyValue("Wavelength_min", m_uiForm.wavMinBox->text());
  storePropertyValue("Wavelength_max", m_uiForm.wavMaxBox->text());
  storePropertyValue("Wavelength_delta", m_uiForm.wavBinBox->text());

  storePropertyValue("Q_min", m_uiForm.momMinBox->text());
  storePropertyValue("Q_max", m_uiForm.momMaxBox->text());
  storePropertyValue("Q_delta", m_uiForm.momBinBox->text());

  storePropertyValue("Beam_Centre_X", m_uiForm.beamXBox->text());
  storePropertyValue("Beam_Centre_Y", m_uiForm.beamYBox->text());
  
  //Efficiency correction
  storePropertyValue("EfficiencyCorrectionFile", m_uiForm.effFileBox->text());

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

  QString filepath = this->openFileDialog("EfficiencyCorrectionFile");
  if( !filepath.isEmpty() ) m_uiForm.effFileBox->setText(filepath);
}

