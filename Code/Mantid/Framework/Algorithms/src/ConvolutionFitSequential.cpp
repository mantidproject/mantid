#include "MantidAlgorithms/ConvolutionFitSequential.h"
#include "MantidKernel/MandatoryValidator.h"

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
      "Start X", "", boost::make_shared<MandatoryValidator<double>>(),
      "The start of the range for the fit function.", Direction::Input);

  declareProperty("End X", "", boost::make_shared<MandatoryValidator<double>>(),
                  "The end of the range for the fit function.",
                  Direction::Input);
  //Needs validation
  declareProperty("Temperature", "", "The Temperature correction for the fit.", Direction::Input);

  declareProperty("Spec Min", 0, boost::make_shared<MandatoryValidator<int>>(),
                  "The first spectrum to be used in the fit.", Direction::Input);

  declareProperty("Spec Max", 0, boost::make_shared<MandatoryValidator<int>>(),
                  "The final spectrum to be used in the fit.", Direction::Input);

  declareProperty("Convolve", true,
                  "If true, the fit is treated as a convolution workspace.",
                  Direction::Input);

  declareProperty(
      "Maximum Iterations", 500, boost::make_shared<MandatoryValidator<int>>(),
      "The maximum number of iterations permitted", Direction::Input);

  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The ouput workspace for the fit.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvolutionFitSequential::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid