#include "MantidAlgorithms/GetEiMonDet3.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"

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
}


/** A private namespace holding the property names of
 *  GetEiMonDet algorithm, version 2.
 */
namespace Prop {
/// Name of the detector epp table property
const static std::string DETECTOR_EPP_TABLE("DetectorEPPTable");
/// Name of the detector workspace property
const static std::string DETECTOR_WORKSPACE("DetectorWorkspace");
/// Name of the incident energy output property
const static std::string INCIDENT_ENERGY("IncidentEnergy");
/// Name of the monitor index property
const static std::string MONITOR("Monitor");
/// Name of the monitor epp table property
const static std::string MONITOR_EPP_TABLE("MonitorEPPTable");
/// Name of the monitor workspace property
const static std::string MONITOR_WORKSPACE("MonitorWorkspace");
/// Name of the neutron pulse interval property
const static std::string PULSE_INTERVAL("PulseInterval");
}
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the algorithm factory.
DECLARE_ALGORITHM(GetEiMonDet3)

/// Returns algorithm's name for identification
const std::string GetEiMonDet3::name() const {
  return "GetEiMonDet";
}

/// Returns a summary of algorithm's purpose
const std::string GetEiMonDet3::summary() const {
  return "Calculates the kinetic energy of neutrons leaving the source based "
         "on the time it takes for them to travel between a monitor and a "
         "set of detectors.";
}

/// Returns algorithm's version for identification
int GetEiMonDet3::version() const {
  return 3;
}

const std::vector<std::string> GetEiMonDet3::seeAlso() const {
  return {"GetEi"};
}

/// Algorithm's category for identification overriding a virtual method
const std::string GetEiMonDet3::category() const {
  return "Inelastic\\Ei";
}

/** A private namespace holding names for sample log entries.
 */
namespace SampleLogs {
/// Name of the pulse interval sample log
const static std::string PULSE_INTERVAL("pulse_interval");
}

/** Initialized the algorithm.
 *
 */
void GetEiMonDet3::init() {
  auto tofWorkspace = boost::make_shared<Kernel::CompositeValidator>();
  tofWorkspace->add<API::WorkspaceUnitValidator>("TOF");
  tofWorkspace->add<API::InstrumentValidator>();
  auto mandatoryIntProperty = boost::make_shared<Kernel::MandatoryValidator<int>>();
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0);

  declareWorkspaceInputProperties<API::MatrixWorkspace, API::IndexType::SpectrumNum | API::IndexType::WorkspaceIndex>(Prop::DETECTOR_WORKSPACE, "A workspace containing the detector spectra.", tofWorkspace);
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
          Prop::DETECTOR_EPP_TABLE, "", Kernel::Direction::Input),
      "An EPP table corresponding to " + Prop::DETECTOR_WORKSPACE +
          ".");
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      Prop::MONITOR_WORKSPACE.c_str(), "",
                      Kernel::Direction::Input, API::PropertyMode::Optional, tofWorkspace),
                  "A Workspace containing the monitor spectrum. If empty, " +
                      Prop::DETECTOR_WORKSPACE + " will be used.");
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
                      Prop::MONITOR_EPP_TABLE.c_str(), "",
                      Kernel::Direction::Input, API::PropertyMode::Optional),
                  "An EPP table corresponding to " +
                      Prop::MONITOR_WORKSPACE);
  setPropertySettings(
      Prop::MONITOR_EPP_TABLE,
      Kernel::make_unique<Kernel::EnabledWhenProperty>(Prop::MONITOR_WORKSPACE,
                                       Kernel::IS_NOT_DEFAULT));
  declareProperty(Prop::MONITOR, EMPTY_INT(), mandatoryIntProperty,
                  "Usable monitor's workspace index.");
  declareProperty(Prop::PULSE_INTERVAL, EMPTY_DBL(),
                  "Interval between neutron pulses, in microseconds. Taken "
                  "from the sample logs, if not specified.");
  declareProperty(Prop::INCIDENT_ENERGY, EMPTY_DBL(), mustBePositive,
                  "Calculated incident energy.", Kernel::Direction::Output);
}

/** Executes the algorithm.
 *
 */
void GetEiMonDet3::exec() {
  progress(0);
  Indexing::SpectrumIndexSet detectorIndices;
  std::tie(m_detectorWs, detectorIndices) =
      getWorkspaceAndIndices<API::MatrixWorkspace>(Prop::DETECTOR_WORKSPACE);

  m_detectorEPPTable = getProperty(Prop::DETECTOR_EPP_TABLE);
  m_monitorWs = getProperty(Prop::MONITOR_WORKSPACE);
  if (!m_monitorWs) {
    m_monitorWs = m_detectorWs;
  }
  m_monitorEPPTable = getProperty(Prop::MONITOR_EPP_TABLE);
  if (!m_monitorEPPTable) {
    m_monitorEPPTable = m_detectorEPPTable;
  }
  const int monitorIndex = getProperty(Prop::MONITOR);


  double sampleToDetectorDistance;
  double detectorEPP;
  averageDetectorDistanceAndTOF(detectorIndices, sampleToDetectorDistance,
                                detectorEPP);
  progress(0.9);
  double monitorToSampleDistance;
  double monitorEPP;
  monitorDistanceAndTOF(monitorIndex, monitorToSampleDistance, monitorEPP);
  double timeOfFlight = computeTOF(detectorEPP, monitorEPP);
  const double flightLength =
      sampleToDetectorDistance + monitorToSampleDistance;
  const double velocity = flightLength / timeOfFlight * 1e6;
  using namespace PhysicalConstants;
  const double energy = 0.5 * NeutronMass * velocity * velocity / meV;
  progress(1.0);
  g_log.notice() << "Final time-of-flight:" << timeOfFlight << " which gives "
                 << energy << " as " << Prop::INCIDENT_ENERGY << ".\n";

  setProperty(Prop::INCIDENT_ENERGY, energy);
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
void GetEiMonDet3::averageDetectorDistanceAndTOF(const Indexing::SpectrumIndexSet &detectorIndices,
    double &sampleToDetectorDistance, double &detectorEPP) {
  auto peakPositionColumn =
      m_detectorEPPTable->getColumn(EPPTableLiterals::PEAK_CENTRE_COLUMN);
  auto fitStatusColumn =
      m_detectorEPPTable->getColumn(EPPTableLiterals::FIT_STATUS_COLUMN);
  if (!peakPositionColumn || !fitStatusColumn) {
    throw std::runtime_error("The workspace specified by " +
                             Prop::DETECTOR_EPP_TABLE +
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
  for (int i = 0; i < static_cast<int>(detectorIndices.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    const size_t index = detectorIndices[i];
    interruption_point();
    if (fitStatusColumn->cell<std::string>(index) ==
        EPPTableLiterals::FIT_STATUS_SUCCESS) {
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
                             Prop::DETECTOR_EPP_TABLE);
  }
  sampleToDetectorDistance = distanceSum / static_cast<double>(n);
  g_log.information() << "Average sample-to-detector distance: "
                      << sampleToDetectorDistance << ".\n";
  detectorEPP = eppSum / static_cast<double>(n);
  g_log.information() << "Average detector EPP: " << detectorEPP << ".\n";
}

/** Calculates the time of flight from the monitor to the detectors.
 *  @param detectorEPP The position of the detectors' elastic peak
 *  @param monitorEPP The position of the monitor's elastic peak
 *  @return The time of flight between the monitor and the detectors
 */
double GetEiMonDet3::computeTOF(const double detectorEPP,
                                const double monitorEPP) {
  using namespace PhysicalConstants;
  double timeOfFlight = detectorEPP - monitorEPP;
  // Check if the obtained time-of-flight makes any sense.
  if (timeOfFlight <= 0.) {
    double pulseInterval = getProperty(Prop::PULSE_INTERVAL);
    if (pulseInterval == EMPTY_DBL()) {
      if (m_detectorWs->run().hasProperty(SampleLogs::PULSE_INTERVAL)) {
        pulseInterval = m_detectorWs->run().getPropertyAsSingleValue(
            SampleLogs::PULSE_INTERVAL);
        pulseInterval *= 1e6; // To microseconds.
      }
    }
    g_log.notice() << "Frame delay of " << pulseInterval << " microseconds will be added to the time-of-flight.\n";
    timeOfFlight += pulseInterval;
  }
  g_log.notice() << "Calculated time-of-flight: " << timeOfFlight << ".\n";
  return timeOfFlight;
}

/** Obtains the distance between the monitor and the sample.
 *  @param monitorIndex Workspace index specifying the monitor spectra
 *  @param monitorToSampleDistance Output parameter for the monitor to
 *         sample distance
 *  @param monitorEPP Output parameter for the monitors elastic peak
 *         position
 */
void GetEiMonDet3::monitorDistanceAndTOF(const size_t monitorIndex,
                                         double &monitorToSampleDistance,
                                         double &monitorEPP) const {
  // Monitor-to-sample distance.
  const auto peakPositionColumn =
      m_monitorEPPTable->getColumn(EPPTableLiterals::PEAK_CENTRE_COLUMN);
  const auto fitStatusColumn =
      m_monitorEPPTable->getColumn(EPPTableLiterals::FIT_STATUS_COLUMN);
  if (!peakPositionColumn || !fitStatusColumn) {
    throw std::runtime_error("The workspace specified by " +
                             Prop::DETECTOR_EPP_TABLE +
                             " doesn't seem to contain the expected table");
  }
  if (monitorIndex >= peakPositionColumn->size()) {
    throw std::runtime_error("Invalid " + Prop::MONITOR);
  }
  if (fitStatusColumn->cell<std::string>(monitorIndex) !=
      EPPTableLiterals::FIT_STATUS_SUCCESS) {
    throw std::runtime_error("No successful monitor fit found in " +
                             Prop::MONITOR_EPP_TABLE);
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

} // namespace Algorithms
} // namespace Mantid
