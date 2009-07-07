//-----------------------------------
// Includes
//-----------------------------------
#include "MantidAlgorithms/GEMScriptInput.h"
#include "MantidKernel/FileValidator.h"
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
  MandatoryValidator<std::string> *notEmpty = new MandatoryValidator<std::string>();

  declareProperty("SampleFile", "", new FileValidator());
  declareProperty("SampleBackgroudFile", "", new FileValidator());
  declareProperty("VanadiumFile", "", new FileValidator());
  declareProperty("VanadiumBackgroundFile", "", new FileValidator());

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
  declareProperty("CalibFile", "", new FileValidator());


}

void GEMScriptInput::exec()
{
  //This algorithm doesn't actually do anything, it is a placeholder for retrieving
  //properties
}

