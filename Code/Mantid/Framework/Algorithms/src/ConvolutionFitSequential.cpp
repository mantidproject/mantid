#include "MantidAlgorithms/ConvolutionFitSequential.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"

#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringContainsValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvolutionFitSequential)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvolutionFitSequential::ConvolutionFitSequential() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvolutionFitSequential::~ConvolutionFitSequential() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConvolutionFitSequential::name() const {
  return "ConvolutionFitSequential";
}

/// Algorithm's version for identification. @see Algorithm::version
int ConvolutionFitSequential::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvolutionFitSequential::category() const {
  return "Workflow\\MIDAS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConvolutionFitSequential::summary() const {
  return "Performs a sequential fit for a convolution workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvolutionFitSequential::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The input workspace for the fit.");

  auto scv = boost::make_shared<StringContainsValidator>();
  auto requires = std::vector<std::string>();
  requires.push_back("Convolution");
  requires.push_back("Resolution");
  scv->setRequiredStrings(requires);

  declareProperty("Function", "", scv,
                  "The function that describes the parameters of the fit.",
                  Direction::Input);

  declareProperty(
      "Start X", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double>>(),
      "The start of the range for the fit function.", Direction::Input);

  declareProperty(
      "End X", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double>>(),
      "The end of the range for the fit function.", Direction::Input);

  declareProperty("Temperature", EMPTY_DBL(),
                  boost::make_shared<MandatoryValidator<double>>(),
                  "The Temperature correction for the fit.", Direction::Input);

  auto boundedV = boost::make_shared<BoundedValidator<int>>();
  boundedV->setLower(0);

  declareProperty("Spec Min", 0, boundedV, "The first spectrum to be used in "
                                           "the fit. Spectra values can not be "
                                           "negative",
                  Direction::Input);

  declareProperty("Spec Max", 0, boundedV, "The final spectrum to be used in "
                                           "the fit. Spectra values can not be "
                                           "negative",
                  Direction::Input);

  declareProperty("Convolve", true,
                  "If true, the fit is treated as a convolution workspace.",
                  Direction::Input);

  std::vector<std::string> minimizers;
  minimizers.push_back("Levenberg-Marquardt");
  minimizers.push_back("Simplex");
  minimizers.push_back("FABADA");
  minimizers.push_back("Conjugate gradient (Fletcher-Reeves imp.)");
  minimizers.push_back("Conjugate gradient (Polak-Ribiere imp.)");
  minimizers.push_back("BFGS");
  declareProperty("Minimizer", std::string("Levenberg-Marquardt"),
                  boost::make_shared<StringListValidator>(minimizers),
                  "Minimizer to use for fitting. Minimizers available are: "
                  "'Levenberg-Marquardt', 'Simplex', 'FABADA', 'Conjugate "
                  "gradient (Fletcher-Reeves imp.)', 'Conjugate gradient "
                  "(Polak-Ribiere imp.)' and 'BFGS'",
                  Direction::Input);

  declareProperty("Max Iterations", 500, boundedV,
                  "The maximum number of iterations permitted",
                  Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvolutionFitSequential::exec() {
  // Start Timer
  // Initialise variables with properties
  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  std::string function = getProperty("Function");
  double startX = getProperty("Start X");
  double endX = getProperty("End X");
  std::string temperature = getProperty("Temperature");
  int specMin = getProperty("Spec Min");
  int specMax = getProperty("Spec max");
  bool convolve = getProperty("Convolve");
  int maxIter = getProperty("Max Iterations");
  std::string minimizer = getProperty("Minimizer");

  // Handle empty/non-empty temp property
  if (temperature.compare("0") == 0) {
    temperature = "None";
  }
  // Pull in UI settings (Plot / Save (minimizer?)

  // Inspect function to obtain fit Type and background
  std::vector<std::string> functionValues = findValuesFromFunction(function);

  // Check if a delta function is being used
  bool delta = false;
  auto pos = function.find("Delta");
  if (pos != std::string::npos) {
    delta = true;
  }

  // Add logger information

  // Output workspace name
  std::string outWSName = inWS->getName();
  outWSName += "conv_" + functionValues[0] + functionValues[1] + "s_";
  outWSName += specMin + "_to_" + specMax;

  // Convert input workspace to get Q axis

  // Fit all spectra in workspace

  // Fit args

  // Run PlotPeaksByLogValue
  auto plotPeaks = createChildAlgorithm("PlotPeakByLogValue", -1, -1, true);
  plotPeaks->setProperty("Input", ""); // need input name
  plotPeaks->setProperty("OutputWorkspace", outWSName);
  plotPeaks->setProperty("Function", function);
  plotPeaks->setProperty("StartX", startX);
  plotPeaks->setProperty("EndX", endX);
  plotPeaks->setProperty("FitType", "Sequential");
  plotPeaks->setProperty("CreateOutput", true);
  plotPeaks->setProperty("OutputCompositeMembers", true);
  plotPeaks->setProperty("ConvoleMembers", convolve);
  plotPeaks->setProperty("MaxIterations", maxIter);
  plotPeaks->setProperty("Minimizer", minimizer);
  // fit args?
  plotPeaks->executeAsChildAlg();

  // Delete workspaces

  // Construct output workspace name
  std::string wsName = outWSName + "_Result";

  // Define params for use in convertParametersToWorkSpace
  using namespace Mantid::API;
  auto paramNames = std::vector<std::string>();
  const std::string funcName = functionValues[2];
  paramNames.push_back("Height");
  if (funcName.find("Diffusion") != std::string::npos) {
    paramNames.push_back("Intensity");
    paramNames.push_back("Radius");
  } else if (funcName.find("Sphere") != std::string::npos) {
    paramNames.push_back("Diffusion");
    paramNames.push_back("Shift");
  } else if (funcName.find("Circle") != std::string::npos) {
    paramNames.push_back("Decay");
    paramNames.push_back("Shift");
  } else if (funcName.find("Stretch") != std::string::npos) {
    paramNames.pop_back();
    paramNames.push_back("height");
    paramNames.push_back("tau");
    paramNames.push_back("beta");
  } else {
    paramNames.push_back("Amplitude");
    paramNames.push_back("FWHM");
    paramNames.push_back("EISF");
  }

  // Run calcEISF if Delta
  if (delta) {
    // calc EISF
  }

  // Run convertParametersToWorkspace

  // Set x units to be momentum transfer

  // Handle sample logs

  // Rename workspaces

  // Save

  // Plot

  // End Timer
}

/**
 * Check function to establish if it is for one lorentzian or Two
 * @param subfunction The unchecked substring of the function
 * @return true if the function is two lorentzian false if one lorentzian
 */
bool ConvolutionFitSequential::checkForTwoLorentz(
    const std::string &subFunction) {
  std::string fitType = "";
  auto pos = subFunction.rfind("name=");
  if (pos != std::string::npos) {
    fitType = subFunction.substr(0, pos);
    pos = fitType.rfind("name=");
    fitType = subFunction.substr(pos, subFunction.size());
    pos = fitType.find_first_of(",");
    fitType = fitType.substr(5, pos - 5);
    if (fitType.compare("Lorentzian") == 0) {
      return true;
    }
  }
  return false;
}

/**
 * Finds specific values embedded in the function supplied to the algorithm
 * @param function The full function string
 * @return all values of interest from the function ((0 - fitType, 1 -
 * background, 2 - functionName)
 */
std::vector<std::string>
ConvolutionFitSequential::findValuesFromFunction(const std::string &function) {
  std::vector<std::string> result;
  std::string fitType = "";
  std::string functionName = "";
  auto startPos = function.rfind("name=");
  if (startPos != std::string::npos) {
    fitType = function.substr(startPos, function.size());
    auto nextPos = fitType.find_first_of(",");
    fitType = fitType.substr(5, nextPos - 5);
    functionName = fitType;
    if (fitType.compare("Lorentzian") == 0) {
      std::string newSub = function.substr(0, startPos);
      bool isTwoL = checkForTwoLorentz(function.substr(0, startPos));
      if (isTwoL == true) {
        fitType = "2";
      } else {
        fitType = "1";
      }
    } else {
      fitType = "0";
    }
    result.push_back(fitType);
  }

  std::string background = "";
  auto pos = function.find("name=");
  if (pos != std::string::npos) {
    background = function.substr(pos, function.size());
    pos = background.find_first_of(",");
    background = background.substr(5, pos - 5);
    result.push_back(background);
  }
  result.push_back(functionName);
  return result;
}

} // namespace Algorithms
} // namespace Mantid