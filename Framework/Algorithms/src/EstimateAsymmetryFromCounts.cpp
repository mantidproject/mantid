//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AsymmetryHelper.h"
#include "MantidAlgorithms/EstimateAsymmetryFromCounts.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <numeric>
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
DECLARE_ALGORITHM(EstimateAsymmetryFromCounts)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void EstimateAsymmetryFromCounts::init() {
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
  declareProperty("XStart",0.1,
	  "The lower limit for calculating the asymmetry (an X value).");
  declareProperty("XEnd",15.0,
	  "The upper limit for calculating the asymmetry  (an X value).");
}

/** Executes the algorithm
 *
 */
void EstimateAsymmetryFromCounts::exec() {
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

  // No spectra specified = process all spectra
  if (spectra.empty()) {
    std::vector<int> allSpectra(numSpectra);
    std::iota(allSpectra.begin(), allSpectra.end(), 0);
    spectra.swap(allSpectra);
  }

  Progress prog(this, 0.0, 1.0, numSpectra + spectra.size());
  if (inputWS != outputWS) {

    // Copy all the Y and E data
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
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
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
	  for (int i = 0; i < specLength; ++i) {
		  PARALLEL_START_INTERUPT_REGION
			  const auto specNum = static_cast<size_t>(spectra[i]);
		  if (spectra[i] > numSpectra) {
			  g_log.error("Spectra size greater than the number of spectra!");
			  throw std::invalid_argument(
				  "Spectra size greater than the number of spectra!");
		  }

	// check start and end times
	double startX = getProperty("XStart");
	double endX = getProperty("XEnd");
	
	if (startX > endX) {
		g_log.warning() << "Start time is after the end time. Swapping the start and end." << '\n';
		double tmp = endX;
		endX = startX;
		startX = tmp;
	}
	else if (startX == endX) {
		throw std::runtime_error("Start and end times are equal, there is no data to apply the algorithm to.");
	}

	auto xData = inputWS->histogram(specNum).binEdges();
	if (startX < xData[0]) {
		  g_log.warning() << "Start time is before the first data point. Using first data point." << '\n';
	}
	if (endX > xData[xData.size()-1]) {
		g_log.warning() << "End time is after the last data point. Using last data point." << '\n';
		g_log.warning() << "Data at late times may dominate the normalisation." << '\n';
	}
	// Calculate the normalised counts
    const Mantid::API::Run &run = inputWS->run();
    const double numGoodFrames= std::stod(run.getProperty("goodfrm")-> value());
	
	const double normConst = estimateNormalisationConst(inputWS->histogram(specNum), numGoodFrames, startX, endX);
	// Calculate the asymmetry  
	outputWS->setHistogram(specNum, normaliseCounts(inputWS->histogram(specNum),numGoodFrames));
    outputWS->mutableY(specNum) /= normConst;
	outputWS->mutableY(specNum) -=  1.0;
    outputWS->mutableE(specNum) /= normConst;
	
    prog.report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Update Y axis units
  outputWS->setYUnit("Asymmetry");

  setProperty("OutputWorkspace", outputWS);
}
} // namespace Algorithm
} // namespace Mantid
