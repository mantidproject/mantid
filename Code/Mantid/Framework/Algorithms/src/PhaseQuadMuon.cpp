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

  //for (size_t i = 0; i < 50; i++)
  //  std::cout << inputWs->readX(0)[i] << "\t" << inputWs->readY(0)[i] << "\t"
  //            << inputWs->readE(0)[i] << "\n";

  // Remove exponential decay and save results into tempWs
  API::MatrixWorkspace_sptr tempws = loseExponentialDecay(inputWs);

  //std::cout << "--------------------------\n";
  //for (size_t i = 0; i < 50; i++)
  //  std::cout << tempws->readX(0)[i] << "\t" << tempws->readY(0)[i] << "\t"
  //            << tempws->readE(0)[i] << "\n";

  //// Compute squashograms
  //API::MatrixWorkspace_sptr ows = squash(tempws, phaseTable);

  //// Regain exponential decay
  //regainExponential(ows);

  //setProperty("OutputWorkspace", ows);
  setProperty("OutputWorkspace",tempws);
}



//----------------------------------------------------------------------------------------------
/** Remove exponential decay from input histograms, i.e., calculate asymmetry
* @param ws :: [Input/Output] Workspace containing the spectra to remove exponential from
*/
API::MatrixWorkspace_sptr
PhaseQuadMuon::loseExponentialDecay(const API::MatrixWorkspace_sptr &ws) {

#define MULIFE 2.19703

  size_t nspec = ws->getNumberHistograms();
  size_t npoints = ws->blocksize();

  API::MatrixWorkspace_sptr ows = API::WorkspaceFactory::Instance().create(ws);

  for (size_t h = 0; h < ws->getNumberHistograms(); h++) {

    MantidVec X = ws->getSpectrum(h)->readX();
    MantidVec Y = ws->getSpectrum(h)->readY();
    MantidVec E = ws->getSpectrum(h)->readE();

    //for (int i = 0; i < npoints; i++) {
    //  double usey = specIn->readY()[i];
    //  double oops = ((usey <= 0) || (specIn->readE()[i] >= m_bigNumber));
    //  outY[i] = oops ? 0 : log(usey);
    //  outE[i] = oops ? m_bigNumber : specIn->readE()[i] / usey;
    //}

    double s, sx, sy;
    s = sx = sy = 0;
    for (int i = 0; i < npoints; i++) {

      if (Y[i]>0) {
        double sig = E[i] * E[i] / Y[i] / Y[i];
        s += 1. / sig;
        sx += (X[i]-X[0]) / sig;
        sy += log(Y[i]) / sig;

      }
    }
    double N0 = (sy + sx / MULIFE ) / s;
    N0 = exp(N0);

    std::vector<double> outX(npoints, 0.);
    std::vector<double> outY(npoints, 0.);
    std::vector<double> outE(npoints, 0.);

    for (int i = 0; i < npoints; i++) {
      outX[i] = X[i];
      outY[i] = Y[i] - N0 * exp(-X[i] / MULIFE);
      outE[i] = E[i];
    }

    ows->dataX(h).assign(outX.begin(), outX.end());
    ows->dataY(h).assign(outY.begin(), outY.end());
    ows->dataE(h).assign(outE.begin(), outE.end());

  } // Histogram loop

  return ows;

#undef MULIFE
}

//----------------------------------------------------------------------------------------------
/** Compute Squashograms
* @param ws :: [Input/Output] workspace containing the asymmetry in the lab frame
*/
API::MatrixWorkspace_sptr
PhaseQuadMuon::squash(const API::MatrixWorkspace_sptr &ws,
                      const API::ITableWorkspace_sptr &phase) {

  double sxx = 0;
  double syy = 0;
  double sxy = 0;

  size_t nspec = ws->getNumberHistograms();
  size_t npoints = ws->blocksize();

  for (size_t h = 0; h < nspec; h++) {
    double phi = phase->Double(h,1);
    double X = cos(phi);
    double Y = sin(phi);
    sxx += X * X;
    syy += Y * Y;
    sxy += X * Y;
  }

  double lam1 = 2 * syy / (sxx * syy - sxy * sxy);
  double mu1 = 2 * sxy / (sxy * sxy - sxx * syy);
  double lam2 = 2 * sxy / (sxy * sxy - sxx * syy);
  double mu2 = 2 * sxx / (sxx * syy - sxy * sxy);
  std::vector<double> aj, bj;
  for (int h = 0; h < nspec; h++) {
    double phi = phase->Double(h,1);
    double X = cos(phi);
    double Y = sin(phi);
    aj.push_back((lam1 * X + mu1 * Y) * 0.5);
    bj.push_back((lam2 * X + mu2 * Y) * 0.5);
  }

  std::vector<double> data1(npoints, 0), data2(npoints, 0);
  std::vector<double> sigm1(npoints, 0), sigm2(npoints, 0);
  for (int i = 0; i < npoints; i++) {
    for (int h = 0; h < nspec; h++) {
      auto spec = ws->getSpectrum(h);
      data1[i] += aj[h] * spec->readY()[i];
      data2[i] += bj[h] * spec->readY()[i];
      sigm1[i] += aj[h] * aj[h] * spec->readE()[i] * spec->readE()[i];
      sigm2[i] += bj[h] * bj[h] * spec->readE()[i] * spec->readE()[i];
    }
    sigm1[i] = sqrt(sigm1[i]);
    sigm2[i] = sqrt(sigm2[i]);
  }

  API::MatrixWorkspace_sptr ows = API::WorkspaceFactory::Instance().create("Workspace2D",2,npoints,npoints);
  ows->dataY(0).assign(data1.begin(),data1.end());
  ows->dataE(0).assign(sigm1.begin(),sigm1.end());
  ows->dataY(1).assign(data2.begin(),data2.end());
  ows->dataE(1).assign(sigm2.begin(),sigm2.end());
  // X
  MantidVec x = ws->readX(0);
  ows->dataX(0).assign(x.begin(),x.end());
  ows->dataX(1).assign(x.begin(),x.end());
  return ows;
}

//----------------------------------------------------------------------------------------------
/** Put back in exponential decay
* @param ws :: [Input/Output] Workspace containing squashograms to update
*/
void PhaseQuadMuon::regainExponential(API::MatrixWorkspace_sptr &ws) {

#define MULIFE 2.19703

  auto specRe = ws->getSpectrum(0);
  auto specIm = ws->getSpectrum(1);

  size_t npoints = ws->blocksize();

  for (int i = 0; i < npoints; i++) {
    double x = ws->getSpectrum(0)->readX()[i];
    double e = exp(-x / MULIFE);
    specRe->dataY()[i] /= e;
    specIm->dataY()[i] /= e;
    specRe->dataE()[i] /= e;
    specIm->dataE()[i] /= e;
  }

#undef MULIFE
}
}
}
