//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;

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
      new API::WorkspaceProperty<API::ITableWorkspace>(
          "DetectorTable", "", Direction::Input, API::PropertyMode::Optional),
      "Name of the table containing detector phases");

  declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace to hold squashograms");
}

/** Executes the algorithm
 *
 */
void PhaseQuadMuon::exec() {

  // Get input workspace
  API::MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");

  // Get input phase table
  API::ITableWorkspace_sptr phaseTable = getProperty("DetectorTable");

  // Get N0, the normalization constant: N(t) = N0 * exp(-x/tau)
  std::vector<double> n0 = getExponentialDecay(inputWs);

  //// Compute squashograms
  API::MatrixWorkspace_sptr ows = squash(inputWs, phaseTable, n0);

  std::cout << "--------------------------\n";
  for (size_t i = 0; i < ows->blocksize(); i++)
    std::cout << ows->readX(0)[i] << "\t" << ows->readY(0)[i] << "\t"
              << ows->readE(0)[i] << "\t" << ows->readX(1)[i] << "\t"
              << ows->readY(1)[i] << "\t" << ows->readE(1)[i] << "\n";

  setProperty("OutputWorkspace", ows);
  //setProperty("OutputWorkspace",tempws);
}



//----------------------------------------------------------------------------------------------
/** Calculates the normalization constant for the exponential decay
* @param ws :: [Input] Workspace containing the spectra to remove exponential from
* @return :: Vector containing the normalization constants, N0, for each spectrum
*/
std::vector<double>
PhaseQuadMuon::getExponentialDecay(const API::MatrixWorkspace_sptr &ws) {

#define MULIFE 2.19703

  size_t nspec = ws->getNumberHistograms();
  size_t npoints = ws->blocksize();

  std::vector<double> n0(nspec, 0.);

  for (size_t h = 0; h < ws->getNumberHistograms(); h++) {

    MantidVec X = ws->getSpectrum(h)->readX();
    MantidVec Y = ws->getSpectrum(h)->readY();
    MantidVec E = ws->getSpectrum(h)->readE();

    double s, sx, sy;
    s = sx = sy = 0;
    for (int i = 0; i < npoints; i++) {

      if (Y[i] > 0) {
        double sig = E[i] * E[i] / Y[i] / Y[i];
        s += 1. / sig;
        sx += (X[i] - X[0]) / sig;
        sy += log(Y[i]) / sig;
      }
    }
    n0[h] = exp((sy + sx / MULIFE) / s);
  }

  return n0;

#undef MULIFE
}

//----------------------------------------------------------------------------------------------
/** Compute Squashograms
* @param ws :: [Input/Output] workspace containing the asymmetry in the lab frame
*/
API::MatrixWorkspace_sptr
PhaseQuadMuon::squash(const API::MatrixWorkspace_sptr &ws,
                      const API::ITableWorkspace_sptr &phase,
                      const std::vector<double> &n0) {

#define MULIFE 2.19703
#define MPOISSONLIM 30

  size_t nspec = ws->getNumberHistograms();
  size_t npoints = ws->blocksize();

  if (n0.size() != nspec) {
    throw std::invalid_argument("Invalid normalization constants");
  }

  std::vector<double> aj, bj;
  {
    // Calculate coefficients aj, bj

    double sxx = 0;
    double syy = 0;
    double sxy = 0;

    for (size_t h = 0; h < nspec; h++) {
      double phi = phase->Double(h, 1);
      double X = n0[h] * cos(phi);
      double Y = n0[h] * sin(phi);
      sxx += X * X;
      syy += Y * Y;
      sxy += X * Y;
    }

    double lam1 = 2 * syy / (sxx * syy - sxy * sxy);
    double mu1 = 2 * sxy / (sxy * sxy - sxx * syy);
    double lam2 = 2 * sxy / (sxy * sxy - sxx * syy);
    double mu2 = 2 * sxx / (sxx * syy - sxy * sxy);
    for (int h = 0; h < nspec; h++) {
      double phi = phase->Double(h, 1);
      double X = n0[h] * cos(phi);
      double Y = n0[h] * sin(phi);
      aj.push_back((lam1 * X + mu1 * Y) * 0.5);
      bj.push_back((lam2 * X + mu2 * Y) * 0.5);
    }
  }

  // Phase quadrature
  std::vector<double> realY(npoints, 0), imagY(npoints, 0);
  std::vector<double> realE(npoints, 0), imagE(npoints, 0);
  for (int i = 0; i < npoints; i++) {
    for (int h = 0; h < nspec; h++) {

      // (X,Y,E) with exponential decay removed
      double X = ws->readX(h)[i];
      double Y = ws->readY(h)[i] - n0[h] * exp(-X / MULIFE);
      double E =
          (Y > MPOISSONLIM) ? ws->readE(h)[i] : sqrt(n0[h] * exp(-X / MULIFE));

      realY[i] += aj[h] * Y;
      imagY[i] += bj[h] * Y;
      realE[i] += aj[h] * aj[h] * E * E;
      imagE[i] += bj[h] * bj[h] * E * E;
    }
    realE[i] = sqrt(realE[i]);
    imagE[i] = sqrt(imagE[i]);

    // Regain exponential decay
    double x = ws->getSpectrum(0)->readX()[i];
    double e = exp(-x / MULIFE);
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

#undef MPOISSONLIM
#undef MULIFE
}

}
}
