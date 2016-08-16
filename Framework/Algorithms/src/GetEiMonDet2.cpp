#include "MantidAlgorithms/GetEiMonDet2.h"

#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UserStringParser.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Algorithms {

namespace PropertyNames {
  const static std::string DETECTOR_SPECTRA("DetectorSpectra");
  const static std::string DETECTOR_EPP_TABLE("DetectorEPPTable");
  const static std::string DETECTOR_WORKSPACE("DetectorWorkspace");
  const static std::string ENERGY_TOLERANCE("EnergyTolerance");
  const static std::string INCIDENT_ENERGY("IncidentEnergy");
  const static std::string MONITOR_EPP_TABLE("MonitorEPPTable");
  const static std::string MONITOR_SPECTRUM_NUMBER("MonitorSpectrumNumber");
  const static std::string MONITOR_WORKSPACE("MonitorWorkspace");
  const static std::string PULSE_INTERVAL("PulseInterval");
  const static std::string WAVELENGTH("Wavelength");
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
      "The X units of this workspace must be TOF in microseconds");
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(PropertyNames::DETECTOR_EPP_TABLE, "", Direction::Input),
      "EPP table corresponding to " + PropertyNames::DETECTOR_WORKSPACE);
  declareProperty(
      PropertyNames::DETECTOR_SPECTRA, "", "Formatting example: 1,3-7,10-15", mandatoryStringProperty);
  declareProperty(
      make_unique<WorkspaceProperty<>>(PropertyNames::MONITOR_WORKSPACE, "", Direction::Input, PropertyMode::Optional, tofWorkspace),
      "If empty, " + PropertyNames::DETECTOR_WORKSPACE + " will be used");
  declareProperty(
      make_unique<WorkspaceProperty<ITableWorkspace>>(PropertyNames::MONITOR_EPP_TABLE,"", Direction::Input, PropertyMode::Optional),
       "EPP table corresponding to " + PropertyNames::MONITOR_WORKSPACE);
  setPropertySettings(PropertyNames::MONITOR_EPP_TABLE, make_unique<EnabledWhenProperty>(PropertyNames::MONITOR_WORKSPACE, IS_NOT_DEFAULT));
  declareProperty(
      PropertyNames::MONITOR_SPECTRUM_NUMBER, static_cast<detid_t>(EMPTY_INT()), mandatoryDetectorIdProperty);
  declareProperty(
      PropertyNames::PULSE_INTERVAL, EMPTY_DBL(), "In microseconds");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0);
  declareProperty(
      PropertyNames::INCIDENT_ENERGY, EMPTY_DBL(), mustBePositive, "Taken from the sample logs, if not specified.", Direction::InOut);
  declareProperty(
      PropertyNames::ENERGY_TOLERANCE, 10.0, boost::make_shared<BoundedValidator<double>>(0, 100), "Tolerance between calculated energy and " + PropertyNames::INCIDENT_ENERGY + ", in percents.");
  declareProperty(
      PropertyNames::WAVELENGTH, -1.0, Direction::Output);
}

void GetEiMonDet2::exec() {
  // TODO report progress
  MatrixWorkspace_const_sptr detectorWs = getProperty(PropertyNames::DETECTOR_WORKSPACE);
  ITableWorkspace_const_sptr detectorEPPTable = getProperty(PropertyNames::DETECTOR_EPP_TABLE);
  UserStringParser spectraListParser;
  // TODO Convert all indices to workspace indices.
  auto detectorIndices = VectorHelper::flattenVector(spectraListParser.parse(getProperty(PropertyNames::DETECTOR_SPECTRA)));
  // Remove duplicates from detectorIndices.
  std::sort(detectorIndices.begin(), detectorIndices.end());
  detectorIndices.erase(std::unique(detectorIndices.begin(), detectorIndices.end()), detectorIndices.end());
  MatrixWorkspace_const_sptr monitorWs = getProperty(PropertyNames::MONITOR_WORKSPACE);
  if (!monitorWs) {
    monitorWs = detectorWs;
  }
  ITableWorkspace_const_sptr monitorEPPTable = getProperty(PropertyNames::MONITOR_EPP_TABLE);
  if (!monitorEPPTable) {
    monitorEPPTable = detectorEPPTable;
  }
  const detid_t monitorIndex = getProperty(PropertyNames::MONITOR_SPECTRUM_NUMBER);
  if (monitorWs == detectorWs) {
    if (std::find(detectorIndices.begin(), detectorIndices.end(), monitorIndex) != detectorIndices.end()) {
      throw std::runtime_error(PropertyNames::MONITOR_SPECTRUM_NUMBER + " is also listed in " + PropertyNames::DETECTOR_SPECTRA);
    }
  }
  double nominalIncidentEnergy = getProperty(PropertyNames::INCIDENT_ENERGY);
  if (nominalIncidentEnergy == EMPTY_DBL()) {
    if (!detectorWs->run().hasProperty("Ei")) {
      throw std::runtime_error("No " + PropertyNames::INCIDENT_ENERGY + " given and no Ei field found in sample logs");
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
  // Average detector EPPs.
  double detectorEPP = 0.0;
  size_t n = 0;
  for (const auto index : detectorIndices) {
    if (index >= peakPositionColumn->size()) {
      throw std::runtime_error("Invalid value in " + PropertyNames::DETECTOR_SPECTRA);
    }
    if (fitStatusColumn->cell<std::string>(index) == FIT_STATUS_SUCCESS) {
      detectorEPP += (*peakPositionColumn)[index];
      ++n;
    }
  }
  if (n == 0) {
    throw std::runtime_error("No successful detector fits found in " + PropertyNames::DETECTOR_EPP_TABLE);
  }
  detectorEPP /= static_cast<double>(n);
  // Monitor peak position.
  peakPositionColumn = monitorEPPTable->getColumn(PEAK_CENTRE_COLUMN);
  fitStatusColumn = monitorEPPTable->getColumn(FIT_STATUS_COLUMN);
  if (!peakPositionColumn || !fitStatusColumn) {
    throw std::runtime_error("The workspace specified by " + PropertyNames::DETECTOR_EPP_TABLE + " doesn't seem to contain the expected table");
  }
  if (monitorIndex < 0 || static_cast<size_t>(monitorIndex) >= peakPositionColumn->size()) {
    throw std::runtime_error("Invalid " + PropertyNames::MONITOR_SPECTRUM_NUMBER);
  }
  if (fitStatusColumn->cell<std::string>(monitorIndex) != FIT_STATUS_SUCCESS) {
    throw std::runtime_error("No successful monitor fit found in " + PropertyNames::MONITOR_EPP_TABLE);
  }
  const double monitorEPP = (*peakPositionColumn)[monitorIndex];

  // Average sample-to-detector distances.
  const auto sample = detectorWs->getInstrument()->getSample();
  double sampleToDetectorDistance = 0.0;
  n = 0;
  for (const auto index : detectorIndices) {
    // Skip detectors without EPP.
    if (fitStatusColumn->cell<std::string>(index) == FIT_STATUS_SUCCESS) {
      const auto detector = detectorWs->getDetector(index);
      if (!detector) {
        throw std::runtime_error("No detector specified by " + PropertyNames::DETECTOR_SPECTRA + " found");
      }
      sampleToDetectorDistance += detector->getDistance(*sample);
      ++n;
    }
  }
  sampleToDetectorDistance /= static_cast<double>(n);
  // Monitor-to-sample distance.
  const auto monitor = monitorWs->getDetector(monitorIndex);
  const double monitorToSampleDistance = monitor->getDistance(*sample);

  const double flightLength = sampleToDetectorDistance + monitorToSampleDistance;

  // Calculate actual time of flight from monitor to detectors.
  double timeOfFlight = detectorEPP - monitorEPP;
  const double energyTolerance = static_cast<double>(getProperty(PropertyNames::ENERGY_TOLERANCE)) / 100;
  const double lowerTimeTolerance = 1 / std::sqrt(1 + energyTolerance);
  const double upperTimeTolerance = 1 / std::sqrt(1 - energyTolerance);
  unsigned delayFrameCount = 0;
  const double nominalTimeOfFlight = flightLength / std::sqrt(2 * nominalIncidentEnergy * meV / NeutronMass) * 1e6; // In microseconds.
  while (timeOfFlight <= lowerTimeTolerance * nominalTimeOfFlight) {
    // Neutrons hit the detectors in a later frame.
    const double pulseInterval = getProperty(PropertyNames::PULSE_INTERVAL);
    if (pulseInterval == EMPTY_DBL()) {
      throw std::runtime_error("Too small or negative time-of-flight and no " + PropertyNames::PULSE_INTERVAL + " specified");
    }
    ++delayFrameCount;
    timeOfFlight = delayFrameCount * pulseInterval - monitorEPP + detectorEPP;
  }
  if (timeOfFlight > upperTimeTolerance * nominalTimeOfFlight) {
    throw std::runtime_error("Calculated time-of-flight too large");
  }

  const double velocity = flightLength / timeOfFlight * 1e6;
  const double energy = 0.5 * NeutronMass * velocity * velocity / meV;
  const double wavelength = h / (NeutronMass * velocity) * 1e10; // in Ångströms.
  // Set output properties.
  setProperty(PropertyNames::INCIDENT_ENERGY, energy);
  setProperty(PropertyNames::WAVELENGTH, wavelength);
}

} // namespace Algorithms
} // namespace Mantid
