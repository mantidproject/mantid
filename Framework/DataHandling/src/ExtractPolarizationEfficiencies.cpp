#include "MantidDataHandling/ExtractPolarizationEfficiencies.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/Unit.h"

#include <sstream>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ExtractPolarizationEfficiencies)

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;

namespace {

std::string const METHOD_FREDRIKZE("Fredrikze");
std::string const METHOD_WILDES("Wildes");
std::string const METHOD_PARAMETER("polarization_correction_method");
std::string const LAMBDA_PARAMETER("efficiency_lambda");

std::map<std::string, std::vector<std::string>> const EFFICIENCIES{
    {METHOD_FREDRIKZE, {"Pp", "Ap", "Rho", "Alpha"}},
    {METHOD_WILDES, {"P1", "P2", "F1", "F2"}}};

std::vector<double> parseVector(std::string const &name,
                                std::string const &value) {
  std::istringstream istr(value);
  std::vector<double> result;
  double number;
  while (istr >> number) {
    result.push_back(number);
  }

  if (!istr.eof()) {
    throw std::invalid_argument("Error while parsing vector " + name);
  }

  return result;
}

MatrixWorkspace_sptr createWorkspace(std::vector<double> const &x,
                                     std::vector<double> const &y) {
  Points xVals(x);
  Counts yVals(y);
  auto retVal = boost::make_shared<Workspace2D>();
  retVal->initialize(1, Histogram(xVals, yVals));
  return retVal;
}

} // namespace

/// Algorithm's name for identification. @see Algorithm::name
const std::string ExtractPolarizationEfficiencies::name() const {
  return "ExtractPolarizationEfficiencies";
}

/// Algorithm's version for identification. @see Algorithm::version
int ExtractPolarizationEfficiencies::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractPolarizationEfficiencies::category() const {
  return "DataHandling;ISIS\\Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractPolarizationEfficiencies::summary() const {
  return "Retrieves a workspace of monitor data held within the input "
         "workspace, if present.";
}

/** Initialize the algorithm's properties.
 */
void ExtractPolarizationEfficiencies::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "A workspace with attached instrument whose parameters contain "
      "polarization efficiencies.");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The workspace with extracted eficiencies.");
}

/** Execute the algorithm.
 */
void ExtractPolarizationEfficiencies::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  auto instrument = inputWS->getInstrument();
  auto const method = instrument->getParameterAsString(METHOD_PARAMETER);
  if (method.empty()) {
    throw std::invalid_argument("Correction method is undefined");
  }
  if (method != METHOD_FREDRIKZE && method != METHOD_WILDES) {
    throw std::invalid_argument("Unknown correction method: " + method);
  }
  auto const lambdaValue = instrument->getParameterAsString(LAMBDA_PARAMETER);
  if (lambdaValue.empty()) {
    throw std::invalid_argument(
        "Wavelengths are missing from the correction parameters");
  }
  auto const lambda = parseVector(LAMBDA_PARAMETER, lambdaValue);

  auto alg = createChildAlgorithm("JoinISISPolarizationEfficiencies");
  auto const &efficiencies = EFFICIENCIES.at(method);
  for (auto const &name : efficiencies) {
    auto const propValue = instrument->getParameterAsString(name);
    if (propValue.empty()) {
      throw std::invalid_argument("Parameter " + name +
                                  " is missing from the correction parameters");
    }
    auto const prop = parseVector(name, propValue);
    auto ws = createWorkspace(lambda, prop);
    alg->setProperty(name, ws);
  }
  alg->execute();
  MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", result);
}

} // namespace DataHandling
} // namespace Mantid
