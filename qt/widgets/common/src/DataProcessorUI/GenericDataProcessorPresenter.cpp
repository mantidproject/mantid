#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorGenerateNotebook.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorWorkspaceCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenterRowReducerWorker.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenterGroupReducerWorker.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenterThread.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ParseKeyValueString.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QtDataProcessorOptionsDialog.h"
#include "MantidQtWidgets/Common/ProgressableView.h"

#include <QHash>
#include <QStringList>
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iterator>
#include <sstream>

#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace {

std::map<QString, QString> convertStringToMap(const QString &options) {
  std::map<QString, QString> optionsMap;
  auto optionsVec = options.split(";");

  for (auto const &option : optionsVec) {
    auto opt = option.split(",");
    auto const key = opt[0];
    opt.removeFirst();
    optionsMap[key] = opt.join(",");
  }

  return optionsMap;
}

QHash<QString, std::set<QString>>
convertStringToMapWithSet(const QString &properties) {
  // The provided string has the form
  // key1: value11, value12; key2: value21;
  // The keys are keys in a map which maps to a set of values
  QHash<QString, std::set<QString>> props;

  if (properties.isEmpty()) {
    return props;
  }

  // Split by each map pair
  auto propVec = properties.split(";");
  for (const auto &prop : propVec) {
    // Split the key and values
    auto elements = prop.split(":");
    auto vals = elements[1].split(",");
    std::set<QString> values(vals.begin(), vals.end());
    props[elements[0]] = values;
  }
  return props;
}

void setAlgorithmProperty(IAlgorithm *const alg, std::string const &name,
                          std::string const &value) {
  alg->setProperty(name, value);
}

void setAlgorithmProperty(IAlgorithm *const alg, QString const &name,
                          std::string const &value) {
  setAlgorithmProperty(alg, name.toStdString(), value);
}

void setAlgorithmProperty(IAlgorithm *const alg, QString const &name,
                          QString const &value) {
  setAlgorithmProperty(alg, name.toStdString(), value.toStdString());
}

bool workspaceExists(QString const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName.toStdString());
}

void removeWorkspace(QString const &workspaceName) {
  AnalysisDataService::Instance().remove(workspaceName.toStdString());
}
}

namespace MantidQt {
namespace MantidWidgets {

/**
* Constructor
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A DataProcessorProcessingAlgorithm
* @param postprocessor : A DataProcessorPostprocessingAlgorithm
* workspaces
* @param postprocessMap : A map containing instructions for post-processing.
* This map links column name to properties of the post-processing algorithm
* @param loader : The algorithm responsible for loading data
*/
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<QString, DataProcessorPreprocessingAlgorithm> &preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::map<QString, QString> &postprocessMap, const QString &loader)
    : WorkspaceObserver(), m_view(nullptr), m_progressView(nullptr),
      m_mainPresenter(), m_loader(loader), m_whitelist(whitelist),
      m_preprocessMap(preprocessMap), m_processor(processor),
      m_postprocessor(postprocessor), m_postprocessMap(postprocessMap),
      m_progressReporter(nullptr), m_postprocess(true), m_promptUser(true),
      m_tableDirty(false), m_pauseReduction(false), m_reductionPaused(true),
      m_nextActionFlag(ReductionFlag::StopReduceFlag) {

  // Column Options must be added to the whitelist
  m_whitelist.addElement("Options", "Options",
                         "<b>Override <samp>" + processor.name() +
                             "</samp> properties</b><br /><i>optional</i><br "
                             "/>This column allows you to "
                             "override the properties used when executing "
                             "the main reduction algorithm. "
                             "Options are given as "
                             "key=value pairs, separated by commas. Values "
                             "containing commas must be quoted. In case of "
                             "conflict between options "
                             "specified via this column and global options "
                             "specified externally, the former prevail.");

  // Column Hidden Options must be added to the whitelist
  m_whitelist.addElement("HiddenOptions", "HiddenOptions",
                         "<b>Override <samp>" + processor.name() +
                             "</samp> properties</b><br /><i>optional</i><br "
                             "/>This column allows you to "
                             "override the properties used when executing "
                             "the main reduction algorithm in the same way"
                             "as the Options column, but this column is hidden"
                             "from the user. "
                             "Hidden Options are given as "
                             "key=value pairs, separated by commas. Values "
                             "containing commas must be quoted. In case of "
                             "conflict between options "
                             "specified via this column and global options "
                             "specified externally, the former prevail.");

  m_columns = static_cast<int>(m_whitelist.size());

  if (m_postprocessor.name().isEmpty()) {
    m_postprocess = false;
    m_manager = Mantid::Kernel::make_unique<DataProcessorOneLevelTreeManager>(
        this, m_whitelist);
  } else {
    m_manager = Mantid::Kernel::make_unique<DataProcessorTwoLevelTreeManager>(
        this, m_whitelist);
  }
}

/**
* Delegating constructor (no pre-processing needed)
* @param whitelist : The set of properties we want to show as columns
* @param processor : A DataProcessorProcessingAlgorithm
* @param postprocessor : A DataProcessorPostprocessingAlgorithm
* workspaces
*/
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor)
    : GenericDataProcessorPresenter(
          whitelist, std::map<QString, DataProcessorPreprocessingAlgorithm>(),
          processor, postprocessor) {}

/**
 * Delegating constructor (only whitelist specified)
 * @param whitelist : The set of properties we want to show as columns
 */
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist)
    : GenericDataProcessorPresenter(
          whitelist, std::map<QString, DataProcessorPreprocessingAlgorithm>(),
          DataProcessorProcessingAlgorithm(),
          DataProcessorPostprocessingAlgorithm()) {}

/**
* Delegating constructor (no post-processing needed)
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A DataProcessorProcessingAlgorithm
* workspaces
*/
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<QString, DataProcessorPreprocessingAlgorithm> &preprocessMap,
    const DataProcessorProcessingAlgorithm &processor)
    : GenericDataProcessorPresenter(whitelist, preprocessMap, processor,
                                    DataProcessorPostprocessingAlgorithm()) {}

/**
* Delegating constructor (no pre-processing needed, no post-processing needed)
* @param whitelist : The set of properties we want to show as columns
* @param processor : A DataProcessorProcessingAlgorithm
* workspaces
*/
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const DataProcessorProcessingAlgorithm &processor)
    : GenericDataProcessorPresenter(
          whitelist, std::map<QString, DataProcessorPreprocessingAlgorithm>(),
          processor, DataProcessorPostprocessingAlgorithm()) {}

/**
* Destructor
*/
GenericDataProcessorPresenter::~GenericDataProcessorPresenter() {}

namespace {
std::set<std::string> toStdStringSet(std::set<QString> in) {
  auto out = std::set<std::string>();
  std::transform(std::begin(in), std::end(in), std::inserter(out, out.begin()),
                 [](QString const &inStr)
                     -> std::string { return inStr.toStdString(); });
  return out;
}
}
/**
* Sets the views this presenter is going to handle
* @param tableView : The table view
* @param progressView : The progress view
*/
void GenericDataProcessorPresenter::acceptViews(
    DataProcessorView *tableView, ProgressableView *progressView) {

  // As soon as we are given a view, initialize everything
  m_view = tableView;
  m_progressView = progressView;

  // Add actions to toolbar
  addCommands();

  // Initialise options
  // Load saved values from disk
  initOptions();

  // Populate an initial list of valid tables to open, and subscribe to the ADS
  // to keep it up to date
  Mantid::API::AnalysisDataServiceImpl &ads =
      Mantid::API::AnalysisDataService::Instance();

  auto items = ads.getObjectNames();
  for (auto const &name : items) {
    Workspace_sptr ws = ads.retrieve(name);

    if (m_manager->isValidModel(
            boost::dynamic_pointer_cast<ITableWorkspace>(ws),
            m_whitelist.size()))
      m_workspaceList.insert(QString::fromStdString(name));
  }
  observeAdd();
  observePostDelete();
  observeRename();
  observeADSClear();
  observeAfterReplace();
  m_view->setTableList(m_workspaceList);

  // Provide autocompletion hints for the options column. We use the algorithm's
  // properties minus those we blacklist. We blacklist any useless properties or
  // ones we're handling that the user should'nt touch.
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create(m_processor.name().toStdString());
  m_view->setOptionsHintStrategy(
      new AlgorithmHintStrategy(alg, toStdStringSet(m_processor.blacklist())),
      m_columns - 2);

  // Start with a blank table
  newTable();

  // The view should currently be in the paused state
  m_view->pause();
}

/**
Process selected data
*/
void GenericDataProcessorPresenter::process() {
  // Emit a signal hat the process is starting
  m_view->emitProcessClicked();

  m_selectedData = m_manager->selectedData(m_promptUser);

  // Don't continue if there are no items selected
  if (m_selectedData.size() == 0)
    return;

  // Set the global settings. If any have been changed, set all groups and rows
  // as unprocessed
  QString newPreprocessingOptions =
      m_mainPresenter->getPreprocessingOptionsAsString();
  QString newProcessingOptions = m_mainPresenter->getProcessingOptions();
  QString newPostprocessingOptions =
      m_mainPresenter->getPostprocessingOptions();

  bool settingsChanged = m_preprocessingOptions != newPreprocessingOptions ||
                         m_processingOptions != newProcessingOptions ||
                         m_postprocessingOptions != newPostprocessingOptions;

  m_preprocessingOptions = newPreprocessingOptions;
  m_processingOptions = newProcessingOptions;
  m_postprocessingOptions = newPostprocessingOptions;

  // Clear the group queue
  m_gqueue = GroupQueue();

  // Progress: each group and each row within count as a progress step.
  int maxProgress = 0;

  for (const auto &item : m_selectedData) {
    // Loop over each group

    // Set group as unprocessed if settings have changed or the expected output
    // workspace cannot be found
    bool groupWSFound = workspaceExists(
        getPostprocessedWorkspaceName(item.second, m_postprocessor.prefix()));

    if (settingsChanged || !groupWSFound)
      m_manager->setProcessed(false, item.first);

    // Groups that are already processed or cannot be post-processed (only 1
    // child row selected) do not count in progress
    if (!isProcessed(item.first) && item.second.size() > 1)
      maxProgress++;

    RowQueue rowQueue;

    for (const auto &data : item.second) {

      // Add all row items to queue
      rowQueue.push(data);

      // Set row as unprocessed if settings have changed or the expected output
      // workspaces cannot be found
      bool rowWSFound = true;
      for (auto i = 0u;
           i < m_processor.numberOfOutputProperties() && rowWSFound; i++) {
        rowWSFound = workspaceExists(
            getReducedWorkspaceName(data.second, m_processor.prefix(i)));
      }

      if (settingsChanged || !rowWSFound)
        m_manager->setProcessed(false, data.first, item.first);

      // Rows that are already processed do not count in progress
      if (!isProcessed(data.first, item.first))
        maxProgress++;
    }
    m_gqueue.emplace(item.first, rowQueue);
  }

  // Create progress reporter bar
  if (maxProgress > 0) {
    int progress = 0;
    m_progressReporter = new ProgressPresenter(progress, maxProgress,
                                               maxProgress, m_progressView);
  }
  // Start processing the first group
  m_nextActionFlag = ReductionFlag::ReduceGroupFlag;
  resume();
}

/**
Decide which processing action to take next
*/
void GenericDataProcessorPresenter::doNextAction() {

  switch (m_nextActionFlag) {
  case ReductionFlag::ReduceRowFlag:
    nextRow();
    break;
  case ReductionFlag::ReduceGroupFlag:
    nextGroup();
    break;
  case ReductionFlag::StopReduceFlag:
    endReduction();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/**
Process a new row
*/
void GenericDataProcessorPresenter::nextRow() {

  if (m_pauseReduction) {
    // Notify presenter that reduction is paused
    m_mainPresenter->confirmReductionPaused();
    m_reductionPaused = true;
    return;
  }

  // Add processed row data to the group
  int rowIndex = m_rowItem.first;
  m_groupData[rowIndex] = m_rowItem.second;
  int groupIndex = m_gqueue.front().first;
  auto &rqueue = m_gqueue.front().second;

  if (!rqueue.empty()) {
    // Set next action flag
    m_nextActionFlag = ReductionFlag::ReduceRowFlag;
    // Reduce next row
    m_rowItem = rqueue.front();
    rqueue.pop();
    // Skip reducing rows that are already processed
    if (!isProcessed(m_rowItem.first, groupIndex)) {
      startAsyncRowReduceThread(&m_rowItem, groupIndex);
      return;
    }
  } else {
    m_gqueue.pop();
    // Set next action flag
    m_nextActionFlag = ReductionFlag::ReduceGroupFlag;

    // Skip post-processing groups that are already processed or only contain a
    // single row
    if (!isProcessed(groupIndex) && m_groupData.size() > 1) {
      startAsyncGroupReduceThread(m_groupData, groupIndex);
      return;
    }
  }

  // Row / group skipped, perform next action
  doNextAction();
}

/**
Process a new group
*/
void GenericDataProcessorPresenter::nextGroup() {

  if (m_pauseReduction) {
    // Notify presenter that reduction is paused
    m_mainPresenter->confirmReductionPaused();
    m_reductionPaused = true;
    return;
  }

  // Clear group data from any previously processed groups
  m_groupData.clear();

  if (!m_gqueue.empty()) {
    // Set next action flag
    m_nextActionFlag = ReductionFlag::ReduceRowFlag;
    // Reduce first row
    auto &rqueue = m_gqueue.front().second;
    m_rowItem = rqueue.front();
    rqueue.pop();
    // Skip reducing rows that are already processed
    if (!isProcessed(m_rowItem.first, m_gqueue.front().first))
      startAsyncRowReduceThread(&m_rowItem, m_gqueue.front().first);
    else
      doNextAction();
  } else {
    // If "Output Notebook" checkbox is checked then create an ipython notebook
    if (m_view->getEnableNotebook())
      saveNotebook(m_selectedData);
    endReduction();
  }
}

/*
Reduce the current row asynchronously
*/
void GenericDataProcessorPresenter::startAsyncRowReduceThread(RowItem *rowItem,
                                                              int groupIndex) {

  auto *worker = new GenericDataProcessorPresenterRowReducerWorker(
      this, rowItem, groupIndex);
  m_workerThread.reset(new GenericDataProcessorPresenterThread(this, worker));
  m_workerThread->start();
}

/*
Reduce the current group asynchronously
*/
void GenericDataProcessorPresenter::startAsyncGroupReduceThread(
    GroupData &groupData, int groupIndex) {

  auto *worker = new GenericDataProcessorPresenterGroupReducerWorker(
      this, groupData, groupIndex);
  m_workerThread.reset(new GenericDataProcessorPresenterThread(this, worker));
  m_workerThread->start();
}

/**
End reduction
*/
void GenericDataProcessorPresenter::endReduction() {

  pause();
  m_reductionPaused = true;
  m_mainPresenter->confirmReductionPaused();
}

/**
Handle reduction error
*/
void GenericDataProcessorPresenter::reductionError(QString ex) {
  m_view->giveUserCritical(ex, "Error");
}

/**
Handle thread completion
*/
void GenericDataProcessorPresenter::threadFinished(const int exitCode) {

  m_workerThread.release();

  if (exitCode == 0) { // Success
    m_progressReporter->report();
    doNextAction();
  } else { // Error
    m_progressReporter->clear();
    endReduction();
  }
}

/**
Display a dialog to choose save location for notebook, then save the notebook
there
@param data : the processed data
*/
void GenericDataProcessorPresenter::saveNotebook(const TreeData &data) {

  QString filename = m_view->requestNotebookPath();
  if (!filename.isEmpty()) {
    // Global pre-processing options as a map where keys are column
    // name and values are pre-processing options as a string
    const auto preprocessingOptionsMap =
        convertStringToMap(m_preprocessingOptions);

    auto notebook = Mantid::Kernel::make_unique<DataProcessorGenerateNotebook>(
        m_wsName, m_view->getProcessInstrument(), m_whitelist, m_preprocessMap,
        m_processor, m_postprocessor, preprocessingOptionsMap,
        m_processingOptions, m_postprocessingOptions);
    auto generatedNotebook =
        std::string(notebook->generateNotebook(data).toStdString());

    std::ofstream file(filename.toStdString(), std::ofstream::trunc);
    file << generatedNotebook;
    file.flush();
    file.close();
  }
}

/**
Post-processes the workspaces created by the given rows together.
@param groupData : the data in a given group as received from the tree manager
*/
void GenericDataProcessorPresenter::postProcessGroup(
    const GroupData &groupData) {

  // If no post processing has been defined, then we are dealing with a
  // one-level tree
  // where all rows are in one group. We don't want to perform post-processing
  // in
  // this case.
  if (!m_postprocess)
    return;

  // The input workspace names
  QStringList inputNames;

  // The name to call the post-processed ws
  auto const outputWSName =
      getPostprocessedWorkspaceName(groupData, m_postprocessor.prefix());

  // Go through each row and get the input ws names
  for (auto const &row : groupData) {

    // The name of the reduced workspace for this row
    auto const inputWSName =
        getReducedWorkspaceName(row.second, m_processor.prefix(0));

    if (workspaceExists(inputWSName)) {
      inputNames.append(inputWSName);
    }
  }

  auto const inputWSNames = inputNames.join(", ");

  // If the previous result is in the ADS already, we'll need to remove it.
  // If it's a group, we'll get an error for trying to group into a used group
  // name
  if (workspaceExists(outputWSName)) {
    removeWorkspace(outputWSName);
  }

  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create(m_postprocessor.name().toStdString());
  alg->initialize();
  setAlgorithmProperty(alg.get(), m_postprocessor.inputProperty(),
                       inputWSNames);
  setAlgorithmProperty(alg.get(), m_postprocessor.outputProperty(),
                       outputWSName);

  auto optionsMap = parseKeyValueString(m_postprocessingOptions.toStdString());
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    try {
      setAlgorithmProperty(alg.get(), kvp->first, kvp->second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp->first);
    }
  }

  // Options specified via post-process map
  for (auto const &prop : m_postprocessMap) {
    auto const propName = prop.second;
    auto const propValueStr =
        groupData.begin()->second[m_whitelist.colIndexFromColName(prop.first)];
    if (!propValueStr.isEmpty()) {
      // Warning: we take minus the value of the properties because in
      // Reflectometry this property refers to the rebin step, and they want a
      // logarithmic binning. If other technique areas need to use a
      // post-process map we'll need to re-think how to do this.
      alg->setPropertyValue(propName.toStdString(),
                            ("-" + propValueStr).toStdString());
    }
  }

  alg->execute();

  if (!alg->isExecuted())
    throw std::runtime_error("Failed to post-process workspaces.");
}

/**
Takes a user specified run, or list of runs, and returns a pointer to the
desired workspace
@param runStr : The run or list of runs (separated by '+')
@param preprocessor : The pre-processing algorithm acting on this column
@param optionsMap : User-specified options as a map
@throws std::runtime_error if the workspace could not be prepared
@returns a shared pointer to the workspace
*/
Workspace_sptr GenericDataProcessorPresenter::prepareRunWorkspace(
    const QString &runStr,
    const DataProcessorPreprocessingAlgorithm &preprocessor,
    const std::map<std::string, std::string> &optionsMap) {
  auto const instrument = m_view->getProcessInstrument();

  auto runs = runStr.split(QRegExp("[+,]"));
  if (runs.isEmpty())
    throw std::runtime_error("No runs given");

  // Remove leading/trailing whitespace from each run
  std::transform(runs.begin(), runs.end(), runs.begin(),
                 [](QString in) -> QString { return in.trimmed(); });

  // If we're only given one run, just return that
  if (runs.size() == 1)
    return getRun(runs[0], instrument, preprocessor.prefix());

  auto const outputName = preprocessor.prefix() + runs.join("_");

  /* Ideally, this should be executed as a child algorithm to keep the ADS tidy,
  * but that doesn't preserve history nicely, so we'll just take care of tidying
  * up in the event of failure.
  */
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create(preprocessor.name().toStdString());
  alg->initialize();
  setAlgorithmProperty(
      alg.get(), preprocessor.lhsProperty(),
      getRun(runs[0], instrument, preprocessor.prefix())->getName());
  setAlgorithmProperty(alg.get(), preprocessor.outputProperty(), outputName);

  // Drop the first run from the runs list
  runs.erase(runs.begin());

  try {
    // Iterate through all the remaining runs, adding them to the first run
    for (auto runIt = runs.begin(); runIt != runs.end(); ++runIt) {

      for (auto& kvp : optionsMap) {
        try {
          if (kvp.first != preprocessor.lhsProperty().toStdString() &&
              kvp.first != preprocessor.rhsProperty().toStdString())
          setAlgorithmProperty(alg.get(), kvp.first, kvp.second);
        } catch (Mantid::Kernel::Exception::NotFoundError &) {
          // We can't apply this option to this pre-processing alg
          throw;
        }
      }

      setAlgorithmProperty(
          alg.get(), preprocessor.rhsProperty(),
          getRun(*runIt, instrument, preprocessor.prefix())->getName());
      alg->execute();

      if (runIt != --runs.end()) {
        // After the first execution we replace the LHS with the previous output
        setAlgorithmProperty(alg.get(), preprocessor.lhsProperty(), outputName);
      }
    }
  } catch (...) {
    // If we're unable to create the full workspace, discard the partial version
    removeWorkspace(outputName);
    // We've tidied up, now re-throw.
    throw;
  }

  return AnalysisDataService::Instance().retrieveWS<Workspace>(
      outputName.toStdString());
}

/**
Returns the name of the reduced workspace for a given row
@param data :: [input] The data for this row
@param prefix : A prefix to be appended to the generated ws name
@throws std::runtime_error if the workspace could not be prepared
@returns : The name of the workspace
*/
QString
GenericDataProcessorPresenter::getReducedWorkspaceName(const QStringList &data,
                                                       const QString &prefix) {

  if (static_cast<int>(data.size()) != m_columns)
    throw std::invalid_argument("Can't find reduced workspace name");

  /* This method calculates, for a given row, the name of the output (processed)
  * workspace. This is done using the white list, which contains information
  * about the columns that should be included to create the ws name. In
  * Reflectometry for example, we want to include values in the 'Run(s)' and
  * 'Transmission Run(s)' columns. We may also use a prefix associated with
  * the column when specified. Finally, to construct the ws name we may also
  * use a 'global' prefix associated with the processing algorithm (for
  * instance 'IvsQ_' in Reflectometry) this is given by the second argument to
  * this method */

  // Temporary vector of strings to construct the name
  QStringList names;

  for (int col = 0; col < m_columns; col++) {

    // Do we want to use this column to generate the name of the output ws?
    if (m_whitelist.showValue(col)) {

      // Get what's in the column
      auto const valueStr = data.at(col);

      // If it's not empty, use it
      if (!valueStr.isEmpty()) {
        // But we may have things like '1+2' which we want to replace with '1_2'
        auto value = valueStr.split("+", QString::SkipEmptyParts);
        names.append(m_whitelist.prefix(col) + value.join("_"));
      }
    }
  } // Columns

  auto wsname = prefix;
  wsname += names.join("_");
  return wsname;
}

/**
Returns the name of the reduced workspace for a given group
@param groupData : The data in a given group
@param prefix : A prefix to be appended to the generated ws name
@returns : The name of the workspace
*/
QString GenericDataProcessorPresenter::getPostprocessedWorkspaceName(
    const GroupData &groupData, const QString &prefix) {

  if (!m_postprocess)
    return QString();

  /* This method calculates, for a given set of rows, the name of the output
  * (post-processed) workspace */

  QStringList outputNames;

  for (const auto &data : groupData) {
    outputNames.append(getReducedWorkspaceName(data.second));
  }
  return prefix + outputNames.join("_");
}

/** Loads a run found from disk or AnalysisDataService
 *
 * @param run : The name of the run
 * @param instrument : The instrument the run belongs to
 * @param prefix : The prefix to be prepended to the run number
 * @throws std::runtime_error if the run could not be loaded
 * @returns a shared pointer to the workspace
 */
Workspace_sptr GenericDataProcessorPresenter::getRun(const QString &run,
                                                     const QString &instrument,
                                                     const QString &prefix) {

  bool runFound;
  QString outName;
  QString fileName = instrument + run;

  outName = findRunInADS(run, prefix, runFound);
  if (!runFound) {
    outName = loadRun(run, instrument, prefix, m_loader, runFound);
    if (!runFound)
      throw std::runtime_error("Could not open " + fileName.toStdString());
  }

  return AnalysisDataService::Instance().retrieveWS<Workspace>(
      outName.toStdString());
}

bool isNumeric(QString const &numericCandidate) {
  return QRegExp("\\d+").exactMatch(numericCandidate);
}
/** Tries fetching a run from AnalysisDataService
 *
 * @param run : The name of the run
 * @param prefix : The prefix to be prepended to the run number
 * @param runFound : Whether or not the run was actually found
 * @returns string name of the run
 */
QString GenericDataProcessorPresenter::findRunInADS(const QString &run,
                                                    const QString &prefix,
                                                    bool &runFound) {

  runFound = true;

  // First, let's see if the run given is the name of a workspace in the ADS
  if (workspaceExists(run))
    return run;

  // Try with prefix
  if (workspaceExists(prefix + run))
    return prefix + run;

  // Is the run string is numeric?
  if (isNumeric(run)) {
    // Look for "<run_number>" in the ADS
    if (workspaceExists(run))
      return run;

    // Look for "<instrument><run_number>" in the ADS
    if (workspaceExists(prefix + run))
      return prefix + run;
  }

  // Run not found in ADS;
  runFound = false;
  return "";
}

/** Tries loading a run from disk
 *
 * @param run : The name of the run
 * @param instrument : The instrument the run belongs to
 * @param prefix : The prefix to be prepended to the run number
 * @param loader : The algorithm used for loading runs
 * @param runFound : Whether or not the run was actually found
 * @returns string name of the run
 */
QString GenericDataProcessorPresenter::loadRun(const QString &run,
                                               const QString &instrument,
                                               const QString &prefix,
                                               const QString &loader,
                                               bool &runFound) {

  runFound = true;
  auto const fileName = instrument + run;
  auto const outputName = prefix + run;

  IAlgorithm_sptr algLoadRun =
      AlgorithmManager::Instance().create(loader.toStdString());
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", fileName.toStdString());
  algLoadRun->setProperty("OutputWorkspace", outputName.toStdString());
  algLoadRun->execute();
  if (!algLoadRun->isExecuted()) {
    // Run not loaded from disk
    runFound = false;
    return "";
  }

  return outputName;
}

/** Reduce a row
 *
 * @param data :: [input] The data in this row as a vector where elements
 * correspond to column contents
 * @throws std::runtime_error if reduction fails
 */
void GenericDataProcessorPresenter::reduceRow(RowData *data) {

  /* Create the processing algorithm */

  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create(m_processor.name().toStdString());
  alg->initialize();

  /* Read input properties from the table */
  /* excluding 'Group' and 'Options' */

  // Global pre-processing options as a map
  std::map<QString, QString> globalOptions;
  if (!m_preprocessMap.empty())
    globalOptions = convertStringToMap(m_preprocessingOptions);

  // Pre-processing properties
  auto preProcessPropMap =
      convertStringToMapWithSet(m_mainPresenter->getPreprocessingProperties());

  // Properties not to be used in processing
  std::set<QString> restrictedProps;

  // Loop over all columns in the whitelist except 'Options' and 'Hidden
  // Options'
  for (int i = 0; i < m_columns - 2; i++) {

    // The algorithm's property linked to this column
    auto propertyName = m_whitelist.algPropFromColIndex(i);
    // The column's name
    auto columnName = m_whitelist.colNameFromColIndex(i);

    // The value for which preprocessing can be conducted on
    QString preProcessValue;

    if (globalOptions.count(columnName) &&
        !globalOptions[columnName].isEmpty()) {
      auto tmpOptionsMap =
          parseKeyValueString(globalOptions[columnName].toStdString());
      QStringList valueList;
      for (auto &optionMapEntry : tmpOptionsMap) {
        valueList.append(QString::fromStdString(optionMapEntry.second));
      }
      preProcessValue = valueList.join(",");
    } else if (!data->at(i).isEmpty()) {
      preProcessValue = data->at(i);
    } else {
      continue;
    }

    if (m_preprocessMap.count(columnName)) {
      // This column needs pre-processing

      // We do not want the associated properties to be set again in
      // processing
      if (preProcessPropMap.count(columnName) > 0) {
        for (auto &prop : preProcessPropMap[columnName]) {
          restrictedProps.insert(prop);
        }
      }

      auto preprocessor = m_preprocessMap.at(columnName);

      auto const globalOptionsForColumn = globalOptions.count(columnName) > 0
                                              ? globalOptions.at(columnName)
                                              : "";

      auto optionsMap =
          parseKeyValueString(globalOptionsForColumn.toStdString());
      auto runWS =
          prepareRunWorkspace(preProcessValue, preprocessor, optionsMap);
      setAlgorithmProperty(alg.get(), propertyName.toStdString(),
                           runWS->getName());
    } else {
      // No pre-processing needed
      auto propertyValue = data->at(i);
      if (!propertyValue.isEmpty())
        alg->setPropertyValue(propertyName.toStdString(),
                              propertyValue.toStdString());
    }
  }

  // Parse and set any user-specified options
  auto optionsMap = parseKeyValueString(m_processingOptions.toStdString());
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    try {
      if (restrictedProps.find(QString::fromStdString(kvp->first)) ==
          restrictedProps.end())
        setAlgorithmProperty(alg.get(), kvp->first, kvp->second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp->first);
    }
  }

  /* Now deal with 'Options' column */
  const auto userOptions = data->at(m_columns - 2);

  // Parse and set any user-specified options
  optionsMap = parseKeyValueString(userOptions.toStdString());
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    try {
      setAlgorithmProperty(alg.get(), kvp->first, kvp->second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp->first);
    }
  }

  // Now deal with the 'Hidden Options' column
  const auto hiddenOptions = data->back();

  // Parse and set any user-specified options
  auto hiddenOptionsMap = parseKeyValueString(hiddenOptions.toStdString());
  for (auto kvp = hiddenOptionsMap.begin(); kvp != hiddenOptionsMap.end();
       ++kvp) {
    try {
      alg->setProperty(kvp->first, kvp->second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in hidden options column: " +
                               kvp->first);
    }
  }

  /* We need to give a name to the output workspaces */
  for (auto i = 0u; i < m_processor.numberOfOutputProperties(); i++) {
    setAlgorithmProperty(alg.get(), m_processor.outputPropertyName(i),
                         getReducedWorkspaceName(*data, m_processor.prefix(i)));
  }

  /* Now run the processing algorithm */
  alg->execute();

  auto newData = data;
  if (alg->isExecuted()) {

    /* The reduction is complete, try to populate the columns */
    for (int i = 0; i < m_columns - 2; i++) {

      auto columnName = m_whitelist.colNameFromColIndex(i);

      if (data->at(i).isEmpty() && !m_preprocessMap.count(columnName)) {

        QString propValue = QString::fromStdString(alg->getPropertyValue(
            m_whitelist.algPropFromColIndex(i).toStdString()));

        if (m_options["Round"].toBool()) {
          QString exp = (propValue.indexOf("e") != -1)
                            ? propValue.right(propValue.indexOf("e"))
                            : "";
          propValue =
              propValue.mid(0, propValue.indexOf(".") +
                                   m_options["RoundPrecision"].toInt() + 1) +
              exp;
        }

        (*newData)[i] = propValue;
      }
    }
  }
}

/**
Insert a new row
*/
void GenericDataProcessorPresenter::appendRow() { m_manager->appendRow(); }

/**
Insert a new group
*/
void GenericDataProcessorPresenter::appendGroup() { m_manager->appendGroup(); }

/**
Delete row(s) from the model
*/
void GenericDataProcessorPresenter::deleteRow() { m_manager->deleteRow(); }

/**
Delete group(s) from the model
*/
void GenericDataProcessorPresenter::deleteGroup() { m_manager->deleteGroup(); }

/**
Group rows together
*/
void GenericDataProcessorPresenter::groupRows() { m_manager->groupRows(); }

/**
Expand all groups
*/
void GenericDataProcessorPresenter::expandAll() { m_view->expandAll(); }

/**
Collapse all groups
*/
void GenericDataProcessorPresenter::collapseAll() { m_view->collapseAll(); }

/**
Select all rows / groups
*/
void GenericDataProcessorPresenter::selectAll() { m_view->selectAll(); }

/**
Used by the view to tell the presenter something has changed
*/
void GenericDataProcessorPresenter::notify(DataProcessorPresenter::Flag flag) {

  switch (flag) {
  case DataProcessorPresenter::SaveAsFlag:
    saveTableAs();
    break;
  case DataProcessorPresenter::SaveFlag:
    saveTable();
    break;
  case DataProcessorPresenter::AppendRowFlag:
    appendRow();
    break;
  case DataProcessorPresenter::AppendGroupFlag:
    appendGroup();
    break;
  case DataProcessorPresenter::DeleteRowFlag:
    deleteRow();
    break;
  case DataProcessorPresenter::DeleteGroupFlag:
    deleteGroup();
    break;
  case DataProcessorPresenter::ProcessFlag:
    process();
    break;
  case DataProcessorPresenter::GroupRowsFlag:
    groupRows();
    break;
  case DataProcessorPresenter::NewTableFlag:
    newTable();
    break;
  case DataProcessorPresenter::TableUpdatedFlag:
    m_tableDirty = true;
    break;
  case DataProcessorPresenter::ExpandSelectionFlag:
    expandSelection();
    break;
  case DataProcessorPresenter::OptionsDialogFlag:
    showOptionsDialog();
    break;
  case DataProcessorPresenter::ClearSelectedFlag:
    clearSelected();
    break;
  case DataProcessorPresenter::CopySelectedFlag:
    copySelected();
    break;
  case DataProcessorPresenter::CutSelectedFlag:
    cutSelected();
    break;
  case DataProcessorPresenter::PasteSelectedFlag:
    pasteSelected();
    break;
  case DataProcessorPresenter::ImportTableFlag:
    importTable();
    break;
  case DataProcessorPresenter::OpenTableFlag:
    openTable();
    break;
  case DataProcessorPresenter::ExportTableFlag:
    exportTable();
    break;
  case DataProcessorPresenter::PlotRowFlag:
    plotRow();
    break;
  case DataProcessorPresenter::PlotGroupFlag:
    plotGroup();
    break;
  case DataProcessorPresenter::ExpandAllGroupsFlag:
    expandAll();
    break;
  case DataProcessorPresenter::CollapseAllGroupsFlag:
    collapseAll();
    break;
  case DataProcessorPresenter::SelectAllFlag:
    selectAll();
    break;
  case DataProcessorPresenter::PauseFlag:
    pause();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/**
Press changes to the same item in the ADS
*/
void GenericDataProcessorPresenter::saveTable() {
  if (!m_wsName.isEmpty()) {
    AnalysisDataService::Instance().addOrReplace(
        m_wsName.toStdString(),
        boost::shared_ptr<ITableWorkspace>(
            m_manager->getTableWorkspace()->clone().release()));
    m_tableDirty = false;
  } else {
    saveTableAs();
  }
}

/**
Press changes to a new item in the ADS
*/
void GenericDataProcessorPresenter::saveTableAs() {
  auto const userString =
      m_view->askUserString("Save As", "Enter a workspace name:", "Workspace");
  if (!userString.isEmpty()) {
    m_wsName = userString;
    saveTable();
  }
}

/**
Start a new, untitled table
*/
void GenericDataProcessorPresenter::newTable() {
  if (m_tableDirty && m_options["WarnDiscardChanges"].toBool())
    if (!m_view->askUserYesNo("Your current table has unsaved changes. Are you "
                              "sure you want to discard them?",
                              "Start New Table?"))
      return;

  m_manager->newTable(m_whitelist);
  m_wsName.clear();
  m_view->showTable(m_manager->getModel());

  m_tableDirty = false;
}

/**
Open a table from the ADS
*/
void GenericDataProcessorPresenter::openTable() {
  if (m_tableDirty && m_options["WarnDiscardChanges"].toBool())
    if (!m_view->askUserYesNo("Your current table has unsaved changes. Are you "
                              "sure you want to discard them?",
                              "Open Table?"))
      return;

  auto &ads = AnalysisDataService::Instance();
  auto const toOpen = m_view->getWorkspaceToOpen();

  if (toOpen.isEmpty())
    return;

  if (!ads.isValid(toOpen.toStdString()).empty()) {
    m_view->giveUserCritical("Could not open workspace: " + toOpen, "Error");
    return;
  }

  ITableWorkspace_sptr origTable =
      AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
          toOpen.toStdString());

  // We create a clone of the table for live editing. The original is not
  // updated unless we explicitly save.
  ITableWorkspace_sptr newTable =
      boost::shared_ptr<ITableWorkspace>(origTable->clone().release());
  try {
    m_manager->isValidModel(newTable, m_whitelist.size());
    m_manager->newTable(newTable, m_whitelist);
    m_wsName = toOpen;
    m_view->showTable(m_manager->getModel());
    m_tableDirty = false;
  } catch (std::runtime_error &e) {
    m_view->giveUserCritical(
        QString(QString("Could not open workspace: ") + e.what()), "Error");
  }
}

/**
Import a table from TBL file
*/
void GenericDataProcessorPresenter::importTable() {

  QString pythonSrc;
  pythonSrc += "try:\n";
  pythonSrc += "  algm = LoadTBLDialog()\n";
  pythonSrc += "  print(algm.getPropertyValue(\"OutputWorkspace\"))\n";
  pythonSrc += "except:\n";
  pythonSrc += "  pass\n";

  auto const result = m_view->runPythonAlgorithm(pythonSrc);

  // result will hold the name of the output workspace
  // otherwise this should be an empty string.
  QString outputWorkspaceName = result.trimmed();
  if (!outputWorkspaceName.isEmpty())
    m_view->setModel(outputWorkspaceName);
}

/**
Export a table to TBL file
*/
void GenericDataProcessorPresenter::exportTable() {

  QString pythonSrc;
  pythonSrc += "try:\n";
  pythonSrc += "  algm = SaveTBLDialog()\n";
  pythonSrc += "except:\n";
  pythonSrc += "  pass\n";

  m_view->runPythonAlgorithm(pythonSrc);
}

/**
Handle ADS add events
*/
void GenericDataProcessorPresenter::addHandle(
    const std::string &name, Mantid::API::Workspace_sptr workspace) {
  if (Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(
          name))
    return;

  if (!m_manager->isValidModel(workspace, m_columns))
    return;

  m_workspaceList.insert(QString::fromStdString(name));
  m_view->setTableList(m_workspaceList);
  m_mainPresenter->notifyADSChanged(m_workspaceList);
}

/**
Handle ADS remove events
*/
void GenericDataProcessorPresenter::postDeleteHandle(const std::string &name) {
  m_workspaceList.remove(QString::fromStdString(name));
  m_view->setTableList(m_workspaceList);
  m_mainPresenter->notifyADSChanged(m_workspaceList);
}

/**
Handle ADS clear events
*/
void GenericDataProcessorPresenter::clearADSHandle() {
  m_workspaceList.clear();
  m_view->setTableList(m_workspaceList);
  m_mainPresenter->notifyADSChanged(m_workspaceList);
}

/**
Handle ADS rename events
*/
void GenericDataProcessorPresenter::renameHandle(const std::string &oldName,
                                                 const std::string &newName) {

  // if a workspace with oldName exists then replace it for the same workspace
  // with newName
  auto qOldName = QString::fromStdString(oldName);
  auto qNewName = QString::fromStdString(newName);
  if (m_workspaceList.contains(qOldName)) {
    m_workspaceList.remove(qOldName);
    m_workspaceList.insert(qNewName);
    m_view->setTableList(m_workspaceList);
    m_mainPresenter->notifyADSChanged(m_workspaceList);
  }
}

/**
Handle ADS replace events
*/
void GenericDataProcessorPresenter::afterReplaceHandle(
    const std::string &name, Mantid::API::Workspace_sptr workspace) {
  auto qName = QString::fromStdString(name);
  // Erase it
  m_workspaceList.remove(qName);

  // If it's a table workspace, bring it back
  if (m_manager->isValidModel(workspace, m_columns))
    m_workspaceList.insert(qName);

  m_view->setTableList(m_workspaceList);
}

/** Expands the current selection */
void GenericDataProcessorPresenter::expandSelection() {

  auto selection = m_manager->expandSelection();

  if (!selection.empty())
    m_view->setSelection(selection);
}

/** Clear current selection */
void GenericDataProcessorPresenter::clearSelected() {

  m_manager->clearSelected();
}

/** Copy current selection to clipboard */
void GenericDataProcessorPresenter::copySelected() {

  m_view->setClipboard(m_manager->copySelected());
}

/** Copy currently selected rows to the clipboard, and then delete them. */
void GenericDataProcessorPresenter::cutSelected() {
  copySelected();
  deleteRow();
}

/** Paste the contents of the clipboard */
void GenericDataProcessorPresenter::pasteSelected() {
  auto const text = m_view->getClipboard();

  if (!text.isEmpty())
    m_manager->pasteSelected(text);
}

/** Transfers the selected runs in the search results to the processing table
* @param runs : [input] the set of runs to transfer as a vector of maps
*/
void GenericDataProcessorPresenter::transfer(
    const std::vector<std::map<QString, QString>> &runs) {

  m_manager->transfer(runs, m_whitelist);
  m_view->showTable(m_manager->getModel());
}

/**
Set the list of available instruments to search for and updates the list of
available instruments in the table view
@param instruments : The list of instruments available
@param defaultInstrument : The instrument to have selected by default
*/
void GenericDataProcessorPresenter::setInstrumentList(
    const QStringList &instruments, const QString &defaultInstrument) {

  QString instrList = instruments.join(",");

  m_view->setInstrumentList(instrList, defaultInstrument);
}

/** Plots any currently selected rows */
void GenericDataProcessorPresenter::plotRow() {
  if (m_processor.name().isEmpty())
    return;

  // Set of workspaces to plot
  QOrderedSet<QString> workspaces;
  // Set of workspaces not found in the ADS
  QSet<QString> notFound;

  const auto items = m_manager->selectedData();

  for (const auto &item : items) {

    for (const auto &run : item.second) {

      auto const wsName =
          getReducedWorkspaceName(run.second, m_processor.prefix(0));

      if (workspaceExists(wsName))
        workspaces.insert(wsName, nullptr);
      else
        notFound.insert(wsName);
    }
  }

  if (!notFound.isEmpty())
    issueNotFoundWarning("rows", notFound);

  plotWorkspaces(workspaces);
}

void GenericDataProcessorPresenter::issueNotFoundWarning(
    QString const &granule, QSet<QString> const &missingWorkspaces) {
  m_view->giveUserWarning(
      "The following workspaces were not plotted because they were not "
      "found:\n" +
          QStringList(QStringList::fromSet(missingWorkspaces)).join("\n") +
          "\n\nPlease check that the " + granule +
          " you are trying to plot have been "
          "fully processed.",
      "Error plotting " + granule + ".");
}

/** Plots any currently selected groups */
void GenericDataProcessorPresenter::plotGroup() {
  if (m_processor.name().isEmpty())
    return;

  // This method shouldn't be called if a post-processing algorithm is not
  // defined
  if (!m_postprocess)
    throw std::runtime_error("Can't plot group.");

  // Set of workspaces to plot
  QOrderedSet<QString> workspaces;
  // Set of workspaces not found in the ADS
  QSet<QString> notFound;

  auto const items = m_manager->selectedData();

  for (const auto &item : items) {
    if (item.second.size() > 1) {
      auto const wsName =
          getPostprocessedWorkspaceName(item.second, m_postprocessor.prefix());

      if (workspaceExists(wsName))
        workspaces.insert(wsName, nullptr);
      else
        notFound.insert(wsName);
    }
  }

  if (!notFound.empty())
    issueNotFoundWarning("groups", notFound);

  plotWorkspaces(workspaces);
}

/**
Plot a set of workspaces
* @param workspaces : [input] The list of workspaces as a set
*/
void GenericDataProcessorPresenter::plotWorkspaces(
    const QOrderedSet<QString> &workspaces) {
  if (!workspaces.isEmpty()) {
    QString pythonSrc;
    pythonSrc += "base_graph = None\n";
    for (auto ws = workspaces.begin(); ws != workspaces.end(); ++ws)
      pythonSrc += "base_graph = plotSpectrum(\"" + ws.key() +
                   "\", 0, True, window = base_graph)\n";

    pythonSrc += "base_graph.activeLayer().logLogAxes()\n";

    m_view->runPythonAlgorithm(pythonSrc);
  }
}

/** Shows the Refl Options dialog */
void GenericDataProcessorPresenter::showOptionsDialog() {
  auto options =
      new QtDataProcessorOptionsDialog(m_view, m_view->getPresenter());
  // By default the dialog is only destroyed when ReflMainView is and so they'll
  // stack up.
  // This way, they'll be deallocated as soon as they've been closed.
  options->setAttribute(Qt::WA_DeleteOnClose, true);
  options->exec();
}

/** Gets the options used by the presenter
@returns The options used by the presenter
*/
const std::map<QString, QVariant> &
GenericDataProcessorPresenter::options() const {
  return m_options;
}

/** Sets the options used by the presenter
@param options : The new options for the presenter to use
*/
void GenericDataProcessorPresenter::setOptions(
    const std::map<QString, QVariant> &options) {
  // Overwrite the given options
  for (auto it = options.begin(); it != options.end(); ++it)
    m_options[it->first] = it->second;

  // Save any changes to disk
  m_view->saveSettings(m_options);
}

/** Load options from disk if possible, or set to defaults */
void GenericDataProcessorPresenter::initOptions() {
  m_options.clear();

  // Set defaults
  m_options["WarnProcessAll"] = true;
  m_options["WarnDiscardChanges"] = true;
  m_options["WarnProcessPartialGroup"] = true;
  m_options["Round"] = false;
  m_options["RoundPrecision"] = 3;

  // Load saved values from disk
  m_view->loadSettings(m_options);
}

/** Tells the view which of the actions should be added to the toolbar
*/
void GenericDataProcessorPresenter::addCommands() {

  auto commands = m_manager->publishCommands();
  std::vector<std::unique_ptr<DataProcessorCommand>> commandsToShow;
  for (auto comm = 10u; comm < commands.size(); comm++)
    commandsToShow.push_back(std::move(commands.at(comm)));
  m_view->addActions(std::move(commandsToShow));
}

/**
Pauses reduction. If currently reducing runs, this does not take effect until
the current thread for reducing a row or group has finished
*/
void GenericDataProcessorPresenter::pause() {

  m_view->pause();
  m_mainPresenter->pause();

  m_pauseReduction = true;
}

/** Resumes reduction if currently paused
*/
void GenericDataProcessorPresenter::resume() {

  m_view->resume();
  m_mainPresenter->resume();

  m_pauseReduction = false;
  m_reductionPaused = false;
  m_mainPresenter->confirmReductionResumed();

  doNextAction();
}

/**
* Tells the view to load a table workspace
* @param name : [input] The workspace's name
*/
void GenericDataProcessorPresenter::setModel(QString const &name) {
  m_view->setModel(name);
}

/**
* Sets whether to prompt user when getting selected runs
* @param allowPrompt : [input] Enable setting user prompt
*/
void GenericDataProcessorPresenter::setPromptUser(bool allowPrompt) {
  m_promptUser = allowPrompt;
}

/**
* Publishes a list of available commands
* @return : The list of available commands
*/
std::vector<std::unique_ptr<DataProcessorCommand>>
GenericDataProcessorPresenter::publishCommands() {

  auto commands = m_manager->publishCommands();

  // "Open Table" needs the list of "child" commands, i.e. the list of
  // available workspaces in the ADS
  commands.at(0)->setChild(getTableList());

  return commands;
}

/** Register a workspace receiver
* @param mainPresenter : [input] The outer presenter
*/
void GenericDataProcessorPresenter::accept(
    DataProcessorMainPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
  // Notify workspace receiver with the list of valid workspaces as soon as it
  // is registered
  m_mainPresenter->notifyADSChanged(m_workspaceList);
  // Presenter should initially be in the paused state
  m_mainPresenter->pause();
}

/** Returs the list of valid workspaces currently in the ADS
* @return : The vector of workspaces (as commands)
*/
std::vector<DataProcessorCommand_uptr>
GenericDataProcessorPresenter::getTableList() {

  std::vector<DataProcessorCommand_uptr> workspaces;
  workspaces.reserve(m_workspaceList.size());
  // Create a command for each of the workspaces in the ADS
  for (const auto &name : m_workspaceList) {
    workspaces.push_back(
        Mantid::Kernel::make_unique<DataProcessorWorkspaceCommand>(this, name));
  }
  return workspaces;
}

/** Asks the view for selected parent items
 * @return :: the selected parent items
 */
ParentItems GenericDataProcessorPresenter::selectedParents() const {
  return m_view->getSelectedParents();
}

/** Asks the view for selected child items
 * @return :: the selected child items
 */
ChildItems GenericDataProcessorPresenter::selectedChildren() const {
  return m_view->getSelectedChildren();
}

/** Ask user for Yes/No
 * @param prompt :: the question to ask
 * @param title :: the title
 * @return :: Yes/No
 */
bool GenericDataProcessorPresenter::askUserYesNo(const QString &prompt,
                                                 const QString &title) const {

  return m_view->askUserYesNo(prompt, title);
}

/** Print warning message
 * @param prompt :: the message
 * @param title :: the title
 */
void GenericDataProcessorPresenter::giveUserWarning(
    const QString &prompt, const QString &title) const {

  m_view->giveUserWarning(prompt, title);
}

/** Checks whether data reduction is still in progress or not
* @return :: the reduction state
*/
bool GenericDataProcessorPresenter::isProcessing() const {
  return !m_reductionPaused;
}

/** Checks if a row in the table has been processed.
 * @param position :: the row to check
 * @return :: true if the row has already been processed else false.
 */
bool GenericDataProcessorPresenter::isProcessed(int position) const {
  // processing truth table
  // isProcessed      manager    force
  //    0               1          1
  //    0               0          1
  //    1               1          0
  //    0               0          0
  return m_manager->isProcessed(position) && !m_forceProcessing;
}

/** Checks if a row in the table has been processed.
 * @param position :: the row to check
 * @param parent :: the parent
 * @return :: true if the row has already been processed else false.
 */
bool GenericDataProcessorPresenter::isProcessed(int position,
                                                int parent) const {
  // processing truth table
  // isProcessed      manager    force
  //    0               1          1
  //    0               0          1
  //    1               1          0
  //    0               0          0
  return m_manager->isProcessed(position, parent) && !m_forceProcessing;
}

/** Set the forced reprocessing flag
 * @param forceReProcessing :: the row to check
 */
void GenericDataProcessorPresenter::setForcedReProcessing(
    bool forceReProcessing) {
  m_forceProcessing = forceReProcessing;
}

/** Set a value in the table
 *
 * @param row : the row index
 * @param column : the column index
 * @param parentRow : the row index of the parent item
 * @param parentColumn : the column index of the parent item
 * @param value : the new value
*/
void GenericDataProcessorPresenter::setCell(int row, int column, int parentRow,
                                            int parentColumn,
                                            const std::string &value) {

  m_manager->setCell(row, column, parentRow, parentColumn, value);
}

/** Gets a cell from the table
 *
 * @param row : the row index
 * @param column : the column index
 * @param parentRow : the row index of the parent item
 * @param parentColumn : the column index of the parent item
 * @return : the value in the cell
*/
std::string GenericDataProcessorPresenter::getCell(int row, int column,
                                                   int parentRow,
                                                   int parentColumn) {

  return m_manager->getCell(row, column, parentRow, parentColumn);
}

/**
 * Gets the number of rows.
 * @return : the number of rows.
 */
int GenericDataProcessorPresenter::getNumberOfRows() {
  return m_manager->getNumberOfRows();
}

/**
  * Clear the table
 **/
void GenericDataProcessorPresenter::clearTable() { m_manager->deleteRow(); }

}
}
