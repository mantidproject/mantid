//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PhaseQuadMuon)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PhaseQuadMuon::init() {

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the input workspace containing the spectra");

  declareProperty(
      new API::WorkspaceProperty<API::ITableWorkspace>("PhaseTable", "",
                                                       Direction::Input),
      "Name of the table containing the detector phases and asymmetries");

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
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

  // Copy X axis to output workspace
  ows->getAxis(0)->unit() = inputWs->getAxis(0)->unit();
  // New Y axis label
  ows->setYUnit("Asymmetry");

  setProperty("OutputWorkspace", ows);
}

//------------------------------------------------------------------------------------------------
/** Checks that the input workspace and table have compatible dimensions
 * @return a map where: Key = string name of the the property; Value = string
 * describing the problem with the property.
*/
std::map<std::string, std::string> PhaseQuadMuon::validateInputs() {

  std::map<std::string, std::string> result;

  // Check that input ws and table ws have compatible dimensions
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  API::ITableWorkspace_const_sptr tabWS = getProperty("PhaseTable");

  size_t nspec = inputWS->getNumberHistograms();
  size_t ndet = tabWS->rowCount();

  if (nspec != ndet) {
    result["PhaseTable"] = "PhaseTable must have one row per spectrum";
  }

  // PhaseTable should have two columns: (detector, phase)
  if (tabWS->columnCount() != 3) {
    result["PhaseTable"] = "PhaseTable must have three columns";
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
std::vector<double>
PhaseQuadMuon::getExponentialDecay(const API::MatrixWorkspace_sptr &ws) {

  size_t nspec = ws->getNumberHistograms();
  size_t npoints = ws->blocksize();

  // Muon life time in microseconds
  double muLife = PhysicalConstants::MuonLifetime * 1e6;

  std::vector<double> n0(nspec, 0.);

  for (size_t h = 0; h < ws->getNumberHistograms(); h++) {

    MantidVec X = ws->getSpectrum(h)->readX();
    MantidVec Y = ws->getSpectrum(h)->readY();
    MantidVec E = ws->getSpectrum(h)->readE();

    double s, sx, sy;
    s = sx = sy = 0;
    for (size_t i = 0; i < npoints; i++) {

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
API::MatrixWorkspace_sptr
PhaseQuadMuon::squash(const API::MatrixWorkspace_sptr &ws,
                      const API::ITableWorkspace_sptr &phase,
                      const std::vector<double> &n0) {

  // Poisson limit: below this number we consider we don't have enough
  // statistics
  // to apply sqrt(N). This is an arbitrary number used in the original code
  // provided by scientists
  double poissonLimit = 30.;

  size_t nspec = ws->getNumberHistograms();
  size_t npoints = ws->blocksize();

  // Muon life time in microseconds
  double muLife = PhysicalConstants::MuonLifetime * 1e6;

  if (n0.size() != nspec) {
    throw std::invalid_argument("Invalid normalization constants");
  }

  // Get the maximum asymmetry
  double maxAsym = 0.;
  for (size_t h = 0; h < nspec; h++) {
    if (phase->Double(h, 1) > maxAsym) {
      maxAsym = phase->Double(h, 1);
    }
  }
  if (!maxAsym) {
    throw std::invalid_argument("Invalid detector asymmetries");
  }

  std::vector<double> aj, bj;
  {
    // Calculate coefficients aj, bj

    double sxx = 0;
    double syy = 0;
    double sxy = 0;

    for (size_t h = 0; h < nspec; h++) {
      double asym = phase->Double(h, 1) / maxAsym;
      double phi = phase->Double(h, 2);
      double X = n0[h] * asym * cos(phi);
      double Y = n0[h] * asym * sin(phi);
      sxx += X * X;
      syy += Y * Y;
      sxy += X * Y;
    }

    double lam1 = 2 * syy / (sxx * syy - sxy * sxy);
    double mu1 = 2 * sxy / (sxy * sxy - sxx * syy);
    double lam2 = 2 * sxy / (sxy * sxy - sxx * syy);
    double mu2 = 2 * sxx / (sxx * syy - sxy * sxy);
    for (size_t h = 0; h < nspec; h++) {
      double asym = phase->Double(h, 1) / maxAsym;
      double phi = phase->Double(h, 2);
      double X = n0[h] * asym * cos(phi);
      double Y = n0[h] * asym * sin(phi);
      aj.push_back((lam1 * X + mu1 * Y) * 0.5);
      bj.push_back((lam2 * X + mu2 * Y) * 0.5);
    }
  }

  // First X value
  double X0 = ws->readX(0).front();

  // Phase quadrature
  std::vector<double> realY(npoints, 0), imagY(npoints, 0);
  std::vector<double> realE(npoints, 0), imagE(npoints, 0);
  for (size_t i = 0; i < npoints; i++) {
    for (size_t h = 0; h < nspec; h++) {

      // (X,Y,E) with exponential decay removed
      double X = ws->readX(h)[i];
      double Y = ws->readY(h)[i] - n0[h] * exp(-(X - X0) / muLife);
      double E = (ws->readY(h)[i] > poissonLimit)
                     ? ws->readE(h)[i]
                     : sqrt(n0[h] * exp(-(X - X0) / muLife));

      realY[i] += aj[h] * Y;
      imagY[i] += bj[h] * Y;
      realE[i] += aj[h] * aj[h] * E * E;
      imagE[i] += bj[h] * bj[h] * E * E;
    }
    realE[i] = sqrt(realE[i]);
    imagE[i] = sqrt(imagE[i]);

    // Regain exponential decay
    double X = ws->getSpectrum(0)->readX()[i];
    double e = exp(-(X - X0) / muLife);
    realY[i] /= e;
    imagY[i] /= e;
    realE[i] /= e;
    imagE[i] /= e;
  }

  // Populate output workspace
  API::MatrixWorkspace_sptr ows = API::WorkspaceFactory::Instance().create(
      "Workspace2D", 2, npoints + 1, npoints);
  ows->dataY(0).assign(realY.begin(), realY.end());
  ows->dataE(0).assign(realE.begin(), realE.end());
  ows->dataY(1).assign(imagY.begin(), imagY.end());
  ows->dataE(1).assign(imagE.begin(), imagE.end());
  // X
  MantidVec x = ws->readX(0);
  ows->dataX(0).assign(x.begin(), x.end());
  ows->dataX(1).assign(x.begin(), x.end());
  return ows;
}
}
}
