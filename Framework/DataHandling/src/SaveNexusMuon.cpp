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
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
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
const std::string LAST_GOOD_DATA{"LastGoodData"};
const std::string GOOD_DATA_RELATIVE{"GoodDataRelativeToTimeZero"};
const std::string MAIN_FIELD_DIRECTION{"MainFieldDirection"};
const std::string DEAD_TIME_TABLE{"DeadTimeTable"};
const std::string GROUPING_TABLE{"DetectorGroupingTable"};
} // namespace Prop

/// Logs which must be present because LoadMuonNexusV2 cannot load a file without the fields written from them
const std::vector<std::string> REQUIRED_LOGS{"run_number", "goodfrm", "sample_temp", "sample_magn_field"};
/// Logs which are written as integers, so must hold one if present
const std::vector<std::string> INTEGER_LOGS{"goodfrm", "rawfrm", "current_period"};
/// Logs which are written as single numbers, so must convert to one if present
const std::vector<std::string> NUMERIC_LOGS{"sample_temp", "sample_magn_field"};
/// Time series logs which LoadISISNexus regenerates on loading, so must not be written to the file
const std::set<std::string> REGENERATED_LOGS{"periods"};
/// Length of the ISO8601 date and time strings read by LoadISISNexus
constexpr size_t ISO_TIME_LENGTH = 19;

/// Writes a block of strings as a two-dimensional character array
void writeStringArray(Nexus::File &file, const std::string &name, const std::vector<std::string> &values) {
  const size_t maxLength =
      std::accumulate(values.cbegin(), values.cend(), size_t{1},
                      [](const size_t currentMax, const std::string &a) { return std::max(currentMax, a.size()); });
  std::vector<char> buffer(values.size() * maxLength, ' ');
  for (size_t i = 0; i < values.size(); ++i) {
    std::copy(values[i].cbegin(), values[i].cend(), buffer.begin() + i * maxLength);
  }
  file.makeData(name, NXnumtype::CHAR, Nexus::DimVector{values.size(), maxLength}, true);
  file.putData(buffer.data());
  file.closeData();
}

using Attributes = std::vector<std::pair<std::string, std::string>>;

/// Writes a dataset with the given text attributes, leaving out those that are empty, and returns its link
template <typename T>
NXlink writeWithAttributes(Nexus::File &file, const std::string &name, const T &value,
                           const Attributes &attributes = {}) {
  if constexpr (std::is_same_v<T, std::vector<std::string>>) {
    writeStringArray(file, name, value);
  } else {
    file.writeData(name, value);
  }
  file.openData(name);
  for (const auto &[attribute, text] : attributes) {
    if (!text.empty()) {
      file.putAttr(attribute, text);
    }
  }
  const auto link = file.getDataID();
  file.closeData();
  return link;
}

/// Writes a scalar as a one-element array, which is how ISIS muon files store scalars
template <typename T>
void writeScalar(Nexus::File &file, const std::string &name, const T value, const std::string &units = "") {
  writeWithAttributes(file, name, std::vector<T>{value}, {{"units", units}});
}

/// The value of a log as text, or an empty string if the log does not exist
std::string logValue(const Run &run, const std::string &logName) {
  return run.hasProperty(logName) ? run.getProperty(logName)->value() : "";
}

/// The log with the given name and type, or null if there is no such log
template <typename T> const T *findLog(const Run &run, const std::string &logName) {
  return run.hasProperty(logName) ? dynamic_cast<const T *>(run.getProperty(logName)) : nullptr;
}

/// The zero-based index of the period the workspace holds, from the current_period log
size_t currentPeriodIndex(const Run &run) {
  return run.hasProperty("current_period")
             ? static_cast<size_t>(std::max(1, run.getPropertyAsIntegerValue("current_period")) - 1)
             : 0;
}

/// The single-period value of a period log, which holds a ';' separated value for each period of the original file
std::optional<std::string> periodLogValue(const Run &run, const std::string &logName, Logger &logger) {
  if (!run.hasProperty(logName)) {
    return std::nullopt;
  }
  // An empty log holds one empty value, but the tokenizer gives no tokens for it
  const auto values = run.getProperty(logName)->value();
  const auto tokens = values.empty() ? std::vector<std::string>{""} : StringTokenizer(values, ";").asVector();
  const auto period = currentPeriodIndex(run);
  if (period >= tokens.size()) {
    logger.warning() << "The " << logName << " log has no value for period " << period + 1
                     << ", so the value for period 1 is saved.\n";
    return tokens[0];
  }
  return tokens[period];
}

/// The single-period value of a period log as a whole number, or defaultValue if it has none
int32_t periodLogNumber(const Run &run, const std::string &logName, const int32_t defaultValue, Logger &logger) {
  const auto value = periodLogValue(run, logName, logger);
  try {
    return value ? std::stoi(*value) : defaultValue;
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
double firstBinWidth(const MatrixWorkspace &ws) {
  const auto &x = ws.x(0);
  return x[1] - x[0];
}

/// Whether a bin width in microseconds rounds to a resolution in picoseconds that the file can store
bool isStorableResolution(const double width) {
  const double picoseconds = width * 1e6;
  return picoseconds >= 0.5 && picoseconds < static_cast<double>(std::numeric_limits<int32_t>::max()) + 0.5;
}

/// The resolution in picoseconds, as stored in the file
int32_t resolutionPicoseconds(const double width) { return static_cast<int32_t>(std::lround(width * 1e6)); }

/// Whether a time in microseconds can be stored as a bin number at the given resolution in picoseconds
bool isStorableBin(const double time, const int32_t resolution) {
  return std::abs(time * 1e6 / static_cast<double>(resolution)) <
         static_cast<double>(std::numeric_limits<int32_t>::max());
}

/// Allows for float rounding in a time that lies on a bin edge, as a fraction of a bin
constexpr double BIN_TOLERANCE = 1e-6;

/// The good data bin to store: the bin nearest to time, kept within the bins that are not before earliest or after
/// latest. Bins are measured in the resolution the loader decodes them with. If no bin lies in that range, the first
/// bin not before earliest is used.
int32_t goodDataBin(const double time, const double earliest, const double latest, const int32_t resolution) {
  const double binWidth = static_cast<double>(resolution) / 1e6;
  const auto firstBin = static_cast<int32_t>(std::ceil(earliest / binWidth - BIN_TOLERANCE));
  const auto lastBin = static_cast<int32_t>(std::floor(latest / binWidth + BIN_TOLERANCE));
  const auto bin = static_cast<int32_t>(std::lround(time / binWidth));
  return std::max(firstBin, std::min(bin, lastBin));
}

/// Writes the validity of each log value, recorded on loading as a separate boolean log
void writeValidity(Nexus::File &file, const Run &run, const std::string &logName, const size_t numValues) {
  const auto *validity = findLog<TimeSeriesProperty<bool>>(run, LogManager::getInvalidValuesFilterLogName(logName));
  if (!validity || validity->size() == 0) {
    return;
  }
  // Loading appends the last value at the run end time to the value log but not to its validity log
  const auto flags = validity->valuesAsVector();
  std::vector<int32_t> valid(numValues, static_cast<int32_t>(flags.back()));
  std::copy_n(flags.cbegin(), std::min(flags.size(), numValues), valid.begin());
  file.writeData("value_valid", valid);
}

/// Writes a time series log as an NXlog group of its times, values and their validity
template <typename T>
void writeLog(Nexus::File &file, const Run &run, const TimeSeriesProperty<T> &log, const DateAndTime &start) {
  const auto values = log.valuesAsVector();
  file.makeGroup(log.name(), "NXlog", true);
  writeWithAttributes(file, "time", log.timesAsVectorSeconds(start), {{"start", isoTime(start)}, {"units", "second"}});
  writeWithAttributes(file, "value", values, {{"units", log.units()}});
  writeValidity(file, run, log.name(), values.size());
  file.closeGroup();
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
  declareProperty(Prop::FIRST_GOOD_DATA, EMPTY_DBL(),
                  "First good data in microseconds, which must not be before time zero. GoodDataRelativeToTimeZero "
                  "sets whether it is measured from time zero or from the start of the raw time axis. If not set, it "
                  "is taken from the FirstGoodData log.");
  declareProperty(Prop::LAST_GOOD_DATA, EMPTY_DBL(),
                  "Last good data in microseconds, which must be after FirstGoodData and not after the end of the "
                  "time axis. GoodDataRelativeToTimeZero sets whether it is measured from time zero or from the start "
                  "of the raw time axis. If not set, the data is good up to the end of the time axis.");
  declareProperty(Prop::GOOD_DATA_RELATIVE, true,
                  "Whether FirstGoodData and LastGoodData are measured from time zero, as shown in Muon Analysis, "
                  "rather than from the start of the raw time axis, as given by the FirstGoodData and LastGoodData "
                  "outputs of Load.");
  declareProperty(Prop::MAIN_FIELD_DIRECTION, "",
                  std::make_shared<StringListValidator>(std::vector<std::string>{"", "Longitudinal", "Transverse"}),
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
  if (const double timeZero = getProperty(Prop::TIME_ZERO); isEmpty(timeZero)) {
    issues[Prop::TIME_ZERO] = "TimeZero must be set. Use the TimeZero output of Load.";
  } else if (!std::isfinite(timeZero)) {
    issues[Prop::TIME_ZERO] = "TimeZero must be a finite number.";
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

  // blocksize() is not used, as it throws for spectra of different sizes, which isCommonBins reports
  if (ws.getNumberHistograms() == 0 || ws.y(0).empty()) {
    addIssue("The workspace contains no data.");
  } else if (!ws.isHistogramData()) {
    addIssue("The workspace must contain histogram data.");
  } else if (!ws.isCommonBins()) {
    addIssue("All spectra must have the same time bins.");
  } else {
    const auto &x = ws.x(0);
    if (std::any_of(x.cbegin(), x.cend(), [](const double time) { return !std::isfinite(time); })) {
      addIssue("The time axis must only contain finite values.");
    } else if (std::adjacent_find(x.cbegin(), x.cend(), std::greater_equal<double>()) != x.cend()) {
      addIssue("The time axis must be increasing.");
    } else if (const double width = firstBinWidth(ws); !isStorableResolution(width)) {
      std::ostringstream message;
      message << "The first time bin is " << width
              << " microseconds wide, which cannot be stored as the resolution of the file. It must round to between "
                 "1 and "
              << std::numeric_limits<int32_t>::max() << " picoseconds.";
      addIssue(message.str());
    }
  }
  if (const auto unit = ws.getAxis(0)->unit(); unit->label().ascii() != Units::Symbol::Microsecond.ascii()) {
    addIssue("The time axis must be in microseconds.");
  }

  std::set<specnum_t> spectrumNumbers;
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    const auto &spectrum = ws.getSpectrum(i);
    const auto &ids = spectrum.getDetectorIDs();
    if (ids.size() != 1 || *ids.begin() != spectrum.getSpectrumNo() || spectrum.getSpectrumNo() < 1) {
      addIssue("Each spectrum must contain a single detector whose ID equals its spectrum number, which must be "
               "positive. Spectrum " +
               std::to_string(spectrum.getSpectrumNo()) + " does not.");
      break;
    }
    if (!spectrumNumbers.insert(spectrum.getSpectrumNo()).second) {
      addIssue("Each spectrum number must be unique. Spectrum " + std::to_string(spectrum.getSpectrumNo()) +
               " appears more than once.");
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
  // Logs which are converted when writing the file, which must be checked before it is created
  const auto checkLogs = [&](const std::vector<std::string> &logs, const std::string &kind, const auto &convert) {
    for (const auto &log : logs) {
      try {
        if (run.hasProperty(log)) {
          static_cast<void>(convert(log));
        }
      } catch (const std::exception &) {
        addIssue("The " + log + " log must be " + kind + ".");
      }
    }
  };
  checkLogs(INTEGER_LOGS, "a whole number", [&run](const auto &log) { return run.getPropertyAsIntegerValue(log); });
  checkLogs(NUMERIC_LOGS, "a number", [&run](const auto &log) { return run.getPropertyAsSingleValue(log); });
  checkLogs({"run_number"}, "a whole number",
            [&run](const auto &log) { return std::stoi(run.getProperty(log)->value()); });
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

  // A missing or non-finite TimeZero is reported in validateInputs
  const double timeZero = getProperty(Prop::TIME_ZERO);
  if (isEmpty(timeZero) || !std::isfinite(timeZero)) {
    return;
  }
  const double lastTime = ws.x(0).back() + timeZeroIf(Prop::TIME_ZERO_CORRECTED);
  const auto resolution = resolutionPicoseconds(firstBinWidth(ws));
  if (!isStorableBin(timeZero, resolution)) {
    std::ostringstream message;
    message << "TimeZero (" << timeZero << ") is too far from zero to be stored as a bin number at the resolution of "
            << resolution << " picoseconds.";
    issues[Prop::TIME_ZERO] = message.str();
    return;
  }
  if (!isStorableBin(lastTime, resolution)) {
    std::ostringstream message;
    message << "The time axis ends at " << lastTime
            << " microseconds, which is too far from zero to be stored as a bin number at the resolution of "
            << resolution << " picoseconds.";
    issues[Prop::INPUT_WORKSPACE] = message.str();
    return;
  }
  const double firstGoodData = absoluteFirstGoodData(ws);
  if (isEmpty(firstGoodData)) {
    issues[Prop::FIRST_GOOD_DATA] = "FirstGoodData must be set, as the workspace has no FirstGoodData log.";
    return;
  }
  const bool fromProperty = !isEmpty(static_cast<double>(getProperty(Prop::FIRST_GOOD_DATA)));
  const std::string source = fromProperty ? "FirstGoodData" : "The FirstGoodData log";
  const std::string check = fromProperty ? "Check GoodDataRelativeToTimeZero." : "Check TimeZeroCorrected.";
  std::ostringstream message;
  if (!std::isfinite(firstGoodData)) {
    message << source << " must be a finite number.";
  } else if (firstGoodData < timeZero) {
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
    return;
  }

  const bool lastFromProperty = !isEmpty(static_cast<double>(getProperty(Prop::LAST_GOOD_DATA)));
  const double lastGoodData = absoluteLastGoodData(ws);
  std::ostringstream lastMessage;
  if (lastFromProperty) {
    if (!std::isfinite(lastGoodData)) {
      lastMessage << "LastGoodData must be a finite number.";
    } else if (lastGoodData <= firstGoodData) {
      lastMessage << "LastGoodData gives a last good data of " << lastGoodData
                  << " microseconds from the start of the raw time axis, which is not after first good data ("
                  << firstGoodData << "). Check GoodDataRelativeToTimeZero.";
    } else if (lastGoodData > lastTime) {
      lastMessage << "LastGoodData gives a last good data of " << lastGoodData
                  << " microseconds from the start of the raw time axis, which is after the end of the time axis ("
                  << lastTime << "). Check GoodDataRelativeToTimeZero.";
    }
  }
  // The file stores whole bins, so good data that is valid as a time can still round to an empty range
  if (lastMessage.str().empty()) {
    const auto firstBin = goodDataBin(firstGoodData, timeZero, lastTime, resolution);
    const auto lastBin = goodDataBin(lastGoodData, timeZero, lastTime, resolution);
    if (lastBin <= firstBin) {
      lastMessage << "First and last good data are stored as bins " << firstBin << " and " << lastBin
                  << " at the resolution of " << resolution
                  << " picoseconds, which leaves no good data. The last good data must be at least one bin after the "
                     "first.";
    }
  }
  if (!lastMessage.str().empty()) {
    issues[lastFromProperty ? Prop::LAST_GOOD_DATA : Prop::FIRST_GOOD_DATA] = lastMessage.str();
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
  std::set<int32_t> seen;
  for (size_t row = 0; row < table->rowCount(); ++row) {
    const auto spectrum = table->cell<int>(row, 0);
    if (!detectors.contains(spectrum)) {
      issues[Prop::DEAD_TIME_TABLE] =
          "The dead time table contains spectrum " + std::to_string(spectrum) + ", which is not in the workspace.";
      return;
    }
    if (!seen.insert(spectrum).second) {
      issues[Prop::DEAD_TIME_TABLE] = "The dead time table contains spectrum " + std::to_string(spectrum) +
                                      " more than once. The file can only store one dead time per detector.";
      return;
    }
    if (!std::isfinite(table->cell<double>(row, 1))) {
      issues[Prop::DEAD_TIME_TABLE] =
          "The dead time of spectrum " + std::to_string(spectrum) + " must be a finite number.";
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

/// TimeZero if the given boolean property is set, otherwise zero
double SaveNexusMuon::timeZeroIf(const std::string &flagProperty) const {
  const bool flag = getProperty(flagProperty);
  return flag ? static_cast<double>(getProperty(Prop::TIME_ZERO)) : 0.0;
}

/// First good data measured from the start of the raw time axis, or EMPTY_DBL if it is not available
double SaveNexusMuon::absoluteFirstGoodData(const MatrixWorkspace &ws) const {
  if (const double property = getProperty(Prop::FIRST_GOOD_DATA); !isEmpty(property)) {
    return property + timeZeroIf(Prop::GOOD_DATA_RELATIVE);
  }
  // The FirstGoodData log is relative to time zero when the time axis has been corrected
  const auto &run = ws.run();
  return run.hasProperty("FirstGoodData")
             ? run.getPropertyAsSingleValue("FirstGoodData") + timeZeroIf(Prop::TIME_ZERO_CORRECTED)
             : EMPTY_DBL();
}

/// Last good data measured from the start of the raw time axis. Without a LastGoodData property, the data is good to
/// the end of the time axis, as in ISIS muon files, where last_good_bin is the end of the data.
double SaveNexusMuon::absoluteLastGoodData(const MatrixWorkspace &ws) const {
  const double property = getProperty(Prop::LAST_GOOD_DATA);
  return isEmpty(property) ? ws.x(0).back() + timeZeroIf(Prop::TIME_ZERO_CORRECTED)
                           : property + timeZeroIf(Prop::GOOD_DATA_RELATIVE);
}

std::string SaveNexusMuon::mainFieldDirection(const MatrixWorkspace &ws) const {
  const std::string property = getProperty(Prop::MAIN_FIELD_DIRECTION);
  return property.empty() ? logValue(ws.run(), "main_field_direction") : property;
}

void SaveNexusMuon::exec() {
  const auto ws =
      std::dynamic_pointer_cast<MatrixWorkspace>(static_cast<Workspace_sptr>(getProperty(Prop::INPUT_WORKSPACE)));
  const std::string filename = getPropertyValue(Prop::FILENAME);

  Nexus::File file(filename, NXaccess::CREATE5);
  file.makeGroup(NeXusEntry::RAWDATA_NAME, "NXentry", true);

  const auto instrumentName = ws->getInstrument()->getName();
  writeScalar<int32_t>(file, "IDF_version", 2);
  writeWithAttributes(file, "definition", std::string("pulsedTD"), {{"version", "2.0"}});
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
  file.writeData("title", ws.getTitle().empty() ? logValue(run, "run_title") : ws.getTitle());
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

  writeScalar(file, "good_frames", static_cast<int32_t>(run.getPropertyAsIntegerValue("goodfrm")));
  if (!run.hasProperty("rawfrm")) {
    notRecorded.emplace_back("raw_frames");
  }
  writeScalar(file, "raw_frames",
              run.hasProperty("rawfrm") ? static_cast<int32_t>(run.getPropertyAsIntegerValue("rawfrm")) : 0);
  if (!run.hasProperty("rb_proposal")) {
    notRecorded.emplace_back("experiment_identifier");
  }
  file.writeData("experiment_identifier", logValue(run, "rb_proposal"));

  if (!notRecorded.empty()) {
    g_log.notice() << "The workspace does not record " << Strings::join(notRecorded.cbegin(), notRecorded.cend(), ", ")
                   << ", so they are written as zero or blank. Muon analysis does not use them.\n";
  }
}

void SaveNexusMuon::writeInstrument(Nexus::File &file, const MatrixWorkspace &ws) const {
  const auto &run = ws.run();
  const auto numSpectra = ws.getNumberHistograms();
  const auto numBins = ws.blocksize();
  const double timeZero = getProperty(Prop::TIME_ZERO);
  const double timeShift = timeZeroIf(Prop::TIME_ZERO_CORRECTED);
  const double lastTime = ws.x(0).back() + timeShift;
  const auto resolution = resolutionPicoseconds(firstBinWidth(ws));
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
  file.putAttr(NeXusEntry::FIRSTGOODBIN, goodDataBin(absoluteFirstGoodData(ws), timeZero, lastTime, resolution));
  file.putAttr(NeXusEntry::LASTGOODBIN, goodDataBin(absoluteLastGoodData(ws), timeZero, lastTime, resolution));
  links.emplace_back(file.getDataID());
  file.closeData();

  const auto &x = ws.x(0);
  std::vector<float> rawTime(x.size());
  std::transform(x.cbegin(), x.cend(), rawTime.begin(),
                 [timeShift](const double time) { return static_cast<float>(time + timeShift); });
  links.emplace_back(writeWithAttributes(file, "raw_time", rawTime, {{"units", "microseconds"}}));
  links.emplace_back(writeWithAttributes(file, "spectrum_index", ids));
  links.emplace_back(writeWithAttributes(file, "period_index", std::vector<int32_t>{1}));

  writeScalar(file, NeXusEntry::RESOLUTION, resolution, "picoseconds");
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
  writeWithAttributes(file, NeXusEntry::DEADTIME, deadTimeValues, {{"units", "microseconds"}});
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
  double totalCounts = 0.0;
  for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
    const auto &y = ws.y(i);
    totalCounts += std::accumulate(y.cbegin(), y.cend(), 0.0);
  }

  file.makeGroup(NeXusEntry::PERIOD, "IXperiods", true);
  writeScalar<int32_t>(file, "number", 1);
  file.writeData("labels", periodLogValue(run, "period_labels", g_log).value_or("Period 1"));
  writeScalar(file, NeXusEntry::SEQUENCES, periodLogNumber(run, "period_sequences", 1, g_log));
  writeScalar(file, NeXusEntry::TYPE, periodLogNumber(run, "period_type", 1, g_log));
  writeScalar(file, NeXusEntry::REQUESTED, periodLogNumber(run, "frames_period_requested", 0, g_log));
  writeScalar(file, NeXusEntry::RAWFRAMES, periodLogNumber(run, "frames_period_raw", 0, g_log));
  writeScalar(file, NeXusEntry::PERIODOUTPUT, periodLogNumber(run, "period_output", 0, g_log));
  // Total counts are in millions of events. They are taken from the counts rather than the total_counts_period log,
  // so that they match the counts saved after any changes to them.
  writeScalar(file, NeXusEntry::PERIODCOUNTS, static_cast<float>(totalCounts / 1e6), "Mev");
  writeScalar(file, NeXusEntry::GOODFRAMES, static_cast<int32_t>(run.getPropertyAsIntegerValue("goodfrm")));
  if (const auto *charges = findLog<ArrayProperty<double>>(run, "proton_charge_by_period")) {
    const auto &values = (*charges)();
    if (const auto period = currentPeriodIndex(run); period < values.size()) {
      writeScalar(file, "proton_charge", static_cast<float>(values[period]), "uAh");
    } else {
      g_log.warning() << "The proton_charge_by_period log has no value for period " << period + 1
                      << ", so the period proton charge is not saved.\n";
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
    file.writeData(field, logValue(run, "measurement_" + field));
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
    if (!timeSeries || dynamic_cast<const TimeSeriesProperty<bool> *>(log) || timeSeries->realSize() == 0) {
      continue;
    }
    if (!doubleLog && !intLog && !stringLog) {
      g_log.warning() << "The log " << logName << " is not saved, as only time series of doubles, 32-bit integers "
                      << "and text can be saved.\n";
      continue;
    }
    if (logName.contains('/')) {
      g_log.warning() << "The log " << logName << " is not saved, as its name contains '/'.\n";
      continue;
    }
    if (doubleLog) {
      writeLog(file, run, *doubleLog, start);
    } else if (intLog) {
      writeLog(file, run, *intLog, start);
    } else {
      writeLog(file, run, *stringLog, start);
    }
  }
  file.closeGroup();
}

} // namespace Mantid::DataHandling
