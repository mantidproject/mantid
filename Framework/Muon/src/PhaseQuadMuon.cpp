// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/PhaseQuadMuon.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspaceValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

namespace {
const std::array<std::string, 2> phaseNames = {{"phase", "phi"}};
const std::array<std::string, 3> asymmNames = {{"asymmetry", "asymm", "asym"}};

template <typename T1, typename T2> int findName(const T1 &patterns, const T2 &names) {
  for (const std::string &pattern : patterns) {
    auto it = std::find_if(names.begin(), names.end(), [pattern](const std::string &s) {
      if (s == pattern) {
        return true;
      } else {
        return false;
      }
    });
    if (it != names.end()) {
      return static_cast<int>(std::distance(names.begin(), it));
    }
  }
  return -1;
}
double ASYMM_ERROR = 999.0;
} // namespace

namespace Mantid::Algorithms {

using namespace Kernel;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PhaseQuadMuon)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PhaseQuadMuon::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
      "Name of the input workspace containing the spectra");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("PhaseTable", "", Direction::Input),
                  "Name of the table containing the detector phases and asymmetries");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
      "Name of the output workspace");
}

/** Executes the algorithm
 *
 */
void PhaseQuadMuon::exec() {

  // Get the input workspace
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // Get the input phase table
  // Should have two columns (detector, phase)
  API::ITableWorkspace_sptr phaseTable = getProperty("PhaseTable");

  // Get N0, the normalization constant: N(t) = N0 * exp(-x/tau)
  // for each spectrum/detector
  std::vector<double> n0 = getExponentialDecay(inputWs);

  // Compute squashograms
  API::MatrixWorkspace_sptr ows = squash(inputWs, phaseTable, n0);

  setProperty("OutputWorkspace", ows);
}

//------------------------------------------------------------------------------------------------
/** Checks that the input workspace and table have compatible dimensions
 * @return a map where: Key = string name of the property; Value = string
 * describing the problem with the property.
 */
std::map<std::string, std::string> PhaseQuadMuon::validateInputs() {

  std::map<std::string, std::string> result;

  // Check that input ws and table ws have compatible dimensions
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  API::ITableWorkspace_const_sptr tabWS = getProperty("PhaseTable");
  if (!inputWS) {
    result["InputWorkspace"] = "InputWorkspace is of Incorrect type. Please "
                               "provide a MatrixWorkspace as the "
                               "InputWorkspace";
    return result;
  }
  size_t nspec = inputWS->getNumberHistograms();
  size_t ndet = tabWS->rowCount();

  if (tabWS->columnCount() == 0) {
    result["PhaseTable"] = "Please provide a non-empty PhaseTable.";
  }

  if (nspec != ndet) {
    result["PhaseTable"] = "PhaseTable must have one row per spectrum";
  }

  // PhaseTable should have three columns: (detector, asymmetry, phase)
  if (tabWS->columnCount() != 3) {
    result["PhaseTable"] = "PhaseTable must have three columns";
  }
  auto columnNames = tabWS->getColumnNames();
  for (auto &columnName : columnNames) {
    std::transform(columnName.begin(), columnName.end(), columnName.begin(), ::tolower);
  }
  int phaseCount = 0;
  int asymmetryCount = 0;
  for (const std::string &columnName : columnNames) {
    phaseCount +=
        static_cast<int>(std::count_if(phaseNames.cbegin(), phaseNames.cend(),
                                       [&columnName](const auto &goodName) { return goodName == columnName; }));
    asymmetryCount +=
        static_cast<int>(std::count_if(asymmNames.cbegin(), asymmNames.cend(),
                                       [&columnName](const auto &goodName) { return goodName == columnName; }));
  }
  if (phaseCount == 0) {
    result["PhaseTable"] = "PhaseTable needs phases column";
  }
  if (asymmetryCount == 0) {
    result["PhaseTable"] = "PhaseTable needs a asymmetry/asymm/asym column";
  }
  if (phaseCount > 1) {
    result["PhaseTable"] = "PhaseTable has " + std::to_string(phaseCount) + " phase columns";
  }
  if (asymmetryCount > 1) {
    result["PhaseTable"] = "PhaseTable has " + std::to_string(asymmetryCount) + " asymmetry/asymm/asym columns";
  }
  // Check units, should be microseconds
  Unit_const_sptr unit = inputWS->getAxis(0)->unit();
  if ((unit->caption() != "Time") || (unit->label().ascii() != "microsecond")) {
    result["InputWorkspace"] = "InputWorkspace units must be microseconds";
  }

  return result;
}

//----------------------------------------------------------------------------------------------
/** Calculates the normalization constant for the exponential decay
 * @param ws :: [input] Workspace containing the spectra to remove exponential
 * from
 * @return :: Vector containing the normalization constants, N0, for each
 * spectrum
 */
std::vector<double> PhaseQuadMuon::getExponentialDecay(const API::MatrixWorkspace_sptr &ws) {

  const size_t nspec = ws->getNumberHistograms();

  // Muon life time in microseconds
  constexpr double muLife = PhysicalConstants::MuonLifetime * 1e6;

  std::vector<double> n0(nspec, 0.);

  for (size_t h = 0; h < ws->getNumberHistograms(); h++) {

    const auto &X = ws->getSpectrum(h).x();
    const auto &Y = ws->getSpectrum(h).y();
    const auto &E = ws->getSpectrum(h).e();

    double s, sx, sy;
    s = sx = sy = 0.;
    for (size_t i = 0; i < Y.size(); i++) {

      if (Y[i] > 0) {
        double sig = E[i] * E[i] / Y[i] / Y[i];
        s += 1. / sig;
        sx += (X[i] - X[0]) / sig;
        sy += log(Y[i]) / sig;
      }
    }
    n0[h] = exp((sy + sx / muLife) / s);
  }

  return n0;
}

//----------------------------------------------------------------------------------------------
/** Forms the quadrature phase signal (squashogram)
 * @param ws :: [input] workspace containing the measured spectra
 * @param phase :: [input] table workspace containing the detector phases
 * @param n0 :: [input] vector containing the normalization constants
 * @return :: workspace containing the quadrature phase signal
 */
API::MatrixWorkspace_sptr PhaseQuadMuon::squash(const API::MatrixWorkspace_sptr &ws,
                                                const API::ITableWorkspace_sptr &phase, const std::vector<double> &n0) {

  // Poisson limit: below this number we consider we don't have enough
  // statistics
  // to apply sqrt(N). This is an arbitrary number used in the original code
  // provided by scientists
  const double poissonLimit = 30.;

  // Muon life time in microseconds
  const double muLife = PhysicalConstants::MuonLifetime * 1e6;

  const size_t nspec = ws->getNumberHistograms();

  if (n0.size() != nspec) {
    throw std::invalid_argument("Invalid normalization constants");
  }

  auto columnNames = phase->getColumnNames();
  for (auto &columnName : columnNames) {
    std::transform(columnName.begin(), columnName.end(), columnName.begin(), ::tolower);
  }
  auto phaseIndex = findName(phaseNames, columnNames);
  auto asymmetryIndex = findName(asymmNames, columnNames);

  // Get the maximum asymmetry
  double maxAsym = 0.;
  for (size_t h = 0; h < nspec; h++) {
    if (phase->Double(h, asymmetryIndex) > maxAsym && phase->Double(h, asymmetryIndex) != ASYMM_ERROR) {
      maxAsym = phase->Double(h, asymmetryIndex);
    }
  }

  if (maxAsym == 0.0) {
    throw std::invalid_argument("Invalid detector asymmetries");
  }
  std::vector<bool> emptySpectrum;
  emptySpectrum.reserve(nspec);
  std::vector<double> aj, bj;
  {
    // Calculate coefficients aj, bj

    double sxx = 0.;
    double syy = 0.;
    double sxy = 0.;
    for (size_t h = 0; h < nspec; h++) {
      emptySpectrum.emplace_back(
          std::all_of(ws->y(h).begin(), ws->y(h).end(), [](double value) { return value == 0.; }) ||
          phase->Double(h, asymmetryIndex) == ASYMM_ERROR);
      if (!emptySpectrum[h]) {
        const double asym = phase->Double(h, asymmetryIndex) / maxAsym;
        const double phi = phase->Double(h, phaseIndex);
        const double X = n0[h] * asym * cos(phi);
        const double Y = n0[h] * asym * sin(phi);
        sxx += X * X;
        syy += Y * Y;
        sxy += X * Y;
      }
    }

    const double lam1 = 2 * syy / (sxx * syy - sxy * sxy);
    const double mu1 = 2 * sxy / (sxy * sxy - sxx * syy);
    const double lam2 = 2 * sxy / (sxy * sxy - sxx * syy);
    const double mu2 = 2 * sxx / (sxx * syy - sxy * sxy);
    for (size_t h = 0; h < nspec; h++) {
      if (emptySpectrum[h]) {
        aj.emplace_back(0.0);
        bj.emplace_back(0.0);
      } else {
        const double asym = phase->Double(h, asymmetryIndex) / maxAsym;
        const double phi = phase->Double(h, phaseIndex);
        const double X = n0[h] * asym * cos(phi);
        const double Y = n0[h] * asym * sin(phi);
        aj.emplace_back((lam1 * X + mu1 * Y) * 0.5);
        bj.emplace_back((lam2 * X + mu2 * Y) * 0.5);
      }
    }
  }

  const size_t npoints = ws->blocksize();
  // Create and populate output workspace
  API::MatrixWorkspace_sptr ows = create<API::MatrixWorkspace>(*ws, 2, BinEdges(npoints + 1));

  // X
  ows->setSharedX(0, ws->sharedX(0));
  ows->setSharedX(1, ws->sharedX(0));

  // Phase quadrature
  auto &realY = ows->mutableY(0);
  auto &imagY = ows->mutableY(1);
  auto &realE = ows->mutableE(0);
  auto &imagE = ows->mutableE(1);

  const auto xPointData = ws->histogram(0).points();
  // First X value
  const double X0 = xPointData.front();

  // calculate exponential decay outside of the loop
  std::vector<double> expDecay = xPointData.rawData();
  std::transform(expDecay.begin(), expDecay.end(), expDecay.begin(),
                 [X0, muLife](double x) { return exp(-(x - X0) / muLife); });

  for (size_t i = 0; i < npoints; i++) {
    for (size_t h = 0; h < nspec; h++) {
      if (!emptySpectrum[h]) {
        // (X,Y,E) with exponential decay removed
        const double X = ws->x(h)[i];
        const double exponential = n0[h] * exp(-(X - X0) / muLife);
        const double Y = ws->y(h)[i] - exponential;
        const double E = (ws->y(h)[i] > poissonLimit) ? ws->e(h)[i] : sqrt(exponential);

        realY[i] += aj[h] * Y;
        imagY[i] += bj[h] * Y;
        realE[i] += aj[h] * aj[h] * E * E;
        imagE[i] += bj[h] * bj[h] * E * E;
      }
    }
    realE[i] = sqrt(realE[i]);
    imagE[i] = sqrt(imagE[i]);

    // Regain exponential decay
    realY[i] /= expDecay[i];
    imagY[i] /= expDecay[i];
    realE[i] /= expDecay[i];
    imagE[i] /= expDecay[i];
  }

  // New Y axis label
  ows->setYUnit("Asymmetry");

  return ows;
}
} // namespace Mantid::Algorithms
