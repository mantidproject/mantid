// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MuonAnalysisHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>

#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <stdexcept>
#include <utility>

namespace {
/// Colors for workspace (Black, Red, Green, Blue, Orange, Purple, if there are
/// more than this then use black as default.)
QColor getWorkspaceColor(size_t index) {
  switch (index) {
  case (1):
    return QColor("red");
  case (2):
    return QColor("green");
  case (3):
    return QColor("blue");
  case (4):
    return QColor("orange");
  case (5):
    return QColor("purple");
  default:
    return QColor("black");
  }
}

/// Get keys from parameter table
std::vector<std::string> getKeysFromTable(const Mantid::API::ITableWorkspace_sptr &tab) {
  std::vector<std::string> keys;
  if (tab) {
    Mantid::API::TableRow row = tab->getFirstRow();
    do {
      std::string key;
      row >> key;
      keys.emplace_back(key);
    } while (row.next());
  }
  return keys;
}
} // namespace

namespace MantidQt::CustomInterfaces::MuonAnalysisHelper {

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

/**
 * Sets double validator for specified field.
 * @param field :: Field to set validator for
 * @param allowEmpty :: Whether the validator should accept empty inputs as well
 */
void setDoubleValidator(QLineEdit *field, bool allowEmpty) {
  QDoubleValidator *newValidator;

  if (allowEmpty) {
    newValidator = new DoubleOrEmptyValidator(field);
  } else {
    newValidator = new QDoubleValidator(field);
  }

  newValidator->setNotation(QDoubleValidator::StandardNotation);
  field->setValidator(newValidator);
}

/**
 * Return a first period MatrixWorkspace in a run workspace. If the run
 * workspace has one period
 * only - it is returned.
 * @param ws :: Run workspace
 */
MatrixWorkspace_sptr firstPeriod(const Workspace_sptr &ws) {
  if (auto group = std::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
    return std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
  } else {
    return std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  }
}

/**
 * Returns a number of periods in a run workspace
 * @param ws :: Run wokspace
 * @return Number of periods
 */
size_t numPeriods(const Workspace_sptr &ws) {
  if (auto group = std::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
    return group->size();
  } else {
    return 1;
  }
}

/**
 * Print various informaion about the run
 * @param runWs :: Run workspace to retrieve information from
 * @param out :: Stream to print to
 */
void printRunInfo(const MatrixWorkspace_sptr &runWs, std::ostringstream &out) {
  // Remember current out stream format
  std::ios_base::fmtflags outFlags(out.flags());
  std::streamsize outPrecision(out.precision());

  BOOST_SCOPE_EXIT((&out)(&outFlags)(&outPrecision)) {
    // Restore the flags when exiting the function
    out.precision(outPrecision);
    out.flags(outFlags);
  }
  BOOST_SCOPE_EXIT_END

  // Set display style for floating point values
  out << std::fixed << std::setprecision(12);

  out << "\nTitle: " << runWs->getTitle();
  out << "\nComment: " << runWs->getComment();

  const Run &run = runWs->run();

  Mantid::Types::Core::DateAndTime start, end;

  // Add the start time for the run
  out << "\nStart: ";
  if (run.hasProperty("run_start")) {
    start = run.getProperty("run_start")->value();
    out << start.toSimpleString();
  }

  // Add the end time for the run
  out << "\nEnd: ";
  if (run.hasProperty("run_end")) {
    end = run.getProperty("run_end")->value();
    out << end.toSimpleString();
  }
  // Add counts to run information
  out << "\nCounts: ";
  double counts(0.0);
  for (size_t i = 0; i < runWs->getNumberHistograms(); ++i) {
    const auto &y = runWs->y(i);
    counts = std::accumulate(y.begin(), y.end(), counts);
  }
  // output this number to three decimal places
  out << std::setprecision(3);
  out << counts / 1000000 << " MEv";
  // Add the end time for the run
  out << "\nGood frames: ";
  if (run.hasProperty("goodfrm")) {
    const auto goodFrames = run.getProperty("goodfrm")->value();
    out << goodFrames;
    // Add counts divided by good frames to run information
    out << "\nCounts/Good frames: ";
    out << std::setprecision(3);
    const auto countsPerFrame = counts / std::stod(goodFrames);
    out << countsPerFrame << " Events per frame";
    // Add counts per detector per good frame
    out << "\nCounts/(Good frames*number detectors): ";
    out << std::setprecision(3);
    out << countsPerFrame / static_cast<double>(runWs->getNumberHistograms()) << " Events per frame per detector";
  }
  // Add average temperature.
  out << "\nAverage Temperature: ";
  if (run.hasProperty("Temp_Sample")) {
    // Filter the temperatures by the start and end times for the run.
    Mantid::Kernel::TimeROI timeroi(start, end);
    auto tempSample = run.getProperty("Temp_Sample");

    if (const auto *tempSampleTimeSeries = dynamic_cast<const ITimeSeriesProperty *>(tempSample)) {
      out << tempSampleTimeSeries->timeAverageValue(&timeroi);
    } else {
      out << "Not set";
    }
  } else {
    out << "Not found";
  }

  // Add sample temperature
  // Could be stored as a double or as a string (range e.g. "1000.0 - 1020.0")
  out << "\nSample Temperature: ";
  if (run.hasProperty("sample_temp")) {
    // extract as string, then convert to double to allow precision to apply
    std::string valueAsString = run.getProperty("sample_temp")->value();
    try {
      out << std::stod(valueAsString);
    } catch (const std::invalid_argument &) {
      // problem converting to double, output as string
      out << valueAsString;
    }
  } else {
    out << "Not found";
  }

  // Add sample magnetic field
  // Could be stored as a double or as a string (range e.g. "1000.0 - 1020.0")
  out << "\nSample Magnetic Field: ";
  if (run.hasProperty("sample_magn_field")) {
    // extract as string, then convert to double to allow precision to apply
    std::string valueAsString = run.getProperty("sample_magn_field")->value();
    try {
      out << std::stod(valueAsString);
    } catch (const std::invalid_argument &) {
      // problem converting to double, output as string
      out << valueAsString;
    }
  } else {
    out << "Not found";
  }
}

/**
 * Constructor
 * @param groupName :: The top-level group to use for all the widgets
 */
WidgetAutoSaver::WidgetAutoSaver(const QString &groupName) { m_settings.beginGroup(groupName); }

/**
 * Register new widget for auto-saving.
 * @param widget :: A pointer to the widget
 * @param name :: A name to use when saving/loading
 * @param defaultValue :: A value to load when the widget has not been saved yet
 */
void WidgetAutoSaver::registerWidget(QWidget *widget, const QString &name, QVariant defaultValue) {
  m_registeredWidgets.push_back(widget);
  m_widgetNames[widget] = name;
  m_widgetDefaultValues[widget] = std::move(defaultValue);
  m_widgetGroups[widget] = m_settings.group(); // Current group set up using beginGroup and endGroup
}

/**
 * Return a signal (which can be used instead of SIGNAL()) which is emitted when
 * given widget is
 * changed.
 * @param widget
 * @return A signal you can use instead of SIGNAL() to determine when widget
 * value was changed
 */
const char *WidgetAutoSaver::changedSignal(QWidget *widget) {
  if (qobject_cast<QLineEdit *>(widget)) {
    return SIGNAL(textChanged(QString));
  } else if (qobject_cast<QCheckBox *>(widget)) {
    return SIGNAL(stateChanged(int));
  } else if (qobject_cast<QComboBox *>(widget)) {
    return SIGNAL(currentIndexChanged(int));
  } else if (qobject_cast<QSpinBox *>(widget)) {
    return SIGNAL(valueChanged(int));
  }
  // ... add more as necessary
  else {
    throw std::runtime_error("Unsupported widget type");
  }
}

/**
 * Enable/disable auto-saving of all the registered widgets.
 * @param enabled :: Whether auto-saving should be enabled or disabled
 */
void WidgetAutoSaver::setAutoSaveEnabled(bool enabled) {
  foreach (QWidget *w, m_registeredWidgets) {
    setAutoSaveEnabled(w, enabled);
  }
}

/**
 * Enable/disable auto-saving of all the registered widgets.
 * @param widget :: Registered widget for which to enable/disable auto-saving
 * @param enabled :: Whether auto-saving should be enabled or disabled
 */
void WidgetAutoSaver::setAutoSaveEnabled(QWidget *widget, bool enabled) {
  if (enabled)
    connect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
  else
    disconnect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
}

/**
 * Saves the value of the registered widget which signalled the slot
 */
void WidgetAutoSaver::saveWidgetValue() {
  // Get the widget which called the slot
  QWidget *sender = qobject_cast<QWidget *>(QObject::sender());

  if (!sender)
    throw std::runtime_error("Unable to save value of non-widget QObject");

  const QString &senderName = m_widgetNames[sender];
  const QString &senderGroup = m_widgetGroups[sender];

  QSettings settings;
  settings.beginGroup(senderGroup);

  if (auto w = qobject_cast<QLineEdit *>(sender)) {
    settings.setValue(senderName, w->text());
  } else if (auto w = qobject_cast<QCheckBox *>(sender)) {
    settings.setValue(senderName, w->isChecked());
  } else if (auto w = qobject_cast<QComboBox *>(sender)) {
    settings.setValue(senderName, w->currentIndex());
  } else if (auto w = qobject_cast<QSpinBox *>(sender)) {
    settings.setValue(senderName, w->value());
  }
  // ... add more as neccessary
}

/**
 * Load the auto-saved (or default) value of the given widget.
 * @param widget :: Widget to load saved value for
 */
void WidgetAutoSaver::loadWidgetValue(QWidget *widget) {
  const QString &name = m_widgetNames[widget];
  const QString &group = m_widgetGroups[widget];
  QVariant defaultValue = m_widgetDefaultValues[widget];

  QSettings settings;
  settings.beginGroup(group);

  QVariant value = settings.value(name, defaultValue);

  if (auto w = qobject_cast<QLineEdit *>(widget)) {
    w->setText(value.toString());
  } else if (auto w = qobject_cast<QCheckBox *>(widget)) {
    w->setChecked(value.toBool());
  } else if (auto w = qobject_cast<QComboBox *>(widget)) {
    w->setCurrentIndex(value.toInt());
  } else if (auto w = qobject_cast<QSpinBox *>(widget)) {
    w->setValue(value.toInt());
  }
  // ... add more as neccessary
}

/**
 * Load the auto-saved (or default) value of all the registered widgets.
 */
void WidgetAutoSaver::loadWidgetValues() {
  foreach (QWidget *w, m_registeredWidgets) {
    loadWidgetValue(w);
  }
}

/**
 * Begin new-auto save group. All the registerWidget calls between this and next
 * beginGroup will be
 * put in the given group.
 * @param name :: The name of the group
 */
void WidgetAutoSaver::beginGroup(const QString &name) { m_settings.beginGroup(name); }

/**
 * Ends the scope of the previous begin group.
 */
void WidgetAutoSaver::endGroup() { m_settings.endGroup(); }
/**
 * Checks if a QString is a numeric value
 * @param qstring:: QString to test
 * @returns :: bool if it is a number
 */
bool isNumber(const QString &qstring) {
  bool isNumber = false;
  auto value = qstring.toDouble(&isNumber);
  UNUSED_ARG(value);
  return isNumber;
}

/**
 * Get a run label for the workspace.
 * E.g. for MUSR data of run 15189 it will look like MUSR00015189.
 * @param ws :: Workspace to get label for.
 * @return :: run label
 */
std::string getRunLabel(const Workspace_sptr &ws) {
  const std::vector<Workspace_sptr> wsList{ws};
  return getRunLabel(wsList);
}

/**
 * Get a run label for a list of workspaces.
 * E.g. for MUSR data of runs 15189, 15190, 15191 it will look like
 * MUSR00015189-91.
 * (Assumes all runs have the same instrument)
 * @param wsList :: [input] Vector of workspace pointers
 * @return :: run label
 * @throws std::invalid_argument if empty list given
 */
std::string getRunLabel(const std::vector<Workspace_sptr> &wsList) {
  if (wsList.empty())
    throw std::invalid_argument("Unable to run on an empty list");

  const std::string instrument = firstPeriod(wsList.front())->getInstrument()->getName();

  // Extract the run numbers
  std::vector<int> runNumbers;
  int numWorkspaces = static_cast<int>(wsList.size());
  for (int i = 0; i < numWorkspaces; i++) {
    int runNumber = firstPeriod(wsList[i])->getRunNumber();
    runNumbers.emplace_back(runNumber);
  }

  return getRunLabel(instrument, runNumbers);
}

/**
 * Get a run label for a given instrument and list of runs.
 * E.g. for MUSR data of runs 15189, 15190, 15191 it will look like
 * MUSR00015189-91.
 * (Assumes all runs have the same instrument)
 * @param instrument :: [input] instrument name
 * @param runNumbers :: [input] List of run numbers
 * @return :: run label
 * @throws std::invalid_argument if empty run list given
 */
std::string getRunLabel(const std::string &instrument, const std::vector<int> &runNumbers) {
  if (runNumbers.empty()) {
    throw std::invalid_argument("Cannot run on an empty list");
  }

  // Find ranges of consecutive runs
  auto ranges = findConsecutiveRuns(runNumbers);

  // Zero-padding for the first run
  int zeroPadding;
  try {
    zeroPadding = ConfigService::Instance().getInstrument(instrument).zeroPadding(ranges.begin()->first);
  } catch (const Mantid::Kernel::Exception::NotFoundError &) {
    // Old muon instrument without an IDF - default to 3 zeros
    zeroPadding = 3;
  }

  // Begin string output with full label of first run
  std::ostringstream label;
  label << instrument;
  label << std::setw(zeroPadding) << std::setfill('0') << std::right;

  for (auto range : ranges) {
    std::string firstRun = std::to_string(range.first);
    std::string lastRun = std::to_string(range.second);
    label << firstRun;
    if (range.second != range.first) {
      // Remove the common part of the first and last run, so we get e.g.
      // "12345-56" instead of "12345-12356"
      for (size_t i = 0; i < firstRun.size() && i < lastRun.size(); ++i) {
        if (firstRun[i] != lastRun[i]) {
          lastRun.erase(0, i);
          break;
        }
      }
      label << "-" << lastRun;
    }
    if (range != ranges.back()) {
      label << ", ";
    }
  }

  return label.str();
}

/**
 * Given a vector of run numbers, returns the consecutive ranges of runs.
 * e.g. 1,2,3,5,6,8 -> (1,3), (5,6), (8,8)
 * @param runs :: [input] Vector of run numbers - need not be sorted
 * @returns Vector of pairs of (start, end) of consecutive runs
 */
std::vector<std::pair<int, int>> findConsecutiveRuns(const std::vector<int> &runs) {
  // Groups to output
  std::vector<std::pair<int, int>> ranges;
  // Sort the vector to begin with
  std::vector<int> runNumbers(runs); // local copy
  std::sort(runNumbers.begin(), runNumbers.end());

  // Iterate through vector looking for consecutive groups
  auto a = runNumbers.begin();
  auto start = a;
  auto b = a + 1;
  while (b != runNumbers.end()) {
    if (*b - 1 == *a) { // Still consecutive
      a++;
      b++;
    } else { // Reached end of consecutive group
      ranges.emplace_back(*start, *a);
      start = b++;
      a = start;
    }
  }
  // Reached end of last consecutive group
  ranges.emplace_back(*start, *(runNumbers.end() - 1));
  return ranges;
}

/**
 * Sums a given list of workspaces
 * @param workspaces :: List of workspaces
 * @return Result workspace
 */
Workspace_sptr sumWorkspaces(const std::vector<Workspace_sptr> &workspaces) {
  if (workspaces.size() < 1)
    throw std::invalid_argument("Couldn't sum an empty list of workspaces");

  ScopedWorkspace firstEntry(workspaces.front());
  ScopedWorkspace accumulatorEntry;

  // Comparison function for dates
  auto dateCompare = [](const std::string &first, const std::string &second) {
    return DateAndTime(first) < DateAndTime(second);
  };

  // Comparison function for doubles
  auto numericalCompare = [](const std::string &first, const std::string &second) {
    try {
      return boost::lexical_cast<double>(first) < boost::lexical_cast<double>(second);
    } catch (boost::bad_lexical_cast & /*e*/) {
      return false;
    }
  };

  // Range of log values
  auto runNumRange = findLogRange(workspaces, "run_number", numericalCompare);
  auto startRange = findLogRange(workspaces, "run_start", dateCompare);
  auto endRange = findLogRange(workspaces, "run_end", dateCompare);
  auto tempRange = findLogRange(workspaces, "sample_temp", numericalCompare);
  auto fieldRange = findLogRange(workspaces, "sample_magn_field", numericalCompare);

  // Create accumulator workspace by cloning the first one from the list
  IAlgorithm_sptr cloneAlg = AlgorithmManager::Instance().create("CloneWorkspace");
  cloneAlg->setLogging(false);
  cloneAlg->setRethrows(true);
  cloneAlg->setPropertyValue("InputWorkspace", firstEntry.name());
  cloneAlg->setPropertyValue("OutputWorkspace", accumulatorEntry.name());
  cloneAlg->execute();

  for (auto it = (workspaces.begin() + 1); it != workspaces.end(); ++it) {
    // Add this workspace on to the sum
    ScopedWorkspace wsEntry(*it);

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Plus");
    alg->setLogging(false);
    alg->setRethrows(true);
    alg->setPropertyValue("LHSWorkspace", accumulatorEntry.name());
    alg->setPropertyValue("RHSWorkspace", wsEntry.name());
    alg->setPropertyValue("OutputWorkspace", accumulatorEntry.name());
    alg->execute();

    appendTimeSeriesLogs(wsEntry.retrieve(), accumulatorEntry.retrieve(), "Temp_Sample");
  }

  // Replace the start and end times with the earliest start and latest end
  replaceLogValue(accumulatorEntry.name(), "run_start", startRange.first);
  replaceLogValue(accumulatorEntry.name(), "run_end", endRange.second);

  // Put in range of temperatures and magnetic fields
  auto rangeString = [](const std::pair<std::string, std::string> &range) {
    std::ostringstream oss;
    oss << range.first;
    if (range.second != range.first) {
      oss << " to " << range.second;
    }
    return oss.str();
  };
  replaceLogValue(accumulatorEntry.name(), "sample_temp", rangeString(tempRange));
  replaceLogValue(accumulatorEntry.name(), "sample_magn_field", rangeString(fieldRange));
  // Construct range of run numbers differently
  replaceLogValue(accumulatorEntry.name(), "run_number", [](std::pair<std::string, std::string> range) {
    for (size_t i = 0; i < range.first.size() && i < range.second.size(); ++i) {
      if (range.first[i] != range.second[i]) {
        range.second.erase(0, i);
        break;
      }
    }
    return range.first + "-" + range.second;
  }(runNumRange));

  return accumulatorEntry.retrieve();
}

/*
 * Validates and returns a double value. If it is not invalid, the widget is set
 * to default value,
 * appropriate warning is printed and default value is returned.
 * @param field :: Field to get value from
 * @param defaultValue :: Default value to return/set if field value is invalid
 * @param valueDescr :: Description of the value
 * @param log :: Log to print warning to in case value is invalid
 * @return Value if field is valid, default value otherwise. If default value is
 * empty, EMPTY_DBL() is returned
 */
double getValidatedDouble(QLineEdit *field, const QString &defaultValue, const QString &valueDescr, Logger &log) {
  bool ok;
  double value = field->text().toDouble(&ok);

  if (!ok) {
    log.warning() << "The value of " << valueDescr.toStdString() << " is invalid. ";
    log.warning() << "Reset to default.\n";
    field->setText(defaultValue);

    if (defaultValue.isEmpty()) {
      return Mantid::EMPTY_DBL();
    } else {
      return defaultValue.toDouble();
    }
  }

  return value;
}

/**
 * Makes sure the specified workspaces are in specified group. If group exists
 * already - missing
 * workspaces are added to it, otherwise new group is created. If ws exists in
 * ADS under groupName,
 * and it is not a group - it's overwritten.
 * @param groupName :: Name of the group workspaces should be in
 * @param inputWorkspaces :: Names of the workspaces to group
 */
void groupWorkspaces(const std::string &groupName, const std::vector<std::string> &inputWorkspaces) {
  auto &ads = AnalysisDataService::Instance();

  WorkspaceGroup_sptr group;
  if (ads.doesExist(groupName)) {
    group = ads.retrieveWS<WorkspaceGroup>(groupName);
  }

  if (group) {
    // Exists and is a group -> add missing workspaces to it
    for (const auto &inputWorkspace : inputWorkspaces) {
      if (!group->contains(inputWorkspace)) {
        group->add(inputWorkspace);
      }
    }
  } else {
    // Doesn't exist or isn't a group -> create/overwrite
    IAlgorithm_sptr groupingAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
    groupingAlg->setProperty("InputWorkspaces", inputWorkspaces);
    groupingAlg->setPropertyValue("OutputWorkspace", groupName);
    groupingAlg->execute();
  }
}
/**
 * Replaces the named log value in the given workspace with the given value
 * @param wsName :: [input] Name of workspace
 * @param logName :: [input] Name of log to replace
 * @param logValue :: [input] Name of value to replace it with
 */
void replaceLogValue(const std::string &wsName, const std::string &logName, const std::string &logValue) {
  IAlgorithm_sptr removeAlg = AlgorithmManager::Instance().create("DeleteLog");
  removeAlg->setLogging(false);
  removeAlg->setRethrows(true);
  removeAlg->setPropertyValue("Workspace", wsName);
  removeAlg->setPropertyValue("Name", logName);
  removeAlg->execute();
  IAlgorithm_sptr addAlg = AlgorithmManager::Instance().create("AddSampleLog");
  addAlg->setLogging(false);
  addAlg->setRethrows(true);
  addAlg->setPropertyValue("Workspace", wsName);
  addAlg->setPropertyValue("LogName", logName);
  addAlg->setPropertyValue("LogText", logValue);
  addAlg->execute();
}

/**
 * Returns the range of values for the given log in the workspace given, which
 * could be a group. If it isn't a group, the vector will have only one entry
 * (or none, if log not present).
 * @param ws :: [input] Workspace (could be group)
 * @param logName :: [input] Name of log
 * @returns All values found for the given log
 */
std::vector<std::string> findLogValues(const Workspace_sptr &ws, const std::string &logName) {
  std::vector<std::string> values;
  MatrixWorkspace_sptr matrixWS;

  // Try casting input to a MatrixWorkspace_sptr directly
  matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  if (matrixWS) {
    if (matrixWS->run().hasProperty(logName)) {
      values.emplace_back(matrixWS->run().getProperty(logName)->value());
    }
  } else {
    // It could be a workspace group
    auto groupWS = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
    if (groupWS && groupWS->getNumberOfEntries() > 0) {
      for (int index = 0; index < groupWS->getNumberOfEntries(); index++) {
        matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(groupWS->getItem(index));
        if (matrixWS) {
          if (matrixWS->run().hasProperty(logName)) {
            values.emplace_back(matrixWS->run().getProperty(logName)->value());
          }
        }
      }
    }
  }
  return values;
}

/**
 * Finds the range of values for the given log in the supplied workspace.
 * @param ws :: [input] Workspace (can be group)
 * @param logName :: [input] Name of log
 * @param isLessThan :: [input] Function to sort values (<)
 * @returns :: Pair of (smallest, largest) values
 */
std::pair<std::string, std::string> findLogRange(const Workspace_sptr &ws, const std::string &logName,
                                                 bool (*isLessThan)(const std::string &first,
                                                                    const std::string &second)) {
  auto values = findLogValues(ws, logName);
  if (!values.empty()) {
    auto minmax = std::minmax_element(values.begin(), values.end(), isLessThan);
    return std::make_pair(*(minmax.first), *(minmax.second));
  } else {
    return std::make_pair("", "");
  }
}

/**
 * Finds the range of values for the given log in the supplied vector of
 * workspaces.
 * @param workspaces :: [input] Vector of workspaces
 * @param logName :: [input] Name of log
 * @param isLessThan :: [input] Function to sort values (<)
 * @returns :: Pair of (smallest, largest) values
 */
std::pair<std::string, std::string>
findLogRange(const std::vector<Workspace_sptr> &workspaces, const std::string &logName,
             bool (*isLessThan)(const std::string &first, const std::string &second)) {
  std::string smallest, largest;
  for (const auto &ws : workspaces) {
    auto range = findLogRange(ws, logName, isLessThan);
    if (smallest.empty() || isLessThan(range.first, smallest)) {
      smallest = range.first;
    }
    if (largest.empty() || isLessThan(largest, range.second)) {
      largest = range.second;
    }
  }
  return std::make_pair(smallest, largest);
}

/**
 * Takes the values in the named time series log of the first workspace
 * and appends them to the same log in the second.
 * Silently fails if either workspace is missing the named log.
 * @param toAppend :: [input] Workspace with log to append to the other
 * @param resultant :: [input, output] Workspace whose log will be appended to
 * @param logName :: [input] Name of time series log
 * @throws std::invalid_argument if the named log is of the incorrect type
 * @throws std::invalid_argument if the workspaces supplied are null or have
 * different number of periods
 */
void appendTimeSeriesLogs(const Workspace_sptr &toAppend, const Workspace_sptr &resultant, const std::string &logName) {
  // check input
  if (!toAppend || !resultant) {
    throw std::invalid_argument("Cannot append logs: workspaces supplied are null");
  }

  // Cast the inputs to MatrixWorkspace (could be a group)
  auto getWorkspaces = [](const Workspace_sptr &ws) {
    std::vector<MatrixWorkspace_sptr> workspaces;
    MatrixWorkspace_sptr matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
    if (matrixWS) {
      workspaces.emplace_back(matrixWS);
    } else { // it's a workspace group
      auto groupWS = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
      if (groupWS && groupWS->getNumberOfEntries() > 0) {
        for (int index = 0; index < groupWS->getNumberOfEntries(); index++) {
          matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(groupWS->getItem(index));
          if (matrixWS) {
            workspaces.emplace_back(matrixWS);
          }
        }
      }
    }
    return workspaces;
  };

  // Extract time series log from workspace
  auto getTSLog = [&logName](const MatrixWorkspace_sptr &ws) {
    const Mantid::API::Run &run = ws->run();
    TimeSeriesProperty<double> *prop = nullptr;
    if (run.hasProperty(logName)) {
      prop = dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData(logName));
      if (!prop) {
        throw std::invalid_argument("Property" + logName + " of wrong type");
      }
    }
    return prop;
  };

  auto firstWorkspaces = getWorkspaces(toAppend);
  auto secondWorkspaces = getWorkspaces(resultant);
  if (firstWorkspaces.size() == secondWorkspaces.size()) {
    for (size_t i = 0; i < firstWorkspaces.size(); i++) {
      TimeSeriesProperty<double> *firstProp = getTSLog(firstWorkspaces[i]);
      TimeSeriesProperty<double> *secondProp = getTSLog(secondWorkspaces[i]);
      if (firstProp && secondProp) {
        secondProp->operator+=(static_cast<const Property *>(firstProp)); // adds the values
        secondProp->eliminateDuplicates();
      }
    }
  } else {
    throw std::invalid_argument("Workspaces have different number of periods");
  }
}

/**
 * Uses the format of the workspace name
 * (INST00012345-8; Pair; long; Asym; [1+2-3+4]; #2)
 * to get a string in the format "run number: period"
 * @param workspaceName :: [input] Name of the workspace
 * @param firstRun :: [input] First run number - use this if tokenizing fails
 * @returns Run number/period string
 */
QString runNumberString(const std::string &workspaceName, const std::string &firstRun) {
  std::string periods = "";        // default
  std::string instRuns = firstRun; // default

  Mantid::Kernel::StringTokenizer tokenizer(workspaceName, ";", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  const size_t numTokens = tokenizer.count();
  if (numTokens > 4) { // format is ok
    instRuns = tokenizer[0];
    // Remove "INST000" off the start
    // No muon instruments have numbers in their names
    size_t numPos = instRuns.find_first_of("123456789");
    if (numPos != std::string::npos) {
      instRuns = instRuns.substr(numPos, instRuns.size());
    } else { // run number was zero?
      instRuns = "0";
    }
    if (numTokens > 5) { // periods included
      periods = tokenizer[4];
    }
  }

  QString ret(instRuns.c_str());
  if (!periods.empty()) {
    ret.append(": ").append(periods.c_str());
  }
  return ret;
}

/**
 * Determines if the grouping already loaded can be reused, or if
 * grouping must be re-loaded.
 * Criteria: reload if
 * - instrument has changed
 * - instrument same, but field direction has changed
 * - number of histograms has changed
 * @param currentWorkspace :: [input] Data already in interface
 * @param loadedWorkspace :: [input] New data just loaded
 * @returns :: True or false to load new grouping
 * @throws std::invalid_argument if loadedWorkspace is null
 */
bool isReloadGroupingNecessary(const std::shared_ptr<Mantid::API::Workspace> &currentWorkspace,
                               const std::shared_ptr<Mantid::API::Workspace> &loadedWorkspace) {
  if (!loadedWorkspace) {
    throw std::invalid_argument("No loaded workspace to get grouping for!");
  }
  if (!currentWorkspace) {
    // No previous data, so we need to load grouping from scratch
    return true;
  }

  bool reloadNecessary = false;
  const auto loadedData = firstPeriod(loadedWorkspace);
  const auto currentData = firstPeriod(currentWorkspace);

  // Check if instrument has changed
  const auto loadedInstrument = loadedData->getInstrument()->getName();
  const auto currentInstrument = currentData->getInstrument()->getName();
  if (loadedInstrument != currentInstrument) {
    reloadNecessary = true;
  }

  // Check if field direction has changed, even if instrument hasn't
  // (e.g. MUSR - same instrument can have different field directions)
  if (!reloadNecessary) {
    Mantid::Kernel::Property *loadedField = nullptr, *currentField = nullptr;
    try {
      loadedField = loadedData->run().getLogData("main_field_direction");
      currentField = currentData->run().getLogData("main_field_direction");
    } catch (std::exception &) {
      // Log not found in one or both workspaces - ignore it
    }
    if (loadedField && currentField && loadedField->value() != currentField->value()) {
      reloadNecessary = true;
    }
  }

  // Check if number of spectra have changed
  if (!reloadNecessary) {
    const auto loadedNumSpectra = loadedData->getNumberHistograms();
    const auto currentNumSpectra = currentData->getNumberHistograms();
    reloadNecessary = (loadedNumSpectra != currentNumSpectra);
  }

  return reloadNecessary;
}

/**
 * Parse a workspace name into dataset parameters
 * Format: "INST00012345; Pair; long; Asym;[ 1;] #1"
 * @param wsName :: [input] Name of workspace
 * @returns :: Struct containing dataset parameters
 */
Muon::DatasetParams parseWorkspaceName(const std::string &wsName) {
  Muon::DatasetParams params;

  Mantid::Kernel::StringTokenizer tokenizer(wsName, ";", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  const size_t numTokens = tokenizer.count();
  if (numTokens < 5) {
    throw std::invalid_argument("Could not parse workspace name: " + wsName);
  }

  params.label = tokenizer[0];
  parseRunLabel(params.label, params.instrument, params.runs);
  const std::string itemType = tokenizer[1];
  params.itemType = (itemType == "Group") ? Muon::Group : Muon::Pair;
  params.itemName = tokenizer[2];
  const std::string plotType = tokenizer[3];
  if (plotType == "Asym") {
    params.plotType = Muon::Asymmetry;
  } else if (plotType == "Counts") {
    params.plotType = Muon::Counts;
  } else {
    params.plotType = Muon::Logarithm;
  }
  std::string versionString;
  if (numTokens > 5) { // periods included
    params.periods = tokenizer[4];
    versionString = tokenizer[5];
  } else {
    versionString = tokenizer[4];
  }
  // Remove the # from the version string
  versionString.erase(std::remove(versionString.begin(), versionString.end(), '#'), versionString.end());

  try {
    params.version = boost::lexical_cast<size_t>(versionString);
  } catch (const boost::bad_lexical_cast &) {
    params.version = 1; // Set to 1 and ignore the error
  }

  return params;
}

/**
 * Parse a run label e.g. "MUSR00015189-91, 15193" into instrument
 * ("MUSR") and set of runs (15189, 15190, 15191, 15193).
 * Assumes instrument name doesn't contain a digit (true for muon instruments).
 * @param label :: [input] Label to parse
 * @param instrument :: [output] Name of instrument
 * @param runNumbers :: [output] Vector to fill with run numbers
 * @throws std::invalid_argument if input cannot be parsed
 */
void parseRunLabel(const std::string &label, std::string &instrument, std::vector<int> &runNumbers) {
  const size_t instPos = label.find_first_of("0123456789");
  instrument = label.substr(0, instPos);
  const size_t numPos = label.find_first_not_of('0', instPos);
  runNumbers.clear();
  if (numPos != std::string::npos) {
    std::string runString = label.substr(numPos, label.size());
    // sets of continuous ranges
    Mantid::Kernel::StringTokenizer rangeTokenizer(runString, ",", Mantid::Kernel::StringTokenizer::TOK_TRIM);
    for (const auto &range : rangeTokenizer.asVector()) {
      Mantid::Kernel::StringTokenizer pairTokenizer(range, "-", Mantid::Kernel::StringTokenizer::TOK_TRIM);
      try {
        if (pairTokenizer.count() == 2) {
          // Range of run numbers
          // Deal with common part of string: "151" in "15189-91"
          const size_t diff = pairTokenizer[0].length() - pairTokenizer[1].length();
          const std::string endRun = pairTokenizer[0].substr(0, diff) + pairTokenizer[1];
          const int start = boost::lexical_cast<int>(pairTokenizer[0]);
          const int end = boost::lexical_cast<int>(endRun);
          for (int run = start; run < end + 1; run++) {
            runNumbers.emplace_back(run);
          }
        } else if (pairTokenizer.count() == 1) {
          // Single run
          runNumbers.emplace_back(boost::lexical_cast<int>(pairTokenizer[0]));
        } else {
          throw std::invalid_argument("Failed to parse run label: " + label + " too many tokens ");
        }
      } catch (const boost::bad_lexical_cast &) {
        throw std::invalid_argument("Failed to parse run label: " + label + " not a good run number");
      } catch (...) {
        throw std::invalid_argument("Failed to parse run label: " + label);
      }
    }
  } else {
    // The string was "INST000" or similar...
    runNumbers.emplace_back(0);
  }
}

/**
 * Generate a workspace name from the given parameters
 * Format: "INST00012345; Pair; long; Asym;[ 1;] #1"
 * @param params :: [input] Struct containing dataset parameters
 * @returns :: Name for analysis workspace
 */
std::string generateWorkspaceName(const Muon::DatasetParams &params) {
  std::ostringstream workspaceName;
  const static std::string sep("; ");

  // Instrument and run number
  if (params.label.empty()) {
    workspaceName << getRunLabel(params.instrument, params.runs) << sep;
  } else {
    workspaceName << params.label << sep;
  }

  // Pair/group and name of pair/group
  if (params.itemType == Muon::ItemType::Pair) {
    workspaceName << "Pair" << sep;
  } else if (params.itemType == Muon::ItemType::Group) {
    workspaceName << "Group" << sep;
  }
  workspaceName << params.itemName << sep;

  // Type of plot
  switch (params.plotType) {
  case Muon::PlotType::Asymmetry:
    workspaceName << "Asym";
    break;
  case Muon::PlotType::Counts:
    workspaceName << "Counts";
    break;
  case Muon::PlotType::Logarithm:
    workspaceName << "Logs";
    break;
  }

  // Period(s)
  const auto periods = params.periods;
  if (!periods.empty()) {
    workspaceName << sep << periods;
  }

  // Version - always "#1" if overwrite is on, otherwise increment
  workspaceName << sep << "#" << params.version;

  return workspaceName.str();
}

/**
 * Get the colors corresponding to their position in the workspace list.
 * Used in fittings table on results tab.
 *
 * New color if:
 * - different model used for fit
 * - different number of runs (groups, periods) used in fit
 *
 * Colors: black, red, green, blue, orange, purple (if more, use black as
 * default).
 *
 * @param workspaces :: Vector of either workspace groups (containing parameter
 * tables) or parameter tables themselves
 * @return :: List of colors with the key being position in input vector.
 */
QMap<int, QColor> getWorkspaceColors(const std::vector<Workspace_sptr> &workspaces) {
  QMap<int, QColor> colors; // position, color

  // Vector of <number of runs in fit, parameters in fit> pairs
  using FitProp = std::pair<size_t, std::vector<std::string>>;
  std::vector<FitProp> fitProperties;

  // Get fit properties for each input workspace
  for (const auto &ws : workspaces) {
    size_t nRuns = 0;
    std::vector<std::string> params;
    if (const auto group = std::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
      for (size_t i = 0; i < group->size(); ++i) {
        const auto &wsInGroup = group->getItem(i);
        if (wsInGroup->getName().find("_Parameters") != std::string::npos) {
          params = getKeysFromTable(std::dynamic_pointer_cast<ITableWorkspace>(wsInGroup));
        } else if (wsInGroup->getName().find("_Workspace") != std::string::npos) {
          ++nRuns;
        }
      }
    } else if (const auto table = std::dynamic_pointer_cast<ITableWorkspace>(ws)) {
      nRuns = 1;
      params = getKeysFromTable(table);
    } else {
      throw std::invalid_argument("Unexpected workspace type for " + ws->getName() +
                                  " (expected WorkspaceGroup or ITableWorkspace)");
    }
    fitProperties.emplace_back(nRuns, params);
  }

  size_t colorCount(0);
  colors[0] = getWorkspaceColor(colorCount);

  if (fitProperties.size() > 1) {
    FitProp firstProps = fitProperties.front();

    while (static_cast<size_t>(colors.size()) < fitProperties.size()) {
      // Go through and assign same color to all similar sets
      for (size_t i = 1; i < fitProperties.size(); ++i) {
        if (fitProperties[i] == firstProps) {
          colors[static_cast<int>(i)] = getWorkspaceColor(colorCount);
        }
      }

      // Increment color for next set
      ++colorCount;

      // Get the first unassigned one to compare to next time
      for (size_t i = 1; i < fitProperties.size(); ++i) {
        if (!colors.contains(static_cast<int>(i))) {
          firstProps = fitProperties[i];
          break;
        }
      }
    }
  }

  return colors;
}
} // namespace MantidQt::CustomInterfaces::MuonAnalysisHelper
