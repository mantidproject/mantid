// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SofQWPolygon.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ReplaceSpecialValues.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQWPolygon)

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

/// Default constructor
SofQWPolygon::SofQWPolygon() : Rebin2D(), m_Qout(), m_thetaPts(), m_thetaWidth(0.0) {
  useAlgorithm("SofQWNormalisedPolygon");
  deprecatedDate("2024-11-07");
}

/**
 * Initialize the algorithm
 */
void SofQWPolygon::init() { SofQW::createCommonInputProperties(*this); }

/**
 * Execute the algorithm.
 */
void SofQWPolygon::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Progress reports & cancellation
  const auto blocksize = inputWS->blocksize();
  const auto nreports(static_cast<size_t>(inputWS->getNumberHistograms() * blocksize));
  m_progress = std::make_unique<API::Progress>(this, 0.0, 1.0, nreports);
  // Compute input caches
  this->initCachedValues(inputWS);

  MatrixWorkspace_sptr outputWS = SofQW::setUpOutputWorkspace<DataObjects::Workspace2D>(
      *inputWS, getProperty("QAxisBinning"), m_Qout, getProperty("EAxisBinning"), m_EmodeProperties);
  setProperty("OutputWorkspace", outputWS);
  const size_t nenergyBins = blocksize;

  const size_t nTheta = m_thetaPts.size();
  const auto &X = inputWS->x(0);

  // Holds the spectrum-detector mapping
  std::vector<SpectrumDefinition> detIDMapping(outputWS->getNumberHistograms());

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < static_cast<int64_t>(nTheta); ++i) // signed for openmp
  {
    PARALLEL_START_INTERRUPT_REGION

    const double theta = m_thetaPts[i];
    if (theta < 0.0) // One to skip
    {
      continue;
    }

    const auto &spectrumInfo = inputWS->spectrumInfo();
    const auto *det = m_EmodeProperties.m_emode == 1 ? nullptr : &spectrumInfo.detector(i);
    const double halfWidth(0.5 * m_thetaWidth);
    const double thetaLower = theta - halfWidth;
    const double thetaUpper = theta + halfWidth;

    for (size_t j = 0; j < nenergyBins; ++j) {
      m_progress->report("Computing polygon intersections");
      // For each input polygon test where it intersects with
      // the output grid and assign the appropriate weights of Y/E
      const double dE_j = X[j];
      const double dE_jp1 = X[j + 1];

      const double lrQ = m_EmodeProperties.q(dE_jp1, thetaLower, det);

      const V2D ll(dE_j, m_EmodeProperties.q(dE_j, thetaLower, det));
      const V2D lr(dE_jp1, lrQ);
      const V2D ur(dE_jp1, m_EmodeProperties.q(dE_jp1, thetaUpper, det));
      const V2D ul(dE_j, m_EmodeProperties.q(dE_j, thetaUpper, det));
      Quadrilateral inputQ = Quadrilateral(ll, lr, ur, ul);

      DataObjects::FractionalRebinning::rebinToOutput(inputQ, inputWS, i, j, *outputWS, m_Qout);

      // Find which q bin this point lies in
      const MantidVec::difference_type qIndex = std::upper_bound(m_Qout.begin(), m_Qout.end(), lrQ) - m_Qout.begin();
      if (qIndex != 0 && qIndex < static_cast<int>(m_Qout.size())) {
        // Add this spectra-detector pair to the mapping
        PARALLEL_CRITICAL(SofQWPolygon_spectramap) {
          // Could do a more complete merge of spectrum definitions here, but
          // historically only the ID of the first detector in the spectrum is
          // used, so I am keeping that for now.
          detIDMapping[qIndex - 1].add(spectrumInfo.spectrumDefinition(i)[0].first);
        }
      }
    }

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  DataObjects::FractionalRebinning::normaliseOutput(outputWS, inputWS, m_progress.get());

  // Set the output spectrum-detector mapping
  auto outputIndices = outputWS->indexInfo();
  outputIndices.setSpectrumDefinitions(std::move(detIDMapping));
  outputWS->setIndexInfo(outputIndices);

  // Replace any NaNs in outputWorkspace with zeroes
  if (this->getProperty("ReplaceNaNs")) {
    auto replaceNans = this->createChildAlgorithm("ReplaceSpecialValues");
    replaceNans->setChild(true);
    replaceNans->initialize();
    replaceNans->setProperty("InputWorkspace", outputWS);
    replaceNans->setProperty("OutputWorkspace", outputWS);
    replaceNans->setProperty("NaNValue", 0.0);
    replaceNans->setProperty("InfinityValue", 0.0);
    replaceNans->setProperty("BigNumberThreshold", DBL_MAX);
    replaceNans->execute();
  }
}

/**
 * Init variables caches
 * @param workspace :: Workspace pointer
 */
void SofQWPolygon::initCachedValues(const API::MatrixWorkspace_const_sptr &workspace) {
  m_EmodeProperties.initCachedValues(*workspace, this);
  // Index theta cache
  initThetaCache(*workspace);
}

/**
 * A map detector ID and Q ranges
 * This method looks unnecessary as it could be calculated on the fly but
 * the parallelization means that lazy instantation slows it down due to the
 * necessary CRITICAL sections required to update the cache. The Q range
 * values are required very frequently so the total time is more than
 * offset by this precaching step
 */
void SofQWPolygon::initThetaCache(const API::MatrixWorkspace &workspace) {
  const size_t nhist = workspace.getNumberHistograms();
  m_thetaPts = std::vector<double>(nhist);
  size_t ndets(0);
  double minTheta(DBL_MAX), maxTheta(-DBL_MAX);

  const auto &spectrumInfo = workspace.spectrumInfo();
  for (int64_t i = 0; i < static_cast<int64_t>(nhist); ++i) {
    m_progress->report("Calculating detector angles");
    m_thetaPts[i] = -1.0; // Indicates a detector to skip
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;
    // Check to see if there is an EFixed, if not skip it
    try {
      m_EmodeProperties.getEFixed(spectrumInfo.detector(i));
    } catch (std::runtime_error &) {
      continue;
    }
    ++ndets;
    const double theta = spectrumInfo.twoTheta(i);
    m_thetaPts[i] = theta;
    minTheta = std::min(minTheta, theta);
    maxTheta = std::max(maxTheta, theta);
  }

  if (0 == ndets)
    throw std::runtime_error("Unexpected inconsistency found. The number of detectors is 0"
                             ", and the theta width parameter cannot be calculated.");

  m_thetaWidth = (maxTheta - minTheta) / static_cast<double>(ndets);
  g_log.information() << "Calculated detector width in theta=" << (m_thetaWidth * 180.0 / M_PI) << " degrees.\n";
}

} // namespace Mantid::Algorithms
