// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ExtractPolarizationEfficiencies.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/Unit.h"

#include <sstream>

namespace Mantid::DataHandling {
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
std::string const OPTION_PARAMETER("polarization_correction_option");
std::string const LAMBDA_PARAMETER("efficiency_lambda");

std::map<std::string, std::vector<std::string>> const EFFICIENCIES{{METHOD_FREDRIKZE, {"Pp", "Ap", "Rho", "Alpha"}},
                                                                   {METHOD_WILDES, {"P1", "P2", "F1", "F2"}}};

std::vector<double> parseVector(std::string const &name, std::string const &value) {
  std::istringstream istr(value);
  std::vector<double> result;
  double number;
  while (istr >> number) {
    result.emplace_back(number);
  }

  if (!istr.eof()) {
    throw std::invalid_argument("Error while parsing instrument vector parameter " + name);
  }

  return result;
}

MatrixWorkspace_sptr createWorkspace(std::vector<double> const &x, std::vector<double> const &y,
                                     std::vector<double> const &e = std::vector<double>()) {
  Points xVals(x);
  Counts yVals(y);
  CountStandardDeviations eVals(e.empty() ? std::vector<double>(y.size()) : e);
  auto retVal = std::make_shared<Workspace2D>();
  retVal->initialize(1, Histogram(xVals, yVals, eVals));
  return retVal;
}

} // namespace

/// Algorithm's name for identification. @see Algorithm::name
const std::string ExtractPolarizationEfficiencies::name() const { return "ExtractPolarizationEfficiencies"; }

/// Algorithm's version for identification. @see Algorithm::version
int ExtractPolarizationEfficiencies::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ExtractPolarizationEfficiencies::category() const { return "DataHandling;Reflectometry\\ISIS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ExtractPolarizationEfficiencies::summary() const {
  return "Extracts polarization efficiencies from instrument's parameter file.";
}

/** Initialize the algorithm's properties.
 */
void ExtractPolarizationEfficiencies::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "A workspace with attached instrument whose parameters contain "
                  "polarization efficiencies.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The workspace with extracted efficiencies.");
  declareProperty<std::string>("CorrectionMethod", "", "Correction method: Fredrikze or Wildes.",
                               Kernel::Direction::Output);
  declareProperty<std::string>("CorrectionOption", "", "Correction option, eg \"PA\" or \"PNR\" for Fredrikze method.",
                               Kernel::Direction::Output);
}

/** Execute the algorithm.
 */
void ExtractPolarizationEfficiencies::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  auto instrument = inputWS->getInstrument();
  auto const method = instrument->getParameterAsString(METHOD_PARAMETER);
  if (method.empty()) {
    throw std::invalid_argument("Polarization Efficiencies method is not provided by the instrument parameter file");
  }
  if (method != METHOD_FREDRIKZE && method != METHOD_WILDES) {
    throw std::invalid_argument("Unknown correction method: " + method);
  }
  setProperty("CorrectionMethod", method);
  auto const lambdaValue = instrument->getParameterAsString(LAMBDA_PARAMETER);
  if (lambdaValue.empty()) {
    throw std::invalid_argument("Wavelengths are missing from the correction parameters");
  }
  auto const lambda = parseVector(LAMBDA_PARAMETER, lambdaValue);
  if (lambda.size() < 2) {
    throw std::runtime_error("Instrument vector parameter \"" + LAMBDA_PARAMETER +
                             "\" must have at least 2 elements but it has " + std::to_string(lambda.size()));
  }

  auto alg = createChildAlgorithm("JoinISISPolarizationEfficiencies");
  auto const &efficiencies = EFFICIENCIES.at(method);
  for (auto const &paramName : efficiencies) {
    auto propValue = instrument->getParameterAsString(paramName);
    if (propValue.empty()) {
      throw std::invalid_argument("Parameter " + paramName + " is missing from the correction parameters");
    }
    auto const prop = parseVector(paramName, propValue);
    if (lambda.size() != prop.size()) {
      throw std::runtime_error("Instrument vector parameter \"" + paramName +
                               "\" is expected to be the same size as \"" + LAMBDA_PARAMETER + "\" but " +
                               std::to_string(prop.size()) + " != " + std::to_string(lambda.size()));
    }
    auto const errorName = paramName + "_Errors";
    propValue = instrument->getParameterAsString(errorName);
    auto const errorProp = propValue.empty() ? std::vector<double>() : parseVector(errorName, propValue);
    auto ws = createWorkspace(lambda, prop, errorProp);
    alg->setProperty(paramName, ws);
  }
  alg->execute();
  MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", result);
  auto const option = instrument->getParameterAsString(OPTION_PARAMETER);
  if (option.empty()) {
    throw std::invalid_argument("Correction option is undefined");
  }
  setProperty("CorrectionOption", option);
}

} // namespace Mantid::DataHandling
