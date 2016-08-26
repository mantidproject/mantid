#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
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
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGenerateNotebook.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWorkspaceCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ParseKeyValueString.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTwoLevelTreeModel.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QtDataProcessorOptionsDialog.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"
#include "MantidQtMantidWidgets/ProgressableView.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {

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
      m_processor(processor), m_postprocessor(postprocessor), m_mainPresenter(),
      m_tableDirty(false) {

  m_manager =
      Mantid::Kernel::make_unique<DataProcessorTwoLevelTreeManager>(this);

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
                             "specified via this column and options specified "
                             "via the <b>Process</b> line edit, the former "
                             "prevail.");
  m_columns = static_cast<int>(m_whitelist.size());
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
          whitelist,
          std::map<std::string, DataProcessorPreprocessingAlgorithm>(),
          processor, postprocessor) {}

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

  // Add actions to toolbar
  addActions();

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
* Validates a table workspace
* @param model : [input] The table workspace to validate
* @throws std::runtime_error if the model is not valid
*/
void GenericDataProcessorPresenter::validateModel(ITableWorkspace_sptr model) {
  if (!model)
    throw std::runtime_error("Null pointer");

  int columns = static_cast<int>(model->columnCount());
  if (columns != m_columns + 1)
    // Table workspace must have one extra column corresponding to the group
    throw std::runtime_error("Selected table has the incorrect number of "
                             "columns to be used as a data processor table.");

  try {
    // All columns must be strings
    size_t ncols = model->columnCount();
    for (size_t i = 0; i < ncols; i++)
      model->String(0, i);
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
* Creates a default model using the whitelist supplied to this presenter
* @returns : The new model
*/
ITableWorkspace_sptr GenericDataProcessorPresenter::createDefaultWorkspace() {
  ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

  // First column is group
  auto column = ws->addColumn("str", "Group");
  column->setPlotType(0);

  for (int col = 0; col < m_columns; col++) {
    // The columns provided to this presenter
    auto column = ws->addColumn("str", m_whitelist.colNameFromColIndex(col));
    column->setPlotType(0);
  }
  ws->appendRow();
  return ws;
}

/**
Process selected rows
*/
void GenericDataProcessorPresenter::process() {
  if (m_model->rowCount() == 0) {
    m_mainPresenter->giveUserWarning("Cannot process an empty Table",
                                     "Warning");
    return;
  }

  // Selected groups
  auto groups = m_view->getSelectedParents();
  // Selected rows
  auto rows = m_view->getSelectedChildren();

  if (groups.empty() && rows.empty()) {
    if (m_options["WarnProcessAll"].toBool()) {
      // Does the user want to abort?
      if (!m_mainPresenter->askUserYesNo(
              "This will process all rows in the table. Continue?",
              "Process all rows?"))
        return;
    }

    // They want to process all rows, so populate rows and groups with every
    // index in the model
    for (int idxGroup = 0; idxGroup < m_model->rowCount(); ++idxGroup) {
      groups.insert(idxGroup);
      for (int idxRun = 0;
           idxRun < m_model->rowCount(m_model->index(idxGroup, 0)); ++idxRun)
        rows[idxGroup].insert(idxRun);
    }
  } else {
    // They may have selected a group, in this case we want to process and
    // post-process the whole group, so populate group with every row
    for (const auto &group : groups) {
      for (int row = 0; row < numRowsInGroup(group); row++)
        rows[group].insert(row);
    }
  }

  // Check each group and warn if we're only partially processing it
  for (const auto &item : rows) {

    const int &groupId = item.first;
    const std::set<int> &rowIds = item.second;

    // Are we only partially processing a group?
    if (static_cast<int>(rowIds.size()) < numRowsInGroup(groupId) &&
        m_options["WarnProcessPartialGroup"].toBool()) {
      const std::string groupName =
          m_model->data(m_model->index(groupId, 0)).toString().toStdString();
      std::stringstream err;
      err << "You have only selected " << rowIds.size() << " of the ";
      err << numRowsInGroup(groupId) << " rows in group '" << groupName << "'.";
      err << " Are you sure you want to continue?";
      if (!m_mainPresenter->askUserYesNo(err.str(), "Continue Processing?"))
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
@param groups : groups that need post-processing
@param rows : rows to reduce
*/
void GenericDataProcessorPresenter::saveNotebook(
    const std::set<int> &groups, const std::map<int, std::set<int>> &rows) {

  std::string filename = m_view->requestNotebookPath();
  if (filename == "") {
    return;
  }

  // Global pre-processing options as a map where keys are column
  // name and values are pre-processing options as a string
  const std::map<std::string, std::string> preprocessingOptionsMap =
      m_mainPresenter->getPreprocessingOptions();
  // Global processing options as a string
  const std::string processingOptions = m_mainPresenter->getProcessingOptions();
  // Global post-processing options as a string
  const std::string postprocessingOptions =
      m_mainPresenter->getPostprocessingOptions();

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
@param group : the group to which the list of rows belongs
@param rows : the list of rows
*/
void GenericDataProcessorPresenter::postProcessGroup(
    int group, const std::set<int> &rows) {

  // If we can get away with doing nothing, do.
  if (rows.size() < 2)
    return;

  // The input workspace names
  std::vector<std::string> inputNames;

  // The name to call the post-processed ws
  const std::string outputWSName =
      getPostprocessedWorkspaceName(group, rows, m_postprocessor.prefix());

  // Go through each row and get the input ws names
  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {

    // The name of the reduced workspace for this row
    const std::string inputWSName =
        getReducedWorkspaceName(group, *rowIt, m_processor.prefix(0));

    if (AnalysisDataService::Instance().doesExist(inputWSName)) {

      inputNames.emplace_back(inputWSName);
    }
  }
  const std::string inputWSNames = boost::algorithm::join(inputNames, ", ");

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

  // Global post-processing options
  const std::string options = m_mainPresenter->getPostprocessingOptions();

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
Process rows and groups
@param groups : groups of rows to post-process
@param rows : rows to process
@returns true if successful, otherwise false
*/
bool GenericDataProcessorPresenter::processGroups(
    const std::set<int> &groups, const std::map<int, std::set<int>> &rows) {

  int progress = 0;
  // Each group and each row within count as a progress step.
  int maxProgress = (int)(groups.size());
  for (const auto row : rows) {
    maxProgress += (int)(row.second.size());
  }
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  for (auto it = rows.begin(); it != rows.end(); ++it) {
    const int groupId = it->first;
    auto rows = it->second;

    // Reduce each row
    for (const auto &row : rows) {
      try {
        reduceRow(groupId, row);
        progressReporter.report();
      } catch (std::exception &ex) {
        const std::string rowNo =
            Mantid::Kernel::Strings::toString<int>(row + 1);
        const std::string groupNo =
            Mantid::Kernel::Strings::toString<int>(groupId);
        const std::string message = "Error encountered while processing row " +
                                    rowNo + " in group " + groupNo + ":\n";
        m_mainPresenter->giveUserCritical(message + ex.what(), "Error");
        progressReporter.clear();
        return false;
      }
    }

    // Post-process if group was selected
    if (groups.find(groupId) != groups.end()) {
      try {
        postProcessGroup(groupId, rows);
        progressReporter.report();
      } catch (std::exception &ex) {
        const std::string groupNo =
            Mantid::Kernel::Strings::toString<int>(groupId);
        const std::string message =
            "Error encountered while stitching group " + groupNo + ":\n";
        m_mainPresenter->giveUserCritical(message + ex.what(), "Error");
        progressReporter.clear();
        return false;
      }
    }
  }
  return true;
}

/**
Validate rows.
@param groups : A map in which keys are groups and values sets or rows
@returns true if all rows are valid and false otherwise
*/
bool GenericDataProcessorPresenter::rowsValid(
    const std::map<int, std::set<int>> &groups) {

  for (auto group = groups.begin(); group != groups.end(); ++group) {

    const std::string groupNo =
        Mantid::Kernel::Strings::toString<int>(group->first + 1);

    auto runs = group->second;
    for (auto it = runs.begin(); it != runs.end(); ++it) {
      try {
        validateRow(group->first, *it);
      } catch (std::exception &ex) {
        // Allow two theta to be blank
        if (ex.what() ==
            std::string("Value for two theta could not be found in log."))
          continue;

        const std::string rowNo =
            Mantid::Kernel::Strings::toString<int>(*it + 1);
        m_mainPresenter->giveUserCritical("Error found in group " + groupNo +
                                              ", row " + rowNo + ":\n" +
                                              ex.what(),
                                          "Error");
        return false;
      }
    }
  }
  return true;
}

/**
Validate a row.
A row may pass validation, but it is not necessarily ready for processing.
@param groupNo : The group to which the row belongs
@param rowNo : The row in the model to validate
@throws std::invalid_argument if the row fails validation
*/
void GenericDataProcessorPresenter::validateRow(int groupNo, int rowNo) const {
  if (rowNo >= numRowsInGroup(groupNo))
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
Returns the name of the reduced workspace for a given row
@param group : The group to which the specified row belongs
@param row : The row
@param prefix : A prefix to be appended to the generated ws name
@throws std::runtime_error if the workspace could not be prepared
@returns : The name of the workspace
*/
std::string GenericDataProcessorPresenter::getReducedWorkspaceName(
    int group, int row, const std::string &prefix) {

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
  std::vector<std::string> names;

  for (int col = 0; col < m_columns; col++) {

    // Do we want to use this column to generate the name of the output ws?
    if (m_whitelist.showValue(col)) {

      // Get what's in the column
      const std::string valueStr =
          m_model->data(m_model->index(row, col, m_model->index(group, 0)))
              .toString()
              .toStdString();

      // If it's not empty, use it
      if (!valueStr.empty()) {

        // But we may have things like '1+2' which we want to replace with '1_2'
        std::vector<std::string> value;
        boost::split(value, valueStr, boost::is_any_of("+"));

        names.push_back(m_whitelist.prefix(col) +
                        boost::algorithm::join(value, "_"));
      }
    }
  } // Columns

  std::string wsname = prefix;
  wsname += boost::algorithm::join(names, "_");

  return wsname;
}

/**
Returns the name of the reduced workspace for a given row
@param group : The id of the group to post-process
@param rows : The set of rows that belong to the same group
@param prefix : A prefix to be appended to the generated ws name
@returns : The name of the workspace
*/
std::string GenericDataProcessorPresenter::getPostprocessedWorkspaceName(
    int group, const std::set<int> &rows, const std::string &prefix) {

  /* This method calculates, for a given set of rows, the name of the output
   * (post-processed) workspace */

  std::vector<std::string> outputNames;

  for (auto itRow = rows.begin(); itRow != rows.end(); ++itRow) {
    outputNames.push_back(getReducedWorkspaceName(group, *itRow));
  }
  return prefix + boost::join(outputNames, "_");
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
@param groupNo : The group to which the row belongs
@param rowNo : The row in the model to reduce
@throws std::runtime_error if reduction fails
*/
void GenericDataProcessorPresenter::reduceRow(int groupNo, int rowNo) {

  /* Create the processing algorithm */

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(m_processor.name());
  alg->initialize();

  /* Read input properties from the table */
  /* excluding 'Group' and 'Options' */

  // Global pre-processing options as a map
  std::map<std::string, std::string> globalOptions;
  if (!m_preprocessMap.empty())
    globalOptions = m_mainPresenter->getPreprocessingOptions();

  // Loop over all columns in the whitelist except 'Options'
  for (int i = 0; i < m_columns - 1; i++) {

    // The algorithm's property linked to this column
    auto propertyName = m_whitelist.algPropFromColIndex(i);
    // The column's name
    auto columnName = m_whitelist.colNameFromColIndex(i);

    if (m_preprocessMap.count(columnName)) {
      // This column needs pre-processing

      const std::string runStr =
          m_model->data(m_model->index(rowNo, i, m_model->index(groupNo, 0)))
              .toString()
              .toStdString();

      if (!runStr.empty()) {

        auto preprocessor = m_preprocessMap[columnName];

        // Global pre-processing options for this algorithm as a string
        const std::string options = globalOptions[columnName];

        auto optionsMap = parseKeyValueString(options);
        auto runWS = prepareRunWorkspace(runStr, preprocessor, optionsMap);
        alg->setProperty(propertyName, runWS->name());
      }
    } else {
      // No pre-processing needed, read from the table
      auto propertyValue =
          m_model->data(m_model->index(rowNo, i, m_model->index(groupNo, 0)))
              .toString()
              .toStdString();
      if (!propertyValue.empty())
        alg->setPropertyValue(propertyName, propertyValue);
    }
  }

  // Global processing options as a string
  std::string options = m_mainPresenter->getProcessingOptions();

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
  options = m_model->data(m_model->index(rowNo, m_model->columnCount() - 1,
                                         m_model->index(groupNo, 0)))
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
    alg->setProperty(
        m_processor.outputPropertyName(i),
        getReducedWorkspaceName(groupNo, rowNo, m_processor.prefix(i)));
  }

  /* Now run the processing algorithm */
  alg->execute();

  if (alg->isExecuted()) {

    /* The reduction is complete, try to populate the columns */
    for (int i = 0; i < m_columns - 1; i++) {
      if (m_model->data(m_model->index(rowNo, i, m_model->index(groupNo, 0)))
              .toString()
              .isEmpty()) {

        std::string propValue =
            alg->getPropertyValue(m_whitelist.algPropFromColIndex(i));

        m_model->setData(m_model->index(rowNo, i, m_model->index(groupNo, 0)),
                         QString::fromStdString(propValue));
      }
    }
  }
}

/**
Inserts a new row to the specified group in the specified location
@param groupIndex :: The index to insert the new row after
@param rowIndex :: The index to insert the new row after
*/
void GenericDataProcessorPresenter::insertRow(int groupIndex, int rowIndex) {

  m_model->insertRow(rowIndex, m_model->index(groupIndex, 0));
}

/**
Inserts a new group in the specified location
@param groupIndex :: The index to insert the new row after
*/
void GenericDataProcessorPresenter::insertGroup(int groupIndex) {

  m_model->insertRow(groupIndex);
}

/**
Insert a row after the last selected row. If a group was selected, the new row
is appended to that group. If nothing was selected, the new row is appended to
the last group in the table.
*/
void GenericDataProcessorPresenter::appendRow() {
  auto selectedGroups = m_view->getSelectedParents();
  auto selectedRows = m_view->getSelectedChildren();

  if (!selectedRows.empty()) {
    // Some rows were selected
    // Insert a row after last selected row

    int groupId = selectedRows.rbegin()->first;
    int lastSelectedRow = *(selectedRows[groupId].rbegin());
    insertRow(groupId, lastSelectedRow + 1);

  } else if (!selectedGroups.empty()) {
    // No rows were selected, but some groups were selected
    // Append row to last selected group

    int lastSelectedGroup = *(selectedGroups.rbegin());
    insertRow(lastSelectedGroup,
              m_model->rowCount(m_model->index(lastSelectedGroup, 0)));

  } else {
    // Nothing was selected

    if (m_model->rowCount() == 0) {
      // Model is empty, we cannot add a row
      return;
    }

    int groupId = m_model->rowCount() - 1;
    int rowId = m_model->rowCount(m_model->index(groupId, 0));

    // Add a new row to last group
    insertRow(groupId, rowId);
  }
  m_tableDirty = true;
}

/**
Insert a group after the last selected group
*/
void GenericDataProcessorPresenter::appendGroup() {
  auto selectedGroups = m_view->getSelectedParents();

  if (selectedGroups.empty()) {
    // Append group at the end of the table
    insertGroup(m_model->rowCount());
  } else {
    // Append group after last selected group
    insertGroup(*(selectedGroups.rbegin()) + 1);
  }
  m_tableDirty = true;
}

/**
Delete row(s) from the model
*/
void GenericDataProcessorPresenter::deleteRow() {
  auto selectedRows = m_view->getSelectedChildren();
  for (auto it = selectedRows.rbegin(); it != selectedRows.rend(); ++it) {
    const int groupId = it->first;
    auto rows = it->second;
    for (auto row = rows.rbegin(); row != rows.rend(); ++row) {
      m_model->removeRow(*row, m_model->index(groupId, 0));
    }
  }
  m_tableDirty = true;
}

/**
Delete group(s) from the model
*/
void GenericDataProcessorPresenter::deleteGroup() {
  auto selectedGroups = m_view->getSelectedParents();
  for (auto group = selectedGroups.rbegin(); group != selectedGroups.rend();
       ++group) {
    m_model->removeRow(*group);
  }
  m_tableDirty = true;
}

/**
Group rows together
*/
void GenericDataProcessorPresenter::groupRows() {

  // Find if rows belong to the same group
  // If they do, do nothing
  // If they don't, remove rows from their groups and add them to a
  // new group

  const auto selectedRows = m_view->getSelectedChildren();

  if (selectedRows.empty()) {
    // no rows were selected
    return;
  }

  // Append a new group where selected rows will be pasted (this will append a
  // group with an empty row)
  int groupId = m_model->rowCount();
  appendGroup();
  // Append as many rows as the number of selected rows minus one
  int rowsToAppend = -1;
  for (const auto &row : selectedRows)
    rowsToAppend += static_cast<int>(row.second.size());
  for (int i = 0; i < rowsToAppend; i++)
    insertRow(groupId, i);

  // Now we just have to set the data
  int rowIndex = 0;
  for (const auto &item : selectedRows) {
    int oldGroupId = item.first;
    auto rows = item.second;
    for (const auto &row : rows) {
      for (int col = 0; col < m_columns; col++) {
        auto value = m_model->data(
            m_model->index(row, col, m_model->index(oldGroupId, 0)));
        m_model->setData(
            m_model->index(rowIndex, col, m_model->index(groupId, 0)), value);
      }
      rowIndex++;
    }
  }

  // Now delete the rows
  deleteRow();

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
  const std::string userString = m_mainPresenter->askUserString(
      "Save As", "Enter a workspace name:", "Workspace");
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
    if (!m_mainPresenter->askUserYesNo(
            "Your current table has unsaved changes. Are you "
            "sure you want to discard them?",
            "Start New Table?"))
      return;

  m_ws = createDefaultWorkspace();
  m_model.reset(new QDataProcessorTwoLevelTreeModel(m_ws, m_whitelist));
  m_wsName.clear();
  m_view->showTable(m_model);

  m_tableDirty = false;
}

/**
Open a table from the ADS
*/
void GenericDataProcessorPresenter::openTable() {
  if (m_tableDirty && m_options["WarnDiscardChanges"].toBool())
    if (!m_mainPresenter->askUserYesNo(
            "Your current table has unsaved changes. Are you "
            "sure you want to discard them?",
            "Open Table?"))
      return;

  auto &ads = AnalysisDataService::Instance();
  const std::string toOpen = m_view->getWorkspaceToOpen();

  if (toOpen.empty())
    return;

  if (!ads.isValid(toOpen).empty()) {
    m_mainPresenter->giveUserCritical("Could not open workspace: " + toOpen,
                                      "Error");
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
    m_model.reset(new QDataProcessorTwoLevelTreeModel(m_ws, m_whitelist));
    m_wsName = toOpen;
    m_view->showTable(m_model);
    m_tableDirty = false;
  } catch (std::runtime_error &e) {
    m_mainPresenter->giveUserCritical(
        "Could not open workspace: " + std::string(e.what()), "Error");
  }
}

/**
Import a table from TBL file
*/
void GenericDataProcessorPresenter::importTable() {

  std::stringstream pythonSrc;
  pythonSrc << "try:\n";
  pythonSrc << "  algm = "
            << "LoadTBL"
            << "Dialog()\n";
  pythonSrc << "  print algm.getPropertyValue(\"OutputWorkspace\")\n";
  pythonSrc << "except:\n";
  pythonSrc << "  pass\n";

  const std::string result =
      m_mainPresenter->runPythonAlgorithm(pythonSrc.str());

  // result will hold the name of the output workspace
  // otherwise this should be an empty string.
  QString outputWorkspaceName = QString::fromStdString(result);
  auto toOpen = outputWorkspaceName.trimmed().toStdString();
  m_view->setModel(toOpen);
}

/**
Export a table to TBL file
*/
void GenericDataProcessorPresenter::exportTable() {

  std::stringstream pythonSrc;
  pythonSrc << "try:\n";
  pythonSrc << "  algm = SaveTBLDialog()\n";
  pythonSrc << "except:\n";
  pythonSrc << "  pass\n";

  m_mainPresenter->runPythonAlgorithm(pythonSrc.str());
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
  m_mainPresenter->notify(DataProcessorMainPresenter::ADSChangedFlag);
}

/**
Handle ADS remove events
*/
void GenericDataProcessorPresenter::postDeleteHandle(const std::string &name) {
  m_workspaceList.erase(name);
  m_view->setTableList(m_workspaceList);
  m_mainPresenter->notify(DataProcessorMainPresenter::ADSChangedFlag);
}

/**
Handle ADS clear events
*/
void GenericDataProcessorPresenter::clearADSHandle() {
  m_workspaceList.clear();
  m_view->setTableList(m_workspaceList);
  m_mainPresenter->notify(DataProcessorMainPresenter::ADSChangedFlag);
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
  m_mainPresenter->notify(DataProcessorMainPresenter::ADSChangedFlag);
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
@param group : The group to count the rows of
@returns The number of rows in the group
*/
int GenericDataProcessorPresenter::numRowsInGroup(int group) const {

  return m_model->rowCount(m_model->index(group, 0));
}

/** Expands the current selection to all the rows in the selected groups, this
 * effectively means selecting the parent item (i.e. the group to which the
 * selected rows belong) */
void GenericDataProcessorPresenter::expandSelection() {
  std::set<int> groupIds;

  auto items = m_view->getSelectedChildren();
  if (items.empty())
    return;

  for (auto group = items.begin(); group != items.end(); ++group)
    groupIds.insert(group->first);

  m_view->setSelection(groupIds);
}

/** Clear the currently selected rows */
void GenericDataProcessorPresenter::clearSelected() {
  const auto selectedRows = m_view->getSelectedChildren();

  for (const auto &item : selectedRows) {
    int group = item.first;
    auto rows = item.second;
    for (const auto &row : rows) {
      for (auto col = 0; col < m_model->columnCount(); col++)
        m_model->setData(m_model->index(row, col, m_model->index(group, 0)),
                         "");
    }
  }
  m_tableDirty = true;
}

/** Copy the currently selected rows to the clipboard */
void GenericDataProcessorPresenter::copySelected() {
  std::vector<std::string> lines;

  const auto selectedRows = m_view->getSelectedChildren();

  if (selectedRows.empty()) {
    m_view->setClipboard(std::string());
    return;
  }

  for (const auto &item : selectedRows) {
    const int group = item.first;
    auto rows = item.second;

    for (const auto &row : rows) {
      std::vector<std::string> line;
      line.push_back(std::to_string(group));

      for (int col = 0; col < m_columns; ++col) {
        line.push_back(
            m_model->data(m_model->index(row, col, m_model->index(group, 0)))
                .toString()
                .toStdString());
      }
      lines.push_back(boost::algorithm::join(line, "\t"));
    }
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

  if (text.empty())
    return;

  // Contains the data to paste plus the original group index in the first
  // element
  std::vector<std::string> lines;
  boost::split(lines, text, boost::is_any_of("\n"));

  // If we have rows selected, we'll overwrite them. If not, we'll append new
  // rows.
  const auto selectedRows = m_view->getSelectedChildren();
  if (selectedRows.empty()) {
    // No rows were selected
    // Use group where rows in clipboard belong and paste new rows to it
    // Add as many new rows as required
    for (size_t i = 0; i < lines.size(); ++i) {
      std::vector<std::string> values;
      boost::split(values, lines[i], boost::is_any_of("\t"));

      int groupId = boost::lexical_cast<int>(values.front());
      int rowId = numRowsInGroup(groupId);
      insertRow(groupId, rowId);
      for (int col = 0; col < m_columns; col++) {
        m_model->setData(m_model->index(rowId, col, m_model->index(groupId, 0)),
                         QString::fromStdString(values[col + 1]));
      }
    }
  } else {
    // Some rows were selected
    // Iterate over rows and lines simultaneously, stopping when we reach the
    // end of either
    auto lineIt = lines.begin();
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it) {
      const int groupId = it->first;
      auto rows = it->second;
      auto rowIt = rows.begin();
      for (; rowIt != rows.end() && lineIt != lines.end(); rowIt++, lineIt++) {
        std::vector<std::string> values;
        boost::split(values, *lineIt, boost::is_any_of("\t"));

        // Paste as many columns as we can from this line
        for (int col = 0;
             col < m_columns && col < static_cast<int>(values.size()); ++col)
          m_model->setData(
              m_model->index(*rowIt, col, m_model->index(groupId, 0)),
              QString::fromStdString(values[col + 1]));
      }
    }
  }
}

/** Transfers the selected runs in the search results to the processing table
* @param runs : [input] the set of runs to transfer as a vector of maps
*/
void GenericDataProcessorPresenter::transfer(
    const std::vector<std::map<std::string, std::string>> &runs) {

  auto newRows = runs;

  // Loop over the rows (vector elements)
  for (auto rowsIt = newRows.begin(); rowsIt != newRows.end(); ++rowsIt) {
    auto &row = *rowsIt;

    TableRow newRow = m_ws->appendRow();
    newRow << row["Group"];
    for (int i = 0; i < m_columns; i++)
      newRow << row[m_whitelist.colNameFromColIndex(i)];
  }

  m_model.reset(new QDataProcessorTwoLevelTreeModel(m_ws, m_whitelist));
  m_view->showTable(m_model);
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
  auto selectedRows = m_view->getSelectedChildren();

  if (selectedRows.empty())
    return;

  std::set<std::string> workspaces, notFound;
  for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it) {
    const int groupIdx = it->first;
    auto rows = it->second;
    for (auto row = rows.begin(); row != rows.end(); ++row) {
      const std::string wsName =
          getReducedWorkspaceName(groupIdx, *row, m_processor.prefix(0));
      if (AnalysisDataService::Instance().doesExist(wsName))
        workspaces.insert(wsName);
      else
        notFound.insert(wsName);
    }
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
        "The following workspaces were not plotted because "
        "they were not found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the rows you are trying "
            "to plot have been fully processed.",
        "Error plotting rows.");

  plotWorkspaces(workspaces);
}

/** Plots any currently selected groups */
void GenericDataProcessorPresenter::plotGroup() {
  auto selectedGroups = m_view->getSelectedParents();

  if (selectedGroups.empty())
    return;

  std::set<std::string> workspaces, notFound;

  // Now, get the rows belonging to the specified groups
  for (auto it = selectedGroups.begin(); it != selectedGroups.end(); ++it) {

    const int groupId = *it;
    const int numRows = numRowsInGroup(groupId);

    std::set<int> rows;
    for (int row = 0; row < numRows; ++row) {
      // Add this row to group 'group'
      rows.insert(row);
    }

    const std::string wsName =
        getPostprocessedWorkspaceName(groupId, rows, m_postprocessor.prefix());

    if (AnalysisDataService::Instance().doesExist(wsName))
      workspaces.insert(wsName);
    else
      notFound.insert(wsName);
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
        "The following workspaces were not plotted because "
        "they were not found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the groups you are "
            "trying to plot have been fully processed.",
        "Error plotting groups.");

  plotWorkspaces(workspaces);
}

/**
Plot a set of workspaces
* @param workspaces : [input] The list of workspaces as a set
*/
void GenericDataProcessorPresenter::plotWorkspaces(
    const std::set<std::string> &workspaces) {
  if (workspaces.empty())
    return;

  std::stringstream pythonSrc;
  pythonSrc << "base_graph = None\n";
  for (auto ws = workspaces.begin(); ws != workspaces.end(); ++ws)
    pythonSrc << "base_graph = plotSpectrum(\"" << *ws
              << "\", 0, True, window = base_graph)\n";

  pythonSrc << "base_graph.activeLayer().logLogAxes()\n";

  m_mainPresenter->runPythonAlgorithm(pythonSrc.str());
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

/** Tells the view which of the actions should be added to the toolbar
*/
void GenericDataProcessorPresenter::addActions() {

  auto commands = m_manager->publishCommands();
  std::vector<std::unique_ptr<DataProcessorCommand>> commandsToShow;
  for (size_t comm = 10; comm < commands.size(); comm++)
    commandsToShow.push_back(std::move(commands.at(comm)));
  m_view->addActions(std::move(commandsToShow));
}

/**
* Tells the view to load a table workspace
* @param name : [input] The workspace's name
*/
void GenericDataProcessorPresenter::setModel(std::string name) {
  m_view->setModel(name);
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
  m_mainPresenter->notify(DataProcessorMainPresenter::ADSChangedFlag);
}

/** Returs the list of valid workspaces currently in the ADS
* @return : The vector of workspaces (as commands)
*/
std::vector<DataProcessorCommand_uptr>
GenericDataProcessorPresenter::getTableList() {

  std::vector<DataProcessorCommand_uptr> workspaces;

  // Create a command for each of the workspaces in the ADS
  for (auto it = m_workspaceList.begin(); it != m_workspaceList.end(); ++it) {
    workspaces.push_back(std::move(
        Mantid::Kernel::make_unique<DataProcessorWorkspaceCommand>(this, *it)));
  }
  return workspaces;
}

}
}
