// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GetEiMonDet3.h"
#include "MantidAPI/Algorithm.tcc"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
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
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/VectorHelper.h"

namespace {
/** A private namespace holding the property names of
 *  GetEiMonDet algorithm, version 3.
 */
namespace Prop {
/// Name of the detector workspace property
const static std::string DETECTOR_WORKSPACE("DetectorWorkspace");
/// Name of the incident energy output property
const static std::string INCIDENT_ENERGY("IncidentEnergy");
/// Name of the monitor workspace index property
const static std::string MONITOR("MonitorIndex");
/// Name of the monitor epp table property
const static std::string MONITOR_EPP_TABLE("MonitorEPPTable");
/// Name of the monitor workspace property
const static std::string MONITOR_WORKSPACE("MonitorWorkspace");
/// Name of the neutron pulse interval property
const static std::string PULSE_INTERVAL("PulseInterval");
/// Name of the maximum energy property
const static std::string MAX_ENERGY("MaximumEnergy");
} // namespace Prop

std::vector<size_t> toWorkspaceIndices(const Mantid::Indexing::SpectrumIndexSet &indices) {
  std::vector<size_t> wsIndices;
  wsIndices.reserve(indices.size());
  for (size_t i = 0; i < indices.size(); ++i) {
    wsIndices.emplace_back(indices[i]);
  }
  return wsIndices;
}
} // namespace

namespace Mantid::Algorithms {

using namespace API;

// Register the algorithm into the algorithm factory.
DECLARE_ALGORITHM(GetEiMonDet3)

/// Returns algorithm's name for identification
const std::string GetEiMonDet3::name() const { return "GetEiMonDet"; }

/// Returns a summary of algorithm's purpose
const std::string GetEiMonDet3::summary() const {
  return "Calculates the kinetic energy of neutrons leaving the source based "
         "on the time it takes for them to travel between a monitor and a "
         "set of detectors.";
}

/// Returns algorithm's version for identification
int GetEiMonDet3::version() const { return 3; }

const std::vector<std::string> GetEiMonDet3::seeAlso() const { return {"GetEi"}; }

/// Algorithm's category for identification overriding a virtual method
const std::string GetEiMonDet3::category() const { return "Inelastic\\Ei"; }

/** A private namespace holding names for sample log entries.
 */
namespace SampleLogs {
/// Name of the pulse interval sample log
const static std::string PULSE_INTERVAL("pulse_interval");
} // namespace SampleLogs

/** Initialized the algorithm.
 *
 */
void GetEiMonDet3::init() {
  auto tofWorkspace = std::make_shared<Kernel::CompositeValidator>();
  tofWorkspace->add<API::WorkspaceUnitValidator>("TOF");
  tofWorkspace->add<API::InstrumentValidator>();
  auto mandatoryIntProperty = std::make_shared<Kernel::MandatoryValidator<int>>();
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0);

  declareWorkspaceInputProperties<API::MatrixWorkspace, static_cast<int>(IndexType::SpectrumNum) |
                                                            static_cast<int>(IndexType::WorkspaceIndex)>(
      Prop::DETECTOR_WORKSPACE, "A workspace containing the detector spectra.", tofWorkspace);
  declareProperty(
      std::make_unique<API::WorkspaceProperty<>>(Prop::MONITOR_WORKSPACE.c_str(), "", Kernel::Direction::Input,
                                                 API::PropertyMode::Optional, tofWorkspace),
      "A Workspace containing the monitor spectrum; if empty, " + Prop::DETECTOR_WORKSPACE + " will be used.");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
                      Prop::MONITOR_EPP_TABLE, "", Kernel::Direction::Input, API::PropertyMode::Optional),
                  "An EPP table corresponding to " + Prop::MONITOR_WORKSPACE);
  setPropertySettings(Prop::MONITOR_EPP_TABLE,
                      std::make_unique<Kernel::EnabledWhenProperty>(Prop::MONITOR_WORKSPACE, Kernel::IS_NOT_DEFAULT));
  declareProperty(Prop::MONITOR, EMPTY_INT(), mandatoryIntProperty, "Usable monitor's workspace index.");
  declareProperty(Prop::PULSE_INTERVAL, EMPTY_DBL(),
                  "Interval between neutron pulses, in microseconds; taken "
                  "from the sample logs, if not specified.");
  declareProperty(Prop::MAX_ENERGY, EMPTY_DBL(), mustBePositive,
                  "Multiple pulse intervals will be added to the flight time "
                  "the until final energy is less than this value.");
  declareProperty(Prop::INCIDENT_ENERGY, EMPTY_DBL(), mustBePositive, "Calculated incident energy, in meV.",
                  Kernel::Direction::Output);
}

/** Executes the algorithm.
 *
 */
void GetEiMonDet3::exec() {
  progress(0.);
  API::MatrixWorkspace_sptr detectorWs;
  Indexing::SpectrumIndexSet detectorIndices;
  std::tie(detectorWs, detectorIndices) = getWorkspaceAndIndices<API::MatrixWorkspace>(Prop::DETECTOR_WORKSPACE);

  API::MatrixWorkspace_sptr monitorWs = getProperty(Prop::MONITOR_WORKSPACE);
  const int monitorIndex = getProperty(Prop::MONITOR);
  if (!monitorWs) {
    monitorWs = detectorWs;
    if (std::find(detectorIndices.begin(), detectorIndices.end(), monitorIndex) != detectorIndices.end()) {
      throw std::runtime_error("MonitorIndex is also listed in DetectorWorkspaceIndexSet.");
    }
  }
  if (!monitorWs->spectrumInfo().isMonitor(monitorIndex)) {
    m_log.warning("The monitor spectrum is not marked as a monitor.");
  }
  const auto detWsIndices = toWorkspaceIndices(detectorIndices);
  auto detectorSumWs = groupSpectra(detectorWs, detWsIndices);
  progress(0.3);
  const auto sampleToDetectorDistance = detectorSumWs->spectrumInfo().l2(0);
  double detectorEPP;
  try {
    detectorEPP = peakPosition(detectorSumWs);
  } catch (std::runtime_error &e) {
    throw std::runtime_error(std::string("Failed to find detector peak for incident energy: ") + e.what());
  }
  progress(0.5);
  const std::vector<size_t> monWsIndices = {static_cast<size_t>(monitorIndex)};
  auto monitorSumWs = groupSpectra(monitorWs, monWsIndices);
  double monitorEPP;
  try {
    if (isDefault(Prop::MONITOR_EPP_TABLE)) {
      monitorEPP = peakPosition(monitorSumWs);
    } else {
      monitorEPP = monitorPeakPosition(monWsIndices.front());
    }
  } catch (std::runtime_error &e) {
    throw std::runtime_error(std::string("Failed to find monitor peak for incident energy: ") + e.what());
  }
  progress(0.7);
  // SpectrumInfo returns a negative l2 for monitor.
  const auto monitorToSampleDistance = std::abs(monitorSumWs->spectrumInfo().l2(0));
  const double minTOF = minimumTOF(*detectorWs, sampleToDetectorDistance);

  double timeOfFlight = computeTOF(*detectorWs, detectorEPP, monitorEPP, minTOF);
  const double flightLength = sampleToDetectorDistance + monitorToSampleDistance;
  const double velocity = flightLength / timeOfFlight * 1e6;
  using namespace PhysicalConstants;
  const double energy = 0.5 * NeutronMass * velocity * velocity / meV;
  progress(1.0);
  g_log.notice() << "Final time-of-flight:" << timeOfFlight << " which gives " << energy << " as "
                 << Prop::INCIDENT_ENERGY << ".\n";
  setProperty(Prop::INCIDENT_ENERGY, energy);
}

/** Calculates the time of flight from the monitor to the detectors.
 *
 *  Adds pulse intervals to the TOF until it is greater than `minTOF`.
 *  @param detectorWs Detector workspace
 *  @param detectorEPP The position of the detectors' elastic peak
 *  @param monitorEPP The position of the monitor's elastic peak
 *  @param minTOF minimum expected time of flight, in microseconds
 *  @return The time of flight between the monitor and the detectors
 */
double GetEiMonDet3::computeTOF(const API::MatrixWorkspace &detectorWs, const double detectorEPP,
                                const double monitorEPP, const double minTOF) {
  double timeOfFlight = detectorEPP - monitorEPP;
  // Check if the obtained time-of-flight makes any sense.
  while (timeOfFlight <= minTOF) {
    double pulseInterval = getProperty(Prop::PULSE_INTERVAL);
    if (pulseInterval == EMPTY_DBL()) {
      if (detectorWs.run().hasProperty(SampleLogs::PULSE_INTERVAL)) {
        pulseInterval = detectorWs.run().getPropertyAsSingleValue(SampleLogs::PULSE_INTERVAL);
        pulseInterval *= 1e6; // To microseconds.
      } else {
        throw std::invalid_argument("PulseInterval not explicitly given nor found in the sample logs.");
      }
    }
    g_log.notice() << "Frame delay of " << pulseInterval << " microseconds will be added to the time-of-flight.\n";
    timeOfFlight += pulseInterval;
  }
  g_log.notice() << "Calculated time-of-flight: " << timeOfFlight << ".\n";
  return timeOfFlight;
}

/** Runs GroupDetectors on given workspace indices
 *
 * @param ws a workspace to group
 * @param wsIndices a vector of workspace indices to group
 * @return a single spectrum workspace
 */
API::MatrixWorkspace_sptr GetEiMonDet3::groupSpectra(const API::MatrixWorkspace_sptr &ws,
                                                     const std::vector<size_t> &wsIndices) {
  auto group = createChildAlgorithm("GroupDetectors");
  group->setProperty("InputWorkspace", ws);
  group->setProperty("OutputWorkspace", "unused");
  group->setProperty("WorkspaceIndexList", wsIndices);
  group->execute();
  return group->getProperty("OutputWorkspace");
}

/** Computes the minimum TOF between monitor and detectors from maximum
 *  energy
 *
 * @param ws a workspace containing the instrument
 * @param sampleToDetectorDistance the l2 distance
 * @return minimum expected TOF, in microseconds
 */
double GetEiMonDet3::minimumTOF(const API::MatrixWorkspace &ws, const double sampleToDetectorDistance) {
  const double maxEnergy = getProperty(Prop::MAX_ENERGY);
  const auto &spectrumInfo = ws.spectrumInfo();
  return Kernel::UnitConversion::run("Energy", "TOF", maxEnergy, spectrumInfo.l1(), sampleToDetectorDistance, 0.,
                                     Kernel::DeltaEMode::Direct, 0.);
}

/** Returns the TOF of the monitor's peak.
 *
 * @param monitorIndex monitor spectrum workspace index
 * @return monitor peak TOF in microseconds
 */
double GetEiMonDet3::monitorPeakPosition(const size_t monitorIndex) {
  API::ITableWorkspace_sptr monitorEPPWs = getProperty(Prop::MONITOR_EPP_TABLE);
  const auto &status = monitorEPPWs->getRef<std::string>("FitStatus", monitorIndex);
  if (status != "success" && status != "narrowPeak") {
    throw std::runtime_error("Monitor EPP fit status shows a failure.");
  }
  return monitorEPPWs->getRef<double>("PeakCentre", monitorIndex);
}

/** Returns the TOF of the grouped detectors' elastic peak.
 *
 * @param ws a single spectrum workspace
 * @return detector peak in microseconds
 */
double GetEiMonDet3::peakPosition(const API::MatrixWorkspace_sptr &ws) {
  auto findEPP = createChildAlgorithm("FindEPP");
  findEPP->setProperty("InputWorkspace", ws);
  findEPP->setProperty("OutputWorkspace", "unused");
  findEPP->execute();
  API::ITableWorkspace_sptr eppTable = findEPP->getProperty("OutputWorkspace");
  const auto &status = eppTable->getRef<std::string>("FitStatus", 0);
  if (status != "success" && status != "narrowPeak") {
    throw std::runtime_error("Could not fit a Gaussian to the data.");
  }
  return eppTable->getRef<double>("PeakCentre", 0);
}

} // namespace Mantid::Algorithms
