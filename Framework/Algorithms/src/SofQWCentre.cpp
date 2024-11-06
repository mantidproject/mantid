// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SofQWCentre.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQWCentre)

using namespace Kernel;
using namespace API;

/// Default constructor
SofQWCentre::SofQWCentre() {
  useAlgorithm("SofQWNormalisedPolygon");
  deprecatedDate("2024-11-07");
}

/**
 * Create the input properties
 */
void SofQWCentre::init() { SofQW::createCommonInputProperties(*this); }

void SofQWCentre::exec() {
  using namespace Geometry;
  using PhysicalConstants::E_mev_toNeutronWavenumberSq;

  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");

  m_EmodeProperties.initCachedValues(*inputWorkspace, this);
  const int emode = m_EmodeProperties.m_emode;

  std::vector<double> verticalAxis;
  MatrixWorkspace_sptr outputWorkspace = SofQW::setUpOutputWorkspace<DataObjects::Workspace2D>(
      *inputWorkspace, getProperty("QAxisBinning"), verticalAxis, getProperty("EAxisBinning"), m_EmodeProperties);
  setProperty("OutputWorkspace", outputWorkspace);
  const auto &xAxis = outputWorkspace->binEdges(0).rawData();

  // Holds the spectrum-detector mapping
  std::vector<specnum_t> specNumberMapping;
  std::vector<detid_t> detIDMapping;

  const auto &detectorInfo = inputWorkspace->detectorInfo();
  const auto &spectrumInfo = inputWorkspace->spectrumInfo();
  const V3D beamDir = normalize(detectorInfo.samplePosition() - detectorInfo.sourcePosition());
  const double l1 = detectorInfo.l1();
  g_log.debug() << "Source-sample distance: " << l1 << '\n';

  // Loop over input workspace bins, reassigning data to correct bin in output
  // qw workspace
  const size_t numHists = inputWorkspace->getNumberHistograms();
  const size_t numBins = inputWorkspace->blocksize();
  Progress prog(this, 0.0, 1.0, numHists);
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    const auto &spectrumDet = spectrumInfo.detector(i);
    const double efixed = m_EmodeProperties.getEFixed(spectrumDet);

    // For inelastic scattering the simple relationship q=4*pi*sinTheta/lambda
    // does not hold. In order to
    // be completely general we must calculate the momentum transfer by
    // calculating the incident and final
    // wave vectors and then use |q| = sqrt[(ki - kf)*(ki - kf)]

    const auto &detIDs = inputWorkspace->getSpectrum(i).getDetectorIDs();
    auto numDets_d = static_cast<double>(detIDs.size());
    const auto &Y = inputWorkspace->y(i);
    const auto &E = inputWorkspace->e(i);
    const auto &X = inputWorkspace->x(i);

    // Loop over the detectors and for each bin calculate Q
    for (const auto detID : detIDs) {
      try {
        size_t idet = detectorInfo.indexOf(detID);
        // Calculate kf vector direction and then Q for each energy bin
        const V3D scatterDir = normalize(detectorInfo.position(idet) - detectorInfo.samplePosition());
        for (size_t j = 0; j < numBins; ++j) {
          if (X[j] < xAxis.front() || X[j + 1] > xAxis.back())
            continue;

          const double deltaE = 0.5 * (X[j] + X[j + 1]);
          // Compute ki and kf wave vectors and therefore q = ki - kf
          double ei(0.0), ef(0.0);
          if (emode == 1) {
            ei = efixed;
            ef = efixed - deltaE;
            if (ef < 0) {
              std::string mess = "Energy transfer requested in Direct mode exceeds incident "
                                 "energy.\n Found for det ID: " +
                                 std::to_string(idet) + " bin No " + std::to_string(j) +
                                 " with Ei=" + boost::lexical_cast<std::string>(efixed) +
                                 " and energy transfer: " + boost::lexical_cast<std::string>(deltaE);
              throw std::runtime_error(mess);
            }
          } else {
            ei = efixed + deltaE;
            ef = efixed;
            if (ef < 0) {
              std::string mess = "Incident energy of a neutron is negative. Are you trying to "
                                 "process Direct data in Indirect mode?\n Found for det ID: " +
                                 std::to_string(idet) + " bin No " + std::to_string(j) +
                                 " with efied=" + boost::lexical_cast<std::string>(efixed) +
                                 " and energy transfer: " + boost::lexical_cast<std::string>(deltaE);
              throw std::runtime_error(mess);
            }
          }

          if (ei < 0)
            throw std::runtime_error("Negative incident energy. Check binning.");

          const V3D ki = beamDir * sqrt(ei / E_mev_toNeutronWavenumberSq);
          const V3D kf = scatterDir * sqrt(ef / E_mev_toNeutronWavenumberSq);
          const double q = (ki - kf).norm();

          // Test whether it's in range of the Q axis
          if (q < verticalAxis.front() || q > verticalAxis.back())
            continue;
          // Find which q bin this point lies in
          const MantidVec::difference_type qIndex =
              std::upper_bound(verticalAxis.begin(), verticalAxis.end(), q) - verticalAxis.begin() - 1;
          // Find which e bin this point lies in
          const MantidVec::difference_type eIndex =
              std::upper_bound(xAxis.begin(), xAxis.end(), deltaE) - xAxis.begin() - 1;

          // Add this spectra-detector pair to the mapping
          specNumberMapping.emplace_back(outputWorkspace->getSpectrum(qIndex).getSpectrumNo());
          detIDMapping.emplace_back(detID);

          // And add the data and it's error to that bin, taking into account
          // the number of detectors contributing to this bin
          outputWorkspace->mutableY(qIndex)[eIndex] += Y[j] / numDets_d;
          // Standard error on the average
          outputWorkspace->mutableE(qIndex)[eIndex] =
              sqrt((pow(outputWorkspace->e(qIndex)[eIndex], 2) + pow(E[j], 2)) / numDets_d);
        }
      } catch (std::out_of_range &) {
        // Skip invalid detector IDs
        numDets_d -= 1.0;
        continue;
      }
    }
    prog.report();
  }

  // If the input workspace was a distribution, need to divide by q bin width
  if (inputWorkspace->isDistribution())
    this->makeDistribution(*outputWorkspace, verticalAxis);

  // Set the output spectrum-detector mapping
  SpectrumDetectorMapping outputDetectorMap(specNumberMapping, detIDMapping);
  outputWorkspace->updateSpectraUsing(outputDetectorMap);

  // Replace any NaNs in outputWorkspace with zeroes
  if (this->getProperty("ReplaceNaNs")) {
    auto replaceNans = this->createChildAlgorithm("ReplaceSpecialValues");
    replaceNans->setChild(true);
    replaceNans->initialize();
    replaceNans->setProperty("InputWorkspace", outputWorkspace);
    replaceNans->setProperty("OutputWorkspace", outputWorkspace);
    replaceNans->setProperty("NaNValue", 0.0);
    replaceNans->setProperty("InfinityValue", 0.0);
    replaceNans->setProperty("BigNumberThreshold", DBL_MAX);
    replaceNans->execute();
  }
}

/** Divide each bin by the width of its q bin.
 *  @param outputWS :: The output workspace
 *  @param qAxis ::    A vector of the q bin boundaries
 */
void SofQWCentre::makeDistribution(API::MatrixWorkspace &outputWS, const std::vector<double> &qAxis) {
  std::vector<double> widths(qAxis.size());
  std::adjacent_difference(qAxis.begin(), qAxis.end(), widths.begin());

  const size_t numQBins = outputWS.getNumberHistograms();
  for (size_t i = 0; i < numQBins; ++i) {
    auto &Y = outputWS.mutableY(i);
    auto &E = outputWS.mutableE(i);
    using std::placeholders::_1;
    std::transform(Y.begin(), Y.end(), Y.begin(), std::bind(std::divides<double>(), _1, widths[i + 1]));
    std::transform(E.begin(), E.end(), E.begin(), std::bind(std::divides<double>(), _1, widths[i + 1]));
  }
}

} // namespace Mantid::Algorithms
