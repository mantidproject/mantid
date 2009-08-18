//----------------------
// Includes
//----------------------
#include "MantidQtCustomDialogs/DiagScriptInputDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include <boost/lexical_cast.hpp>
#include <string>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomDialogs
{
  DECLARE_DIALOG(DiagScriptInputDialog);
}
}

using namespace MantidQt::CustomDialogs;

//----------------------
// Public member functions
//----------------------
///Constructor
DiagScriptInputDialog::DiagScriptInputDialog(QWidget *parent) :
  AlgorithmDialog(parent)
{
}

/// Set up the dialog layout
void DiagScriptInputDialog::initLayout()
{
  m_uiForm.setupUi(this);

  // for each property fill up any comboboxes and display previously selected values
  fillLineEdit("OutputFile", m_uiForm.leOFile);
  fillLineEdit("SignificanceTest", m_uiForm.leSignificance);
  fillAndSetComboBox("WBVanadium1", m_uiForm.cbWBV1);

  fillLineEdit("HighAbsolute", m_uiForm.leHighAbs);
  fillLineEdit("LowAbsolute", m_uiForm.leLowAbs);
  fillLineEdit("HighMedian", m_uiForm.leHighMed);
  fillLineEdit("LowMedian", m_uiForm.leHighMed);

  // the second workspace combobox will have contain all the entries from the first workspace list
  m_uiForm.cbWBV2->addItem("");
  for(int i = 0; i < m_uiForm.cbWBV1->count(); i++ )
  {
    m_uiForm.cbWBV2->addItem(m_uiForm.cbWBV1->itemText(i));
  }
  fillLineEdit("Variation", m_uiForm.leVariation);

  // copy workspace names again
  fillAndSetComboBox("Experimental", m_uiForm.cbExper);
  m_uiForm.cbExper->addItem("");
  for(int i = 0; i < m_uiForm.cbWBV1->count(); i++ )
  {
    m_uiForm.cbExper->addItem(m_uiForm.cbWBV1->itemText(i));
  }
  setCheckBoxState( "RemoveZero", m_uiForm.ckZeroCounts );
  setCheckBoxState( "MaskExper", m_uiForm.ckMaskExper );
  fillLineEdit("BackgroundAccept", m_uiForm.leAcceptance);
  fillLineEdit("RangeLower", m_uiForm.leStartTime);
  fillLineEdit("RangeUpper", m_uiForm.leEndTime);

  // add the standard buttons to the bottom of the form
  QLayout *mainlay = layout();
  QGridLayout *grid = qobject_cast<QGridLayout*>(mainlay);
  grid->addLayout(createDefaultButtonLayout("?", "Run", "Cancel"), grid->rowCount(), 0);

  addValidatorLabels();
}

/// Parse input when the Run button is pressed
void DiagScriptInputDialog::parseInput()
{
  storePropertyValue( "OutputFile", m_uiForm.leOFile->text() );
  storePropertyValue( "SignificanceTest", m_uiForm.leSignificance->text() );
  storePropertyValue( "WBVanadium1", m_uiForm.cbWBV1->currentText() );
  storePropertyValue( "HighAbsolute", m_uiForm.leHighAbs->text() );
  storePropertyValue( "LowAbsolute", m_uiForm.leLowAbs->text() );
  storePropertyValue( "HighMedian", m_uiForm.leHighMed->text() );
  storePropertyValue( "LowMedian", m_uiForm.leHighMed->text() );

  storePropertyValue( "WBVanadium2", m_uiForm.cbWBV2->currentText() );
  storePropertyValue( "Variation", m_uiForm.leVariation->text() );

  storePropertyValue( "Experimental", m_uiForm.cbExper->currentText() );
  storePropertyValue( "RemoveZero", 
    m_uiForm.ckZeroCounts->isChecked() ? "1" : "0" );
  storePropertyValue( "MaskExper",
    m_uiForm.ckMaskExper->isChecked() ? "1" : "0" );
  storePropertyValue( "BackgroundAccept", m_uiForm.leAcceptance->text() );
  storePropertyValue( "RangeLower", m_uiForm.leStartTime->text() );
  storePropertyValue( "RangeUpper", m_uiForm.leEndTime->text() );
}

void DiagScriptInputDialog::addValidatorLabels()
{
  // work on the Individual White Beam Tests groupbox
  QLayout *currentLayout = m_uiForm.gbIndividual->layout();
  QGridLayout *individGrid = qobject_cast<QGridLayout*>(currentLayout);
  QLabel *validlbl = getValidatorMarker("WBVanadium1");
  individGrid->addWidget(validlbl, 0, 3);  
  validlbl = getValidatorMarker("HighAbsolute");
  individGrid->addWidget(validlbl, 1, 3);
  validlbl = getValidatorMarker("LowAbsolute");
  individGrid->addWidget(validlbl, 1, 6);
  validlbl = getValidatorMarker("HighMedian");
  individGrid->addWidget(validlbl, 2, 3);
  validlbl = getValidatorMarker("LowMedian");
  individGrid->addWidget(validlbl, 2, 6);

  // work on the efficency variation test groupbox
  currentLayout = m_uiForm.gbEfficiency->layout();
  QGridLayout *efficiencyGrid = qobject_cast<QGridLayout*>(currentLayout);
  validlbl = getValidatorMarker("WBVanadium2");
  efficiencyGrid->addWidget(validlbl, 0, 3);  
  validlbl = getValidatorMarker("Variation");
  efficiencyGrid->addWidget(validlbl, 0, 6);  
  
  // now the background groupbox
  currentLayout = m_uiForm.gbExperiment->layout();
  QGridLayout *experimentGrid = qobject_cast<QGridLayout*>(currentLayout);
  validlbl = getValidatorMarker("Experimental");
  experimentGrid->addWidget(validlbl, 0, 3);  
  validlbl = getValidatorMarker("RemoveZero");
  experimentGrid->addWidget(validlbl, 0, 5);  
  validlbl = getValidatorMarker("MaskExper");
  experimentGrid->addWidget(validlbl, 1, 5);
}
