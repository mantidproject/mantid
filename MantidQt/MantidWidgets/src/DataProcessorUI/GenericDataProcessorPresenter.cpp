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
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOneLevelTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWorkspaceCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ParseKeyValueString.h"
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
* @param loader : The algorithm responsible for loading data
*/
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::string &loader)
    : WorkspaceObserver(), m_view(nullptr), m_progressView(nullptr),
      m_whitelist(whitelist), m_preprocessMap(preprocessMap),
      m_processor(processor), m_postprocessor(postprocessor), m_loader(loader),
      m_postprocess(true), m_mainPresenter(), m_tableDirty(false) {

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

  if (m_postprocessor.name().empty()) {
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
          whitelist,
          std::map<std::string, DataProcessorPreprocessingAlgorithm>(),
          processor, postprocessor) {}

/**
* Delegating constructor (no post-processing needed)
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A DataProcessorProcessingAlgorithm
* workspaces
*/
GenericDataProcessorPresenter::GenericDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
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
          whitelist,
          std::map<std::string, DataProcessorPreprocessingAlgorithm>(),
          processor, DataProcessorPostprocessingAlgorithm()) {}

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
Process selected data
*/
void GenericDataProcessorPresenter::process() {

  const auto items = m_manager->selectedData(true);

  // Progress: each group and each row within count as a progress step.
  int progress = 0;
  int maxProgress = (int)(items.size());
  for (const auto subitem : items) {
    maxProgress += (int)(subitem.second.size());
  }
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  for (const auto &item : items) {

    // Reduce rows sequentially

    for (const auto &data : item.second) {
      // item.second -> set of vectors containing data

      try {
        auto newData = reduceRow(data.second);
        m_manager->update(item.first, data.first, newData);
        progressReporter.report();

      } catch (std::exception &ex) {
        m_mainPresenter->giveUserCritical(ex.what(), "Error");
        progressReporter.clear();
        return;
      }

      progressReporter.report();
    }

    // Post-process (if needed)
    if (item.second.size() > 1) {
      try {
        postProcessGroup(item.second);
        progressReporter.report();
      } catch (std::exception &ex) {
        m_mainPresenter->giveUserCritical(ex.what(), "Error");
        progressReporter.clear();
        return;
      }
    }
  }

  // If "Output Notebook" checkbox is checked then create an ipython notebook
  if (m_view->getEnableNotebook()) {
    saveNotebook(items);
  }
}

/**
Display a dialog to choose save location for notebook, then save the notebook
there
@param data : the processed data
*/
void GenericDataProcessorPresenter::saveNotebook(const TreeData &data) {

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
      m_wsName, m_view->getProcessInstrument(), m_whitelist, m_preprocessMap,
      m_processor, m_postprocessor, preprocessingOptionsMap, processingOptions,
      postprocessingOptions);
  std::string generatedNotebook = notebook->generateNotebook(data);

  std::ofstream file(filename.c_str(), std::ofstream::trunc);
  file << generatedNotebook;
  file.flush();
  file.close();
}

/**
Post-processes the workspaces created by the given rows together.
@param groupData : the data in a given group as received from the tree manager
*/
void GenericDataProcessorPresenter::postProcessGroup(
    const GroupData &groupData) {

  if (!m_postprocess)
    throw std::runtime_error("Cannot post-process workspaces");

  // The input workspace names
  std::vector<std::string> inputNames;

  // The name to call the post-processed ws
  const std::string outputWSName =
      getPostprocessedWorkspaceName(groupData, m_postprocessor.prefix());

  // Go through each row and get the input ws names
  for (const auto &row : groupData) {

    // The name of the reduced workspace for this row
    const std::string inputWSName =
        getReducedWorkspaceName(row.second, m_processor.prefix(0));

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
@param data :: [input] The data for this row
@param prefix : A prefix to be appended to the generated ws name
@throws std::runtime_error if the workspace could not be prepared
@returns : The name of the workspace
*/
std::string GenericDataProcessorPresenter::getReducedWorkspaceName(
    const std::vector<std::string> &data, const std::string &prefix) {

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
  std::vector<std::string> names;

  for (int col = 0; col < m_columns; col++) {

    // Do we want to use this column to generate the name of the output ws?
    if (m_whitelist.showValue(col)) {

      // Get what's in the column
      const std::string valueStr = data.at(col);

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
@param groupData : The data in a given group
@param prefix : A prefix to be appended to the generated ws name
@returns : The name of the workspace
*/
std::string GenericDataProcessorPresenter::getPostprocessedWorkspaceName(
    const GroupData &groupData, const std::string &prefix) {

  if (!m_postprocess)
    throw std::runtime_error("Cannot retrieve post-processed workspace name");

  /* This method calculates, for a given set of rows, the name of the output
  * (post-processed) workspace */

  std::vector<std::string> outputNames;

  for (const auto &data : groupData) {
    outputNames.push_back(getReducedWorkspaceName(data.second));
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
  IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create(m_loader);
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
@param data :: [input] The data in this row as a vector where elements
correspond to column contents
@throws std::runtime_error if reduction fails
@return :: Data updated after running the algorithm
*/
std::vector<std::string>
GenericDataProcessorPresenter::reduceRow(const std::vector<std::string> &data) {

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

      const std::string runStr = data.at(i);

      if (!runStr.empty()) {

        auto preprocessor = m_preprocessMap[columnName];

        // Global pre-processing options for this algorithm as a string
        const std::string options = globalOptions[columnName];

        auto optionsMap = parseKeyValueString(options);
        auto runWS = prepareRunWorkspace(runStr, preprocessor, optionsMap);
        alg->setProperty(propertyName, runWS->name());
      }
    } else {
      // No pre-processing needed
      auto propertyValue = data.at(i);
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
  options = data.back();

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
                     getReducedWorkspaceName(data, m_processor.prefix(i)));
  }

  /* Now run the processing algorithm */
  alg->execute();

  auto newData = data;
  if (alg->isExecuted()) {

    /* The reduction is complete, try to populate the columns */
    for (int i = 0; i < m_columns - 1; i++) {

      auto columnName = m_whitelist.colNameFromColIndex(i);

      if (data.at(i).empty() && !m_preprocessMap.count(columnName)) {

        std::string propValue =
            alg->getPropertyValue(m_whitelist.algPropFromColIndex(i));

        if (m_options["Round"].toBool()) {
          propValue = propValue.substr(
              0, propValue.find(".") + m_options["RoundPrecision"].toInt() + 1);
        }

        newData[i] = propValue;
      }
    }
  }
  return newData;
}

/**
Insert a new row
*/
void GenericDataProcessorPresenter::appendRow() {

  m_manager->appendRow();
  m_tableDirty = true;
}

/**
Insert a new group
*/
void GenericDataProcessorPresenter::appendGroup() {

  m_manager->appendGroup();
  m_tableDirty = true;
}

/**
Delete row(s) from the model
*/
void GenericDataProcessorPresenter::deleteRow() {

  m_manager->deleteRow();
  m_tableDirty = true;
}

/**
Delete group(s) from the model
*/
void GenericDataProcessorPresenter::deleteGroup() {
  m_manager->deleteGroup();
  m_tableDirty = true;
}

/**
Group rows together
*/
void GenericDataProcessorPresenter::groupRows() {

  m_manager->groupRows();
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
        m_wsName, boost::shared_ptr<ITableWorkspace>(
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
    m_manager->isValidModel(newTable, m_whitelist.size());
    m_manager->newTable(newTable, m_whitelist);
    m_wsName = toOpen;
    m_view->showTable(m_manager->getModel());
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

  if (!m_manager->isValidModel(workspace, m_columns))
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
  if (m_manager->isValidModel(workspace, m_columns))
    m_workspaceList.insert(name);

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
  m_tableDirty = true;
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
  const std::string text = m_view->getClipboard();

  if (text.empty())
    return;

  m_manager->pasteSelected(text);
}

/** Transfers the selected runs in the search results to the processing table
* @param runs : [input] the set of runs to transfer as a vector of maps
*/
void GenericDataProcessorPresenter::transfer(
    const std::vector<std::map<std::string, std::string>> &runs) {

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
    const std::vector<std::string> &instruments,
    const std::string &defaultInstrument) {

  m_view->setInstrumentList(instruments, defaultInstrument);
}

/** Plots any currently selected rows */
void GenericDataProcessorPresenter::plotRow() {

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  const auto items = m_manager->selectedData();

  for (const auto &item : items) {
    for (const auto &run : item.second) {

      const std::string wsName =
          getReducedWorkspaceName(run.second, m_processor.prefix(0));

      if (AnalysisDataService::Instance().doesExist(wsName))
        workspaces.insert(wsName);
      else
        notFound.insert(wsName);
    }
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
        "The following workspaces were not plotted because they were not "
        "found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the rows you are trying to plot have been "
            "fully processed.",
        "Error plotting rows.");

  plotWorkspaces(workspaces);
}

/** Plots any currently selected groups */
void GenericDataProcessorPresenter::plotGroup() {

  // This method shouldn't be called if a post-processing algorithm is not
  // defined
  if (!m_postprocess)
    throw std::runtime_error("Can't plot group.");

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  const auto items = m_manager->selectedData();

  for (const auto &item : items) {

    if (item.second.size() > 1) {

      const std::string wsName =
          getPostprocessedWorkspaceName(item.second, m_postprocessor.prefix());

      if (AnalysisDataService::Instance().doesExist(wsName))
        workspaces.insert(wsName);
      else
        notFound.insert(wsName);
    }
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
        "The following workspaces were not plotted because they were not "
        "found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the groups you are trying to plot have been "
            "fully processed.",
        "Error plotting rows.");

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
bool GenericDataProcessorPresenter::askUserYesNo(
    const std::string &prompt, const std::string &title) const {

  return m_mainPresenter->askUserYesNo(prompt, title);
}

/** Print warning message
 * @param prompt :: the message
 * @param title :: the title
 */
void GenericDataProcessorPresenter::giveUserWarning(
    const std::string &prompt, const std::string &title) const {

  m_mainPresenter->giveUserWarning(prompt, title);
}
}
}
