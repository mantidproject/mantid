//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/SampleTransmission.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"


namespace
{
  Mantid::Kernel::Logger g_log("SampleTransmission");
}

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(SampleTransmission);
}
}

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;


//----------------------
// Public member functions
//----------------------
///Constructor
SampleTransmission::SampleTransmission(QWidget *parent) :
  UserSubWindow(parent),
  m_algRunner(new API::AlgorithmRunner(this))
{
  connect(m_algRunner, SIGNAL(algorithmComplete(bool)), this, SLOT(algorithmComplete(bool)));
}


/**
 * Set up the dialog layout.
 */
void SampleTransmission::initLayout()
{
  m_uiForm.setupUi(this);
  connect(m_uiForm.pbCalculate, SIGNAL(clicked()), this, SLOT(calculate()));

  validate(true);
}


/**
 * Validate user input.
 * Outputs any warnings to the results log at warning level.
 *
 * @param silent If the results should not be logged
 * @return Result of validation
 */
bool SampleTransmission::validate(bool silent)
{
  UserInputValidator uiv;

  // Valudate input binning
  int wavelengthBinning = m_uiForm.cbBinningType->currentIndex();
  switch(wavelengthBinning)
  {
    // Single
    case 0:
      uiv.checkBins(m_uiForm.spSingleLow->value(),
                    m_uiForm.spSingleWidth->value(),
                    m_uiForm.spSingleHigh->value());
      break;

    // Multiple
    case 1:
      uiv.checkFieldIsNotEmpty("Multiple binning",
                               m_uiForm.leMultiple,
                               m_uiForm.valMultiple);
      break;
  }

  // Validate chemical formula
  uiv.checkFieldIsNotEmpty("Chemical Formula",
                           m_uiForm.leChemicalFormula,
                           m_uiForm.valChemicalFormula);

  // Ensure number density is not zero
  uiv.setErrorLabel(m_uiForm.valNumberDensity,
    uiv.checkNotEqual("Number Density", m_uiForm.spNumberDensity->value()));

  // Ensure thickness is not zero
  uiv.setErrorLabel(m_uiForm.valThickness,
    uiv.checkNotEqual("Thickness", m_uiForm.spThickness->value()));

  // Give error message
  if(!silent && !uiv.isAllInputValid())
    g_log.error(uiv.generateErrorMessage().toStdString());

  return uiv.isAllInputValid();
}


/**
 * Performs a calculation with the current settings
 */
void SampleTransmission::calculate()
{
  // Do not try to run with invalid input
  if(!validate())
    return;

  // Create the transmission calculation algorithm
  IAlgorithm_sptr transCalcAlg = AlgorithmManager::Instance().create("CalculateSampleTransmission");
  transCalcAlg->initialize();

  // Set the wavelength binning based on type set in UI
  int wavelengthBinning = m_uiForm.cbBinningType->currentIndex();
  switch(wavelengthBinning)
  {
    // Single
    case 0:
    {
      QStringList params;
      params << m_uiForm.spSingleLow->text()
             << m_uiForm.spSingleWidth->text()
             << m_uiForm.spSingleHigh->text();
      QString binString = params.join(",");
      transCalcAlg->setProperty("WavelengthRange", binString.toStdString());
      break;
    }

    // Multiple
    case 1:
      transCalcAlg->setProperty("WavelengthRange", m_uiForm.leMultiple->text().toStdString());
      break;
  }

  // Set sample material properties
  transCalcAlg->setProperty("ChemicalFormula", m_uiForm.leChemicalFormula->text().toStdString());
  transCalcAlg->setProperty("NumberDensity", m_uiForm.spNumberDensity->value());
  transCalcAlg->setProperty("Thickness", m_uiForm.spThickness->value());

  transCalcAlg->setProperty("OutputWorkspace", "CalculatedSampleTransmission");

  // Run algorithm
  m_algRunner->startAlgorithm(transCalcAlg);
}


/**
 * Handles completion of the calculation algorithm.
 *
 * @param error If the algorithm exited with an error
 */
void SampleTransmission::algorithmComplete(bool error)
{
  // Ignore errors
  if(error)
    return;

  //TODO
}
