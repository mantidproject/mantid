#include "MantidAlgorithms/CreatePolarizationEfficiencies.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
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

const std::string cPpLabel("CPp");
const std::string cApLabel("CAp");
const std::string cRhoLabel("CRho");
const std::string cAlphaLabel("CAlpha");

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
      Kernel::make_unique<ArrayProperty<double>>(cPpLabel, Direction::Input),
      "Effective polarizing power of the polarizing system. "
      "Expressed as a ratio 0 < Pp < 1");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(cApLabel, Direction::Input),
      "Effective polarizing power of the analyzing system. "
      "Expressed as a ratio 0 < Ap < 1");

  declareProperty(
      Kernel::make_unique<ArrayProperty<double>>(cRhoLabel, Direction::Input),
      "Ratio of efficiencies of polarizer spin-down to polarizer "
      "spin-up. This is characteristic of the polarizer flipper. "
      "Values are constants for each term in a polynomial "
      "expression.");

  declareProperty(Kernel::make_unique<ArrayProperty<double>>(cAlphaLabel,
                                                             Direction::Input),
                  "Ratio of efficiencies of analyzer spin-down to analyzer "
                  "spin-up. This is characteristic of the analyzer flipper. "
                  "Values are factors for each term in a polynomial "
                  "expression.");

  declareProperty(make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

void CreatePolarizationEfficiencies::exec() {

  std::vector<std::string> outputLabels;
  std::vector<std::vector<double>> polynomialCoefficients;

  for(auto const &label : {cPpLabel, cApLabel, cRhoLabel, cAlphaLabel}) {
    if (!isDefault(label)) {
      std::vector<double> const coefficients = getProperty(label);
      outputLabels.push_back(label);
      polynomialCoefficients.push_back(coefficients);
    }
  }

  if (outputLabels.empty()) {
    throw std::runtime_error("At least one of the polynomials must be set.");
  }

  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  auto sharedInX = inWS->sharedX(0);

  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(inWS, outputLabels.size(), sharedInX->size(), inWS->blocksize());
  auto axis1 = new TextAxis(outputLabels.size());
  outWS->replaceAxis(1, axis1);

  auto const x = inWS->points(0);
  std::vector<double> y(x.size());
  for(size_t i = 0; i < outputLabels.size(); ++i) {
    outWS->setSharedX(i, sharedInX);
    auto const &coefficients = polynomialCoefficients[i];
    std::transform(x.begin(), x.end(), y.begin(), [&coefficients](double v) {return calculatePolynomial(coefficients, v);});
    outWS->mutableY(i) = y;
    axis1->setLabel(i, outputLabels[i]);
  }

  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
