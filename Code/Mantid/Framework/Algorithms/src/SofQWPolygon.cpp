//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SofQWPolygon.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidGeometry/Math/LaszloIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidGeometry/Math/Vertex2D.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQWPolygon)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Geometry::IDetector_const_sptr;
using Geometry::DetectorGroup;
using Geometry::DetectorGroup_const_sptr;
using Geometry::ConvexPolygon;
using Geometry::Quadrilateral;
using Geometry::Vertex2D;

/// Default constructor
SofQWPolygon::SofQWPolygon() : Rebin2D(), m_Qout(), m_thetaPts(), m_thetaWidth(0.0) {}

//----------------------------------------------------------------------------------------------

/**
 * Initialize the algorithm
 */
void SofQWPolygon::init() { SofQW::createInputProperties(*this); }

/**
 * Execute the algorithm.
 */
void SofQWPolygon::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  // Do the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(inputWS)) {
    throw std::invalid_argument(
        "The input workspace must have common binning across all spectra");
  }

  MatrixWorkspace_sptr outputWS =
      SofQW::setUpOutputWorkspace(inputWS, getProperty("QAxisBinning"), m_Qout);
  setProperty("OutputWorkspace", outputWS);
  const size_t nenergyBins = inputWS->blocksize();

  // Progress reports & cancellation
  const size_t nreports(static_cast<size_t>(inputWS->getNumberHistograms() *
                                            inputWS->blocksize()));
  m_progress = boost::shared_ptr<API::Progress>(
      new API::Progress(this, 0.0, 1.0, nreports));

  // Compute input caches
  this->initCachedValues(inputWS);

  const size_t nTheta = m_thetaPts.size();
  const MantidVec &X = inputWS->readX(0);

  // Holds the spectrum-detector mapping
  std::vector<specid_t> specNumberMapping;
  std::vector<detid_t> detIDMapping;

  // Select the calculate Q method based on the mode
  // rather than doing this repeatedly in the loop
  typedef double (SofQWPolygon::*QCalculation)(double, double, double, double) const;
  QCalculation qCalculator;
  if (m_EmodeProperties.m_emode == 1) {
    qCalculator = &SofQWPolygon::calculateDirectQ;
  } else {
    qCalculator = &SofQWPolygon::calculateIndirectQ;
  }

  /* PARALLEL_FOR2(inputWS, outputWS) */
  for (int64_t i = 0; i < static_cast<int64_t>(nTheta);
       ++i) // signed for openmp
  {
    /* PARALLEL_START_INTERUPT_REGION */

    const double theta = m_thetaPts[i];
    if (theta < 0.0) // One to skip
    {
      continue;
    }

    IDetector_const_sptr det = inputWS->getDetector(i);
    double halfWidth(0.5 * m_thetaWidth);
    const double thetaLower = theta - halfWidth;
    const double thetaUpper = theta + halfWidth;
    const double efixed = m_EmodeProperties.getEFixed(det);

    for (size_t j = 0; j < nenergyBins; ++j) {
      m_progress->report("Computing polygon intersections");
      // For each input polygon test where it intersects with
      // the output grid and assign the appropriate weights of Y/E
      const double dE_j = X[j];
      const double dE_jp1 = X[j + 1];

      const double lrQ = (this->*qCalculator)(efixed, dE_jp1, thetaLower, 0.0);

      const V2D ll(dE_j, (this->*qCalculator)(efixed, dE_j, thetaLower, 0.0));
      const V2D lr(dE_jp1, lrQ);
      const V2D ur(dE_jp1,
                   (this->*qCalculator)(efixed, dE_jp1, thetaUpper, 0.0));
      const V2D ul(dE_j, (this->*qCalculator)(efixed, dE_j, thetaUpper, 0.0));
      Quadrilateral inputQ = Quadrilateral(ll, lr, ur, ul);

      rebinToOutput(inputQ, inputWS, i, j, outputWS, m_Qout);

      // Find which q bin this point lies in
      const MantidVec::difference_type qIndex =
          std::upper_bound(m_Qout.begin(), m_Qout.end(), lrQ) - m_Qout.begin();
      if (qIndex != 0 && qIndex < static_cast<int>(m_Qout.size())) {
        // Add this spectra-detector pair to the mapping
        specNumberMapping.push_back(
            outputWS->getSpectrum(qIndex - 1)->getSpectrumNo());
        detIDMapping.push_back(det->getID());
      }
    }

    /* PARALLEL_END_INTERUPT_REGION */
  }
  /* PARALLEL_CHECK_INTERUPT_REGION */

  normaliseOutput(outputWS, inputWS);

  // Set the output spectrum-detector mapping
  SpectrumDetectorMapping outputDetectorMap(specNumberMapping, detIDMapping);
  outputWS->updateSpectraUsing(outputDetectorMap);
}

/**
 * Calculate the Q value for a direct instrument
 * @param efixed An efixed value
 * @param deltaE The energy change
 * @param twoTheta The value of the scattering angle
 * @param psi The value of the azimuth
 * @return The value of Q
 */
double SofQWPolygon::calculateDirectQ(const double efixed, const double deltaE,
                                const double twoTheta, const double psi) const {
  const double ki = std::sqrt(efixed * SofQW::energyToK());
  const double kf = std::sqrt((efixed - deltaE) * SofQW::energyToK());
  const double Qx = ki - kf * std::cos(twoTheta);
  const double Qy = -kf * std::sin(twoTheta) * std::cos(psi);
  const double Qz = -kf * std::sin(twoTheta) * std::sin(psi);
  return std::sqrt(Qx * Qx + Qy * Qy + Qz * Qz);
}

/**
 * Calculate the Q value for a direct instrument
 * @param efixed An efixed value
 * @param deltaE The energy change
 * @param twoTheta The value of the scattering angle
 * @param psi The value of the azimuth
 * @return The value of Q
 */
double SofQWPolygon::calculateIndirectQ(const double efixed, const double deltaE,
                                  const double twoTheta,
                                  const double psi) const {
  UNUSED_ARG(psi);
  const double ki = std::sqrt((efixed + deltaE) * SofQW::energyToK());
  const double kf = std::sqrt(efixed * SofQW::energyToK());
  const double Qx = ki - kf * std::cos(twoTheta);
  const double Qy = -kf * std::sin(twoTheta);
  return std::sqrt(Qx * Qx + Qy * Qy);
}

/**
 * Init variables caches
 * @param workspace :: Workspace pointer
 */
void SofQWPolygon::initCachedValues(API::MatrixWorkspace_const_sptr workspace) {
  m_EmodeProperties.initCachedValues(workspace, this);
  // Index theta cache
  initThetaCache(workspace);
}

/**
 * A map detector ID and Q ranges
 * This method looks unnecessary as it could be calculated on the fly but
 * the parallelization means that lazy instantation slows it down due to the
 * necessary CRITICAL sections required to update the cache. The Q range
 * values are required very frequently so the total time is more than
 * offset by this precaching step
 */
void SofQWPolygon::initThetaCache(API::MatrixWorkspace_const_sptr workspace) {
  const size_t nhist = workspace->getNumberHistograms();
  m_thetaPts = std::vector<double>(nhist);
  size_t ndets(0);
  double minTheta(DBL_MAX), maxTheta(-DBL_MAX);

  for (int64_t i = 0; i < (int64_t)nhist; ++i) // signed for OpenMP
  {

    m_progress->report("Calculating detector angles");
    IDetector_const_sptr det;
    try {
      det = workspace->getDetector(i);
      // Check to see if there is an EFixed, if not skip it
      try {
        m_EmodeProperties.getEFixed(det);
      } catch (std::runtime_error &) {
        det.reset();
      }
    } catch (Kernel::Exception::NotFoundError &) {
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if (!det || det->isMonitor()) {
      m_thetaPts[i] = -1.0; // Indicates a detector to skip
    } else {
      ++ndets;
      const double theta = workspace->detectorTwoTheta(det);
      m_thetaPts[i] = theta;
      if (theta < minTheta) {
        minTheta = theta;
      } else if (theta > maxTheta) {
        maxTheta = theta;
      }
    }
  }

  m_thetaWidth = (maxTheta - minTheta) / static_cast<double>(ndets);
  g_log.information() << "Calculated detector width in theta="
                      << (m_thetaWidth * 180.0 / M_PI) << " degrees.\n";
}

} // namespace Mantid
} // namespace Algorithms
