//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/SampleTransmission.h"

#include "MantidAPI/AlgorithmManager.h"


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
}


/**
 * Validate user input.
 * Outputs any warnings to the results log at warning level.
 *
 * @return Result of validation
 */
bool SampleTransmission::validate()
{
  //TODO
  return false;
}


/**
 * Performs a calculation with the current settings
 */
void SampleTransmission::calculate()
{
  // Create the transmission calculation algorithm
  IAlgorithm_sptr transCalcAlg = AlgorithmManager::Instance().create("CalculateSampleTransmission");
  transCalcAlg->initialize();

  // Set the wavelength binning based on type set in UI
  int wavelengthBinning = m_uiForm.cbBinningType->currentIndex();
  switch(wavelengthBinning)
  {
    // Multiple
    case 0:
      //TODO
      transCalcAlg->setProperty("WavelengthRange", "");
      break;

    // Single
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
  //TODO
}
