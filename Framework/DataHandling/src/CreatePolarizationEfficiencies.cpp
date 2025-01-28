// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/CreatePolarizationEfficiencies.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"

#include <memory>

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {

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

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(CreatePolarizationEfficiencies)

const std::string CreatePolarizationEfficiencies::name() const { return "CreatePolarizationEfficiencies"; }

int CreatePolarizationEfficiencies::version() const { return 1; }

const std::string CreatePolarizationEfficiencies::summary() const {
  return "Converts polynomial factors to histograms with polarization "
         "efficiencies.";
}

const std::vector<std::string> CreatePolarizationEfficiencies::seeAlso() const {
  return {"JoinISISPolarizationEfficiencies", "LoadISISPolarizationEfficiencies", "PolarizationEfficiencyCor"};
}

void CreatePolarizationEfficiencies::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
      "An input workspace to use the x-values from.");

  declareProperty(std::make_unique<ArrayProperty<double>>(Pp, Direction::Input),
                  "Effective polarizing power of the polarizing system. "
                  "Expressed as a ratio 0 < Pp < 1");

  declareProperty(std::make_unique<ArrayProperty<double>>(Ap, Direction::Input),
                  "Effective polarizing power of the analyzing system. "
                  "Expressed as a ratio 0 < Ap < 1");

  declareProperty(std::make_unique<ArrayProperty<double>>(Rho, Direction::Input),
                  "Ratio of efficiencies of polarizer spin-down to polarizer "
                  "spin-up. This is characteristic of the polarizer flipper. "
                  "Values are constants for each term in a polynomial "
                  "expression.");

  declareProperty(std::make_unique<ArrayProperty<double>>(Alpha, Direction::Input),
                  "Ratio of efficiencies of analyzer spin-down to analyzer "
                  "spin-up. This is characteristic of the analyzer flipper. "
                  "Values are factors for each term in a polynomial "
                  "expression.");

  declareProperty(std::make_unique<ArrayProperty<double>>(P1, Direction::Input), "Polarizer efficiency.");

  declareProperty(std::make_unique<ArrayProperty<double>>(P2, Direction::Input), "Analyzer efficiency.");

  declareProperty(std::make_unique<ArrayProperty<double>>(F1, Direction::Input), "Polarizer flipper efficiency.");

  declareProperty(std::make_unique<ArrayProperty<double>>(F2, Direction::Input), "Analyzer flipper efficiency.");

  initOutputWorkspace();
}

/// Create the efficiencies workspace given names of input properties.
/// @param labels :: Names of efficiencies which to include in the output
/// workspace.
MatrixWorkspace_sptr CreatePolarizationEfficiencies::createEfficiencies(std::vector<std::string> const &labels) {

  std::vector<std::vector<double>> polynomialCoefficients(labels.size());

  std::ranges::transform(labels, polynomialCoefficients.begin(),
                         [this](const std::string &label) { return std::vector<double>(getProperty(label)); });

  MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  auto sharedInX = inWS->sharedX(0);

  MatrixWorkspace_sptr outWS =
      WorkspaceFactory::Instance().create(inWS, labels.size(), sharedInX->size(), inWS->blocksize());
  auto axis1 = std::make_unique<TextAxis>(labels.size());
  auto axis1Raw = axis1.get();
  outWS->replaceAxis(1, std::move(axis1));
  outWS->getAxis(0)->setUnit(inWS->getAxis(0)->unit()->unitID());

  auto const x = inWS->points(0);
  std::vector<double> y(x.size());
  for (size_t i = 0; i < polynomialCoefficients.size(); ++i) {
    outWS->setSharedX(i, sharedInX);
    auto const &coefficients = polynomialCoefficients[i];
    std::transform(x.begin(), x.end(), y.begin(),
                   [&coefficients](double v) { return calculatePolynomial(coefficients, v); });
    outWS->mutableY(i) = y;
    axis1Raw->setLabel(i, labels[i]);
  }

  return outWS;
}

} // namespace Mantid::DataHandling
