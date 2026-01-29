// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/SpectrumInfoIterator.h"
#include "MantidBeamline/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <algorithm>
#include <limits>
#include <memory>

using namespace Mantid::Kernel;

namespace Mantid::API {
/// static logger object
Kernel::Logger g_log("ExperimentInfo");

SpectrumInfo::SpectrumInfo(const Beamline::SpectrumInfo &spectrumInfo, const ExperimentInfo &experimentInfo,
                           Geometry::DetectorInfo &detectorInfo)
    : m_experimentInfo(experimentInfo), m_detectorInfo(detectorInfo), m_spectrumInfo(spectrumInfo),
      m_lastDetector(PARALLEL_GET_MAX_THREADS), m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

// Defined as default in source for forward declaration with std::unique_ptr.
SpectrumInfo::~SpectrumInfo() = default;

/// Returns the size of the SpectrumInfo, i.e., the number of spectra.
size_t SpectrumInfo::size() const { return m_spectrumInfo.size(); }

size_t SpectrumInfo::detectorCount() const { return m_spectrumInfo.detectorCount(); }

/// Returns a const reference to the SpectrumDefinition of the spectrum.
const SpectrumDefinition &SpectrumInfo::spectrumDefinition(const size_t index) const {
  m_experimentInfo.updateSpectrumDefinitionIfNecessary(index);
  return m_spectrumInfo.spectrumDefinition(index);
}

const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &SpectrumInfo::sharedSpectrumDefinitions() const {
  for (size_t i = 0; i < size(); ++i)
    m_experimentInfo.updateSpectrumDefinitionIfNecessary(i);
  return m_spectrumInfo.sharedSpectrumDefinitions();
}

/// Returns true if the detector(s) associated with the spectrum are monitors.
bool SpectrumInfo::isMonitor(const size_t index) const {
  const auto spectrumDef = checkAndGetSpectrumDefinition(index);
  return std::all_of(spectrumDef.cbegin(), spectrumDef.cend(),
                     [this](const std::pair<size_t, size_t> &detIndex) { return m_detectorInfo.isMonitor(detIndex); });
}

/// Returns true if the detector(s) associated with the spectrum are masked.
bool SpectrumInfo::isMasked(const size_t index) const {
  const auto spectrumDef = checkAndGetSpectrumDefinition(index);
  return std::all_of(spectrumDef.cbegin(), spectrumDef.cend(),
                     [this](const std::pair<size_t, size_t> &detIndex) { return m_detectorInfo.isMasked(detIndex); });
}

/** Returns L2 (distance from sample to spectrum).
 *
 * For monitors this is defined such that L1+L2 = source-detector distance,
 * i.e., for a monitor in the beamline between source and sample L2 is negative.
 */
double SpectrumInfo::l2(const size_t index) const {
  const auto spectrumDef = checkAndGetSpectrumDefinition(index);
  auto l2 = std::accumulate(
      spectrumDef.cbegin(), spectrumDef.cend(), 0.0,
      [this](double x, const std::pair<size_t, size_t> &detIndex) { return x + m_detectorInfo.l2(detIndex); });
  return l2 / static_cast<double>(spectrumDef.size());
}

/** Returns the scattering angle 2 theta in radians (angle w.r.t. to beam
 *direction).
 *
 * Throws an exception if the spectrum is a monitor.
 */
double SpectrumInfo::twoTheta(const size_t index) const {
  const auto spectrumDef = checkAndGetSpectrumDefinition(index);
  auto twoTheta = std::accumulate(
      spectrumDef.cbegin(), spectrumDef.cend(), 0.0,
      [this](double x, const std::pair<size_t, size_t> &detIndex) { return x + m_detectorInfo.twoTheta(detIndex); });
  return twoTheta / static_cast<double>(spectrumDef.size());
}

/** Returns the signed scattering angle 2 theta in radians (angle w.r.t. to beam
 * direction).
 *
 * Throws an exception if the spectrum is a monitor.
 */
double SpectrumInfo::signedTwoTheta(const size_t index) const {
  const auto spectrumDef = checkAndGetSpectrumDefinition(index);
  auto signedTwoTheta = std::accumulate(spectrumDef.cbegin(), spectrumDef.cend(), 0.0,
                                        [this](double x, const std::pair<size_t, size_t> &detIndex) {
                                          return x + m_detectorInfo.signedTwoTheta(detIndex);
                                        });
  return signedTwoTheta / static_cast<double>(spectrumDef.size());
}

/** Returns the out-of-plane angle in radians (angle w.r.t. to
 * vecPointingHorizontal direction).
 *
 * Throws an exception if the spectrum is a monitor.
 */
double SpectrumInfo::azimuthal(const size_t index) const {
  const auto spectrumDef = checkAndGetSpectrumDefinition(index);
  auto phi = std::accumulate(
      spectrumDef.cbegin(), spectrumDef.cend(), 0.0,
      [this](double x, const std::pair<size_t, size_t> &detIndex) { return x + m_detectorInfo.azimuthal(detIndex); });
  return phi / static_cast<double>(spectrumDef.size());
}

/** Calculate latitude and longitude for given spectrum index.
 *  @param index Index of the spectrum that lat/long are required for
 *  @return A pair containing the latitude and longitude values.
 */
std::pair<double, double> SpectrumInfo::geographicalAngles(const size_t index) const {
  double lat{0.0}, lon{0.0};
  for (const auto &detIndex : checkAndGetSpectrumDefinition(index)) {
    auto latlong = m_detectorInfo.geographicalAngles(detIndex);
    lat += latlong.first;
    lon += latlong.second;
  }
  return std::pair<double, double>(lat / static_cast<double>(spectrumDefinition(index).size()),
                                   lon / static_cast<double>(spectrumDefinition(index).size()));
}

/// Returns the position of the spectrum with given index.
Kernel::V3D SpectrumInfo::position(const size_t index) const {
  const auto spectrumDef = checkAndGetSpectrumDefinition(index);
  auto newPos = std::accumulate(spectrumDef.cbegin(), spectrumDef.cend(), Kernel::V3D(),
                                [this](const auto &x, const std::pair<size_t, size_t> &detIndex) {
                                  return x + m_detectorInfo.position(detIndex);
                                });
  return newPos / static_cast<double>(spectrumDef.size());
}

/** Calculate average diffractometer constants (DIFA, DIFC, TZERO) of detectors
 * associated with this spectrum. Use calibrated values where possible, filling
 * in with uncalibrated values where they're missing
 *  @param index Index of the spectrum that constants are required for
 *  @param warningDets A vector containing the det ids where an uncalibrated
 * value was used in the situation where some dets have calibrated values and
 * some don't
 *  @return map containing the average constants
 */
UnitParametersMap SpectrumInfo::diffractometerConstants(const size_t index, std::vector<detid_t> &warningDets) const {
  if (m_detectorInfo.isScanning()) {
    throw std::runtime_error("Retrieval of diffractometer constants not "
                             "implemented for scanning instrument");
  }
  const auto &spectrumDef = checkAndGetSpectrumDefinition(index);
  std::vector<size_t> detectorIndicesOnly;
  std::vector<detid_t> calibratedDets;
  std::vector<detid_t> uncalibratedDets;
  std::transform(spectrumDef.cbegin(), spectrumDef.cend(), std::back_inserter(detectorIndicesOnly),
                 [](auto const &pair) { return pair.first; });
  double difa{0.}, difc{0.}, tzero{0.};
  for (const auto &detIndex : detectorIndicesOnly) {
    const auto newDiffConstants = m_detectorInfo.diffractometerConstants(detIndex, calibratedDets, uncalibratedDets);
    difa += std::get<0>(newDiffConstants);
    difc += std::get<1>(newDiffConstants);
    tzero += std::get<2>(newDiffConstants);
  }

  if (calibratedDets.size() > 0 && uncalibratedDets.size() > 0) {
    warningDets.insert(warningDets.end(), uncalibratedDets.begin(), uncalibratedDets.end());
  };
  // if no calibration is found then return difc only based on the average
  // of the detector L2 and twoThetas.
  if (calibratedDets.size() == 0) {
    return {{UnitParams::difc, difcUncalibrated(index)}};
  }
  const auto specDefSize = static_cast<double>(spectrumDefinition(index).size());
  return {{UnitParams::difa, difa / specDefSize},
          {UnitParams::difc, difc / specDefSize},
          {UnitParams::tzero, tzero / specDefSize}};
}

/** Calculate average diffractometer constants (DIFA, DIFC, TZERO) of
 * detectors associated with this spectrum. Use calibrated values where
 * possible, filling in with uncalibrated values where they're missing
 *  @param index Index of the spectrum that constants are required for
 *  @return map containing the average constants
 */
UnitParametersMap SpectrumInfo::diffractometerConstants(const size_t index) const {
  std::vector<int> warningDets;
  return diffractometerConstants(index, warningDets);
}

/** Calculate average uncalibrated DIFC value of detectors associated with
 * this spectrum
 *  @param index Index of the spectrum that DIFC is required for
 *  @return The average DIFC
 */
double SpectrumInfo::difcUncalibrated(const size_t index) const {
  // calculate difc based on the average of the detector L2 and twoThetas.
  // This will be different to the average of the per detector difcs. This is
  // for backwards compatibility because Mantid always used to calculate
  // spectrum level difc's this way
  return 1. / Kernel::Units::tofToDSpacingFactor(l1(), l2(index), twoTheta(index), 0.);
}

/** Get the detector values relevant to unit conversion for a workspace index
 * @param inputUnit :: The input unit (Empty implies "all")
 * @param outputUnit :: The output unit (Empty implies "all")
 * @param emode :: The energy mode
 * @param signedTheta :: Return twotheta with sign or without
 * @param wsIndex :: The workspace index
 * @param pmap :: a map containing values for conversion parameters that are
required by unit classes to perform their conversions eg efixed. It can
contain values on the way in if a look up isn't desired here eg if value
supplied in parameters to the calling algorithm
 */
void SpectrumInfo::getDetectorValues(const Kernel::Unit &inputUnit, const Kernel::Unit &outputUnit,
                                     const Kernel::DeltaEMode::Type emode, const bool signedTheta, int64_t wsIndex,
                                     UnitParametersMap &pmap) const {
  if (!hasDetectors(wsIndex))
    return;
  pmap[UnitParams::l2] = l2(wsIndex);

  if (!isMonitor(wsIndex)) {
    // The scattering angle for this detector (in radians).
    try {
      if (signedTheta)
        pmap[UnitParams::twoTheta] = signedTwoTheta(wsIndex);
      else
        pmap[UnitParams::twoTheta] = twoTheta(wsIndex);
    } catch (const std::runtime_error &e) {
      g_log.warning(e.what());
    }
    if (emode != Kernel::DeltaEMode::Elastic && pmap.find(UnitParams::efixed) == pmap.end()) {
      std::shared_ptr<const Geometry::IDetector> det(&detector(wsIndex), [](auto *) {});
      try {
        pmap[UnitParams::efixed] = m_experimentInfo.getEFixedGivenEMode(det, emode);
        g_log.debug() << "Detector: " << det->getID() << " EFixed: " << pmap[UnitParams::efixed] << "\n";
      } catch (std::runtime_error &) {
        // let the unit classes work out if this is a problem
      }
    }

    try {
      std::set<std::string> diffConstUnits = {"dSpacing", "MomentumTransfer", "Empty"};
      if ((emode == Kernel::DeltaEMode::Elastic) &&
          (diffConstUnits.count(inputUnit.unitID()) || diffConstUnits.count(outputUnit.unitID()))) {
        std::vector<detid_t> warnDetIds;
        auto diffConstsMap = diffractometerConstants(wsIndex, warnDetIds);
        pmap.insert(diffConstsMap.begin(), diffConstsMap.end());
        if (warnDetIds.size() > 0) {
          createDetectorIdLogMessages(warnDetIds, wsIndex);
        }
      } else {
        pmap[UnitParams::difc] = difcUncalibrated(wsIndex);
      }
    } catch (const std::runtime_error &e) {
      g_log.warning(e.what());
    }
  } else {
    pmap[UnitParams::twoTheta] = 0.0;
    pmap[UnitParams::efixed] = std::numeric_limits<double>::min();
    // Energy transfer is meaningless for a monitor, so set l2 to 0.
    if (outputUnit.unitID().find("DeltaE") != std::string::npos) {
      pmap[UnitParams::l2] = 0.0;
    }
    pmap[UnitParams::difc] = 0;
  }
}

void SpectrumInfo::createDetectorIdLogMessages(const std::vector<detid_t> &detids, int64_t wsIndex) const {
  std::string detIDstring;
  auto iter = detids.begin();
  auto itEnd = detids.end();
  for (; iter != itEnd; ++iter) {
    detIDstring += std::to_string(*iter) + ",";
  }

  if (!detIDstring.empty()) {
    detIDstring.pop_back(); // Drop last comma
  }
  g_log.warning("Incomplete set of calibrated diffractometer constants found for "
                "workspace index" +
                std::to_string(wsIndex) + ". Using uncalibrated values for detectors " + detIDstring);
}

/// Returns true if the spectrum is associated with detectors in the
/// instrument.
bool SpectrumInfo::hasDetectors(const size_t index) const {
  // Workspaces can contain invalid detector IDs. Those IDs will be silently
  // ignored here until this is fixed.
  return spectrumDefinition(index).size() > 0;
}

/// Returns true if the spectrum is associated with exactly one detector.
bool SpectrumInfo::hasUniqueDetector(const size_t index) const {
  // Workspaces can contain invalid detector IDs. Those IDs will be silently
  // ignored here until this is fixed.
  return spectrumDefinition(index).size() == 1;
}

/** Set the mask flag of the spectrum with given index. Not thread safe.
 *
 * Currently this simply sets the mask flags for the underlying detectors. */
void SpectrumInfo::setMasked(const size_t index, bool masked) {
  for (const auto &detIndex : checkAndGetSpectrumDefinition(index))
    m_detectorInfo.setMasked(detIndex, masked);
}

/// Return a const reference to the detector or detector group of the spectrum
/// with given index.
const Geometry::IDetector &SpectrumInfo::detector(const size_t index) const { return getDetector(index); }

/// Returns the source position.
Kernel::V3D SpectrumInfo::sourcePosition() const { return m_detectorInfo.sourcePosition(); }

/// Returns the sample position.
Kernel::V3D SpectrumInfo::samplePosition() const { return m_detectorInfo.samplePosition(); }

/// Returns L1 (distance from source to sample).
double SpectrumInfo::l1() const { return m_detectorInfo.l1(); }

const Geometry::IDetector &SpectrumInfo::getDetector(const size_t index) const {
  auto thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] == index)
    return *m_lastDetector[thread];

  // Note: This function body has big overlap with the method
  // MatrixWorkspace::getDetector(). The plan is to eventually remove the
  // latter, once SpectrumInfo is in widespread use.
  const auto &specDef = spectrumDefinition(index);
  const size_t ndets = specDef.size();
  if (ndets == 1) {
    // If only 1 detector for the spectrum number, just return it
    const auto detIndex = specDef[0].first;
    m_lastDetector[thread] = m_detectorInfo.getDetectorPtr(detIndex);
  } else if (ndets == 0) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No "
                                           "detectors for this workspace "
                                           "index.",
                                           std::to_string(index));
  } else {
    // Else need to construct a DetectorGroup and use that
    std::vector<std::shared_ptr<const Geometry::IDetector>> det_ptrs;
    for (const auto &specDefIndex : specDef) {
      const auto detIndex = specDefIndex.first;
      det_ptrs.emplace_back(m_detectorInfo.getDetectorPtr(detIndex));
    }
    m_lastDetector[thread] = std::make_shared<Geometry::DetectorGroup>(det_ptrs);
  }
  m_lastIndex[thread] = index;
  return *m_lastDetector[thread];
}

const SpectrumDefinition &SpectrumInfo::checkAndGetSpectrumDefinition(const size_t index) const {
  if (spectrumDefinition(index).size() == 0)
    throw Kernel::Exception::NotFoundError("SpectrumInfo: No detectors for this workspace index.",
                                           std::to_string(index));
  return spectrumDefinition(index);
}

// Begin method for iterator
SpectrumInfoIt SpectrumInfo::begin() { return SpectrumInfoIt(*this, 0); }

// End method for iterator
SpectrumInfoIt SpectrumInfo::end() { return SpectrumInfoIt(*this, size()); }

const SpectrumInfoConstIt SpectrumInfo::cbegin() const { return SpectrumInfoConstIt(*this, 0); }

// End method for iterator
const SpectrumInfoConstIt SpectrumInfo::cend() const { return SpectrumInfoConstIt(*this, size()); }

} // namespace Mantid::API
