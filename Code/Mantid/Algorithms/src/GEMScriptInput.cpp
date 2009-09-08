//-----------------------------------
// Includes
//-----------------------------------
#include "MantidAlgorithms/GEMScriptInput.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Algorithms;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GEMScriptInput)

using namespace Mantid::Kernel;
using namespace Mantid::API;

void GEMScriptInput::init()
{
  //First the data files
  declareProperty(new FileProperty("SampleFile", "", FileProperty::Load));
  declareProperty(new FileProperty("SampleBackgroudFile", "", FileProperty::Load));
  declareProperty(new FileProperty("VanadiumFile", "", FileProperty::Load));
  declareProperty(new FileProperty("VanadiumBackgroundFile", "", FileProperty::Load));

  declareProperty<bool>("SampleAbsTF",false);
  declareProperty<bool>("SampleBackgroundFocusTF",false);
  declareProperty<bool>("VanadiumFocusTF",false);

  //Some of these number need to be strictly  positive
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  
  //Sample absorption parameters - lengths in cm, density in atomsA^-3, cross-sections in barns
  declareProperty<double>("SampleRadius",0.381, mustBePositive);
  declareProperty<double>("SampleHeight",4.0, mustBePositive->clone());
  declareProperty<double>("SampleDensity",0.07216, mustBePositive->clone());
  declareProperty<double>("SampleSCS",5.08, mustBePositive->clone());
  declareProperty<double>("SampleACS",5.1, mustBePositive->clone());

  //Vanadium absorption parameters - lengths in cm, density in atomsA^-3, cross-sections in barns
  declareProperty<double>("VanRadius",0.417, mustBePositive->clone());
  declareProperty<double>("VanHeight",4.0, mustBePositive->clone());
  declareProperty<double>("VanDensity",0.07216, mustBePositive->clone());
  declareProperty<double>("VanSCS",5.08, mustBePositive->clone());
  declareProperty<double>("VanACS",5.1, mustBePositive->clone());

  //Working directory

  // - this is probably the wrong style for what I want!
  declareProperty("WorkingDir", "", new FileValidator()); 

  //Calibration file
  declareProperty(new FileProperty("CalibFile", "", FileProperty::Load));


}

void GEMScriptInput::exec()
{
  //This algorithm doesn't actually do anything, it is a placeholder for retrieving
  //properties
}

