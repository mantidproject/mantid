#include "MantidMDAlgorithms/CreateMDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {
/**
Helper type to compute the square in-place.
*/
struct Square : public std::unary_function<double, void> {
  void operator()(double &i) { i *= i; }
};

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateMDHistoWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CreateMDHistoWorkspace::CreateMDHistoWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CreateMDHistoWorkspace::~CreateMDHistoWorkspace() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateMDHistoWorkspace::name() const {
  return "CreateMDHistoWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int CreateMDHistoWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateMDHistoWorkspace::category() const {
  return "MDAlgorithms";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMDHistoWorkspace::init() {
  declareProperty(new ArrayProperty<double>("SignalInput"),
                  "Signal array for n-dimensional workspace");

  declareProperty(new ArrayProperty<double>("ErrorInput"),
                  "Error array for n-dimensional workspace");

  declareProperty(
      new ArrayProperty<double>("NumberOfEvents", std::vector<double>(0)),
      "Number of pixels array for n-dimensional workspace. Optional, defaults "
      "to 1 per bin.");

  // Declare all the generic properties required.
  this->initGenericImportProps();
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMDHistoWorkspace::exec() {
  MDHistoWorkspace_sptr ws = this->createEmptyOutputWorkspace();
  Mantid::signal_t *signals = ws->getSignalArray();
  Mantid::signal_t *errors = ws->getErrorSquaredArray();
  Mantid::signal_t *nEvents = ws->getNumEventsArray();

  std::vector<double> signalValues = getProperty("SignalInput");
  std::vector<double> errorValues = getProperty("ErrorInput");
  std::vector<double> numberOfEvents = getProperty("NumberOfEvents");

  size_t binProduct = this->getBinProduct();
  std::stringstream stream;
  stream << binProduct;
  if (binProduct != signalValues.size()) {
    throw std::invalid_argument("Expected size of the SignalInput is: " +
                                stream.str());
  }
  if (binProduct != errorValues.size()) {
    throw std::invalid_argument("Expected size of the ErrorInput is: " +
                                stream.str());
  }
  if (!numberOfEvents.empty() && binProduct != numberOfEvents.size()) {
    throw std::invalid_argument("Expected size of the NumberOfEvents is: " +
                                stream.str() +
                                ". Leave empty to auto fill with 1.0");
  }

  // Auto fill number of events.
  if (numberOfEvents.empty()) {
    numberOfEvents = std::vector<double>(binProduct, 1.0);
  }

  // Copy from property
  std::copy(signalValues.begin(), signalValues.end(), signals);
  std::vector<double> empty;
  // Clean up.
  signalValues.swap(empty);
  // Copy from property
  std::for_each(errorValues.begin(), errorValues.end(), Square());
  std::copy(errorValues.begin(), errorValues.end(), errors);
  // Clean up
  errorValues.swap(empty);
  // Copy from property
  std::copy(numberOfEvents.begin(), numberOfEvents.end(), nEvents);
  // Clean up
  numberOfEvents.swap(empty);

  setProperty("OutputWorkspace", ws);
}

} // namespace Mantid
} // namespace DataObjects
