#include "MantidQtCustomInterfaces/Reflectometry/GenericDataProcessorPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
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
#include "MantidQtCustomInterfaces/ParseKeyValueString.h"
#include "MantidQtCustomInterfaces/ProgressableView.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAppendRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorClearSelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorCopySelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorCutSelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorDeleteRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorExpandCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorExportTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorGenerateNotebook.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorGroupRowsCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorImportTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorNewTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorOpenTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorOptionsCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorPasteSelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorPlotGroupCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorPlotRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorPrependRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorProcessCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorSaveTableAsCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorSaveTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorSeparatorCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorView.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorWorkspaceCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ProgressPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/QDataProcessorTableModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtDataProcessorOptionsDialog.h"
#include "MantidQtCustomInterfaces/Reflectometry/WorkspaceReceiver.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {

/**
* Constructor
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A DataProcessorProcessingAlgorithm
* @param postprocessor : A DataProcessorPostprocessingAlgorithm
* workspaces
*/
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor)
    : WorkspaceObserver(), m_view(nullptr), m_progressView(nullptr),
      m_whitelist(whitelist), m_preprocessMap(preprocessMap),
      m_processor(processor), m_postprocessor(postprocessor),
      m_tableDirty(false) {

  // Columns Group and Options must be added to the whitelist
  m_whitelist.addElement("Group", "Group",
                         "<b>Grouping for post-processing</b><br "
                         "/><i>required</i><br />The value of this column "
                         "determines which other rows this row's output will "
                         "be post-processed with. All rows with the same group "
                         "number are post-processed together.");
  m_whitelist.addElement("Options", "Options",
                         "<b>Override <samp>" + processor.name() +
                             "</samp> properties</b><br /><i>optional</i><br "
                             "/>This column allows you to "
                             "override the properties used when executing "
                             "<samp>ReflectometryReductionOneAuto</samp>. "
                             "Options are given as "
                             "key=value pairs, separated by commas. Values "
                             "containing commas must be quoted. In case of "
                             "conflict between options "
                             "specified via this column and options specified "
                             "via the <b>Process</b> line edit, the former "
                             "prevail.");
  m_columns = static_cast<int>(m_whitelist.size());
}

/**
* Destructor
*/
GenericDataProcessorPresenter::~GenericDataProcessorPresenter() {}

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

  // Initialise options
  // Load saved values from disk
  initOptions();
  // Create the process layout
  createProcessLayout();

  // Populate an initial list of valid tables to open, and subscribe to the ADS
  // to keep it up to date
  Mantid::API::AnalysisDataServiceImpl &ads =
      Mantid::API::AnalysisDataService::Instance();

  auto items = ads.getObjectNames();
  for (auto const &name : items) {
    Workspace_sptr ws = ads.retrieve(name);

    if (isValidModel(ws))
      m_workspaceList.insert(name);
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
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(m_processor.name());
  m_view->setOptionsHintStrategy(
      new AlgorithmHintStrategy(alg, m_processor.blacklist()), m_columns - 1);

  // Start with a blank table
  newTable();
}

/**
Tells the view how to create the HintingLineEdits for pre-, post- and processing
*/
void GenericDataProcessorPresenter::createProcessLayout() {

  std::vector<std::string> stages;
  std::vector<std::string> algNames;
  std::vector<std::map<std::string, std::string>> hints;

  // Pre-process
  // The number of items depends on the number of algorithms needed for
  // pre-processing the data
  for (auto it = m_preprocessMap.begin(); it != m_preprocessMap.end(); ++it) {

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create(it->second.name());
    AlgorithmHintStrategy strategy(alg, it->second.blacklist());
    stages.push_back("Pre-process");
    algNames.push_back(alg->name());
    hints.push_back(strategy.createHints());
  }

  // Process
  // Only one algorithm
  {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create(m_processor.name());
    AlgorithmHintStrategy strategy(alg, m_processor.blacklist());
    stages.push_back("Process");
    algNames.push_back(alg->name());
    hints.push_back(strategy.createHints());
  }

  // Post-process
  // Only one algorithm
  {
    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create(m_postprocessor.name());
    AlgorithmHintStrategy strategy(alg, m_postprocessor.blacklist());
    stages.push_back("Post-process");
    algNames.push_back(alg->name());
    hints.push_back(strategy.createHints());
  }

  m_view->setGlobalOptions(stages, algNames, hints);
}

/**
* Validates a table workspace
* @param model : [input] The table workspace to validate
* @throws std::runtime_error if the model is not valid
*/
void GenericDataProcessorPresenter::validateModel(ITableWorkspace_sptr model) {
  if (!model)
    throw std::runtime_error("Null pointer");

  int columns = static_cast<int>(model->columnCount());
  if (columns != m_columns)
    throw std::runtime_error("Selected table has the incorrect number of "
                             "columns to be used as a data processor table.");

  try {
    // All columns must be strings
    size_t ncols = model->columnCount();
    for (size_t i = 0; i < ncols - 2; i++)
      model->String(0, i);
    // Except Group, which must be int
    model->Int(0, ncols - 2);
    // Options column must be string too
    model->String(0, ncols - 1);
  } catch (const std::runtime_error &) {
    throw std::runtime_error("Selected table does not meet the specifications "
                             "to become a model for this interface.");
  }
}

/**
* Checks if a model is valid
* @param model : [input] The table workspace to validate
* @returns : True if the model is valid. False otherwise
*/
bool GenericDataProcessorPresenter::isValidModel(Workspace_sptr model) {
  try {
    validateModel(boost::dynamic_pointer_cast<ITableWorkspace>(model));
  } catch (...) {
    return false;
  }
  return true;
}

/**
* Creates a model using the whitelist supplied to this presenter
* @returns : The new model
*/
ITableWorkspace_sptr GenericDataProcessorPresenter::createWorkspace() {
  ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

  for (int col = 0; col < m_columns - 2; col++) {
    // The columns provided to this presenter
    auto column = ws->addColumn("str", m_whitelist.colNameFromColIndex(col));
    column->setPlotType(0);
  }
  // The Group column, must be int
  auto colGroup = ws->addColumn("int", "Group");
  colGroup->setPlotType(0);
  auto colOptions = ws->addColumn("str", "Options");
  colOptions->setPlotType(0);

  return ws;
}

/**
* Creates a default model using the whitelist supplied to this presenter
* @returns : The new model
*/
ITableWorkspace_sptr GenericDataProcessorPresenter::createDefaultWorkspace() {
  auto ws = createWorkspace();
  ws->appendRow();
  return ws;
}

/**
* Finds the first unused group id
*/
int GenericDataProcessorPresenter::getUnusedGroup(
    std::set<int> ignoredRows) const {
  std::set<int> usedGroups;

  // Scan through all the rows, working out which group ids are used
  for (int idx = 0; idx < m_model->rowCount(); ++idx) {
    if (ignoredRows.find(idx) != ignoredRows.end())
      continue;

    // This is an unselected row. Add it to the list of used group ids
    usedGroups.insert(
        m_model->data(m_model->index(idx, m_columns - 2)).toInt());
  }

  int groupId = 0;

  // While the group id is one of the used ones, increment it by 1
  while (usedGroups.find(groupId) != usedGroups.end())
    groupId++;

  return groupId;
}

/**
Process selected rows
*/
void GenericDataProcessorPresenter::process() {
  if (m_model->rowCount() == 0) {
    m_view->giveUserWarning("Cannot process an empty Table", "Warning");
    return;
  }

  std::set<int> rows = m_view->getSelectedRows();
  if (rows.empty()) {
    if (m_options["WarnProcessAll"].toBool()) {
      // Does the user want to abort?
      if (!m_view->askUserYesNo(
              "This will process all rows in the table. Continue?",
              "Process all rows?"))
        return;
    }

    // They want to process all rows, so populate rows with every index in the
    // model
    for (int idx = 0; idx < m_model->rowCount(); ++idx)
      rows.insert(idx);
  }

  // Map group numbers to the set of rows in that group we want to process
  std::map<int, std::set<int>> groups;
  for (auto it = rows.begin(); it != rows.end(); ++it)
    groups[m_model->data(m_model->index(*it, m_columns - 2)).toInt()].insert(
        *it);

  // Check each group and warn if we're only partially processing it
  for (auto gIt = groups.begin(); gIt != groups.end(); ++gIt) {
    const int &groupId = gIt->first;
    const std::set<int> &groupRows = gIt->second;
    // Are we only partially processing a group?
    if (groupRows.size() < numRowsInGroup(gIt->first) &&
        m_options["WarnProcessPartialGroup"].toBool()) {
      std::stringstream err;
      err << "You have only selected " << groupRows.size() << " of the ";
      err << numRowsInGroup(groupId) << " rows in group " << groupId << ".";
      err << " Are you sure you want to continue?";
      if (!m_view->askUserYesNo(err.str(), "Continue Processing?"))
        return;
    }
  }

  if (!rowsValid(rows)) {
    return;
  }

  if (!processGroups(groups, rows)) {
    return;
  }

  // If "Output Notebook" checkbox is checked then create an ipython notebook
  if (m_view->getEnableNotebook()) {
    saveNotebook(groups, rows);
  }
}

/**
Display a dialog to choose save location for notebook, then save the notebook
there
@param groups : groups of rows to stitch
@param rows : rows selected for processing
*/
void GenericDataProcessorPresenter::saveNotebook(
    std::map<int, std::set<int>> groups, std::set<int> rows) {

  std::string filename = m_view->requestNotebookPath();
  if (filename == "") {
    return;
  }

  // Get all the options used for the reduction from the view
  std::map<std::string, std::string> preprocessingOptionsMap;
  for (auto it = m_preprocessMap.begin(); it != m_preprocessMap.end(); ++it) {
    preprocessingOptionsMap[it->first] =
        m_view->getProcessingOptions(it->second.name());
  }
  auto processingOptions = m_view->getProcessingOptions(m_processor.name());
  auto postprocessingOptions =
      m_view->getProcessingOptions(m_postprocessor.name());

  auto notebook = Mantid::Kernel::make_unique<DataProcessorGenerateNotebook>(
      m_wsName, m_model, m_view->getProcessInstrument(), m_whitelist,
      m_preprocessMap, m_processor, m_postprocessor, preprocessingOptionsMap,
      processingOptions, postprocessingOptions);
  std::string generatedNotebook = notebook->generateNotebook(groups, rows);

  std::ofstream file(filename.c_str(), std::ofstream::trunc);
  file << generatedNotebook;
  file.flush();
  file.close();
}

/**
Post-processes the workspaces created by the given rows together.
@param rows : the list of rows
*/
void GenericDataProcessorPresenter::postProcessRows(std::set<int> rows) {
  // If we can get away with doing nothing, do.
  if (rows.size() < 2)
    return;

  // The input workspace names
  std::vector<std::string> inputNames;
  // Vector to construct the output ws name
  std::vector<std::string> outputNames;

  // Go through each row and prepare the properties
  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {

    // The names of the processed workspaces (without prefix)
    const std::string runStr = getWorkspaceName(*rowIt, false);

    if (AnalysisDataService::Instance().doesExist(m_processor.prefix(0) +
                                                  runStr)) {

      inputNames.emplace_back(m_processor.prefix(0) + runStr);
      outputNames.emplace_back(runStr);
    }
  }
  const std::string inputWSNames = boost::algorithm::join(inputNames, ", ");
  const std::string outputWSName =
      m_postprocessor.prefix() + boost::algorithm::join(outputNames, "_");

  // If the previous result is in the ADS already, we'll need to remove it.
  // If it's a group, we'll get an error for trying to group into a used group
  // name
  if (AnalysisDataService::Instance().doesExist(outputWSName))
    AnalysisDataService::Instance().remove(outputWSName);

  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create(m_postprocessor.name());
  alg->initialize();
  alg->setProperty(m_postprocessor.inputProperty(), inputWSNames);
  alg->setProperty(m_postprocessor.outputProperty(), outputWSName);

  // Read the post-processing instructions from the view
  const std::string options =
      m_view->getProcessingOptions(m_postprocessor.name());
  auto optionsMap = parseKeyValueString(options);
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    try {
      alg->setProperty(kvp->first, kvp->second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp->first);
    }
  }

  alg->execute();

  if (!alg->isExecuted())
    throw std::runtime_error("Failed to post-process workspaces.");
}

/**
Process stitch groups
@param rows : rows in the model
@param groups : groups of rows to stitch
@returns true if successful, otherwise false
*/
bool GenericDataProcessorPresenter::processGroups(
    std::map<int, std::set<int>> groups, std::set<int> rows) {
  int progress = 0;
  // Each group and each row within count as a progress step.
  const int maxProgress = (int)(rows.size() + groups.size());
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  for (auto gIt = groups.begin(); gIt != groups.end(); ++gIt) {
    const std::set<int> groupRows = gIt->second;

    // Reduce each row
    for (auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt) {
      try {
        reduceRow(*rIt);
        progressReporter.report();
      } catch (std::exception &ex) {
        const std::string rowNo =
            Mantid::Kernel::Strings::toString<int>(*rIt + 1);
        const std::string message =
            "Error encountered while processing row " + rowNo + ":\n";
        m_view->giveUserCritical(message + ex.what(), "Error");
        progressReporter.clear();
        return false;
      }
    }

    try {
      postProcessRows(groupRows);
      progressReporter.report();
    } catch (std::exception &ex) {
      const std::string groupNo =
          Mantid::Kernel::Strings::toString<int>(gIt->first);
      const std::string message =
          "Error encountered while stitching group " + groupNo + ":\n";
      m_view->giveUserCritical(message + ex.what(), "Error");
      progressReporter.clear();
      return false;
    }
  }
  return true;
}

/**
Validate rows.
@param rows : Rows in the model to validate
@returns true if all rows are valid and false otherwise
*/
bool GenericDataProcessorPresenter::rowsValid(std::set<int> rows) {
  for (auto it = rows.begin(); it != rows.end(); ++it) {
    try {
      validateRow(*it);
    } catch (std::exception &ex) {
      // Allow two theta to be blank
      if (ex.what() ==
          std::string("Value for two theta could not be found in log."))
        continue;

      const std::string rowNo = Mantid::Kernel::Strings::toString<int>(*it + 1);
      m_view->giveUserCritical(
          "Error found in row " + rowNo + ":\n" + ex.what(), "Error");
      return false;
    }
  }
  return true;
}

/**
Validate a row.
A row may pass validation, but it is not necessarily ready for processing.
@param rowNo : The row in the model to validate
@throws std::invalid_argument if the row fails validation
*/
void GenericDataProcessorPresenter::validateRow(int rowNo) const {
  if (rowNo >= m_model->rowCount())
    throw std::invalid_argument("Invalid row");
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
    const std::string &runStr,
    const DataProcessorPreprocessingAlgorithm &preprocessor,
    const std::map<std::string, std::string> &optionsMap) {
  const std::string instrument = m_view->getProcessInstrument();

  std::vector<std::string> runs;
  boost::split(runs, runStr, boost::is_any_of("+,"));

  if (runs.empty())
    throw std::runtime_error("No runs given");

  // Remove leading/trailing whitespace from each run
  for (auto runIt = runs.begin(); runIt != runs.end(); ++runIt)
    boost::trim(*runIt);

  // If we're only given one run, just return that
  if (runs.size() == 1)
    return loadRun(runs[0], instrument, preprocessor.prefix());

  const std::string outputName =
      preprocessor.prefix() + boost::algorithm::join(runs, "_");

  /* Ideally, this should be executed as a child algorithm to keep the ADS tidy,
  * but that doesn't preserve history nicely, so we'll just take care of tidying
  * up in the event of failure.
  */
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().create(preprocessor.name());
  alg->initialize();
  alg->setProperty(preprocessor.lhsProperty(),
                   loadRun(runs[0], instrument, preprocessor.prefix())->name());
  alg->setProperty(preprocessor.outputProperty(), outputName);

  // Drop the first run from the runs list
  runs.erase(runs.begin());

  try {
    // Iterate through all the remaining runs, adding them to the first run
    for (auto runIt = runs.begin(); runIt != runs.end(); ++runIt) {

      for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
        try {
          alg->setProperty(kvp->first, kvp->second);
        } catch (Mantid::Kernel::Exception::NotFoundError &) {
          // We can't apply this option to this pre-processing alg
          throw;
        }
      }

      alg->setProperty(
          preprocessor.rhsProperty(),
          loadRun(*runIt, instrument, preprocessor.prefix())->name());
      alg->execute();

      if (runIt != --runs.end()) {
        // After the first execution we replace the LHS with the previous output
        alg->setProperty(preprocessor.lhsProperty(), outputName);
      }
    }
  } catch (...) {
    // If we're unable to create the full workspace, discard the partial version
    AnalysisDataService::Instance().remove(outputName);

    // We've tidied up, now re-throw.
    throw;
  }

  return AnalysisDataService::Instance().retrieveWS<Workspace>(outputName);
}

/**
Returns the name of the workspace
@param row : The row
@param prefix : Wheter or not to include the prefix
@throws std::runtime_error if the workspace could not be prepared
@returns : The name of the workspace
*/
std::string GenericDataProcessorPresenter::getWorkspaceName(int row,
                                                            bool prefix) {

  /* This method calculates, for a given row, the name of the output (processed)
   * workspace. In Reflectometry for example, where we have two columns that
   * need pre-processing, 'Run(s)' and 'Transmission Run(s)', the name of the
   * output ws will contain the information (i.e. run numbers) displayed in
   * those columns. To construct the ws name we also need the prefix associated
   * with the processor algorithm (for instance 'IvsQ_' in Reflectometry)
   */

  // Temporary vector of strings to construct the name
  std::vector<std::string> names;

  for (int col = 0; col < m_columns; col++) {

    const std::string colName = m_whitelist.colNameFromColIndex(col);

    if (m_preprocessMap.count(colName)) {
      // OK, this column was pre-processed, that means that the output ws name
      // may contain information associated with this pre-processor

      if (m_preprocessMap[colName].show()) {
        // OK, we do want to show the pre-processed run numbers

        const std::string runStr =
            m_model->data(m_model->index(row, col)).toString().toStdString();

        if (!runStr.empty()) {
          std::vector<std::string> runs;
          boost::split(runs, runStr, boost::is_any_of("+"));

          names.push_back(m_preprocessMap[colName].prefix() +
                          boost::algorithm::join(runs, "_"));
        }
      }
    }
  }

  std::string wsname;

  if (prefix)
    wsname += m_processor.prefix(0);
  wsname += boost::algorithm::join(names, "_");

  return wsname;
}

/**
Loads a run from disk or fetches it from the AnalysisDataService
@param run : The name of the run
@param instrument : The instrument the run belongs to
@param prefix : The prefix to be prepended to the run number
@throws std::runtime_error if the run could not be loaded
@returns a shared pointer to the workspace
*/
Workspace_sptr
GenericDataProcessorPresenter::loadRun(const std::string &run,
                                       const std::string &instrument,
                                       const std::string &prefix) {
  // First, let's see if the run given is the name of a workspace in the ADS
  if (AnalysisDataService::Instance().doesExist(run))
    return AnalysisDataService::Instance().retrieveWS<Workspace>(run);
  // Try with prefix
  if (AnalysisDataService::Instance().doesExist(prefix + run))
    return AnalysisDataService::Instance().retrieveWS<Workspace>(prefix + run);

  // Is the run string is numeric
  if (boost::regex_match(run, boost::regex("\\d+"))) {
    std::string wsName;

    // Look for "<run_number>" in the ADS
    wsName = run;
    if (AnalysisDataService::Instance().doesExist(wsName))
      return AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);

    // Look for "<instrument><run_number>" in the ADS
    wsName = instrument + run;
    if (AnalysisDataService::Instance().doesExist(wsName))
      return AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
  }

  // We'll just have to load it ourselves
  const std::string filename = instrument + run;
  const std::string outputName = prefix + run;
  IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create("Load");
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", filename);
  algLoadRun->setProperty("OutputWorkspace", outputName);
  algLoadRun->execute();

  if (!algLoadRun->isExecuted())
    throw std::runtime_error("Could not open " + filename);

  return AnalysisDataService::Instance().retrieveWS<Workspace>(outputName);
}

/**
Reduce a row
@param rowNo : The row in the model to reduce
@throws std::runtime_error if reduction fails
*/
void GenericDataProcessorPresenter::reduceRow(int rowNo) {

  /* Create the processing algorithm */

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(m_processor.name());
  alg->initialize();

  /* Read input properties from the table */
  /* excluding 'Group' and 'Options' */

  // Loop over all columns except 'Group' and 'Options'
  for (int i = 0; i < m_columns - 2; i++) {

    // The algorithm's property linked to this column
    auto propertyName = m_whitelist.algPropFromColIndex(i);
    // The column's name
    auto columnName = m_whitelist.colNameFromColIndex(i);

    if (m_preprocessMap.count(columnName)) {
      // This column needs pre-processing

      const std::string runStr =
          m_model->data(m_model->index(rowNo, i)).toString().toStdString();

      if (!runStr.empty()) {

        auto preprocessor = m_preprocessMap[columnName];

        // Read the pre-processing options from the view
        const std::string options =
            m_view->getProcessingOptions(preprocessor.name());
        auto optionsMap = parseKeyValueString(options);
        auto runWS = prepareRunWorkspace(runStr, preprocessor, optionsMap);
        alg->setProperty(propertyName, runWS->name());
      }
    } else {
      // No pre-processing needed, read from the table
      auto propertyValue =
          m_model->data(m_model->index(rowNo, i)).toString().toStdString();
      if (!propertyValue.empty())
        alg->setPropertyValue(propertyName, propertyValue);
    }
  }

  /* Deal with processing instructions specified via the hinting line edit */
  std::string options = m_view->getProcessingOptions(m_processor.name());
  // Parse and set any user-specified options
  auto optionsMap = parseKeyValueString(options);
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    try {
      alg->setProperty(kvp->first, kvp->second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp->first);
    }
  }

  /* Now deal with 'Options' column */
  options = m_model->data(m_model->index(rowNo, m_model->columnCount() - 1))
                .toString()
                .toStdString();
  // Parse and set any user-specified options
  optionsMap = parseKeyValueString(options);
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    try {
      alg->setProperty(kvp->first, kvp->second);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      throw std::runtime_error("Invalid property in options column: " +
                               kvp->first);
    }
  }

  /* We need to give a name to the output workspaces */
  for (size_t i = 0; i < m_processor.numberOfOutputProperties(); i++) {
    alg->setProperty(m_processor.outputPropertyName(i),
                     m_processor.prefix(i) + getWorkspaceName(rowNo, false));
  }

  /* Now run the processing algorithm */
  alg->execute();

  if (alg->isExecuted()) {

    /* The reduction is complete, try to populate the columns */
    for (int i = 0; i < m_columns - 2; i++) {
      if (m_model->data(m_model->index(rowNo, i)).toString().isEmpty()) {

        std::string propValue =
            alg->getPropertyValue(m_whitelist.algPropFromColIndex(i));

        m_model->setData(m_model->index(rowNo, i),
                         QString::fromStdString(propValue));
      }
    }
  }
}

/**
Inserts a new row in the specified location
@param index The index to insert the new row before
*/
void GenericDataProcessorPresenter::insertRow(int index) {
  const int groupId = getUnusedGroup();
  if (!m_model->insertRow(index))
    return;
  // Set the group id of the new row
  // m_columns - 2 is the index of column 'Group'
  m_model->setData(m_model->index(index, m_columns - 2), groupId);
}

/**
Insert a row after the last selected row
*/
void GenericDataProcessorPresenter::appendRow() {
  std::set<int> rows = m_view->getSelectedRows();
  if (rows.empty())
    insertRow(m_model->rowCount());
  else
    insertRow(*rows.rbegin() + 1);
  m_tableDirty = true;
}

/**
Insert a row before the first selected row
*/
void GenericDataProcessorPresenter::prependRow() {
  std::set<int> rows = m_view->getSelectedRows();
  if (rows.empty())
    insertRow(0);
  else
    insertRow(*rows.begin());
  m_tableDirty = true;
}

/**
Get the index of the first blank row, or if none exists, returns -1.
*/
int GenericDataProcessorPresenter::getBlankRow() {
  // Go through every column of every row (except for the scale column) and
  // check if it's blank.
  // If there's a blank row, return it.
  const int rowCount = m_model->rowCount();
  for (int i = 0; i < rowCount; ++i) {
    bool isBlank = true;
    for (int j = 0; j < m_columns; ++j) {
      // Don't bother checking the group column, it'll always have a
      // value.
      if (j == m_columns - 2)
        continue;

      if (!m_model->data(m_model->index(i, j)).toString().isEmpty()) {
        isBlank = false;
        break;
      }
    }

    if (isBlank)
      return i;
  }

  // There are no blank rows
  return -1;
}

/**
Delete row(s) from the model
*/
void GenericDataProcessorPresenter::deleteRow() {
  std::set<int> rows = m_view->getSelectedRows();
  for (auto row = rows.rbegin(); row != rows.rend(); ++row)
    m_model->removeRow(*row);

  m_tableDirty = true;
}

/**
Group rows together
*/
void GenericDataProcessorPresenter::groupRows() {
  const std::set<int> rows = m_view->getSelectedRows();
  // Find the first unused group id, ignoring the selected rows
  const int groupId = getUnusedGroup(rows);

  // Now we just have to set the group id on the selected rows
  for (auto it = rows.begin(); it != rows.end(); ++it)
    m_model->setData(m_model->index(*it, m_columns - 2), groupId);

  m_tableDirty = true;
}

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
  case DataProcessorPresenter::PrependRowFlag:
    prependRow();
    break;
  case DataProcessorPresenter::DeleteRowFlag:
    deleteRow();
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
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/**
Press changes to the same item in the ADS
*/
void GenericDataProcessorPresenter::saveTable() {
  if (!m_wsName.empty()) {
    AnalysisDataService::Instance().addOrReplace(
        m_wsName, boost::shared_ptr<ITableWorkspace>(m_ws->clone().release()));
    m_tableDirty = false;
  } else {
    saveTableAs();
  }
}

/**
Press changes to a new item in the ADS
*/
void GenericDataProcessorPresenter::saveTableAs() {
  const std::string userString =
      m_view->askUserString("Save As", "Enter a workspace name:", "Workspace");
  if (!userString.empty()) {
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

  m_ws = createDefaultWorkspace();
  m_model.reset(new QDataProcessorTableModel(m_ws, m_whitelist));
  m_wsName.clear();
  m_view->showTable(m_model);

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
  const std::string toOpen = m_view->getWorkspaceToOpen();

  if (toOpen.empty())
    return;

  if (!ads.isValid(toOpen).empty()) {
    m_view->giveUserCritical("Could not open workspace: " + toOpen, "Error");
    return;
  }

  ITableWorkspace_sptr origTable =
      AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(toOpen);

  // We create a clone of the table for live editing. The original is not
  // updated unless we explicitly save.
  ITableWorkspace_sptr newTable =
      boost::shared_ptr<ITableWorkspace>(origTable->clone().release());
  try {
    validateModel(newTable);
    m_ws = newTable;
    m_model.reset(new QDataProcessorTableModel(m_ws, m_whitelist));
    m_wsName = toOpen;
    m_view->showTable(m_model);
    m_tableDirty = false;
  } catch (std::runtime_error &e) {
    m_view->giveUserCritical(
        "Could not open workspace: " + std::string(e.what()), "Error");
  }
}

/**
Import a table from TBL file
*/
void GenericDataProcessorPresenter::importTable() {
  m_view->showImportDialog();
}

/**
Export a table to TBL file
*/
void GenericDataProcessorPresenter::exportTable() {
  m_view->showAlgorithmDialog("SaveTBL");
}

/**
Handle ADS add events
*/
void GenericDataProcessorPresenter::addHandle(
    const std::string &name, Mantid::API::Workspace_sptr workspace) {
  if (Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(
          name))
    return;

  if (!isValidModel(workspace))
    return;

  m_workspaceList.insert(name);
  m_view->setTableList(m_workspaceList);
  m_workspaceReceiver->notify(WorkspaceReceiver::ADSChangedFlag);
}

/**
Handle ADS remove events
*/
void GenericDataProcessorPresenter::postDeleteHandle(const std::string &name) {
  m_workspaceList.erase(name);
  m_view->setTableList(m_workspaceList);
  m_workspaceReceiver->notify(WorkspaceReceiver::ADSChangedFlag);
}

/**
Handle ADS clear events
*/
void GenericDataProcessorPresenter::clearADSHandle() {
  m_workspaceList.clear();
  m_view->setTableList(m_workspaceList);
  m_workspaceReceiver->notify(WorkspaceReceiver::ADSChangedFlag);
}

/**
Handle ADS rename events
*/
void GenericDataProcessorPresenter::renameHandle(const std::string &oldName,
                                                 const std::string &newName) {

  // if a workspace with oldName exists then replace it for the same workspace
  // with newName
  if (m_workspaceList.find(oldName) == m_workspaceList.end())
    return;

  m_workspaceList.erase(oldName);
  m_workspaceList.insert(newName);
  m_view->setTableList(m_workspaceList);
  m_workspaceReceiver->notify(WorkspaceReceiver::ADSChangedFlag);
}

/**
Handle ADS replace events
*/
void GenericDataProcessorPresenter::afterReplaceHandle(
    const std::string &name, Mantid::API::Workspace_sptr workspace) {
  // Erase it
  m_workspaceList.erase(name);

  // If it's a table workspace, bring it back
  if (isValidModel(workspace))
    m_workspaceList.insert(name);

  m_view->setTableList(m_workspaceList);
}

/** Returns how many rows there are in a given group
@param groupId : The id of the group to count the rows of
@returns The number of rows in the group
*/
size_t GenericDataProcessorPresenter::numRowsInGroup(int groupId) const {
  size_t count = 0;
  for (int i = 0; i < m_model->rowCount(); ++i)
    if (m_model->data(m_model->index(i, m_columns - 2)).toInt() == groupId)
      count++;
  return count;
}

/** Expands the current selection to all the rows in the selected groups */
void GenericDataProcessorPresenter::expandSelection() {
  std::set<int> groupIds;

  std::set<int> rows = m_view->getSelectedRows();
  for (auto row = rows.begin(); row != rows.end(); ++row)
    groupIds.insert(m_model->data(m_model->index(*row, m_columns - 2)).toInt());

  std::set<int> selection;

  for (int i = 0; i < m_model->rowCount(); ++i)
    if (groupIds.find(m_model->data(m_model->index(i, m_columns - 2))
                          .toInt()) != groupIds.end())
      selection.insert(i);

  m_view->setSelection(selection);
}

/** Clear the currently selected rows */
void GenericDataProcessorPresenter::clearSelected() {
  std::set<int> rows = m_view->getSelectedRows();
  std::set<int> ignore;
  for (auto row = rows.begin(); row != rows.end(); ++row) {
    ignore.clear();
    ignore.insert(*row);

    for (int i = 0; i < m_columns - 2; i++) {
      m_model->setData(m_model->index(*row, i), "");
    }
    // 'Group' column
    m_model->setData(m_model->index(*row, m_columns - 2),
                     getUnusedGroup(ignore));
    // 'Options' column
    m_model->setData(m_model->index(*row, m_columns - 1), "");
  }
  m_tableDirty = true;
}

/** Copy the currently selected rows to the clipboard */
void GenericDataProcessorPresenter::copySelected() {
  std::vector<std::string> lines;

  std::set<int> rows = m_view->getSelectedRows();
  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {
    std::vector<std::string> line;
    for (int col = 0; col < m_columns; ++col)
      line.push_back(
          m_model->data(m_model->index(*rowIt, col)).toString().toStdString());
    lines.push_back(boost::algorithm::join(line, "\t"));
  }

  m_view->setClipboard(boost::algorithm::join(lines, "\n"));
}

/** Copy currently selected rows to the clipboard, and then delete them. */
void GenericDataProcessorPresenter::cutSelected() {
  copySelected();
  deleteRow();
}

/** Paste the contents of the clipboard into the currently selected rows, or
* append new rows */
void GenericDataProcessorPresenter::pasteSelected() {
  const std::string text = m_view->getClipboard();
  std::vector<std::string> lines;
  boost::split(lines, text, boost::is_any_of("\n"));

  // If we have rows selected, we'll overwrite them. If not, we'll append new
  // rows to write to.
  std::set<int> rows = m_view->getSelectedRows();
  if (rows.empty()) {
    // Add as many new rows as required
    for (size_t i = 0; i < lines.size(); ++i) {
      int index = m_model->rowCount();
      insertRow(index);
      rows.insert(index);
    }
  }

  // Iterate over rows and lines simultaneously, stopping when we reach the end
  // of either
  auto rowIt = rows.begin();
  auto lineIt = lines.begin();
  for (; rowIt != rows.end() && lineIt != lines.end(); rowIt++, lineIt++) {
    std::vector<std::string> values;
    boost::split(values, *lineIt, boost::is_any_of("\t"));

    // Paste as many columns as we can from this line
    for (int col = 0; col < m_columns && col < static_cast<int>(values.size());
         ++col)
      m_model->setData(m_model->index(*rowIt, col),
                       QString::fromStdString(values[col]));
  }
}

/** Transfers the selected runs in the search results to the processing table
* @param runs : [input] the set of runs to transfer as a vector of maps
*/
void GenericDataProcessorPresenter::transfer(
    const std::vector<std::map<std::string, std::string>> &runs) {

  auto newRows = runs;

  std::map<std::string, int> groups;
  // Loop over the rows (vector elements)
  for (auto rowsIt = newRows.begin(); rowsIt != newRows.end(); ++rowsIt) {
    auto &row = *rowsIt;

    if (groups.count(row["Group"]) == 0)
      groups[row["Group"]] = getUnusedGroup();

    // Overwrite the first blank row we find, otherwise, append a new row to the
    // end.
    int rowIndex = getBlankRow();
    if (rowIndex == -1) {
      rowIndex = m_model->rowCount();
      insertRow(rowIndex);
    }

    // Loop over the map (each row with column heading keys to cell values)
    for (auto rowIt = row.begin(); rowIt != row.end(); ++rowIt) {
      const std::string columnHeading = rowIt->first;
      const std::string cellEntry = rowIt->second;
      m_model->setData(m_model->index(rowIndex, m_whitelist.colIndexFromColName(
                                                    columnHeading)),
                       QString::fromStdString(cellEntry));
    }

    // Special case grouping. Group cell entry is string it seems!
    m_model->setData(m_model->index(rowIndex, m_columns - 2),
                     groups[row["Group"]]);
  }
}

/**
Set the list of available instruments to search for and updates the list of
available instruments in the table view
@param instruments : The list of instruments available
@param defaultInstrument : The instrument to have selected by default
*/
void GenericDataProcessorPresenter::setInstrumentList(
    const std::vector<std::string> &instruments,
    const std::string &defaultInstrument) {

  m_view->setInstrumentList(instruments, defaultInstrument);
}

/** Plots any currently selected rows */
void GenericDataProcessorPresenter::plotRow() {
  auto selectedRows = m_view->getSelectedRows();

  if (selectedRows.empty())
    return;

  std::set<std::string> workspaces, notFound;
  for (auto row = selectedRows.begin(); row != selectedRows.end(); ++row) {
    const std::string wsName = getWorkspaceName(*row);
    if (AnalysisDataService::Instance().doesExist(wsName))
      workspaces.insert(wsName);
    else
      notFound.insert(wsName);
  }

  if (!notFound.empty())
    m_view->giveUserWarning("The following workspaces were not plotted because"
                            "they were not found:\n" +
                                boost::algorithm::join(notFound, "\n") +
                                "\n\nPlease check that the rows you are trying"
                                "to plot have been fully processed.",
                            "Error plotting rows.");

  m_view->plotWorkspaces(workspaces);
}

/** Plots any currently selected groups */
void GenericDataProcessorPresenter::plotGroup() {
  auto selectedRows = m_view->getSelectedRows();

  if (selectedRows.empty())
    return;

  // The set of selected groups
  std::set<int> selectedGroups;
  for (auto row = selectedRows.begin(); row != selectedRows.end(); ++row)
    selectedGroups.insert(
        m_model->data(m_model->index(*row, m_columns - 2)).toInt());

  // Now, get the rows belonging to the specified groups
  std::map<int, std::vector<int>> rowsByGroup;
  const int numRows = m_model->rowCount();
  for (int row = 0; row < numRows; ++row) {
    int group = m_model->data(m_model->index(row, m_columns - 2)).toInt();

    // Skip groups we don't care about
    if (selectedGroups.find(group) == selectedGroups.end())
      continue;

    // Add this row to group 'group'
    rowsByGroup[group].push_back(row);
  }

  // Now get the workspace names

  std::set<std::string> workspaces, notFound;
  for (auto rowsMap = rowsByGroup.begin(); rowsMap != rowsByGroup.end();
       ++rowsMap) {

    auto rows = rowsMap->second;

    std::vector<std::string> names;
    for (auto &row : rows) {
      names.emplace_back(getWorkspaceName(row, false));
    }

    const std::string wsName =
        m_postprocessor.prefix() + boost::algorithm::join(names, "_");
    if (AnalysisDataService::Instance().doesExist(wsName))
      workspaces.insert(wsName);
    else
      notFound.insert(wsName);
  }

  if (!notFound.empty())
    m_view->giveUserWarning("The following workspaces were not plotted because"
                            "they were not found:\n" +
                                boost::algorithm::join(notFound, "\n") +
                                "\n\nPlease check that the groups you are "
                                "trying to plot have been fully processed.",
                            "Error plotting groups.");

  m_view->plotWorkspaces(workspaces);
}

/** Shows the Refl Options dialog */
void GenericDataProcessorPresenter::showOptionsDialog() {
  auto options =
      new QtDataProcessorOptionsDialog(m_view, m_view->getTablePresenter());
  // By default the dialog is only destroyed when ReflMainView is and so they'll
  // stack up.
  // This way, they'll be deallocated as soon as they've been closed.
  options->setAttribute(Qt::WA_DeleteOnClose, true);
  options->exec();
}

/** Gets the options used by the presenter
@returns The options used by the presenter
*/
const std::map<std::string, QVariant> &
GenericDataProcessorPresenter::options() const {
  return m_options;
}

/** Sets the options used by the presenter
@param options : The new options for the presenter to use
*/
void GenericDataProcessorPresenter::setOptions(
    const std::map<std::string, QVariant> &options) {
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
  m_options["RoundAngle"] = false;
  m_options["RoundQMin"] = false;
  m_options["RoundQMax"] = false;
  m_options["RoundDQQ"] = false;
  m_options["RoundAnglePrecision"] = 3;
  m_options["RoundQMinPrecision"] = 3;
  m_options["RoundQMaxPrecision"] = 3;
  m_options["RoundDQQPrecision"] = 3;

  // Load saved values from disk
  m_view->loadSettings(m_options);
}

/**
* Tells the view to load a table workspace
* @param name : [input] The workspace's name
*/
void GenericDataProcessorPresenter::setModel(std::string name) {
  m_view->setModel(name);
}

/**
* Adds a command to a vector of commands
* @param commands : [input] The vector where the new command will be added
* @param command : [input] The command to add
*/
void addToCommand(std::vector<DataProcessorCommand_uptr> &commands,
                  DataProcessorCommand_uptr command) {
  commands.push_back(std::move(command));
}

/**
* Publishes a list of available commands
* @return : The list of available commands
*/
std::vector<DataProcessorCommand_uptr>
GenericDataProcessorPresenter::publishCommands() {

  std::vector<DataProcessorCommand_uptr> commands;

  addToCommand(commands, make_unique<DataProcessorOpenTableCommand>(this));
  addToCommand(commands, make_unique<DataProcessorNewTableCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSaveTableCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSaveTableAsCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSeparatorCommand>(this));
  addToCommand(commands, make_unique<DataProcessorImportTableCommand>(this));
  addToCommand(commands, make_unique<DataProcessorExportTableCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSeparatorCommand>(this));
  addToCommand(commands, make_unique<DataProcessorOptionsCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSeparatorCommand>(this));
  addToCommand(commands, make_unique<DataProcessorProcessCommand>(this));
  addToCommand(commands, make_unique<DataProcessorExpandCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSeparatorCommand>(this));
  addToCommand(commands, make_unique<DataProcessorPlotRowCommand>(this));
  addToCommand(commands, make_unique<DataProcessorPlotGroupCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSeparatorCommand>(this));
  addToCommand(commands, make_unique<DataProcessorAppendRowCommand>(this));
  addToCommand(commands, make_unique<DataProcessorPrependRowCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSeparatorCommand>(this));
  addToCommand(commands, make_unique<DataProcessorGroupRowsCommand>(this));
  addToCommand(commands, make_unique<DataProcessorCopySelectedCommand>(this));
  addToCommand(commands, make_unique<DataProcessorCutSelectedCommand>(this));
  addToCommand(commands, make_unique<DataProcessorPasteSelectedCommand>(this));
  addToCommand(commands, make_unique<DataProcessorClearSelectedCommand>(this));
  addToCommand(commands, make_unique<DataProcessorSeparatorCommand>(this));
  addToCommand(commands, make_unique<DataProcessorDeleteRowCommand>(this));

  // "Open Table" needs the list of "child" commands, i.e. the list of
  // available workspaces in the ADS
  commands.at(0)->setChild(getTableList());

  return commands;
}

/** Register a workspace receiver
* @param workspaceReceiver : [input] The outer presenter
*/
void GenericDataProcessorPresenter::accept(
    WorkspaceReceiver *workspaceReceiver) {
  m_workspaceReceiver = workspaceReceiver;
  // Notify workspace receiver with the list of valid workspaces as soon as it
  // is registered
  m_workspaceReceiver->notify(WorkspaceReceiver::ADSChangedFlag);
}

/** Returs the list of valid workspaces currently in the ADS
* @return : The vector of workspaces (as commands)
*/
std::vector<DataProcessorCommand_uptr>
GenericDataProcessorPresenter::getTableList() {

  std::vector<DataProcessorCommand_uptr> workspaces;

  // Create a command for each of the workspaces in the ADS
  for (auto it = m_workspaceList.begin(); it != m_workspaceList.end(); ++it) {
    addToCommand(
        workspaces,
        Mantid::Kernel::make_unique<DataProcessorWorkspaceCommand>(this, *it));
  }
  return workspaces;
}
}
}
