//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/PhaseQuadMuon.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FrameworkManager.h"

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
          "PhaseTable", "", Direction::Input, API::PropertyMode::Optional),
      "Name of the Phase Table");

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
  API::ITableWorkspace_sptr phaseTable = getProperty("PhaseTable");

  // Set number of histograms
  m_nHist = static_cast<int>(inputWs->getNumberHistograms());
  // Set number of data points per histogram
  m_nData = static_cast<int>(inputWs->getSpectrum(0)->readY().size());

  // Create temporary workspace to perform operations on it
  API::MatrixWorkspace_sptr tempWs =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", m_nHist,
                                                   m_nData + 1, m_nData));

  // Create output workspace with two spectra (squashograms)
  API::MatrixWorkspace_sptr outputWs =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", 2,
                                                   m_nData + 1, m_nData));
  outputWs->getAxis(0)->unit() = inputWs->getAxis(0)->unit();

  // Remove exponential decay and save results into tempWs
  loseExponentialDecay(tempWs);

  // Compute squashograms
  squash(tempWs, outputWs);

  // Regain exponential decay
  regainExponential(outputWs);

  setProperty("OutputWorkspace", outputWs);
}



//----------------------------------------------------------------------------------------------
/** Remove exponential decay from input histograms, i.e., calculate asymmetry
* @param tempWs :: workspace containing the spectra to remove exponential from
*/
void PhaseQuadMuon::loseExponentialDecay(API::MatrixWorkspace_sptr tempWs) {

  // TODO rewrite this algorithm using RemoveExponencialDecay
}

//----------------------------------------------------------------------------------------------
/** Compute Squashograms
* @param tempWs :: input workspace containing the asymmetry in the lab frame
* @param outputWs :: output workspace to hold squashograms
*/
void PhaseQuadMuon::squash(const API::MatrixWorkspace_sptr tempWs,
                           API::MatrixWorkspace_sptr outputWs) {

  double sxx = 0;
  double syy = 0;
  double sxy = 0;

  for (int h = 0; h < m_nHist; h++) {
    auto data = m_histData[h];
    double X = data.n0 * data.alpha * cos(data.phi);
    double Y = data.n0 * data.alpha * sin(data.phi);
    sxx += X * X;
    syy += Y * Y;
    sxy += X * Y;
  }

  double lam1 = 2 * syy / (sxx * syy - sxy * sxy);
  double mu1 = 2 * sxy / (sxy * sxy - sxx * syy);
  double lam2 = 2 * sxy / (sxy * sxy - sxx * syy);
  double mu2 = 2 * sxx / (sxx * syy - sxy * sxy);
  std::vector<double> aj, bj;
  for (int h = 0; h < m_nHist; h++) {
    auto data = m_histData[h];
    double X = data.n0 * data.alpha * cos(data.phi);
    double Y = data.n0 * data.alpha * sin(data.phi);
    aj.push_back((lam1 * X + mu1 * Y) * 0.5);
    bj.push_back((lam2 * X + mu2 * Y) * 0.5);
  }

  std::vector<double> data1(m_nData, 0), data2(m_nData, 0);
  std::vector<double> sigm1(m_nData, 0), sigm2(m_nData, 0);
  for (int i = 0; i < m_nData; i++) {
    for (int h = 0; h < m_nHist; h++) {
      auto spec = tempWs->getSpectrum(h);
      data1[i] += aj[h] * spec->readY()[i];
      data2[i] += bj[h] * spec->readY()[i];
      sigm1[i] += aj[h] * aj[h] * spec->readE()[i] * spec->readE()[i];
      sigm2[i] += bj[h] * bj[h] * spec->readE()[i] * spec->readE()[i];
    }
    sigm1[i] = sqrt(sigm1[i]);
    sigm2[i] = sqrt(sigm2[i]);
  }

  outputWs->getSpectrum(0)->dataX() = tempWs->getSpectrum(0)->readX();
  outputWs->getSpectrum(0)->dataY() = data1;
  outputWs->getSpectrum(0)->dataE() = sigm1;
  outputWs->getSpectrum(1)->dataX() = tempWs->getSpectrum(1)->readX();
  outputWs->getSpectrum(1)->dataY() = data2;
  outputWs->getSpectrum(1)->dataE() = sigm2;
}

//----------------------------------------------------------------------------------------------
/** Put back in exponential decay
* @param outputWs :: output workspace with squashograms to update
*/
void PhaseQuadMuon::regainExponential(API::MatrixWorkspace_sptr outputWs) {
  auto specRe = outputWs->getSpectrum(0);
  auto specIm = outputWs->getSpectrum(1);

  for (int i = 0; i < m_nData; i++) {
    double x = outputWs->getSpectrum(0)->readX()[i];
    double e = exp(-x / m_muLife / 1000);
    specRe->dataY()[i] /= e;
    specIm->dataY()[i] /= e;
    specRe->dataE()[i] /= e;
    specIm->dataE()[i] /= e;
  }
}
}
}
