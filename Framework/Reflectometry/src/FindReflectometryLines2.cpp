// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/FindReflectometryLines2.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidKernel/BoundedValidator.h"

namespace {
/// String constants for the algorithm's property names
namespace Prop {
std::string const END_INDEX{"EndWorkspaceIndex"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const LINE_CENTRE{"LineCentre"};
std::string const OUTPUT_WS{"OutputWorkspace"};
std::string const RANGE_LOWER{"RangeLower"};
std::string const RANGE_UPPER{"RangeUpper"};
std::string const START_INDEX{"StartWorkspaceIndex"};
} // namespace Prop

/** Create a single value workspace from the input value.
 *  @param x a value to store in the returned workspace
 *  @return a single value workspace
 */
Mantid::API::MatrixWorkspace_sptr makeOutput(double const x) {
  auto ws = std::make_shared<Mantid::DataObjects::WorkspaceSingleValue>(x);
  return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
}
} // namespace

namespace Mantid::Reflectometry {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(FindReflectometryLines2)

/// Algorithms name for identification. @see Algorithm::name
const std::string FindReflectometryLines2::name() const { return "FindReflectometryLines"; }

/// Algorithm's version for identification. @see Algorithm::version
int FindReflectometryLines2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string FindReflectometryLines2::category() const { return "Reflectometry;ILL\\Reflectometry"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string FindReflectometryLines2::summary() const {
  return "Finds fractional workspace index corresponding to reflected or "
         "direct line in a line detector workspace.";
}

/// Initialize the algorithm's properties.
void FindReflectometryLines2::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(Prop::INPUT_WS, "", Kernel::Direction::Input),
      "A reflectometry workspace.");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      Prop::OUTPUT_WS, "", Kernel::Direction::Output, API::PropertyMode::Optional),
                  "A workspace containing the fractional workspace index of "
                  "the line centre.");
  declareProperty(Prop::LINE_CENTRE, EMPTY_DBL(), "The fractional workspace index of the line centre",
                  Kernel::Direction::Output);
  declareProperty(Prop::RANGE_LOWER, EMPTY_DBL(), "The lower peak search limit (an X value).");
  declareProperty(Prop::RANGE_UPPER, EMPTY_DBL(), "The upper peak search limit (an X value).");
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(Prop::START_INDEX, 0, mustBePositive, "Index of the first histogram to include in the peak search.");
  declareProperty(Prop::END_INDEX, EMPTY_INT(), mustBePositive,
                  "Index of the last histogram to include in the peak search.");
}

/// Validate the algorithm's input properties.
std::map<std::string, std::string> FindReflectometryLines2::validateInputs() {
  std::map<std::string, std::string> issues;
  if (!isDefault(Prop::RANGE_LOWER) && !isDefault(Prop::RANGE_UPPER)) {
    double const lower = getProperty(Prop::RANGE_LOWER);
    double const upper = getProperty(Prop::RANGE_UPPER);
    if (lower >= upper) {
      issues[Prop::RANGE_UPPER] = "The upper limit is smaller than the lower.";
    }
  }
  if (!isDefault(Prop::END_INDEX)) {
    int const start = getProperty(Prop::START_INDEX);
    int const end = getProperty(Prop::END_INDEX);
    if (start > end) {
      issues[Prop::END_INDEX] = "The index is smaller than the start.";
    }
  }
  return issues;
}

/// Execute the algorithm.
void FindReflectometryLines2::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty(Prop::INPUT_WS);
  auto fitPeak = createChildAlgorithm("FitSpecularPeak");
  fitPeak->setProperty("InputWorkspace", inputWS);
  fitPeak->setPropertyValue("BackgroundType", "Linear");
  int const startIndex = getProperty(Prop::START_INDEX);
  fitPeak->setProperty("StartWorkspaceIndex", startIndex);
  if (!isDefault(Prop::END_INDEX)) {
    int const endIndex = getProperty(Prop::END_INDEX);
    fitPeak->setProperty("EndWorkspaceIndex", endIndex);
  }
  if (!isDefault(Prop::RANGE_LOWER)) {
    double const rangeLower = getProperty(Prop::RANGE_LOWER);
    fitPeak->setProperty("RangeLower", rangeLower);
  }
  if (!isDefault(Prop::RANGE_UPPER)) {
    double const rangeUpper = getProperty(Prop::RANGE_UPPER);
    fitPeak->setProperty("RangeUpper", rangeUpper);
  }
  fitPeak->execute();
  double const peakWSIndex = fitPeak->getProperty("PeakCentre");
  setProperty(Prop::LINE_CENTRE, peakWSIndex);
  if (!isDefault(Prop::OUTPUT_WS)) {
    auto outputWS = makeOutput(peakWSIndex);
    setProperty(Prop::OUTPUT_WS, std::move(outputWS));
  }
}

} // namespace Mantid::Reflectometry
