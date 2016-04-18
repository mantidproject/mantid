#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"

#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>

#include <boost/scope_exit.hpp>
#include <stdexcept>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MuonAnalysisHelper
{

using namespace Mantid::Kernel;
using namespace Mantid::API;

/**
 * Sets double validator for specified field.
 * @param field :: Field to set validator for
 * @param allowEmpty :: Whether the validator should accept empty inputs as well
 */
void setDoubleValidator(QLineEdit* field, bool allowEmpty)
{
  QDoubleValidator* newValidator;

  if (allowEmpty)
  {
    newValidator = new DoubleOrEmptyValidator(field);
  }
  else
  {
    newValidator = new QDoubleValidator(field);
  }

  newValidator->setNotation(QDoubleValidator::StandardNotation);
  field->setValidator(newValidator);
}


/**
 * Return a first period MatrixWorkspace in a run workspace. If the run workspace has one period
 * only - it is returned.
 * @param ws :: Run workspace
 */
MatrixWorkspace_sptr firstPeriod(Workspace_sptr ws)
{
  if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws) )
  {
    return boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(0) );
  }
  else
  {
    return boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  }
}

/**
 * Returns a number of periods in a run workspace
 * @param ws :: Run wokspace
 * @return Number of periods
 */
size_t numPeriods(Workspace_sptr ws)
{
  if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws) )
  {
    return group->size();
  }
  else
  {
    return 1;
  }
}

/**
 * Print various informaion about the run
 * @param runWs :: Run workspace to retrieve information from
 * @param out :: Stream to print to
 */
void printRunInfo(MatrixWorkspace_sptr runWs, std::ostringstream& out)
{
  // Remember current out stream format
  std::ios_base::fmtflags outFlags(out.flags());
  std::streamsize outPrecision(out.precision());

  BOOST_SCOPE_EXIT((&out)(&outFlags)(&outPrecision))
  {
    // Restore the flags when exiting the function
    out.precision(outPrecision);
    out.flags(outFlags);
  }
  BOOST_SCOPE_EXIT_END

  // Set display style for floating point values
  out << std::fixed << std::setprecision(12);

  out << "\nTitle: " << runWs->getTitle();
  out << "\nComment: " << runWs->getComment();

  const Run& run = runWs->run();

  Mantid::Kernel::DateAndTime start, end;

  // Add the start time for the run
  out << "\nStart: ";
  if ( run.hasProperty("run_start") )
  {
    start = run.getProperty("run_start")->value();
    out << start.toSimpleString();
  }

  // Add the end time for the run
  out << "\nEnd: ";
  if ( run.hasProperty("run_end") )
  {
    end = run.getProperty("run_end")->value();
    out << end.toSimpleString();
  }

  // Add the end time for the run
  out << "\nGood frames: ";
  if ( run.hasProperty("goodfrm") )
  {
    out << run.getProperty("goodfrm")->value();
  }

  // Add counts to run information
  out << "\nCounts: ";
  double counts(0.0);
  for (size_t i=0; i<runWs->getNumberHistograms(); ++i)
  {
    for (size_t j=0; j<runWs->blocksize(); ++j)
    {
      counts += runWs->dataY(i)[j];
    }
  }
  // output this number to three decimal places
  out << std::setprecision(3);
  out << counts/1000000 << " MEv";
  out << std::setprecision(12);
  // Add average temperature.
  out << "\nAverage Temperature: ";
  if ( run.hasProperty("Temp_Sample") )
  {
    // Filter the temperatures by the start and end times for the run.
    run.getProperty("Temp_Sample")->filterByTime(start, end);

    // Get average of the values
    double average = run.getPropertyAsSingleValue("Temp_Sample");

    if (average != 0.0)
    {
      out << average;
    }
    else
    {
      out << "Not set";
    }
  }
  else
  {
    out << "Not found";
  }

  // Add sample temperature
  // Could be stored as a double or as a string (range e.g. "1000.0 - 1020.0")
  out << "\nSample Temperature: ";
  if (run.hasProperty("sample_temp")) {
    out << run.getProperty("sample_temp")->value();
  } else {
    out << "Not found";
  }

  // Add sample magnetic field
  // Could be stored as a double or as a string (range e.g. "1000.0 - 1020.0")
  out << "\nSample Magnetic Field: ";
  if (run.hasProperty("sample_magn_field")) {
    out << run.getProperty("sample_magn_field")->value();
  } else {
    out << "Not found";
  }
}

/**
 * Constructor
 * @param groupName :: The top-level group to use for all the widgets
 */
WidgetAutoSaver::WidgetAutoSaver(const QString& groupName)
{
  m_settings.beginGroup(groupName);
}

/**
 * Register new widget for auto-saving.
 * @param widget :: A pointer to the widget
 * @param name :: A name to use when saving/loading
 * @param defaultValue :: A value to load when the widget has not been saved yet
 */
void WidgetAutoSaver::registerWidget(QWidget *widget, const QString& name, QVariant defaultValue)
{
  m_registeredWidgets.push_back(widget);
  m_widgetNames[widget] = name;
  m_widgetDefaultValues[widget] = defaultValue;
  m_widgetGroups[widget] = m_settings.group(); // Current group set up using beginGroup and endGroup
}

/**
 * Return a signal (which can be used instead of SIGNAL()) which is emmited when given widget is
 * changed.
 * @param widget
 * @return A signal you can use instead of SIGNAL() to determine when widget value was changed
 */
const char* WidgetAutoSaver::changedSignal(QWidget *widget)
{
  if ( qobject_cast<QLineEdit*>(widget) )
  {
    return SIGNAL(textChanged(QString));
  }
  else if ( qobject_cast<QCheckBox*>(widget) )
  {
    return SIGNAL(stateChanged(int));
  }
  else if ( qobject_cast<QComboBox*>(widget) )
  {
    return SIGNAL(currentIndexChanged(int));
  }
  // ... add more as neccessary
  else
  {
    throw std::runtime_error("Unsupported widget type");
  }
}

/**
 * Enable/disable auto-saving of all the registered widgets.
 * @param enabled :: Whether auto-saving should be enabled or disabled
 */
void WidgetAutoSaver::setAutoSaveEnabled(bool enabled)
{
  foreach (QWidget* w, m_registeredWidgets)
  {
    setAutoSaveEnabled(w, enabled);
  }
}

/**
 * Enable/disable auto-saving of all the registered widgets.
 * @param widget :: Registered widget for which to enable/disable auto-saving
 * @param enabled :: Whether auto-saving should be enabled or disabled
 */
void WidgetAutoSaver::setAutoSaveEnabled(QWidget* widget, bool enabled)
{
  if (enabled)
    connect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
  else
    disconnect(widget, changedSignal(widget), this, SLOT(saveWidgetValue()));
}

/**
 * Saves the value of the registered widget which signalled the slot
 */
void WidgetAutoSaver::saveWidgetValue()
{
  // Get the widget which called the slot
  QWidget* sender = qobject_cast<QWidget*>(QObject::sender());

  if(!sender)
    throw std::runtime_error("Unable to save value of non-widget QObject");

  const QString& senderName = m_widgetNames[sender];
  const QString& senderGroup = m_widgetGroups[sender];

  QSettings settings;
  settings.beginGroup(senderGroup);

  if ( auto w = qobject_cast<QLineEdit*>(sender) )
  {
    settings.setValue(senderName, w->text());
  }
  else if ( auto w = qobject_cast<QCheckBox*>(sender) )
  {
    settings.setValue(senderName, w->isChecked());
  }
  else if ( auto w = qobject_cast<QComboBox*>(sender) )
  {
    settings.setValue(senderName, w->currentIndex());
  }
  // ... add more as neccessary
}

/**
 * Load the auto-saved (or default) value of the given widget.
 * @param widget :: Widget to load saved value for
 */
void WidgetAutoSaver::loadWidgetValue(QWidget *widget)
{
  const QString& name = m_widgetNames[widget];
  const QString& group = m_widgetGroups[widget];
  QVariant defaultValue = m_widgetDefaultValues[widget];

  QSettings settings;
  settings.beginGroup(group);

  QVariant value = settings.value(name, defaultValue);

  if ( auto w = qobject_cast<QLineEdit*>(widget) )
  {
    w->setText(value.toString());
  }
  else if ( auto w = qobject_cast<QCheckBox*>(widget) )
  {
    w->setChecked(value.toBool());
  }
  else if ( auto w = qobject_cast<QComboBox*>(widget) )
  {
    w->setCurrentIndex(value.toInt());
  }
  // ... add more as neccessary
}

/**
 * Load the auto-saved (or default) value of all the registered widgets.
 */
void WidgetAutoSaver::loadWidgetValues()
{
  foreach (QWidget* w, m_registeredWidgets)
  {
    loadWidgetValue(w);
  }
}

/**
 * Begin new-auto save group. All the registerWidget calls between this and next beginGroup will be
 * put in the given group.
 * @param name :: The name of the group
 */
void WidgetAutoSaver::beginGroup(const QString &name)
{
  m_settings.beginGroup(name);
}

/**
 * Ends the scope of the previous begin group.
 */
void WidgetAutoSaver::endGroup()
{
  m_settings.endGroup();
}

/**
 * Get a run label for the workspace.
 * E.g. for MUSR data of run 15189 it will look like MUSR00015189.
 * @param ws :: Workspace to get label for.
 * @return
 */
std::string getRunLabel(const Workspace_sptr& ws)
{
  MatrixWorkspace_const_sptr firstPrd = firstPeriod(ws);

  int runNumber = firstPrd->getRunNumber();
  std::string instrName = firstPrd->getInstrument()->getName();

  int zeroPadding = ConfigService::Instance().getInstrument(instrName).zeroPadding(runNumber);

  std::ostringstream label;
  label << instrName;
  label << std::setw(zeroPadding) << std::setfill('0') << std::right << runNumber;
  return label.str();
}

/**
 * Get a run label for a list of workspaces.
 * E.g. for MUSR data of runs 15189, 15190, 15191 it will look like
 * MUSR00015189-91.
 * @param wsList
 * @return
 */
std::string getRunLabel(std::vector<Workspace_sptr> wsList) {
  if (wsList.empty())
    throw std::invalid_argument("Unable to run on an empty list");

  // Extract the run numbers and find the first run in the list
  std::vector<int> runNumbers;
  int firstRunIndex = 0;
  int firstRunNumber = firstPeriod(wsList.front())->getRunNumber();
  int numWorkspaces = static_cast<int>(wsList.size());
  for (int i = 0; i < numWorkspaces; i++) {
    int runNumber = firstPeriod(wsList[i])->getRunNumber();
    runNumbers.push_back(runNumber);
    if (runNumber < firstRunNumber) {
      firstRunNumber = runNumber;
      firstRunIndex = i;
    }
  }

  // Find ranges of consecutive runs
  auto ranges = findConsecutiveRuns(runNumbers);

  // Begin string output with full label of first run
  std::ostringstream label;
  label << getRunLabel(wsList[firstRunIndex]);

  for (auto range : ranges) {
    std::string firstRun = std::to_string(range.first);
    std::string lastRun = std::to_string(range.second);
    if (range != ranges.front()) {
      label << firstRun;
    }
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
std::vector<std::pair<int, int>>
findConsecutiveRuns(const std::vector<int> &runs) {
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
Workspace_sptr sumWorkspaces(const std::vector<Workspace_sptr>& workspaces)
{
  if (workspaces.size() < 1)
    throw std::invalid_argument("Couldn't sum an empty list of workspaces");

  ScopedWorkspace firstEntry(workspaces.front());
  ScopedWorkspace accumulatorEntry;

  // Comparison function for dates
  auto dateCompare = [](const std::string &first, const std::string &second) {
    return DateAndTime(first) < DateAndTime(second);
  };

  // Comparison function for doubles
  auto numericalCompare = [](const std::string &first,
                             const std::string &second) {
    try {
      return boost::lexical_cast<double>(first) <
             boost::lexical_cast<double>(second);
    } catch (boost::bad_lexical_cast & /*e*/) {
      return false;
    }
  };

  // Range of log values
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

    appendTimeSeriesLogs(wsEntry.retrieve(), accumulatorEntry.retrieve(),
                         "Temp_Sample");
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
  replaceLogValue(accumulatorEntry.name(), "sample_temp",
                  rangeString(tempRange));
  replaceLogValue(accumulatorEntry.name(), "sample_magn_field",
                  rangeString(fieldRange));

  return accumulatorEntry.retrieve();
}

/*
 * Validates and returns a double value. If it is not invalid, the widget is set to default value,
 * appropriate warning is printed and default value is returned.
 * @param field :: Field to get value from
 * @param defaultValue :: Default value to return/set if field value is invalid
 * @param valueDescr :: Description of the value
 * @param log :: Log to print warning to in case value is invalid
 * @return Value if field is valid, default value otherwise. If default value is empty, EMPTY_DBL() is returned
 */
double getValidatedDouble(QLineEdit* field, const QString& defaultValue,
                          const QString& valueDescr, Logger& log)
{
  bool ok;
  double value = field->text().toDouble(&ok);

  if (!ok)
  {
    log.warning() << "The value of " << valueDescr.toStdString() << " is invalid. ";
    log.warning() << "Reset to default.\n";
    field->setText(defaultValue);

    if(defaultValue.isEmpty())
    {
      return Mantid::EMPTY_DBL();
    }
    else
    {
      return defaultValue.toDouble();
    }
  }

  return value;
}

/**
 * Makes sure the specified workspaces are in specified group. If group exists already - missing
 * workspaces are added to it, otherwise new group is created. If ws exists in ADS under groupName,
 * and it is not a group - it's overwritten.
 * @param groupName :: Name of the group workspaces should be in
 * @param inputWorkspaces :: Names of the workspaces to group
 */
void groupWorkspaces(const std::string& groupName, const std::vector<std::string>& inputWorkspaces)
{
  auto& ads = AnalysisDataService::Instance();

  WorkspaceGroup_sptr group;
  if (ads.doesExist(groupName))
  {
    group = ads.retrieveWS<WorkspaceGroup>(groupName);
  }

  if(group)
  {
    // Exists and is a group -> add missing workspaces to it
    for (auto it = inputWorkspaces.begin(); it != inputWorkspaces.end(); ++it)
    {
      if (!group->contains(*it))
      {
        group->add(*it);
      }
    }
  }
  else
  {
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
void replaceLogValue(const std::string &wsName, const std::string &logName,
                     const std::string &logValue) {
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
std::vector<std::string> findLogValues(const Workspace_sptr ws,
                                       const std::string &logName) {
  std::vector<std::string> values;
  MatrixWorkspace_sptr matrixWS;

  // Try casting input to a MatrixWorkspace_sptr directly
  matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  if (matrixWS) {
    if (matrixWS->run().hasProperty(logName)) {
      values.push_back(matrixWS->run().getProperty(logName)->value());
    }
  } else {
    // It could be a workspace group
    auto groupWS = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
    if (groupWS && groupWS->getNumberOfEntries() > 0) {
      for (int index = 0; index < groupWS->getNumberOfEntries(); index++) {
        matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
            groupWS->getItem(index));
        if (matrixWS) {
          if (matrixWS->run().hasProperty(logName)) {
            values.push_back(matrixWS->run().getProperty(logName)->value());
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
std::pair<std::string, std::string> findLogRange(
    const Workspace_sptr ws, const std::string &logName,
    bool (*isLessThan)(const std::string &first, const std::string &second)) {
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
std::pair<std::string, std::string> findLogRange(
    const std::vector<Workspace_sptr> &workspaces, const std::string &logName,
    bool (*isLessThan)(const std::string &first, const std::string &second)) {
  std::string smallest, largest;
  for (auto ws : workspaces) {
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
void appendTimeSeriesLogs(Workspace_sptr toAppend, Workspace_sptr resultant,
                          const std::string &logName) {
  // check input
  if (!toAppend || !resultant) {
    throw std::invalid_argument(
        "Cannot append logs: workspaces supplied are null");
  }

  // Cast the inputs to MatrixWorkspace (could be a group)
  auto getWorkspaces = [](const Workspace_sptr ws) {
    std::vector<MatrixWorkspace_sptr> workspaces;
    MatrixWorkspace_sptr matrixWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
    if (matrixWS) {
      workspaces.push_back(matrixWS);
    } else { // it's a workspace group
      auto groupWS = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
      if (groupWS && groupWS->getNumberOfEntries() > 0) {
        for (int index = 0; index < groupWS->getNumberOfEntries(); index++) {
          matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
              groupWS->getItem(index));
          if (matrixWS) {
            workspaces.push_back(matrixWS);
          }
        }
      }
    }
    return workspaces;
  };

  // Extract time series log from workspace
  auto getTSLog = [&logName](const MatrixWorkspace_sptr ws) {
    const Mantid::API::Run &run = ws->run();
    TimeSeriesProperty<double> *prop = nullptr;
    if (run.hasProperty(logName)) {
      prop =
          dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData(logName));
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
        secondProp->operator+=(
            static_cast<const Property *>(firstProp)); // adds the values
        secondProp->eliminateDuplicates();
      }
    }
  } else {
    throw std::invalid_argument("Workspaces have different number of periods");
  }
}

} // namespace MuonAnalysisHelper
} // namespace CustomInterfaces
} // namespace Mantid
