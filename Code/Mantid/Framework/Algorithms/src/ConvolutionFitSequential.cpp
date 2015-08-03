#include "MantidAlgorithms/ConvolutionFitSequential.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"

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

  declareProperty("Function", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The function that describes the parameters of the fit.",
                  Direction::Input);

  declareProperty(
      "Start X", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double>>(),
      "The start of the range for the fit function.", Direction::Input);

  declareProperty(
      "End X", EMPTY_DBL(), boost::make_shared<MandatoryValidator<double>>(),
      "The end of the range for the fit function.", Direction::Input);
  // Needs validation
  declareProperty("Temperature", std::string(""),
                  "The Temperature correction for the fit.", Direction::Input);

  auto boundedV = boost::make_shared<BoundedValidator<int>>();
  boundedV->setLower(0);

  declareProperty("Spec Min", 0, boundedV, "The first spectrum to be used in "
                                           "the fit. Spectra values can not be "
                                           "negative",
                  Direction::Input);

  declareProperty("Spec Max", 0, boundedV,
                  "The final spectrum to be used in the fit.",
                  Direction::Input);

  declareProperty("Convolve", true,
                  "If true, the fit is treated as a convolution workspace.",
                  Direction::Input);

  declareProperty("Minimizer", std::string("Levenberg-Marquardt"),
                  "Minimizer to use for fitting. Minimizers available are: "
                  "'Levenberg-Marquardt', 'Simplex', 'FABADA', 'Conjugate "
                  "gradient (Fletcher-Reeves imp.)', 'Conjugate gradient "
                  "(Polak-Ribiere imp.)' and 'BFGS'",
                  Direction::Input);

  declareProperty(
      "Max Iterations", 500, boost::make_shared<MandatoryValidator<int>>(),
      "The maximum number of iterations permitted", Direction::Input);

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The ouput workspace for the fit.");
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
  MatrixWorkspace_sptr outWS = getProperty("OutputWorkspace");

  // Handle empty/non-empty temp property
  if (temperature.compare("") == 0) {
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

  // Convert input workspace to get Q axis
  // Fit all spectra in workspace
  // Fit args
  // Run PlotPeaksByLogValue
  // Delete possible pre-existing workspaces
  // Construct output workspace name
  // Define params for use in convertParametersToWorkSpace
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
 * background)
 */
std::vector<std::string>
ConvolutionFitSequential::findValuesFromFunction(const std::string &function) {
  std::vector<std::string> result;
  std::string fitType = "";
  auto startPos = function.rfind("name=");
  if (startPos != std::string::npos) {
    fitType = function.substr(startPos, function.size());
    auto nextPos = fitType.find_first_of(",");
    fitType = fitType.substr(5, nextPos - 5);
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
  return result;
}
} // namespace Algorithms
} // namespace Mantid