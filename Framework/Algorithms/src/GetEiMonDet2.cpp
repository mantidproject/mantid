// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GetEiMonDet2.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PhysicalConstants;

namespace {
/** A private namespace to store string constants dealing with
 *  tables returned by the FindEPP algorithm.
 */
namespace EPPTableLiterals {
/// Title of the fit status column in EPP tables
const static std::string FIT_STATUS_COLUMN("FitStatus");
/// Title of the peak centre column in EPP tables
const static std::string PEAK_CENTRE_COLUMN("PeakCentre");
/// Tag for successfully fitted rows in EPP tables
const static std::string FIT_STATUS_SUCCESS("success");
} // namespace EPPTableLiterals

/** A private namespace listing the different ways to index
 *  spectra in Mantid.
 */
namespace IndexTypes {
/// Tag for detector ids
const static std::string DETECTOR_ID("Detector ID");
/// Tag for spectrum numbers
const static std::string SPECTRUM_NUMBER("Spectrum Number");
/// Tag for workspace indices
const static std::string WORKSPACE_INDEX("Workspace Index");
} // namespace IndexTypes

/** A private namespace holding the property names of
 *  GetEiMonDet algorithm, version 2.
 */
namespace PropertyNames {
/// Name of the detector epp table property
const static std::string DETECTOR_EPP_TABLE("DetectorEPPTable");
/// Name of the detector workspace property
const static std::string DETECTOR_WORKSPACE("DetectorWorkspace");
/// Name of the detector index list property
const static std::string DETECTORS("Detectors");
/// Name of the incident energy output property
const static std::string INCIDENT_ENERGY("IncidentEnergy");
/// Name of the monitor and detector fields' type property
const static std::string INDEX_TYPE("IndexType");
/// Name of the monitor index property
const static std::string MONITOR("Monitor");
/// Name of the monitor epp table property
const static std::string MONITOR_EPP_TABLE("MonitorEPPTable");
/// Name of the monitor workspace property
const static std::string MONITOR_WORKSPACE("MonitorWorkspace");
/// Name of the incident energy guess property
const static std::string NOMINAL_ENERGY("NominalIncidentEnergy");
/// Name of the neutron pulse interval property
const static std::string PULSE_INTERVAL("PulseInterval");
} // namespace PropertyNames

/** A private namespace holding names for sample log entries.
 */
namespace SampleLogs {
/// Name of the pulse interval sample log
const static std::string PULSE_INTERVAL("pulse_interval");
} // namespace SampleLogs
} // anonymous namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the algorithm factory.
DECLARE_ALGORITHM(GetEiMonDet2)

/** Construct a GetEiMonDet2 object.
 *
 */
GetEiMonDet2::GetEiMonDet2() { useAlgorithm("GetEiMonDet", 3); }

/** Initialized the algorithm.
 *
 */
void GetEiMonDet2::init() {
  auto tofWorkspace = boost::make_shared<CompositeValidator>();
  tofWorkspace->add<WorkspaceUnitValidator>("TOF");
  tofWorkspace->add<InstrumentValidator>();
  auto mandatoryArrayProperty =
      boost::make_shared<MandatoryValidator<std::vector<int>>>();
  auto mandatoryIntProperty = boost::make_shared<MandatoryValidator<int>>();
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);

  declareProperty(make_unique<WorkspaceProperty<>>(
                      PropertyNames::DETECTOR_WORKSPACE.c_str(), "",
                      Direction::Input, tofWorkspace),
                  "A workspace containing the detector spectra.");
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(
          PropertyNames::DETECTOR_EPP_TABLE.c_str(), "", Direction::Input),
      "An EPP table corresponding to " + PropertyNames::DETECTOR_WORKSPACE +
          ".");
  const std::vector<std::string> indexTypes{IndexTypes::DETECTOR_ID,
                                            IndexTypes::SPECTRUM_NUMBER,
                                            IndexTypes::WORKSPACE_INDEX};
  declareProperty(PropertyNames::INDEX_TYPE, IndexTypes::DETECTOR_ID,
                  boost::make_shared<StringListValidator>(indexTypes),
                  "The type of indices " + PropertyNames::DETECTORS + " and " +
                      PropertyNames::MONITOR + " refer to.");
  declareProperty(Kernel::make_unique<ArrayProperty<int>>(
                      PropertyNames::DETECTORS.c_str(), mandatoryArrayProperty),
                  "A list of detector ids/spectrum number/workspace indices.");
  declareProperty(make_unique<WorkspaceProperty<>>(
                      PropertyNames::MONITOR_WORKSPACE.c_str(), "",
                      Direction::Input, PropertyMode::Optional, tofWorkspace),
                  "A Workspace containing the monitor spectrum. If empty, " +
                      PropertyNames::DETECTOR_WORKSPACE + " will be used.");
  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      PropertyNames::MONITOR_EPP_TABLE.c_str(), "",
                      Direction::Input, PropertyMode::Optional),
                  "An EPP table corresponding to " +
                      PropertyNames::MONITOR_WORKSPACE);
  setPropertySettings(
      PropertyNames::MONITOR_EPP_TABLE,
      make_unique<EnabledWhenProperty>(PropertyNames::MONITOR_WORKSPACE.c_str(),
                                       IS_NOT_DEFAULT));
  declareProperty(PropertyNames::MONITOR, EMPTY_INT(), mandatoryIntProperty,
                  "Monitor's detector id/spectrum number/workspace index.");
  declareProperty(PropertyNames::PULSE_INTERVAL, EMPTY_DBL(),
                  "Interval between neutron pulses, in microseconds. Taken "
                  "from the sample logs, if not specified.");
  declareProperty(
      PropertyNames::NOMINAL_ENERGY, EMPTY_DBL(), mustBePositive,
      "Incident energy guess. Taken from the sample logs, if not specified.");
  declareProperty(PropertyNames::INCIDENT_ENERGY, EMPTY_DBL(), mustBePositive,
                  "Calculated incident energy.", Direction::Output);
}

/** Executes the algorithm.
 *
 */
void GetEiMonDet2::exec() {
  progress(0);
  m_detectorWs = getProperty(PropertyNames::DETECTOR_WORKSPACE);
  m_detectorEPPTable = getProperty(PropertyNames::DETECTOR_EPP_TABLE);
  m_monitorWs = getProperty(PropertyNames::MONITOR_WORKSPACE);
  if (!m_monitorWs) {
    m_monitorWs = m_detectorWs;
  }
  m_monitorEPPTable = getProperty(PropertyNames::MONITOR_EPP_TABLE);
  if (!m_monitorEPPTable) {
    m_monitorEPPTable = m_detectorEPPTable;
  }

  std::vector<size_t> detectorIndices;
  size_t monitorIndex;
  parseIndices(detectorIndices, monitorIndex);
  sanitizeIndices(detectorIndices, monitorIndex);

  double sampleToDetectorDistance;
  double detectorEPP;
  averageDetectorDistanceAndTOF(detectorIndices, sampleToDetectorDistance,
                                detectorEPP);
  progress(0.9);
  double monitorToSampleDistance;
  double monitorEPP;
  monitorDistanceAndTOF(monitorIndex, monitorToSampleDistance, monitorEPP);
  const double flightLength =
      sampleToDetectorDistance + monitorToSampleDistance;
  double timeOfFlight = computeTOF(flightLength, detectorEPP, monitorEPP);
  const double velocity = flightLength / timeOfFlight * 1e6;
  const double energy = 0.5 * NeutronMass * velocity * velocity / meV;
  progress(1.0);
  g_log.notice() << "Final time-of-flight:" << timeOfFlight << " which gives "
                 << energy << " as " << PropertyNames::INCIDENT_ENERGY << ".\n";

  setProperty(PropertyNames::INCIDENT_ENERGY, energy);
}

/** Calculates the average distance between the sample and given
 *  detectors.
 *  @param detectorIndices A vector containing workspace indices
 *         to the detectors
 *  @param sampleToDetectorDistance An output parameter for the
 *         average distance between the sample and the detectors
 *  @param detectorEPP An output parameter for the average position
 *         of the detectors' elastic peak
 */
void GetEiMonDet2::averageDetectorDistanceAndTOF(
    const std::vector<size_t> &detectorIndices,
    double &sampleToDetectorDistance, double &detectorEPP) {
  auto peakPositionColumn =
      m_detectorEPPTable->getColumn(EPPTableLiterals::PEAK_CENTRE_COLUMN);
  auto fitStatusColumn =
      m_detectorEPPTable->getColumn(EPPTableLiterals::FIT_STATUS_COLUMN);
  if (!peakPositionColumn || !fitStatusColumn) {
    throw std::runtime_error("The workspace specified by " +
                             PropertyNames::DETECTOR_EPP_TABLE +
                             " doesn't seem to contain the expected table");
  }

  const auto sample = m_detectorWs->getInstrument()->getSample();
  double distanceSum = 0;
  double eppSum = 0;
  size_t n = 0;
  auto &spectrumInfo = m_detectorWs->spectrumInfo();
  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for if ( m_detectorEPPTable->threadSafe())
             reduction(+: n, distanceSum, eppSum))
  for (int i = 0; i < static_cast<int>(detectorIndices.size());
       ++i) { // NOLINT (modernize-for-loop)
    PARALLEL_START_INTERUPT_REGION
    const size_t index = detectorIndices[i];
    interruption_point();
    if (index >= peakPositionColumn->size()) {
      throw std::runtime_error("Invalid value in " + PropertyNames::DETECTORS);
    }
    if (fitStatusColumn->cell<std::string>(index) ==
        EPPTableLiterals::FIT_STATUS_SUCCESS) {
      if (!spectrumInfo.hasDetectors(index)) {
        throw std::runtime_error("No detector specified by " +
                                 PropertyNames::DETECTORS + " found");
      }
      if (spectrumInfo.isMonitor(index)) {
        g_log.warning() << "Workspace index " << index
                        << " should be detector, but is marked as monitor.\n";
      }
      if (!spectrumInfo.isMasked(index)) {
        const double d = spectrumInfo.detector(index).getDistance(*sample);
        distanceSum += d;
        const double epp = (*peakPositionColumn)[index];
        eppSum += epp;
        ++n;
        g_log.debug() << "Including detector at workspace index " << index
                      << " - distance: " << d << " EPP: " << epp << ".\n";
      } else {
        g_log.debug() << "Excluding masked detector at workspace index "
                      << index << ".\n";
      }
    } else {
      g_log.debug()
          << "Excluding detector with unsuccessful fit at workspace index "
          << index << ".\n";
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (n == 0) {
    throw std::runtime_error("No successful detector fits found in " +
                             PropertyNames::DETECTOR_EPP_TABLE);
  }
  sampleToDetectorDistance = distanceSum / static_cast<double>(n);
  g_log.information() << "Average sample-to-detector distance: "
                      << sampleToDetectorDistance << ".\n";
  detectorEPP = eppSum / static_cast<double>(n);
  g_log.information() << "Average detector EPP: " << detectorEPP << ".\n";
}

/** Calculates the time of flight from the monitor to the detectors.
 *  @param distance The total distance between the monitor and the detectors
 *  @param detectorEPP The position of the detectors' elastic peak
 *  @param monitorEPP The position of the monitor's elastic peak
 *  @return The time of flight between the monitor and the detectors
 */
double GetEiMonDet2::computeTOF(const double distance, const double detectorEPP,
                                const double monitorEPP) {
  double timeOfFlight = detectorEPP - monitorEPP;
  double nominalIncidentEnergy = getProperty(PropertyNames::NOMINAL_ENERGY);
  if (nominalIncidentEnergy == EMPTY_DBL()) {
    if (!m_detectorWs->run().hasProperty("Ei")) {
      throw std::runtime_error("No " + PropertyNames::NOMINAL_ENERGY +
                               " given and no Ei field found in sample logs");
    }
    nominalIncidentEnergy =
        std::stod(m_detectorWs->run().getProperty("Ei")->value());
  }
  // In microseconds.
  const double nominalTimeOfFlight =
      distance / std::sqrt(2 * nominalIncidentEnergy * meV / NeutronMass) * 1e6;
  g_log.information() << "Nominal time-of-flight: " << nominalTimeOfFlight
                      << ".\n";
  // Check if the obtained time-of-flight makes any sense.
  const double energyTolerance = 0.2; // As a fraction of nominal energy.
  const double toleranceLimit =
      1 / std::sqrt(1 + energyTolerance) * nominalTimeOfFlight;
  double pulseInterval = getProperty(PropertyNames::PULSE_INTERVAL);
  if (pulseInterval == EMPTY_DBL()) {
    if (m_detectorWs->run().hasProperty(SampleLogs::PULSE_INTERVAL)) {
      pulseInterval = m_detectorWs->run().getPropertyAsSingleValue(
          SampleLogs::PULSE_INTERVAL);
      pulseInterval *= 1e6; // To microseconds.
    }
  }
  const double pulseIntervalLimit = nominalTimeOfFlight - pulseInterval / 2;
  const double lowerTimeLimit =
      toleranceLimit > pulseIntervalLimit ? toleranceLimit : pulseIntervalLimit;
  const double upperTimeLimit =
      toleranceLimit > pulseIntervalLimit
          ? 1 / std::sqrt(1 - energyTolerance) * nominalTimeOfFlight
          : nominalTimeOfFlight + pulseInterval / 2;
  g_log.notice() << "Expecting a final time-of-flight between "
                 << lowerTimeLimit << " and " << upperTimeLimit << ".\n";
  g_log.notice() << "Calculated time-of-flight: " << timeOfFlight << ".\n";
  if (timeOfFlight <= lowerTimeLimit) {
    g_log.notice() << "Calculated time-of-flight too small. "
                   << "Frame delay has to be taken into account.\n";
  }
  unsigned delayFrameCount = 0;
  while (timeOfFlight <= lowerTimeLimit) {
    // Neutrons hit the detectors in a later frame.
    interruption_point();
    if (pulseInterval == EMPTY_DBL()) {
      throw std::runtime_error(PropertyNames::PULSE_INTERVAL +
                               " not specified nor found in sample logs");
    }
    ++delayFrameCount;
    timeOfFlight = delayFrameCount * pulseInterval - monitorEPP + detectorEPP;
  }
  if (timeOfFlight > upperTimeLimit) {
    throw std::runtime_error("Calculated time-of-flight too large");
  }
  return timeOfFlight;
}

/** Obtains the distance between the monitor and the sample.
 *  @param monitorIndex Workspace index specifying the monitor spectra
 *  @param monitorToSampleDistance Output parameter for the monitor to
 *         sample distance
 *  @param monitorEPP Output parameter for the monitors elastic peak
 *         position
 */
void GetEiMonDet2::monitorDistanceAndTOF(const size_t monitorIndex,
                                         double &monitorToSampleDistance,
                                         double &monitorEPP) const {
  // Monitor-to-sample distance.
  const auto peakPositionColumn =
      m_monitorEPPTable->getColumn(EPPTableLiterals::PEAK_CENTRE_COLUMN);
  const auto fitStatusColumn =
      m_monitorEPPTable->getColumn(EPPTableLiterals::FIT_STATUS_COLUMN);
  if (!peakPositionColumn || !fitStatusColumn) {
    throw std::runtime_error("The workspace specified by " +
                             PropertyNames::DETECTOR_EPP_TABLE +
                             " doesn't seem to contain the expected table");
  }
  if (static_cast<size_t>(monitorIndex) >= peakPositionColumn->size()) {
    throw std::runtime_error("Invalid " + PropertyNames::MONITOR);
  }
  if (fitStatusColumn->cell<std::string>(monitorIndex) !=
      EPPTableLiterals::FIT_STATUS_SUCCESS) {
    throw std::runtime_error("No successful monitor fit found in " +
                             PropertyNames::MONITOR_EPP_TABLE);
  }
  auto &spectrumInfo = m_monitorWs->spectrumInfo();
  if (spectrumInfo.isMasked(monitorIndex)) {
    throw std::runtime_error("Monitor spectrum is masked");
  }
  if (!spectrumInfo.isMonitor(monitorIndex)) {
    g_log.warning() << "The monitor spectrum is not actually marked "
                    << "as monitor.\n";
  }
  const auto sample = m_detectorWs->getInstrument()->getSample();
  monitorToSampleDistance =
      spectrumInfo.position(monitorIndex).distance(sample->getPos());
  g_log.information() << "Monitor-to-sample distance: "
                      << monitorToSampleDistance << ".\n";

  // Monitor peak position.
  monitorEPP = (*peakPositionColumn)[monitorIndex];
  g_log.information() << "Monitor EPP: " << monitorEPP << ".\n";
}

namespace {
/** Transforms detector and monitor indices according to given maps.
 *  @param detectors A vector of detector indices to be transformed
 *  @param monitor A monitor index to be transformed
 *  @param detectorIndexMap A map from detector to workspace indices
 *  @param monitorIndexMap A map from monitor to workspace indices
 *  @param detectorIndices Output parameter for the detector
 *         workspace indices
 *  @param monitorIndex Output parameter for the monitor
 *         workspace indices
 */
template <typename Map>
void mapIndices(const std::vector<int> &detectors, const int monitor,
                const Map &detectorIndexMap, const Map &monitorIndexMap,
                std::vector<size_t> &detectorIndices, size_t &monitorIndex) {
  auto back = std::back_inserter(detectorIndices);
  std::transform(
      detectors.cbegin(), detectors.cend(), back, [&detectorIndexMap](int i) {
        try {
          return detectorIndexMap.at(i);
        } catch (std::out_of_range &) {
          throw std::runtime_error(PropertyNames::DETECTORS + " out of range.");
        }
      });
  try {
    monitorIndex = monitorIndexMap.at(monitor);
  } catch (std::out_of_range &) {
    throw std::runtime_error(PropertyNames::MONITOR + " out of range.");
  }
}
} // namespace

/** Parser detector and monitor indices from user's input and
 *  transfrorms them to workspace indices.
 *  @param detectorIndices Output parameter for the detector workspace
 *         indices
 *  @param monitorIndex Output parameter for the monitor workspace index
 */
void GetEiMonDet2::parseIndices(std::vector<size_t> &detectorIndices,
                                size_t &monitorIndex) const {
  detectorIndices.clear();
  const std::vector<int> detectors = getProperty(PropertyNames::DETECTORS);
  const int monitor = getProperty(PropertyNames::MONITOR);
  const std::string indexType = getProperty(PropertyNames::INDEX_TYPE);
  if (indexType == IndexTypes::DETECTOR_ID) {
    const auto detectorIndexMap =
        m_detectorWs->getDetectorIDToWorkspaceIndexMap();
    const auto monitorIndexMap =
        m_monitorWs->getDetectorIDToWorkspaceIndexMap();
    mapIndices(detectors, monitor, detectorIndexMap, monitorIndexMap,
               detectorIndices, monitorIndex);
  } else if (indexType == IndexTypes::SPECTRUM_NUMBER) {
    const auto detectorIndexMap =
        m_detectorWs->getSpectrumToWorkspaceIndexMap();
    const auto monitorIndexMap = m_monitorWs->getSpectrumToWorkspaceIndexMap();
    mapIndices(detectors, monitor, detectorIndexMap, monitorIndexMap,
               detectorIndices, monitorIndex);
  } else {
    // There is a type mismatch between what UserStringParser returns
    // (unsigned int) and workspace index (size_t), thus the copying.
    auto back = std::back_inserter(detectorIndices);
    std::copy(detectors.begin(), detectors.end(), back);
    if (monitor < 0) {
      throw std::runtime_error("Monitor cannot be negative.");
    }
    monitorIndex = static_cast<size_t>(monitor);
  }
}

/** Erases duplicate indices and check that monitor index is not in
 *  the detector index list.
 *  @param detectorIndices A vector of detector indices
 *  @param monitorIndex Monitor index
 */
void GetEiMonDet2::sanitizeIndices(std::vector<size_t> &detectorIndices,
                                   size_t monitorIndex) const {
  std::sort(detectorIndices.begin(), detectorIndices.end());
  detectorIndices.erase(
      std::unique(detectorIndices.begin(), detectorIndices.end()),
      detectorIndices.end());
  if (m_monitorWs == m_detectorWs) {
    if (std::find(detectorIndices.begin(), detectorIndices.end(),
                  monitorIndex) != detectorIndices.end()) {
      throw std::runtime_error(PropertyNames::MONITOR + " is also listed in " +
                               PropertyNames::DETECTORS);
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
