// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveNexusMuon.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexusV2NexusHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidNexus/NexusFile.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(SaveNexusMuon)

using namespace API;
using namespace Kernel;
using Types::Core::DateAndTime;

namespace {
namespace Prop {
const std::string INPUT_WORKSPACE{"InputWorkspace"};
const std::string FILENAME{"Filename"};
const std::string TIME_ZERO{"TimeZero"};
const std::string TIME_ZERO_CORRECTED{"TimeZeroCorrected"};
const std::string FIRST_GOOD_DATA{"FirstGoodData"};
const std::string FIRST_GOOD_DATA_RELATIVE{"FirstGoodDataRelativeToTimeZero"};
const std::string MAIN_FIELD_DIRECTION{"MainFieldDirection"};
const std::string DEAD_TIME_TABLE{"DeadTimeTable"};
const std::string GROUPING_TABLE{"DetectorGroupingTable"};
} // namespace Prop

/// Logs which must be present because LoadMuonNexusV2 cannot load a file without the fields written from them
const std::vector<std::string> REQUIRED_LOGS{"run_number", "goodfrm", "sample_temp", "sample_magn_field"};
/// Time series logs which LoadISISNexus regenerates on loading, so must not be written to the file
const std::set<std::string> REGENERATED_LOGS{"periods"};
/// Length of the ISO8601 date and time strings read by LoadISISNexus
constexpr size_t ISO_TIME_LENGTH = 19;

/// Writes a scalar as a one-element array, which is how ISIS muon files store scalars
template <typename T>
void writeScalar(Nexus::File &file, const std::string &name, const T value, const std::string &units = "") {
  file.writeData(name, std::vector<T>{value});
  if (!units.empty()) {
    file.openData(name);
    file.putAttr("units", units);
    file.closeData();
  }
}

/// Writes a block of strings as a two-dimensional character array
void writeStringArray(Nexus::File &file, const std::string &name, const std::vector<std::string> &values) {
  size_t maxLength = 1;
  for (const auto &value : values) {
    maxLength = std::max(maxLength, value.size());
  }
  std::vector<char> buffer(values.size() * maxLength, ' ');
  for (size_t i = 0; i < values.size(); ++i) {
    std::copy(values[i].cbegin(), values[i].cend(), buffer.begin() + i * maxLength);
  }
  file.makeData(name, NXnumtype::CHAR, Nexus::DimVector{values.size(), maxLength}, true);
  file.putData(buffer.data());
  file.closeData();
}

/// The single-period value of a period log, which holds a ';' separated value for each period of the original file
std::optional<std::string> periodLogValue(const Run &run, const std::string &logName) {
  if (!run.hasProperty(logName)) {
    return std::nullopt;
  }
  const auto values = run.getProperty(logName)->value();
  size_t period = 0;
  if (run.hasProperty("current_period")) {
    period = static_cast<size_t>(std::max(1, run.getPropertyAsIntegerValue("current_period")) - 1);
  }
  std::vector<std::string> tokens;
  size_t start = 0;
  while (true) {
    const auto end = values.find(';', start);
    tokens.emplace_back(values.substr(start, end - start));
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return tokens[period < tokens.size() ? period : 0];
}

template <typename T> T periodLogNumber(const Run &run, const std::string &logName, const T defaultValue) {
  const auto value = periodLogValue(run, logName);
  if (!value) {
    return defaultValue;
  }
  try {
    if constexpr (std::is_integral_v<T>) {
      return static_cast<T>(std::stoi(*value));
    } else {
      return static_cast<T>(std::stod(*value));
    }
  } catch (const std::logic_error &) {
    return defaultValue;
  }
}

/// A numeric log value with its units, if the log exists
std::optional<std::pair<double, std::string>> numericLog(const Run &run, const std::string &logName) {
  if (!run.hasProperty(logName)) {
    return std::nullopt;
  }
  try {
    return std::make_pair(run.getPropertyAsSingleValue(logName), run.getProperty(logName)->units());
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::string isoTime(const DateAndTime &time) { return time.toISO8601String().substr(0, ISO_TIME_LENGTH); }

/// The detector IDs of the workspace, which are the same as its spectrum numbers once validated
std::vector<int32_t> detectorIDs(const MatrixWorkspace &ws) {
  std::vector<int32_t> ids;
  ids.reserve(ws.getNumberHistograms());
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    ids.emplace_back(static_cast<int32_t>(ws.getSpectrum(i).getSpectrumNo()));
  }
  return ids;
}

/// The resolution stored in the file, which is the width of the first time bin. The bin edges are stored
/// separately, so it only sets the bin numbers used for first and last good data, as in the muon loaders.
double binWidth(const MatrixWorkspace &ws) {
  const auto &x = ws.x(0);
  return x[1] - x[0];
}

int32_t binIndex(const double time, const double width) { return static_cast<int32_t>(std::lround(time / width)); }

/// Writes the times and values of a log, returning the number of values written
template <typename T>
size_t writeTimeSeriesValues(Nexus::File &file, const TimeSeriesProperty<T> &log, const DateAndTime &start) {
  const auto values = log.valuesAsVector();
  const auto times = log.timesAsVectorSeconds(start);
  file.writeData("time", times);
  file.openData("time");
  file.putAttr("start", isoTime(start));
  file.putAttr("units", std::string("second"));
  file.closeData();
  if constexpr (std::is_same_v<T, std::string>) {
    writeStringArray(file, "value", values);
  } else {
    file.writeData("value", values);
  }
  if (!log.units().empty()) {
    file.openData("value");
    file.putAttr("units", log.units());
    file.closeData();
  }
  return values.size();
}

/// Writes the validity of each log value, recorded on loading as a separate boolean log
void writeValidity(Nexus::File &file, const Run &run, const std::string &logName, const size_t numValues) {
  const auto validityName = LogManager::getInvalidValuesFilterLogName(logName);
  if (!run.hasProperty(validityName)) {
    return;
  }
  const auto *validity = dynamic_cast<const TimeSeriesProperty<bool> *>(run.getProperty(validityName));
  if (!validity || validity->size() == 0) {
    return;
  }
  // Loading appends the last value at the run end time to the value log but not to its validity log
  auto flags = validity->valuesAsVector();
  std::vector<int32_t> valid(numValues, flags.back() ? 1 : 0);
  std::transform(flags.cbegin(), flags.cbegin() + std::min(flags.size(), numValues), valid.begin(),
                 [](const bool flag) { return flag ? 1 : 0; });
  file.writeData("value_valid", valid);
}
} // namespace

void SaveNexusMuon::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(Prop::INPUT_WORKSPACE, "", Direction::Input),
                  "The single-period muon workspace to save. The counts must be whole numbers, as they are when "
                  "loaded from a muon file, and all spectra must have the same time bins.");
  declareProperty(
      std::make_unique<FileProperty>(Prop::FILENAME, "", FileProperty::Save, std::vector<std::string>{".nxs"}),
      "The name of the Muon NeXus file to write, as a full or relative path.");
  declareProperty(Prop::TIME_ZERO, EMPTY_DBL(),
                  "Time zero in microseconds, written to the file. Use the TimeZero output of Load.");
  declareProperty(Prop::TIME_ZERO_CORRECTED, true,
                  "Whether TimeZero has already been subtracted from the time axis of the workspace, as Load does "
                  "when CorrectTime is set. If so, TimeZero is added back before saving.");
  declareProperty(
      Prop::FIRST_GOOD_DATA, EMPTY_DBL(),
      "First good data in microseconds, which must not be before time zero. FirstGoodDataRelativeToTimeZero "
      "sets whether it is measured from time zero or from the start of the raw time axis. If not set, it "
      "is taken from the FirstGoodData log.");
  declareProperty(Prop::FIRST_GOOD_DATA_RELATIVE, true,
                  "Whether FirstGoodData is measured from time zero, as shown in Muon Analysis, rather than from the "
                  "start of the raw time axis, as given by the FirstGoodData output of Load.");
  const std::vector<std::string> fieldOptions{"", "Longitudinal", "Transverse"};
  declareProperty(Prop::MAIN_FIELD_DIRECTION, "", std::make_shared<StringListValidator>(fieldOptions),
                  "The main field direction. If not set, it is taken from the main_field_direction log. If neither "
                  "is available, it is not written and loading assumes Longitudinal.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(Prop::DEAD_TIME_TABLE, "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "A table of detector dead times, with columns of spectrum (int) and dead-time (double), as "
                  "given by the DeadTimeTable output of Load. Detectors not in the table, or all detectors if it "
                  "is not set, are given a dead time of zero.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(Prop::GROUPING_TABLE, "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "A table of detector groups, with a single column of detector lists (vector_int), as given by "
                  "the DetectorGroupingTable output of Load. If not set, loading uses the default grouping of the "
                  "instrument.");
}

std::map<std::string, std::string> SaveNexusMuon::validateInputs() {
  std::map<std::string, std::string> issues;
  const Workspace_sptr input = getProperty(Prop::INPUT_WORKSPACE);
  if (std::dynamic_pointer_cast<WorkspaceGroup>(input)) {
    issues[Prop::INPUT_WORKSPACE] =
        "Saving multi-period data is not supported. Save each period separately by loading it with EntryNumber.";
    return issues;
  }
  const auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(input);
  if (!ws) {
    issues[Prop::INPUT_WORKSPACE] = "The input workspace must be a MatrixWorkspace.";
    return issues;
  }
  if (isEmpty(static_cast<double>(getProperty(Prop::TIME_ZERO)))) {
    issues[Prop::TIME_ZERO] = "TimeZero must be set. Use the TimeZero output of Load.";
  }
  validateWorkspace(*ws, issues);
  if (issues.contains(Prop::INPUT_WORKSPACE)) {
    return issues;
  }
  validateDeadTimeTable(*ws, issues);
  validateGroupingTable(*ws, issues);
  return issues;
}

void SaveNexusMuon::validateWorkspace(const MatrixWorkspace &ws, std::map<std::string, std::string> &issues) const {
  auto &issue = issues[Prop::INPUT_WORKSPACE];
  const auto addIssue = [&issue](const std::string &message) { issue += (issue.empty() ? "" : "\n") + message; };

  if (ws.getNumberHistograms() == 0 || ws.blocksize() == 0) {
    addIssue("The workspace contains no data.");
  } else if (!ws.isHistogramData()) {
    addIssue("The workspace must contain histogram data.");
  } else if (!ws.isCommonBins()) {
    addIssue("All spectra must have the same time bins.");
  } else {
    const auto &x = ws.x(0);
    if (std::adjacent_find(x.cbegin(), x.cend(), std::greater_equal<double>()) != x.cend()) {
      addIssue("The time axis must be increasing.");
    }
  }
  if (const auto unit = ws.getAxis(0)->unit(); unit->label().ascii() != Units::Symbol::Microsecond.ascii()) {
    addIssue("The time axis must be in microseconds.");
  }

  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    const auto &spectrum = ws.getSpectrum(i);
    const auto &ids = spectrum.getDetectorIDs();
    if (ids.size() != 1 || *ids.begin() != spectrum.getSpectrumNo() || spectrum.getSpectrumNo() < 1) {
      addIssue("Each spectrum must contain a single detector whose ID equals its spectrum number, which must be "
               "positive. Spectrum " +
               std::to_string(spectrum.getSpectrumNo()) + " does not.");
      break;
    }
    const auto &y = spectrum.y();
    if (std::any_of(y.cbegin(), y.cend(), [](const double count) {
          return count < 0 || count > std::numeric_limits<int32_t>::max() || count != std::round(count);
        })) {
      addIssue("The counts must be non-negative whole numbers, as in raw muon data. Spectrum " +
               std::to_string(spectrum.getSpectrumNo()) + " contains other values.");
      break;
    }
  }

  const auto instrument = ws.getInstrument();
  const auto &run = ws.run();
  if (!instrument || instrument->getName().empty()) {
    addIssue("The workspace has no instrument.");
  } else if (run.hasProperty("run_start") &&
             InstrumentFileFinder::getInstrumentFilename(instrument->getName(), ws.getWorkspaceStartDate()).empty()) {
    addIssue("No instrument definition was found for " + instrument->getName() +
             ", so the saved file could not be loaded.");
  }

  for (const auto &log : REQUIRED_LOGS) {
    if (!run.hasProperty(log)) {
      addIssue("The workspace has no " + log + " log, which is needed to load the file.");
    }
  }
  if (run.hasProperty("run_number")) {
    try {
      static_cast<void>(std::stoi(run.getProperty("run_number")->value()));
    } catch (const std::logic_error &) {
      addIssue("The run_number log must be a whole number.");
    }
  }
  try {
    if (run.endTime() < run.startTime()) {
      addIssue("The run_end log is before the run_start log.");
    }
  } catch (const std::exception &) {
    addIssue("The workspace needs valid run_start and run_end logs.");
  }

  if (!issue.empty()) {
    return;
  }
  issues.erase(Prop::INPUT_WORKSPACE);

  const double timeZero = getProperty(Prop::TIME_ZERO);
  if (isEmpty(timeZero)) {
    return;
  }
  const bool corrected = getProperty(Prop::TIME_ZERO_CORRECTED);
  const double lastTime = ws.x(0).back() + (corrected ? timeZero : 0.0);
  const double firstGoodData = absoluteFirstGoodData(ws);
  if (isEmpty(firstGoodData)) {
    issues[Prop::FIRST_GOOD_DATA] = "FirstGoodData must be set, as the workspace has no FirstGoodData log.";
    return;
  }
  const bool fromProperty = !isEmpty(static_cast<double>(getProperty(Prop::FIRST_GOOD_DATA)));
  const std::string source = fromProperty ? "FirstGoodData" : "The FirstGoodData log";
  const std::string check = fromProperty ? "Check FirstGoodDataRelativeToTimeZero." : "Check TimeZeroCorrected.";
  std::ostringstream message;
  if (firstGoodData < timeZero) {
    message << source << " gives a first good data of " << firstGoodData
            << " microseconds from the start of the raw time axis, which is before TimeZero (" << timeZero << "). "
            << check;
  } else if (firstGoodData < 0) {
    message << "TimeZero and FirstGoodData must not be before the start of the raw time axis.";
  } else if (firstGoodData > lastTime) {
    message << source << " gives a first good data of " << firstGoodData
            << " microseconds from the start of the raw time axis, which is after the end of the time axis ("
            << lastTime << "). " << check;
  }
  if (!message.str().empty()) {
    issues[Prop::FIRST_GOOD_DATA] = message.str();
  }
}

void SaveNexusMuon::validateDeadTimeTable(const MatrixWorkspace &ws, std::map<std::string, std::string> &issues) const {
  const ITableWorkspace_sptr table = getProperty(Prop::DEAD_TIME_TABLE);
  if (!table) {
    return;
  }
  if (table->columnCount() != 2 || table->getColumn(0)->type() != "int" || table->getColumn(1)->type() != "double") {
    issues[Prop::DEAD_TIME_TABLE] = "The dead time table must have two columns: spectrum (int) and dead-time (double).";
    return;
  }
  const auto ids = detectorIDs(ws);
  const std::set<int32_t> detectors(ids.cbegin(), ids.cend());
  for (size_t row = 0; row < table->rowCount(); ++row) {
    if (!detectors.contains(table->cell<int>(row, 0))) {
      issues[Prop::DEAD_TIME_TABLE] = "The dead time table contains spectrum " +
                                      std::to_string(table->cell<int>(row, 0)) + ", which is not in the workspace.";
      return;
    }
  }
}

void SaveNexusMuon::validateGroupingTable(const MatrixWorkspace &ws, std::map<std::string, std::string> &issues) const {
  const ITableWorkspace_sptr table = getProperty(Prop::GROUPING_TABLE);
  if (!table) {
    return;
  }
  if (table->columnCount() != 1 || table->getColumn(0)->type() != "vector_int") {
    issues[Prop::GROUPING_TABLE] = "The grouping table must have a single column of detector lists (vector_int).";
    return;
  }
  const auto ids = detectorIDs(ws);
  const std::set<int32_t> detectors(ids.cbegin(), ids.cend());
  std::set<int32_t> grouped;
  for (size_t row = 0; row < table->rowCount(); ++row) {
    for (const auto detector : table->cell<std::vector<int>>(row, 0)) {
      if (!detectors.contains(detector)) {
        issues[Prop::GROUPING_TABLE] =
            "The grouping table contains detector " + std::to_string(detector) + ", which is not in the workspace.";
        return;
      }
      if (!grouped.insert(detector).second) {
        issues[Prop::GROUPING_TABLE] = "Detector " + std::to_string(detector) +
                                       " is in more than one group. The file can only store one group per detector.";
        return;
      }
    }
  }
}

/// First good data measured from the start of the raw time axis, or EMPTY_DBL if it is not available
double SaveNexusMuon::absoluteFirstGoodData(const MatrixWorkspace &ws) const {
  const double timeZero = getProperty(Prop::TIME_ZERO);
  const double property = getProperty(Prop::FIRST_GOOD_DATA);
  if (!isEmpty(property)) {
    const bool relative = getProperty(Prop::FIRST_GOOD_DATA_RELATIVE);
    return property + (relative ? timeZero : 0.0);
  }
  const auto &run = ws.run();
  if (!run.hasProperty("FirstGoodData")) {
    return EMPTY_DBL();
  }
  // The FirstGoodData log is relative to time zero when the time axis has been corrected
  const bool corrected = getProperty(Prop::TIME_ZERO_CORRECTED);
  return run.getPropertyAsSingleValue("FirstGoodData") + (corrected ? timeZero : 0.0);
}

std::string SaveNexusMuon::mainFieldDirection(const MatrixWorkspace &ws) const {
  const std::string property = getProperty(Prop::MAIN_FIELD_DIRECTION);
  if (!property.empty()) {
    return property;
  }
  const auto &run = ws.run();
  return run.hasProperty("main_field_direction") ? run.getProperty("main_field_direction")->value() : "";
}

void SaveNexusMuon::exec() {
  const auto ws =
      std::dynamic_pointer_cast<MatrixWorkspace>(static_cast<Workspace_sptr>(getProperty(Prop::INPUT_WORKSPACE)));
  const std::string filename = getPropertyValue(Prop::FILENAME);

  Nexus::File file(filename, NXaccess::CREATE5);
  file.makeGroup(NeXusEntry::RAWDATA_NAME, "NXentry", true);

  const auto instrumentName = ws->getInstrument()->getName();
  writeScalar<int32_t>(file, "IDF_version", 2);
  file.writeData("definition", "pulsedTD");
  file.openData("definition");
  file.putAttr("version", std::string("2.0"));
  file.closeData();
  file.writeData("beamline", instrumentName);
  file.writeData("name", instrumentName);
  file.writeData("program_name", "mantid");

  writeRunDetails(file, *ws);
  writeInstrument(file, *ws);
  writeSample(file, *ws);
  writePeriods(file, *ws);
  writeMeasurement(file, *ws);
  writeLogs(file, *ws);

  file.closeGroup();
  file.close();
}

void SaveNexusMuon::writeRunDetails(Nexus::File &file, const MatrixWorkspace &ws) const {
  const auto &run = ws.run();
  const auto title =
      ws.getTitle().empty() && run.hasProperty("run_title") ? run.getProperty("run_title")->value() : ws.getTitle();
  file.writeData("title", title);
  file.writeData("notes", ws.getComment());
  writeScalar<int32_t>(file, "run_number", std::stoi(run.getProperty("run_number")->value()));
  file.writeData("start_time", isoTime(run.startTime()));
  file.writeData("end_time", isoTime(run.endTime()));

  // Duration of the run in seconds, from the dur log when it is in seconds, otherwise from the start and end times
  auto duration = DateAndTime::secondsFromDuration(run.endTime() - run.startTime());
  if (const auto dur = numericLog(run, "dur"); dur && (dur->second.empty() || dur->second == "second")) {
    duration = dur->first;
  }
  writeScalar(file, "duration", static_cast<float>(duration), "second");

  std::vector<std::string> notRecorded;
  const auto writeLogOrZero = [&](const std::string &field, const std::string &logName, const std::string &units) {
    const auto log = numericLog(run, logName);
    if (!log) {
      notRecorded.emplace_back(field);
    }
    writeScalar(file, field, static_cast<float>(log ? log->first : 0.0),
                log && !log->second.empty() ? log->second : units);
  };
  writeLogOrZero("proton_charge", "gd_prtn_chrg", "uAh");
  writeLogOrZero("proton_charge_raw", "tot_prtn_chrg", "uAh");

  const auto goodFrames = static_cast<int32_t>(run.getPropertyAsIntegerValue("goodfrm"));
  writeScalar(file, "good_frames", goodFrames);
  if (!run.hasProperty("rawfrm")) {
    notRecorded.emplace_back("raw_frames");
  }
  writeScalar(file, "raw_frames",
              run.hasProperty("rawfrm") ? static_cast<int32_t>(run.getPropertyAsIntegerValue("rawfrm")) : 0);
  if (!run.hasProperty("rb_proposal")) {
    notRecorded.emplace_back("experiment_identifier");
  }
  file.writeData("experiment_identifier",
                 run.hasProperty("rb_proposal") ? run.getProperty("rb_proposal")->value() : std::string(""));

  if (!notRecorded.empty()) {
    std::string fields;
    for (const auto &field : notRecorded) {
      fields += (fields.empty() ? "" : ", ") + field;
    }
    g_log.notice() << "The workspace does not record " << fields
                   << ", so they are written as zero or blank. Muon analysis does not use them.\n";
  }
}

void SaveNexusMuon::writeInstrument(Nexus::File &file, const MatrixWorkspace &ws) const {
  const auto &run = ws.run();
  const auto numSpectra = ws.getNumberHistograms();
  const auto numBins = ws.blocksize();
  const bool corrected = getProperty(Prop::TIME_ZERO_CORRECTED);
  const double timeZero = getProperty(Prop::TIME_ZERO);
  const double timeShift = corrected ? timeZero : 0.0;
  const auto width = binWidth(ws);
  const auto ids = detectorIDs(ws);
  const auto maxDetector = static_cast<size_t>(*std::max_element(ids.cbegin(), ids.cend()));

  file.makeGroup("instrument", "NXinstrument", true);
  file.writeData("name", ws.getInstrument()->getName());
  file.makeGroup("source", "NXsource", true);
  file.writeData("name", "ISIS");
  if (const auto frequency = numericLog(run, "freq")) {
    writeScalar(file, "frequency", static_cast<float>(frequency->first), "Hz");
  }
  file.closeGroup();

  file.makeGroup("detector_1", "NXdetector", true);
  std::vector<NXlink> links;

  std::vector<int32_t> counts;
  counts.reserve(numSpectra * numBins);
  for (size_t i = 0; i < numSpectra; ++i) {
    const auto &y = ws.y(i);
    std::transform(y.cbegin(), y.cend(), std::back_inserter(counts),
                   [](const double count) { return static_cast<int32_t>(std::lround(count)); });
  }
  file.writeData(NeXusEntry::COUNTS, counts, Nexus::DimVector{1, numSpectra, numBins});
  file.openData(NeXusEntry::COUNTS);
  file.putAttr("signal", int32_t{1});
  file.putAttr("axes", std::string("period_index,spectrum_index,raw_time"));
  file.putAttr("units", std::string("counts"));
  file.putAttr(NeXusEntry::FIRSTGOODBIN, binIndex(absoluteFirstGoodData(ws), width));
  file.putAttr(NeXusEntry::LASTGOODBIN, binIndex(ws.x(0).back() + timeShift, width));
  links.emplace_back(file.getDataID());
  file.closeData();

  const auto &x = ws.x(0);
  std::vector<float> rawTime(x.size());
  std::transform(x.cbegin(), x.cend(), rawTime.begin(),
                 [timeShift](const double time) { return static_cast<float>(time + timeShift); });
  file.writeData("raw_time", rawTime);
  file.openData("raw_time");
  file.putAttr("units", std::string("microseconds"));
  links.emplace_back(file.getDataID());
  file.closeData();

  file.writeData("spectrum_index", ids);
  file.openData("spectrum_index");
  links.emplace_back(file.getDataID());
  file.closeData();
  file.writeData("period_index", std::vector<int32_t>{1});
  file.openData("period_index");
  links.emplace_back(file.getDataID());
  file.closeData();

  writeScalar(file, NeXusEntry::RESOLUTION, static_cast<int32_t>(std::lround(width * 1e6)), "picoseconds");
  writeScalar(file, NeXusEntry::TIMEZERO, static_cast<float>(timeZero), "microseconds");

  // Dead times and grouping are indexed by detector ID - 1 when loaded. Dead times are always written, as
  // LoadMuonNexusV2 expects them; zero dead times apply no correction.
  std::vector<float> deadTimeValues(maxDetector, 0.0f);
  if (const ITableWorkspace_sptr deadTimes = getProperty(Prop::DEAD_TIME_TABLE)) {
    for (size_t row = 0; row < deadTimes->rowCount(); ++row) {
      deadTimeValues[deadTimes->cell<int>(row, 0) - 1] = static_cast<float>(deadTimes->cell<double>(row, 1));
    }
  } else {
    g_log.notice("No DeadTimeTable was given, so zero dead times are written.\n");
  }
  file.writeData(NeXusEntry::DEADTIME, deadTimeValues);
  file.openData(NeXusEntry::DEADTIME);
  file.putAttr("units", std::string("microseconds"));
  file.closeData();
  if (const ITableWorkspace_sptr grouping = getProperty(Prop::GROUPING_TABLE)) {
    std::vector<int32_t> groups(maxDetector, 0);
    for (size_t row = 0; row < grouping->rowCount(); ++row) {
      for (const auto detector : grouping->cell<std::vector<int>>(row, 0)) {
        groups[detector - 1] = static_cast<int32_t>(row + 1);
      }
    }
    file.writeData(NeXusEntry::GROUPING, groups);
  }
  if (const auto direction = mainFieldDirection(ws); !direction.empty()) {
    file.writeData("orientation", direction == "Transverse" ? "t" : "l");
  }
  file.closeGroup(); // detector_1
  file.closeGroup(); // instrument

  // LoadISISNexus reads the counts and time bins from the NXdata group, which links to the detector
  file.makeGroup("detector_1", "NXdata", true);
  for (const auto &link : links) {
    file.makeLink(link);
  }
  file.closeGroup();
}

void SaveNexusMuon::writeSample(Nexus::File &file, const MatrixWorkspace &ws) const {
  const auto &run = ws.run();
  const auto &sample = ws.sample();
  file.makeGroup(NeXusEntry::SAMPLE, "NXsample", true);
  file.writeData("name", sample.getName());
  file.writeData("id", std::to_string(sample.getGeometryFlag()));
  writeScalar(file, "thickness", static_cast<float>(sample.getThickness()));
  writeScalar(file, "height", static_cast<float>(sample.getHeight()));
  writeScalar(file, "width", static_cast<float>(sample.getWidth()));
  writeScalar(file, NeXusEntry::TEMPERATURE, static_cast<float>(run.getPropertyAsSingleValue("sample_temp")), "Kelvin");
  writeScalar(file, NeXusEntry::MAGNETICFIELD, static_cast<float>(run.getPropertyAsSingleValue("sample_magn_field")),
              "Gauss");
  file.closeGroup();
}

void SaveNexusMuon::writePeriods(Nexus::File &file, const MatrixWorkspace &ws) const {
  const auto &run = ws.run();
  const auto goodFrames = static_cast<int32_t>(run.getPropertyAsIntegerValue("goodfrm"));
  double totalCounts = 0.0;
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    const auto &y = ws.y(i);
    totalCounts += std::accumulate(y.cbegin(), y.cend(), 0.0);
  }

  file.makeGroup(NeXusEntry::PERIOD, "IXperiods", true);
  writeScalar<int32_t>(file, "number", 1);
  file.writeData("labels", periodLogValue(run, "period_labels").value_or("Period 1"));
  writeScalar(file, NeXusEntry::SEQUENCES, periodLogNumber<int32_t>(run, "period_sequences", 1));
  writeScalar(file, NeXusEntry::TYPE, periodLogNumber<int32_t>(run, "period_type", 1));
  writeScalar(file, NeXusEntry::REQUESTED, periodLogNumber<int32_t>(run, "frames_period_requested", 0));
  writeScalar(file, NeXusEntry::RAWFRAMES, periodLogNumber<int32_t>(run, "frames_period_raw", 0));
  writeScalar(file, NeXusEntry::PERIODOUTPUT, periodLogNumber<int32_t>(run, "period_output", 0));
  // Total counts are in millions of events
  writeScalar(file, NeXusEntry::PERIODCOUNTS,
              periodLogNumber<float>(run, "total_counts_period", static_cast<float>(totalCounts / 1e6)), "Mev");
  writeScalar(file, NeXusEntry::GOODFRAMES, goodFrames);
  if (run.hasProperty("proton_charge_by_period")) {
    const auto *charges = dynamic_cast<ArrayProperty<double> *>(run.getProperty("proton_charge_by_period"));
    if (charges && !charges->operator()().empty()) {
      writeScalar(file, "proton_charge", static_cast<float>(charges->operator()().front()), "uAh");
    }
  }
  file.closeGroup();
}

void SaveNexusMuon::writeMeasurement(Nexus::File &file, const MatrixWorkspace &ws) const {
  const auto &run = ws.run();
  const std::vector<std::string> fields{"id", "label", "subid", "type"};
  if (std::none_of(fields.cbegin(), fields.cend(),
                   [&run](const auto &field) { return run.hasProperty("measurement_" + field); })) {
    return;
  }
  file.makeGroup("measurement", "NXcollection", true);
  for (const auto &field : fields) {
    const auto logName = "measurement_" + field;
    file.writeData(field, run.hasProperty(logName) ? run.getProperty(logName)->value() : std::string(""));
  }
  file.closeGroup();
}

void SaveNexusMuon::writeLogs(Nexus::File &file, const MatrixWorkspace &ws) const {
  const auto &run = ws.run();
  // Log times are written relative to the start time stored in the file
  const DateAndTime start(isoTime(run.startTime()));

  file.makeGroup("runlog", "IXrunlog", true);
  for (const auto *log : run.getProperties()) {
    const auto &logName = log->name();
    if (REGENERATED_LOGS.contains(logName)) {
      continue;
    }
    const auto *doubleLog = dynamic_cast<const TimeSeriesProperty<double> *>(log);
    const auto *intLog = dynamic_cast<const TimeSeriesProperty<int32_t> *>(log);
    const auto *stringLog = dynamic_cast<const TimeSeriesProperty<std::string> *>(log);
    const auto *timeSeries = dynamic_cast<const ITimeSeriesProperty *>(log);
    // Boolean logs, such as running and the period filters, are regenerated on loading
    if ((!doubleLog && !intLog && !stringLog) || timeSeries->realSize() == 0) {
      continue;
    }
    if (logName.contains('/')) {
      g_log.warning() << "The log " << logName << " is not saved, as its name contains '/'.\n";
      continue;
    }
    file.makeGroup(logName, "NXlog", true);
    size_t numValues = 0;
    if (doubleLog) {
      numValues = writeTimeSeriesValues(file, *doubleLog, start);
    } else if (intLog) {
      numValues = writeTimeSeriesValues(file, *intLog, start);
    } else {
      numValues = writeTimeSeriesValues(file, *stringLog, start);
    }
    writeValidity(file, run, logName, numValues);
    file.closeGroup();
  }
  file.closeGroup();
}

} // namespace Mantid::DataHandling
