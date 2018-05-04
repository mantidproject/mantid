#include "MantidAlgorithms/CreatePolarizationEfficiencies.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidGeometry/Instrument.h"

#include <boost/shared_ptr.hpp>

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {

const std::string PpLabel("Pp");
const std::string ApLabel("Ap");
const std::string RhoLabel("Rho");
const std::string AlphaLabel("Alpha");

const std::string P1Label("P1");
const std::string P2Label("P2");
const std::string F1Label("F1");
const std::string F2Label("F2");

double calculatePolynomial(std::vector<double> const &coefficients, double x) {
  double polynomial = coefficients[0];
  double xPow = 1.0;
  // Build up the polynomial in ascending powers of x
  for (size_t i = 1; i < coefficients.size(); ++i) {
    xPow *= x;
    polynomial += coefficients[i] * xPow;
  }
  return polynomial;
}

} // namespace

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CreatePolarizationEfficiencies)

const std::string CreatePolarizationEfficiencies::name() const {
  return "CreatePolarizationEfficiencies";
}

int CreatePolarizationEfficiencies::version() const { return 1; }

const std::string CreatePolarizationEfficiencies::category() const {
  return "Reflectometry";
}

const std::string CreatePolarizationEfficiencies::summary() const {
  return "Converts polynomial factors to histograms with polarization "
         "efficiencies.";
}

void CreatePolarizationEfficiencies::init() {
  declareProperty(make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace to use the x-values from.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(PpLabel, Direction::Input),
      "Effective polarizing power of the polarizing system. "
      "Expressed as a ratio 0 < Pp < 1");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(ApLabel, Direction::Input),
      "Effective polarizing power of the analyzing system. "
      "Expressed as a ratio 0 < Ap < 1");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(RhoLabel, Direction::Input),
      "Ratio of efficiencies of polarizer spin-down to polarizer "
      "spin-up. This is characteristic of the polarizer flipper. "
      "Values are constants for each term in a polynomial "
      "expression.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(AlphaLabel, Direction::Input),
      "Ratio of efficiencies of analyzer spin-down to analyzer "
      "spin-up. This is characteristic of the analyzer flipper. "
      "Values are factors for each term in a polynomial "
      "expression.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(P1Label, Direction::Input),
      "Polarizer efficiency.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(P2Label, Direction::Input),
      "Analyzer efficiency.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(F1Label, Direction::Input),
      "Polarizer flipper efficiency.");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(F2Label, Direction::Input),
      "Analyzer flipper efficiency.");

  declareProperty(make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

void CreatePolarizationEfficiencies::exec() {
  auto const labelsFredrikze =
      getNonDefaultProperties({PpLabel, ApLabel, RhoLabel, AlphaLabel});
  auto const labelsWildes =
      getNonDefaultProperties({P1Label, P2Label, F1Label, F2Label});

  if (labelsFredrikze.empty() && labelsWildes.empty()) {
    throw std::invalid_argument("At least one of the polynomials must be set.");
  }

  if (!labelsFredrikze.empty() && !labelsWildes.empty()) {
    throw std::invalid_argument(
        "Efficiencies belonging to different methods cannot mix.");
  }

  MatrixWorkspace_sptr efficiencies;
  if (!labelsFredrikze.empty()) {
    efficiencies = createEfficiencies(labelsFredrikze);
  } else {
    efficiencies = createEfficiencies(labelsWildes);
  }

  setProperty("OutputWorkspace", efficiencies);
}

/// Get names of non-default properties out of a list of names
/// @param labels :: Names of properties to check.
std::vector<std::string>
CreatePolarizationEfficiencies::getNonDefaultProperties(
    std::vector<std::string> const &labels) const {
  std::vector<std::string> outputLabels;
  for (auto const &label : labels) {
    if (!isDefault(label)) {
      outputLabels.push_back(label);
    }
  }
  return outputLabels;
}

/// Create the efficiencies workspace given names of input properties.
/// @param labels :: Names of efficiencies which to include in the output
/// workspace.
MatrixWorkspace_sptr CreatePolarizationEfficiencies::createEfficiencies(
    std::vector<std::string> const &labels) {

  std::vector<std::vector<double>> polynomialCoefficients;

  for (auto const &label : labels) {
    std::vector<double> const coefficients = getProperty(label);
    polynomialCoefficients.push_back(coefficients);
  }

  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  auto sharedInX = inWS->sharedX(0);

  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(
      inWS, labels.size(), sharedInX->size(), inWS->blocksize());
  auto axis1 = new TextAxis(labels.size());
  outWS->replaceAxis(1, axis1);
  outWS->getAxis(0)->setUnit(inWS->getAxis(0)->unit()->unitID());

  auto const x = inWS->points(0);
  std::vector<double> y(x.size());
  for (size_t i = 0; i < labels.size(); ++i) {
    outWS->setSharedX(i, sharedInX);
    auto const &coefficients = polynomialCoefficients[i];
    std::transform(x.begin(), x.end(), y.begin(), [&coefficients](double v) {
      return calculatePolynomial(coefficients, v);
    });
    outWS->mutableY(i) = y;
    axis1->setLabel(i, labels[i]);
  }

  return outWS;
}

} // namespace Algorithms
} // namespace Mantid
