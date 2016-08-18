#include "MantidAlgorithms/GetEiMonDet2.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UserStringParser.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Algorithms {

namespace IndexTypes {
  const static std::string DETECTOR_ID("DetectorID");
  const static std::string SPECTRUM_NUMBER("SpectrumNumber");
  const static std::string WORKSPACE_INDEX("WorkspaceIndex");
}

namespace PropertyNames {
  const static std::string DETECTOR_EPP_TABLE("DetectorEPPTable");
  const static std::string DETECTOR_WORKSPACE("DetectorWorkspace");
  const static std::string DETECTORS("Detectors");
  const static std::string ENERGY_TOLERANCE("EnergyTolerance");
  const static std::string INCIDENT_ENERGY("IncidentEnergy");
  const static std::string INDEX_TYPE("IndexType");
  const static std::string MONITOR("Monitor");
  const static std::string MONITOR_EPP_TABLE("MonitorEPPTable");
  const static std::string MONITOR_WORKSPACE("MonitorWorkspace");
  const static std::string NOMINAL_ENERGY("NominalIncidentEnergy");
  const static std::string PULSE_INTERVAL("PulseInterval");
}

// Register the algorithm into the algorithm factory.
DECLARE_ALGORITHM(GetEiMonDet2)

void GetEiMonDet2::init() {
  auto tofWorkspace = boost::make_shared<CompositeValidator>();
  tofWorkspace->add<WorkspaceUnitValidator>("TOF");
  tofWorkspace->add<InstrumentValidator>();

  auto mandatoryStringProperty = boost::make_shared<MandatoryValidator<std::string>>();
  auto mandatoryDetectorIdProperty = boost::make_shared<MandatoryValidator<detid_t>>();

  declareProperty(
      make_unique<WorkspaceProperty<>>(PropertyNames::DETECTOR_WORKSPACE, "", Direction::Input,
                                       tofWorkspace),
      "A workspace containing the detector spectra.");
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(PropertyNames::DETECTOR_EPP_TABLE, "", Direction::Input),
      "An EPP table corresponding to " + PropertyNames::DETECTOR_WORKSPACE + ".");
  const std::vector<std::string> indexTypes{IndexTypes::DETECTOR_ID, IndexTypes::SPECTRUM_NUMBER, IndexTypes::WORKSPACE_INDEX};
  declareProperty(
      PropertyNames::INDEX_TYPE, IndexTypes::DETECTOR_ID, boost::make_shared<StringListValidator>(indexTypes), "The type of indices " + PropertyNames::DETECTORS + " and " + PropertyNames::MONITOR + " refer to.");
  declareProperty(
      PropertyNames::DETECTORS, "", "A list of detector ids/spectrum number/workspace indices.", mandatoryStringProperty);
  declareProperty(
      make_unique<WorkspaceProperty<>>(PropertyNames::MONITOR_WORKSPACE, "", Direction::Input, PropertyMode::Optional, tofWorkspace),
      "A Workspace containing the monitor spectrum. If empty, " + PropertyNames::DETECTOR_WORKSPACE + " will be used.");
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(PropertyNames::MONITOR_EPP_TABLE,"", Direction::Input, PropertyMode::Optional),
       "An EPP table corresponding to " + PropertyNames::MONITOR_WORKSPACE);
  setPropertySettings(PropertyNames::MONITOR_EPP_TABLE, make_unique<EnabledWhenProperty>(PropertyNames::MONITOR_WORKSPACE, IS_NOT_DEFAULT));
  declareProperty(
      PropertyNames::MONITOR, EMPTY_INT(), mandatoryDetectorIdProperty, "Monitor's detector id/spectrum number/workspace index.");
  declareProperty(
      PropertyNames::PULSE_INTERVAL, EMPTY_DBL(), "Interval between neutron pulses, in microseconds.");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(
      PropertyNames::NOMINAL_ENERGY, EMPTY_DBL(), mustBePositive, "Incident energy guess. Taken from the sample logs, if not specified.");
  declareProperty(
      PropertyNames::INCIDENT_ENERGY, EMPTY_DBL(), mustBePositive, "Calculated incident energy.", Direction::Output);
}

template<typename T, typename Map>
void mapIndices(const std::vector<unsigned int>& detectors, const T monitor, const Map& detectorIndexMap, const Map& monitorIndexMap, std::vector<size_t>& detectorIndices, size_t& monitorIndex) {
  auto back = std::back_inserter(detectorIndices);
  std::transform(detectors.cbegin(), detectors.cend(), back, [&detectorIndexMap](T i) {
    try {
      return detectorIndexMap.at(i);
    }
    catch (std::out_of_range& e) {
      throw std::runtime_error(PropertyNames::DETECTORS + " out of range.");
    }
  });
  try {
    monitorIndex = monitorIndexMap.at(monitor);
  }
  catch (std::out_of_range& e) {
    throw std::runtime_error(PropertyNames::MONITOR + " out of range.");
  }
}

void GetEiMonDet2::exec() {
  // TODO report progress
  MatrixWorkspace_const_sptr detectorWs = getProperty(PropertyNames::DETECTOR_WORKSPACE);
  ITableWorkspace_const_sptr detectorEPPTable = getProperty(PropertyNames::DETECTOR_EPP_TABLE);
  // Remove duplicates from detectorIndices.
  MatrixWorkspace_const_sptr monitorWs = getProperty(PropertyNames::MONITOR_WORKSPACE);
  if (!monitorWs) {
    monitorWs = detectorWs;
  }
  ITableWorkspace_const_sptr monitorEPPTable = getProperty(PropertyNames::MONITOR_EPP_TABLE);
  if (!monitorEPPTable) {
    monitorEPPTable = detectorEPPTable;
  }
  // Get the workspace indices for detectors and monitor, converting
  // from detector id's or spectrum numbers if necessary.
  std::vector<size_t> detectorIndices;
  size_t monitorIndex;
  UserStringParser spectraListParser;
  const std::string indexType = getProperty(PropertyNames::INDEX_TYPE);
  if (indexType == IndexTypes::DETECTOR_ID) {
    const auto detectors = VectorHelper::flattenVector(spectraListParser.parse(getProperty(PropertyNames::DETECTORS)));
    const detid_t monitor = getProperty(PropertyNames::MONITOR);
    const auto detectorIndexMap = detectorWs->getDetectorIDToWorkspaceIndexMap();
    const auto monitorIndexMap = monitorWs->getDetectorIDToWorkspaceIndexMap();
    mapIndices<detid_t>(detectors, monitor, detectorIndexMap, monitorIndexMap, detectorIndices, monitorIndex);
  }
  else if (indexType == IndexTypes::SPECTRUM_NUMBER) {
    const auto detectors(VectorHelper::flattenVector(spectraListParser.parse(getProperty(PropertyNames::DETECTORS))));
    const specnum_t monitor = getProperty(PropertyNames::MONITOR);
    const auto detectorIndexMap = detectorWs->getSpectrumToWorkspaceIndexMap();
    const auto monitorIndexMap = monitorWs->getSpectrumToWorkspaceIndexMap();
    mapIndices<specnum_t>(detectors, monitor, detectorIndexMap, monitorIndexMap, detectorIndices, monitorIndex);
  }
  else {
    // There is a type mismatch between what UserStringParser returns
    // (unsigned int) and workspace index (size_t), thus the moving.
    auto detectors = VectorHelper::flattenVector(spectraListParser.parse(getProperty(PropertyNames::DETECTORS)));
    auto back = std::back_inserter(detectorIndices);
    std::move(detectors.begin(), detectors.end(), back);
    monitorIndex = getProperty(PropertyNames::MONITOR);
  }
  std::sort(detectorIndices.begin(), detectorIndices.end());
  detectorIndices.erase(std::unique(detectorIndices.begin(), detectorIndices.end()), detectorIndices.end());
  if (monitorWs == detectorWs) {
    if (std::find(detectorIndices.begin(), detectorIndices.end(), monitorIndex) != detectorIndices.end()) {
      throw std::runtime_error(PropertyNames::MONITOR + " is also listed in " + PropertyNames::DETECTORS);
    }
  }
  double nominalIncidentEnergy = getProperty(PropertyNames::NOMINAL_ENERGY);
  if (nominalIncidentEnergy == EMPTY_DBL()) {
    if (!detectorWs->run().hasProperty("Ei")) {
      throw std::runtime_error("No " + PropertyNames::NOMINAL_ENERGY + " given and no Ei field found in sample logs");
    }
    nominalIncidentEnergy = std::stod(detectorWs->run().getProperty("Ei")->value());
  }

  const std::string FIT_STATUS_COLUMN("FitStatus");
  const std::string PEAK_CENTRE_COLUMN("PeakCentre");
  const std::string FIT_STATUS_SUCCESS("success");
  auto peakPositionColumn = detectorEPPTable->getColumn(PEAK_CENTRE_COLUMN);
  auto fitStatusColumn = detectorEPPTable->getColumn(FIT_STATUS_COLUMN);
  if (!peakPositionColumn || !fitStatusColumn) {
    throw std::runtime_error("The workspace specified by " + PropertyNames::DETECTOR_EPP_TABLE + " doesn't seem to contain the expected table");
  }

  // Average sample-to-detector distances.
  const auto sample = detectorWs->getInstrument()->getSample();
  double sampleToDetectorDistance = 0.0;
  // Average detector EPPs.
  double detectorEPP = 0.0;
  size_t n = 0;
  for (const auto index : detectorIndices) {
    if (index >= peakPositionColumn->size()) {
      throw std::runtime_error("Invalid value in " + PropertyNames::DETECTORS);
    }
    if (fitStatusColumn->cell<std::string>(index) == FIT_STATUS_SUCCESS) {
      const auto detector = detectorWs->getDetector(index);
      if (!detector) {
        throw std::runtime_error("No detector specified by " + PropertyNames::DETECTORS + " found");
      }
      if (detector->isMonitor()) {
        g_log.warning() << "Workspace index " << index << " should be detector, but is marked as monitor.\n";
      }
      if (!detector->isMasked()) {
        const double d = detector->getDistance(*sample);
        sampleToDetectorDistance += d;
        const double epp = (*peakPositionColumn)[index];
        detectorEPP += epp;
        ++n;
        g_log.debug() << "Including detector at workspace index " << index << " - distance: " << d << " EPP: " << epp << ".\n";
      }
      else {
        g_log.debug() << "Excluding masked detector at workspace index " << index << ".\n";
      }
    }
    else {
      g_log.debug() << "Excluding detector with unsuccessful fit at workspace index " << index << ".\n";
    }
  }
  if (n == 0) {
    throw std::runtime_error("No successful detector fits found in " + PropertyNames::DETECTOR_EPP_TABLE);
  }
  sampleToDetectorDistance /= static_cast<double>(n);
  g_log.information() << "Average sample-to-detector distance: " << sampleToDetectorDistance << ".\n";
  detectorEPP /= static_cast<double>(n);
  g_log.information() << "Average detector EPP: " << detectorEPP << ".\n";

  // Monitor-to-sample distance.
  peakPositionColumn = monitorEPPTable->getColumn(PEAK_CENTRE_COLUMN);
  fitStatusColumn = monitorEPPTable->getColumn(FIT_STATUS_COLUMN);
  if (!peakPositionColumn || !fitStatusColumn) {
    throw std::runtime_error("The workspace specified by " + PropertyNames::DETECTOR_EPP_TABLE + " doesn't seem to contain the expected table");
  }
  if (static_cast<size_t>(monitorIndex) >= peakPositionColumn->size()) {
    throw std::runtime_error("Invalid " + PropertyNames::MONITOR);
  }
  if (fitStatusColumn->cell<std::string>(monitorIndex) != FIT_STATUS_SUCCESS) {
    throw std::runtime_error("No successful monitor fit found in " + PropertyNames::MONITOR_EPP_TABLE);
  }
  const auto monitor = monitorWs->getDetector(monitorIndex);
  if (monitor->isMasked()) {
    throw std::runtime_error("Monitor spectrum is masked");
  }
  if (!monitor->isMonitor()) {
    g_log.warning() << "The monitor spectrum is not actually marked as monitor.\n";
  }
  const double monitorToSampleDistance = monitor->getDistance(*sample);
  g_log.information() << "Monitor-to-sample distance: " << monitorToSampleDistance << ".\n";

  // Monitor peak position.
  const double monitorEPP = (*peakPositionColumn)[monitorIndex];
  g_log.information() << "Monitor EPP: " << monitorEPP << ".\n";

  const double flightLength = sampleToDetectorDistance + monitorToSampleDistance;

  // Calculate actual time of flight from monitor to detectors.
  double timeOfFlight = detectorEPP - monitorEPP;
  const double nominalTimeOfFlight = flightLength / std::sqrt(2 * nominalIncidentEnergy * meV / NeutronMass) * 1e6; // In microseconds.
  g_log.information() << "Nominal time-of-flight: " << nominalTimeOfFlight << ".\n";
  const double energyTolerance = 20;
  const double toleranceLimit = 1 / std::sqrt(1 + energyTolerance) * nominalTimeOfFlight;
  const double pulseInterval = getProperty(PropertyNames::PULSE_INTERVAL);
  const double pulseIntervalLimit = nominalTimeOfFlight - pulseInterval / 2;
  const double lowerTimeLimit = toleranceLimit > pulseIntervalLimit ? toleranceLimit : pulseIntervalLimit;
  const double upperTimeLimit = toleranceLimit > pulseIntervalLimit ? 1 / std::sqrt(1 - energyTolerance) * nominalTimeOfFlight : nominalTimeOfFlight + pulseInterval / 2;
  g_log.notice() << "Expecting a final time-of-flight between " << lowerTimeLimit << " and " << upperTimeLimit << ".\n";
  g_log.notice() << "Calculated time-of-flight: " << timeOfFlight << ".\n";
  if (timeOfFlight <= lowerTimeLimit) {
    g_log.notice() << "Calculated time-of-flight too small. Frame delay has to be taken into account.\n";
  }
  unsigned delayFrameCount = 0;
  while (timeOfFlight <= lowerTimeLimit) {
    // Neutrons hit the detectors in a later frame.
    if (pulseInterval == EMPTY_DBL()) {
      throw std::runtime_error("No " + PropertyNames::PULSE_INTERVAL + " specified");
    }
    ++delayFrameCount;
    timeOfFlight = delayFrameCount * pulseInterval - monitorEPP + detectorEPP;
  }
  if (timeOfFlight > upperTimeLimit) {
    throw std::runtime_error("Calculated time-of-flight too large");
  }

  const double velocity = flightLength / timeOfFlight * 1e6;
  const double energy = 0.5 * NeutronMass * velocity * velocity / meV;
  g_log.notice() << "Final time-of-flight:" << timeOfFlight << " which gives " << energy << " as " + PropertyNames::INCIDENT_ENERGY + ".\n";
  // Set output properties.
  setProperty(PropertyNames::INCIDENT_ENERGY, energy);
}

} // namespace Algorithms
} // namespace Mantid
