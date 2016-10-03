//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RemoveExpDecay.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <vector>

namespace {
/// Number of microseconds in one second (10^6)
constexpr double MICROSECONDS_PER_SECOND{1000000.0};
/// Muon lifetime in microseconds
constexpr double MUON_LIFETIME_MICROSECONDS{
    Mantid::PhysicalConstants::MuonLifetime * MICROSECONDS_PER_SECOND};
}

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using API::Progress;
using std::size_t;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(MuonRemoveExpDecay)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void MuonRemoveExpDecay::init() {
  declareProperty(make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The name of the input 2D workspace.");
  declareProperty(make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name of the output 2D workspace.");
  std::vector<int> empty;
  declareProperty(
      Kernel::make_unique<Kernel::ArrayProperty<int>>("Spectra", empty),
      "The workspace indices to remove the exponential decay from.");
}

/** Executes the algorithm
 *
 */
void MuonRemoveExpDecay::exec() {
  std::vector<int> spectra = getProperty("Spectra");

  // Get original workspace
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  int numSpectra = static_cast<int>(inputWS->size() / inputWS->blocksize());

  // Create output workspace with same dimensions as input
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (inputWS != outputWS) {
    outputWS = API::WorkspaceFactory::Instance().create(inputWS);
  }
  // Share the X values
  for (size_t i = 0; i < static_cast<size_t>(numSpectra); ++i) {
    outputWS->setSharedX(i, inputWS->sharedX(i));
  }

  if (spectra.empty()) {

    Progress prog(this, 0.0, 1.0, numSpectra);
    // Do all the spectra
    PARALLEL_FOR2(inputWS, outputWS)
    for (int i = 0; i < numSpectra; ++i) {
      PARALLEL_START_INTERUPT_REGION
      const auto index = static_cast<size_t>(i);
      // Make sure reference to input X vector is obtained after output one
      // because in the case
      // where the input & output workspaces are the same, it might move if the
      // vectors were shared.
      const auto &xIn = inputWS->x(index);

      auto &yOut = outputWS->mutableY(index);
      auto &eOut = outputWS->mutableE(index);

      removeDecayData(xIn, inputWS->y(index), yOut);
      removeDecayError(xIn, inputWS->e(index), eOut);
      double normConst = calNormalisationConst(outputWS, i);

      // do scaling and substract by minus 1.0
      const size_t nbins = outputWS->y(index).size();
      for (size_t j = 0; j < nbins; j++) {
        yOut[j] /= normConst;
        yOut[j] -= 1.0;
        eOut[j] /= normConst;
      }

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  } else {
    Progress prog(this, 0.0, 1.0, numSpectra + spectra.size());
    if (inputWS != outputWS) {

      // Copy all the Y and E data
      PARALLEL_FOR2(inputWS, outputWS)
      for (int64_t i = 0; i < int64_t(numSpectra); ++i) {
        PARALLEL_START_INTERUPT_REGION
        const auto index = static_cast<size_t>(i);
        outputWS->setSharedY(index, inputWS->sharedY(index));
        outputWS->setSharedE(index, inputWS->sharedE(index));
        prog.report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }

    // Do the specified spectra only
    int specLength = static_cast<int>(spectra.size());
    PARALLEL_FOR2(inputWS, outputWS)
    for (int i = 0; i < specLength; ++i) {
      PARALLEL_START_INTERUPT_REGION
      if (spectra[i] > numSpectra) {
        g_log.error("Spectra size greater than the number of spectra!");
        throw std::invalid_argument(
            "Spectra size greater than the number of spectra!");
      }
      // Get references to the x data
      const auto &xIn = inputWS->x(spectra[i]);
      auto &yOut = outputWS->mutableY(spectra[i]);
      auto &eOut = outputWS->mutableE(spectra[i]);

      removeDecayData(xIn, inputWS->y(spectra[i]), yOut);
      removeDecayError(xIn, inputWS->e(spectra[i]), eOut);

      const double normConst = calNormalisationConst(outputWS, spectra[i]);

      // do scaling and substract by minus 1.0
      const size_t nbins = outputWS->y(i).size();
      for (size_t j = 0; j < nbins; j++) {
        yOut[j] /= normConst;
        yOut[j] -= 1.0;
        eOut[j] /= normConst;
      }

      prog.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  // Update Y axis units
  outputWS->setYUnit("Asymmetry");

  setProperty("OutputWorkspace", outputWS);
}

/** This method corrects the errors for one spectrum.
 *	 The muon lifetime is in microseconds not seconds, i.e. 2.1969811 rather
 *   than 0.0000021969811.
 *   This is because the data is in microseconds.
 *   @param inX ::  The X vector
 *   @param inE ::  The input error vector
 *   @param outE :: The output error vector
 */
void MuonRemoveExpDecay::removeDecayError(const HistogramData::HistogramX &inX,
                                          const HistogramData::HistogramE &inE,
                                          HistogramData::HistogramE &outE) {
  // Do the removal
  for (size_t i = 0; i < inE.size(); ++i) {
    if (inE[i] != 0.0)
      outE[i] = inE[i] * exp(inX[i] / MUON_LIFETIME_MICROSECONDS);
    else
      outE[i] = exp(inX[i] / MUON_LIFETIME_MICROSECONDS);
  }
}

/** This method corrects the data for one spectrum.
 *	 The muon lifetime is in microseconds not seconds, i.e. 2.1969811 rather
 *   than 0.0000021969811.
 *   This is because the data is in microseconds.
 *   @param inX ::  The X vector
 *   @param inY ::  The input data vector
 *   @param outY :: The output data vector
 */
void MuonRemoveExpDecay::removeDecayData(const HistogramData::HistogramX &inX,
                                         const HistogramData::HistogramY &inY,
                                         HistogramData::HistogramY &outY) {
  // Do the removal
  for (size_t i = 0; i < inY.size(); ++i) {
    if (inY[i] != 0.0)
      outY[i] = inY[i] * exp(inX[i] / MUON_LIFETIME_MICROSECONDS);
    else
      outY[i] = 0.1 * exp(inX[i] / MUON_LIFETIME_MICROSECONDS);
  }
}

/**
 * calculate normalisation constant after the exponential decay has been removed
 * to a linear fitting function
 * @param ws ::  workspace
 * @param wsIndex :: workspace index
 * @return normalisation constant
*/
double MuonRemoveExpDecay::calNormalisationConst(API::MatrixWorkspace_sptr ws,
                                                 int wsIndex) {
  double retVal = 1.0;

  API::IAlgorithm_sptr fit;
  fit = createChildAlgorithm("Fit", -1, -1, true);

  std::stringstream ss;
  ss << "name=LinearBackground,A0=" << ws->y(wsIndex)[0] << ",A1=" << 0.0
     << ",ties=(A1=0.0)";
  std::string function = ss.str();

  fit->setPropertyValue("Function", function);
  fit->setProperty("InputWorkspace", ws);
  fit->setProperty("WorkspaceIndex", wsIndex);
  fit->setPropertyValue("Minimizer", "Levenberg-MarquardtMD");
  fit->setProperty("Ties", "A1=0.0");
  fit->execute();

  std::string fitStatus = fit->getProperty("OutputStatus");
  API::IFunction_sptr result = fit->getProperty("Function");
  std::vector<std::string> paramnames = result->getParameterNames();

  // Check order of names
  if (paramnames[0].compare("A0") != 0) {
    g_log.error() << "Parameter 0 should be A0, but is " << paramnames[0]
                  << '\n';
    throw std::invalid_argument(
        "Parameters are out of order @ 0, should be A0");
  }
  if (paramnames[1].compare("A1") != 0) {
    g_log.error() << "Parameter 1 should be A1, but is " << paramnames[1]
                  << '\n';
    throw std::invalid_argument(
        "Parameters are out of order @ 0, should be A1");
  }

  if (!fitStatus.compare("success")) {
    const double A0 = result->getParameter(0);

    if (A0 < 0) {
      g_log.warning() << "When trying to fit Asymmetry normalisation constant "
                         "this constant comes out negative."
                      << "To proceed Asym norm constant set to 1.0\n";
    } else {
      retVal = A0;
    }
  } else {
    g_log.warning() << "Fit falled. Status = " << fitStatus
                    << "\nFor workspace index " << wsIndex
                    << "\nAsym norm constant set to 1.0\n";
  }

  return retVal;
}

} // namespace Algorithm
} // namespace Mantid
