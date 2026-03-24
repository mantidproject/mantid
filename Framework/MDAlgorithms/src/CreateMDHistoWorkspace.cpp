// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateMDHistoWorkspace)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateMDHistoWorkspace::name() const { return "CreateMDHistoWorkspace"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateMDHistoWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateMDHistoWorkspace::category() const { return "MDAlgorithms\\Creation"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMDHistoWorkspace::init() {
  auto validator = std::make_shared<CompositeValidator>();
  validator->add(std::make_shared<BoundedValidator<int>>(1, 9));
  validator->add(std::make_shared<MandatoryValidator<int>>());
  auto mandatoryIntArrayValidator = std::make_shared<MandatoryValidator<std::vector<int>>>();
  auto mandatoryDoubleArrayValidator = std::make_shared<MandatoryValidator<std::vector<double>>>();
  auto mandatoryStrArrayValidator = std::make_shared<MandatoryValidator<std::vector<std::string>>>();

  declareProperty(std::make_unique<ArrayProperty<double>>("SignalInput", mandatoryDoubleArrayValidator),
                  "Signal array for n-dimensional workspace");

  declareProperty(std::make_unique<ArrayProperty<double>>("ErrorInput", mandatoryDoubleArrayValidator),
                  "Error array for n-dimensional workspace");

  declareProperty(std::make_unique<ArrayProperty<double>>("NumberOfEvents", std::vector<double>(0)),
                  "Number of pixels array for n-dimensional workspace. Optional, defaults "
                  "to 1 per bin.");
  declareProperty(std::make_unique<PropertyWithValue<int>>("Dimensionality", -1, validator, Direction::Input),
                  "Dimensionality of the data in the file.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Extents", mandatoryDoubleArrayValidator),
                  "A comma separated list of min, max for each dimension,\n"
                  "specifying the extents of each dimension.");

  declareProperty(std::make_unique<ArrayProperty<int>>("NumberOfBins", mandatoryIntArrayValidator),
                  "Number of bin in each dimension.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("Names", mandatoryStrArrayValidator),
                  "A comma separated list of the name of each dimension. "
                  "e.g. ('[H,0,0]','[0,K,0]','[0,0,L]') ");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("Units", mandatoryStrArrayValidator),
                  "A comma separated list of the units of each dimension.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDHistoWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "MDHistoWorkspace reflecting the input text file.");
  declareProperty(std::make_unique<ArrayProperty<std::string>>("Frames"),
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

std::map<std::string, std::string> CreateMDHistoWorkspace::validateInputs() {
  std::map<std::string, std::string> errors;

  const std::vector<double> &signalValues = getProperty("SignalInput");
  const std::vector<double> &errorValues = getProperty("ErrorInput");
  const std::vector<double> &numberOfEvents = getProperty("NumberOfEvents");

  const std::string msg("All inputs must match size: " + std::to_string(signalValues.size()));

  if (signalValues.size() != errorValues.size()) {
    errors["SignalInput"] = msg;
    errors["ErrorInput"] = msg;
  }
  // don't need to add message to empty array
  if ((!numberOfEvents.empty()) && (numberOfEvents.size() != signalValues.size()))
    errors["NumberOfEvents"] = msg;

  return errors;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMDHistoWorkspace::exec() {
  MDHistoWorkspace_sptr ws = this->createEmptyOutputWorkspace();
  auto signals = ws->mutableSignalArray();
  auto errors = ws->mutableErrorSquaredArray();
  auto nEvents = ws->mutableNumEventsArray();
  const size_t binProduct = this->getBinProduct();

  const std::vector<double> &signalValues = getProperty("SignalInput");
  const std::vector<double> &errorValues = getProperty("ErrorInput");
  const std::vector<double> &numberOfEvents = getProperty("NumberOfEvents");

  // this->createEmptyOutputWorkspace() initializes the value returned by this->getBinProduct()
  if (signalValues.size() != binProduct) {
    const std::string msg("All inputs must match size: " + std::to_string(binProduct));
    throw std::invalid_argument(msg);
  }

  // Fast memory copies and squaring
  std::memcpy(signals, signalValues.data(), binProduct * sizeof(double));
  std::transform(errorValues.cbegin(), errorValues.cend(), errors, // first element
                 [](const auto &value) { return value * value; });

  if (numberOfEvents.empty()) {
    std::fill(nEvents, nEvents + binProduct, 1.0);
  } else {
    std::memcpy(nEvents, numberOfEvents.data(), binProduct * sizeof(double));
  }

  setProperty("OutputWorkspace", ws);
}

} // namespace Mantid::MDAlgorithms
