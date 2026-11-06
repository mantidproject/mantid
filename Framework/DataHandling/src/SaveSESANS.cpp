// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveSESANS.h"
#include "MantidDataHandling/FormattingHelpers.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ITimeSeriesProperty.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <string>

namespace Mantid::DataHandling {

namespace {
/**
 * Make a value fit on a single header line: replace line breaks with spaces
 * and remove leading and trailing whitespace
 * @param value The value to sanitise
 * @return The sanitised value, which is empty if the value only contained whitespace
 */
std::string sanitiseHeaderValue(const std::string &value) {
  return Kernel::Strings::strip(Kernel::Strings::replaceAll(value, "\n\r", " "));
}

/**
 * Get the distinct values of a time series log within a time region of interest,
 * in the order they occur. Consecutive repeats and blank values are dropped.
 * @param log The log to read
 * @param roi The time region of interest, e.g. as set by FilterByTime
 * @return The values joined with ", ", or nullopt if the log is not a time
 * series of type T
 */
template <typename T>
std::optional<std::string> distinctValuesInTimeROI(const Kernel::Property *log, const Kernel::TimeROI &roi) {
  const auto *timeSeries = dynamic_cast<const Kernel::TimeSeriesProperty<T> *>(log);
  if (!timeSeries) {
    return std::nullopt;
  }
  if (timeSeries->size() == 0) {
    return std::string();
  }
  std::vector<std::string> values;
  for (const auto &rawValue : timeSeries->filteredValuesAsVector(&roi)) {
    const auto value = sanitiseHeaderValue(Kernel::Strings::toString(rawValue));
    if (!value.empty() && (values.empty() || values.back() != value)) {
      values.emplace_back(value);
    }
  }
  return Kernel::Strings::join(values.cbegin(), values.cend(), ", ");
}
} // namespace

// Register the algorithm with the AlgorithmFactory
DECLARE_ALGORITHM(SaveSESANS)

/// Get the algorithm's name
const std::string SaveSESANS::name() const { return "SaveSESANS"; }

/// Get a summary of the algorithm
const std::string SaveSESANS::summary() const { return "Save a file using the SESANS format"; }

/// Get the version number of the algorithm
int SaveSESANS::version() const { return 1; }

/// Get the algorithm's category
const std::string SaveSESANS::category() const { return "DataHandling\\Text"; }

std::map<std::string, std::string> SaveSESANS::validateInputs() {
  std::map<std::string, std::string> invalidInputs;

  for (const auto &propertyName : mandatoryDoubleProperties) {
    double value = getProperty(propertyName);
    if (value == EMPTY_DBL()) {
      invalidInputs[propertyName] = propertyName + " must be set";
    }
  }

  if (getPropertyValue("Sample").empty()) {
    invalidInputs["Sample"] = "Sample must be set";
  }

  m_sampleThickness = static_cast<double>(getProperty("OverrideSampleThickness"));
  if (m_sampleThickness <= SaveSESANS::TOLERANCE) {
    invalidInputs["OverrideSampleThickness"] = "OverrideSampleThickness value must be greater than 0";
  }

  return invalidInputs;
}

/**
 * Initialise the algorithm
 */
void SaveSESANS::init() {
  auto validOrientation = std::make_shared<Kernel::StringListValidator>(std::set<std::string>{"X", "Y", "Z"});

  declareProperty(std::make_unique<API::WorkspaceProperty<>>("InputWorkspace", "", Kernel::Direction::Input),
                  "The name of the workspace to save");
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save, fileExtensions),
                  "The name to use when saving the file");

  declareProperty("ThetaZMax", EMPTY_DBL(), "The angular acceptance in the encoding direction",
                  Kernel::Direction::Input);
  declareProperty("ThetaZMaxUnit", "radians", Kernel::Direction::Input);
  declareProperty("ThetaYMax", EMPTY_DBL(), "The angular acceptance in the non-encoding direction",
                  Kernel::Direction::Input);
  declareProperty("ThetaYMaxUnit", "radians", Kernel::Direction::Input);
  declareProperty("EchoConstant", EMPTY_DBL(), "The spin echo length, in nanometers, probed by a 1A neutron",
                  Kernel::Direction::Input);

  declareProperty<std::string>("Sample", "", "Sample name", Kernel::Direction::Input);

  declareProperty<std::string>("Orientation", "Z", validOrientation, "Orientation of the instrument");

  declareProperty("OverrideSampleThickness", EMPTY_DBL(),
                  "The sample thickness in mm. If set, this value will be used instead of the thickness from the input "
                  "workspace sample",
                  Kernel::Direction::Input);

  declareProperty<std::string>("Facility", "",
                               "The facility where the data was measured. If not set, the facility is determined "
                               "from the instrument",
                               Kernel::Direction::Input);
  declareProperty<std::string>("Instrument", "",
                               "The instrument used to measure the data. If not set, the instrument name is taken "
                               "from the input workspace",
                               Kernel::Direction::Input);
  declareProperty<std::string>("RunNumber", "",
                               "The run number of the sample measurement. If not set, the run_number log of the "
                               "input workspace is used",
                               Kernel::Direction::Input);
  declareProperty<std::string>("SamplePosition", "",
                               "The name of the sample changer position, e.g. AT or 1CT. If not set, the positions "
                               "recorded in the SamplePos log of the input workspace during the selected time range "
                               "are used",
                               Kernel::Direction::Input);
  declareProperty<std::string>("BlankRunNumber", "", "The run number of the blank measurement. Optional.",
                               Kernel::Direction::Input);
  declareProperty<std::string>("SampleTransmissionRunNumber", "",
                               "The run number of the sample transmission measurement. Optional.",
                               Kernel::Direction::Input);
  declareProperty<std::string>("BlankTransmissionRunNumber", "",
                               "The run number of the blank transmission measurement. Optional.",
                               Kernel::Direction::Input);
}

/**
 * Get the instrument name, either from the Instrument property or from the
 * input workspace
 * @param ws The workspace to save
 * @return The instrument name, or an empty string if it cannot be determined
 */
std::string SaveSESANS::getInstrumentName(const API::MatrixWorkspace_const_sptr &ws) const {
  const std::string instrument = getPropertyValue("Instrument");
  if (!instrument.empty()) {
    return instrument;
  }
  const auto wsInstrument = ws->getInstrument();
  return wsInstrument ? wsInstrument->getName() : "";
}

/**
 * Get the facility name, either from the Facility property or by looking up
 * the instrument in the facilities definitions
 * @param instrumentName The name of the instrument
 * @return The facility name, or an empty string if it cannot be determined
 */
std::string SaveSESANS::getFacilityName(const std::string &instrumentName) const {
  const std::string facility = getPropertyValue("Facility");
  if (!facility.empty() || instrumentName.empty()) {
    return facility;
  }
  try {
    return Kernel::ConfigService::Instance().getInstrument(instrumentName).facility().name();
  } catch (const Kernel::Exception::NotFoundError &) {
    g_log.information("Could not determine the facility for instrument " + instrumentName +
                      ". Use the Facility property to provide it.");
    return "";
  }
}

/**
 * Get the run number, either from the RunNumber property or from the
 * run_number log of the input workspace
 * @param ws The workspace to save
 * @return The run number, or an empty string if it cannot be determined
 */
std::string SaveSESANS::getRunNumber(const API::MatrixWorkspace_const_sptr &ws) const {
  const std::string runNumber = getPropertyValue("RunNumber");
  if (!runNumber.empty()) {
    return runNumber;
  }
  const auto &run = ws->run();
  return run.hasProperty("run_number") ? run.getLogData("run_number")->value() : "";
}

/**
 * Get the time range to read the sample position for. This is the run's time
 * region of interest if it has been filtered, e.g. by FilterByTime. Otherwise it
 * is the time between the start and end of the run, because IBEX logs can start
 * hours before the run. If the run has no valid start and end times, the whole
 * log is used.
 * @param run The run of the workspace to save
 * @return The time region of interest
 */
Kernel::TimeROI SaveSESANS::getSamplePositionTimeROI(const API::Run &run) const {
  const auto &roi = run.getTimeROI();
  if (!roi.useAll()) {
    return roi;
  }
  try {
    const auto startTime = run.startTime();
    const auto endTime = run.endTime();
    if (startTime < endTime) {
      return Kernel::TimeROI(startTime, endTime);
    }
  } catch (const std::runtime_error &) {
    g_log.debug("The run has no valid start and end times, so the whole SamplePos log is used");
  }
  return roi;
}

/**
 * Get the sample position, either from the SamplePosition property or from the
 * SamplePos log of the input workspace. For a time series log, all distinct
 * positions during the run, or during its filtered time range, are returned in
 * order. The position in effect at the start of that time range is included.
 * @param ws The workspace to save
 * @return The sample position, or an empty string if it cannot be determined
 */
std::string SaveSESANS::getSamplePosition(const API::MatrixWorkspace_const_sptr &ws) const {
  const std::string samplePosition = getPropertyValue("SamplePosition");
  if (!samplePosition.empty()) {
    return samplePosition;
  }
  const auto &run = ws->run();
  if (!run.hasProperty("SamplePos")) {
    return "";
  }
  const auto *log = run.getLogData("SamplePos");
  const auto roi = getSamplePositionTimeROI(run);
  // IBEX records the position as a string or a number, depending on how the block is configured
  auto positions = distinctValuesInTimeROI<std::string>(log, roi);
  if (!positions) {
    positions = distinctValuesInTimeROI<double>(log, roi);
  }
  if (!positions) {
    positions = distinctValuesInTimeROI<float>(log, roi);
  }
  if (!positions) {
    positions = distinctValuesInTimeROI<int>(log, roi);
  }
  if (positions) {
    return *positions;
  }
  if (dynamic_cast<const Kernel::ITimeSeriesProperty *>(log)) {
    g_log.warning("The SamplePos log has an unsupported type and is not written to the file. Use the SamplePosition "
                  "property to provide the sample position.");
    return "";
  }
  return log->value();
}

/**
 * Execute the algorithm
 */
void SaveSESANS::exec() {
  API::MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");

  // Check workspace has only one spectrum
  if (ws->getNumberHistograms() != 1) {
    g_log.error("This algorithm expects a workspace with exactly 1 spectrum");
    throw std::runtime_error("SaveSESANS passed workspace with incorrect "
                             "number of spectra, expected 1");
  }

  if (m_sampleThickness == EMPTY_DBL()) {
    m_sampleThickness = ws->sample().getThickness();
    if (m_sampleThickness <= SaveSESANS::TOLERANCE) {
      g_log.error("The workspace passed in does not provide the sample thickness. Please use the "
                  "OverrideSampleThickness property to "
                  "provide this value instead");
      throw std::runtime_error("SaveSESANS passed workspace with sample thickness less than "
                               "or equal to 0 and no value provided for OverrideSampleThickness property");
    }
  }

  auto filename = getPropertyValue("Filename");
  std::ofstream outfile(filename, std::ofstream::trunc);
  if (outfile.fail()) {
    const std::string error = strerror(errno);
    g_log.error("Failed to open file. Error was: " + error);
    throw std::runtime_error("Could not open file at the following path: " + filename);
  }

  writeHeaders(outfile, ws);
  outfile << "\n"
          << "BEGIN_DATA"
          << "\n";

  const auto &wavelength = ws->points(0);
  const auto &yValues = ws->y(0);
  const auto &eValues = ws->e(0);

  const auto spinEchoLength = calculateSpinEchoLength(wavelength);
  const auto depolarisation = calculateDepolarisation(yValues, wavelength);
  const auto error = calculateError(eValues, yValues, wavelength);

  outfile << "SpinEchoLength Depolarisation Depolarisation_error Wavelength\n";

  for (size_t i = 0; i < spinEchoLength.size(); ++i) {
    outfile << formatDouble(spinEchoLength[i]) << " ";
    outfile << formatDouble(depolarisation[i]) << " ";
    outfile << formatDouble(error[i]) << " ";
    outfile << formatDouble(wavelength[i]) << "\n";
  }

  outfile.close();
}

/**
 * Write header values to the output file
 * @param outfile ofstream to the output file
 * @param ws The workspace to save
 */
void SaveSESANS::writeHeaders(std::ofstream &outfile, API::MatrixWorkspace_const_sptr &ws) {
  writeHeader(outfile, "FileFormatVersion", "1.0");
  writeHeader(outfile, "DataFileTitle", ws->getTitle());
  writeHeader(outfile, "Sample", getPropertyValue("Sample"));
  writeHeader(outfile, "Thickness", std::to_string(m_sampleThickness));
  writeHeader(outfile, "Thickness_unit", "mm");
  writeHeader(outfile, "Theta_zmax", getPropertyValue("ThetaZMax"));
  writeHeader(outfile, "Theta_zmax_unit", getPropertyValue("ThetaZMaxUnit"));
  writeHeader(outfile, "Theta_ymax", getPropertyValue("ThetaYMax"));
  writeHeader(outfile, "Theta_ymax_unit", getPropertyValue("ThetaYMaxUnit"));
  writeHeader(outfile, "Orientation", getPropertyValue("Orientation"));
  writeHeader(outfile, "SpinEchoLength_unit", "A");
  writeHeader(outfile, "Depolarisation_unit", "A-2 cm-1");
  writeHeader(outfile, "Wavelength_unit", "A");
  writeHeader(outfile, "Echo_constant", getPropertyValue("EchoConstant"));

  // Optional metadata identifying the measurement. These are only written if
  // a value is available
  const auto instrumentName = getInstrumentName(ws);
  writeOptionalHeader(outfile, "Facility", getFacilityName(instrumentName));
  writeOptionalHeader(outfile, "Instrument", instrumentName);
  writeOptionalHeader(outfile, "Run_number", getRunNumber(ws));
  writeOptionalHeader(outfile, "Sample_position", getSamplePosition(ws));
  writeOptionalHeader(outfile, "Blank_run_number", getPropertyValue("BlankRunNumber"));
  writeOptionalHeader(outfile, "Sample_transmission_run_number", getPropertyValue("SampleTransmissionRunNumber"));
  writeOptionalHeader(outfile, "Blank_transmission_run_number", getPropertyValue("BlankTransmissionRunNumber"));

  writeHeader(outfile, "Reduction_software", "Mantid");
  writeHeader(outfile, "Software_version", Kernel::MantidVersion::version());
  writeHeader(outfile, "Reduction_date", Types::Core::DateAndTime::getCurrentTime().toISO8601String());
}

/**
 * Write a single header to the output file
 * @param outfile ofstream to the output file
 * @param name The name of the attribute being written
 * @param value The attribute's value
 */
void SaveSESANS::writeHeader(std::ofstream &outfile, const std::string &name, const std::string &value) {
  // Always separate the name from the value, even if the name is longer than the usual column width
  outfile << std::setfill(' ') << std::setw(MAX_HDR_LENGTH) << std::left << name;
  if (name.size() >= static_cast<size_t>(MAX_HDR_LENGTH)) {
    outfile << " ";
  }
  outfile << value << "\n";
}

/**
 * Write a single header to the output file, if it has a value
 * @param outfile ofstream to the output file
 * @param name The name of the attribute being written
 * @param value The attribute's value. Line breaks are replaced with spaces and surrounding whitespace is
 * removed. Nothing is written if the result is empty
 */
void SaveSESANS::writeOptionalHeader(std::ofstream &outfile, const std::string &name, const std::string &value) {
  const auto sanitisedValue = sanitiseHeaderValue(value);
  if (!sanitisedValue.empty()) {
    writeHeader(outfile, name, sanitisedValue);
  }
}

/**
 * Calculate SpinEchoLength column from wavelength
 * ( SEL = wavelength * wavelength * echoConstant)
 * @param wavelength The wavelength column
 * @return The corresponding SpinEchoLength column
 */
std::vector<double> SaveSESANS::calculateSpinEchoLength(const HistogramData::Points &wavelength) const {
  std::vector<double> spinEchoLength;
  const double echoConstant = getProperty("EchoConstant");

  // SEL is calculated as wavelength^2 * echoConstant
  transform(wavelength.begin(), wavelength.end(), back_inserter(spinEchoLength),
            [&](double w) { return w * w * echoConstant; });
  return spinEchoLength;
}

/**
 * Calculate the depolarisation column from wavelength and Y values in the
 * workspace
 * (depol = ln(y) / wavelength^2 / sample thickness in cm)
 * @param yValues Y column in the workspace
 * @param wavelength Wavelength column
 * @return The depolarisation column
 */
std::vector<double> SaveSESANS::calculateDepolarisation(const HistogramData::HistogramY &yValues,
                                                        const HistogramData::Points &wavelength) const {
  Mantid::MantidVec depolarisation;
  // The sample thickness is provided in mm but we need to use cm here
  double thickness = m_sampleThickness * 0.1;

  // Depol is calculated as ln(y) / wavelength^2 / sample thickness in cm
  transform(yValues.begin(), yValues.end(), wavelength.begin(), back_inserter(depolarisation),
            [&](double y, double w) { return log(y) / (w * w) / thickness; });
  return depolarisation;
}

/**
 * Calculate the error column from the workspace values
 * (error = e / (y * wavelength^2)) / sample thickness in cm
 * @param eValues Error values from the workspace
 * @param yValues Y values from the workspace
 * @param wavelength Wavelength values (X values from the workspace)
 * @return The error column
 */
Mantid::MantidVec SaveSESANS::calculateError(const HistogramData::HistogramE &eValues,
                                             const HistogramData::HistogramY &yValues,
                                             const HistogramData::Points &wavelength) const {
  Mantid::MantidVec error;

  // The sample thickness is provided in mm but we need to use cm here
  double thickness = m_sampleThickness * 0.1;

  // Error is calculated as e / (y * wavelength^2) / sample thickness in cm
  for (size_t i = 0; i < eValues.size(); i++) {
    error.emplace_back(eValues[i] / (yValues[i] * wavelength[i] * wavelength[i]) / thickness);
  }
  return error;
}

} // namespace Mantid::DataHandling
