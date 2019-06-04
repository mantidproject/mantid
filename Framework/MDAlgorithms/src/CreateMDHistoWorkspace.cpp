// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/CreateMDHistoWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
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
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateMDHistoWorkspace::name() const {
  return "CreateMDHistoWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int CreateMDHistoWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateMDHistoWorkspace::category() const {
  return "MDAlgorithms\\Creation";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMDHistoWorkspace::init() {
  auto validator = boost::make_shared<CompositeValidator>();
  validator->add(boost::make_shared<BoundedValidator<int>>(1, 9));
  validator->add(boost::make_shared<MandatoryValidator<int>>());
  auto mandatoryIntArrayValidator =
      boost::make_shared<MandatoryValidator<std::vector<int>>>();
  auto mandatoryDoubleArrayValidator =
      boost::make_shared<MandatoryValidator<std::vector<double>>>();
  auto mandatoryStrArrayValidator =
      boost::make_shared<MandatoryValidator<std::vector<std::string>>>();

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "SignalInput", mandatoryDoubleArrayValidator),
                  "Signal array for n-dimensional workspace");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "ErrorInput", mandatoryDoubleArrayValidator),
                  "Error array for n-dimensional workspace");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("NumberOfEvents",
                                                 std::vector<double>(0)),
      "Number of pixels array for n-dimensional workspace. Optional, defaults "
      "to 1 per bin.");
  declareProperty(std::make_unique<PropertyWithValue<int>>(
                      "Dimensionality", -1, validator, Direction::Input),
                  "Dimensionality of the data in the file.");

  declareProperty(std::make_unique<ArrayProperty<double>>(
                      "Extents", mandatoryDoubleArrayValidator),
                  "A comma separated list of min, max for each dimension,\n"
                  "specifying the extents of each dimension.");

  declareProperty(std::make_unique<ArrayProperty<int>>(
                      "NumberOfBins", mandatoryIntArrayValidator),
                  "Number of bin in each dimension.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "Names", mandatoryStrArrayValidator),
                  "A comma separated list of the name of each dimension.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "Units", mandatoryStrArrayValidator),
                  "A comma separated list of the units of each dimension.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "MDHistoWorkspace reflecting the input text file.");
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>("Frames"),
      " A comma separated list of the frames of each dimension. "
      " The frames can be"
      " **General Frame**: Any frame which is not a Q-based frame."
      " **QLab**: Wave-vector converted into the lab frame."
      " **QSample**: Wave-vector converted into the frame of the sample."
      " **HKL**: Wave-vector converted into the crystal's HKL indices."
      " Note if nothing is specified then the **General Frame** is being "
      "selected. Also note that if you select a frame then this might override "
      "your unit selection if it is not compatible with the frame.");
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
    throw std::invalid_argument(
        "Expected size of the NumberOfEvents is: " + stream.str() +
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

} // namespace MDAlgorithms
} // namespace Mantid
