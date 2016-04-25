#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorGenerateNotebook.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtCustomInterfaces/ParseKeyValueString.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflVectorString.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <memory>
#include <sstream>

namespace MantidQt {
namespace CustomInterfaces {

/*
 * Split the input string on commas and trim leading and trailing whitespace
 * from the results
 */
std::vector<std::string> splitByCommas(const std::string &names_string) {
  std::vector<std::string> names_split_by_commas;
  boost::split(names_split_by_commas, names_string, boost::is_any_of(","));

  // Remove leading/trailing whitespace from each resulting string
  for (auto &name : names_split_by_commas) {
    boost::trim(name);
  }
  return names_split_by_commas;
}

DataProcessorGenerateNotebook::DataProcessorGenerateNotebook(
    std::string name, QDataProcessorTableModel_sptr model,
    const std::string instrument, const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
    const DataProcessorAlgorithm &processor,
    const DataPostprocessorAlgorithm &postprocessor)
    : m_wsName(name), m_model(model), m_instrument(instrument),
      m_whitelist(whitelist), m_preprocessMap(preprocessMap),
      m_processor(processor), m_postprocessor(postprocessor) {}

/**
  Generate an ipython notebook
  @param groups : groups of rows which were stitched
  @param rows : rows which were processed
  @returns ipython notebook string
  */
std::string DataProcessorGenerateNotebook::generateNotebook(
    std::map<int, std::set<int>> groups, std::set<int> rows) {

  auto notebook = Mantid::Kernel::make_unique<Mantid::API::NotebookWriter>();

  notebook->markdownCell(titleString(m_wsName));

  notebook->markdownCell(tableString(m_model, m_whitelist, rows));

  int groupNo = 1;
  for (auto gIt = groups.begin(); gIt != groups.end(); ++gIt, ++groupNo) {
    const std::set<int> groupRows = gIt->second;

    /** Announce the stitch group in the notebook **/

    std::ostringstream group_title_string;
    group_title_string << "Group " << groupNo;
    notebook->markdownCell(group_title_string.str());

    /**  Reduce all rows **/

    // The python code
    std::ostringstream code_string;
    // A vector to store the output ws produced during the reduction process
    // In the case of Reflectometry those will be the IvsQ_ and IvsLam
    // workspaces
    std::vector<std::string> output_ws;

    code_string << "#Load and reduce\n";
    for (auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt) {
      auto reduce_row_string =
          reduceRowString(*rIt, m_instrument, m_model, m_whitelist,
                          m_preprocessMap, m_processor);
      // The reduction code
      code_string << boost::get<0>(reduce_row_string);
      // The output workspace names
      output_ws.emplace_back(boost::get<1>(reduce_row_string));
    }
    notebook->codeCell(code_string.str());

    /** Post-process group **/

    boost::tuple<std::string, std::string> postprocess_string =
        postprocessGroupString(groupRows, m_instrument, m_model, m_whitelist,
                               m_preprocessMap, m_processor, m_postprocessor);
    notebook->codeCell(boost::get<0>(postprocess_string));

    /** Draw plots **/

    notebook->codeCell(
        plotsString(output_ws, boost::get<1>(postprocess_string), m_processor));
  }

  return notebook->writeNotebook();
}

/**
  Create string of markdown code for title of the data processing part of the
  notebook
  @param wsName : name of the table workspace
  @return string containing markdown code
  */
std::string titleString(const std::string &wsName) {
  std::string title_string;

  if (!wsName.empty()) {
    title_string =
        "Processed data from workspace: " + wsName + "\n---------------";
  } else {
    title_string = "Processed data\n---------------";
  }
  title_string +=
      "\nNotebook generated from the ISIS Reflectometry (Polref) Interface";

  return title_string;
}

/**
  Create string of python code to call plots() with the required workspaces
  @param output_ws : vector containing all the output workspaces produced during
  the reduction
  @param stitched_wsStr : name of post-processed data workspace
  @return string containing the python code
  */
std::string plotsString(const std::vector<std::string> &output_ws,
                        const std::string &stitched_wsStr,
                        const DataProcessorAlgorithm &processor) {

  std::ostringstream plot_string;

  // First, we have to parse 'output_ws'
  // This is a vector containing all the output workspace produced during the
  // reduction for a specific group
  // For example, in the Reflectometry case, assuming the following table (only
  // relevant information is shown below):
  // Run(s), Group
  // 13460,  0
  // 13462,  0
  // 13463,  0
  // output_ws will be:
  // output_ws [0] = 'IvsQ_TOF_13460, IvsLam_TOF_13460'
  // output_ws [1] = 'IvsQ_TOF_13462, IvsLam_TOF_13462'
  // output_ws [3] = 'IvsQ_TOF_13463, IvsLam_TOF_13463'
  // As the reduction algorithm, ReflectometryReductionOneAuto, produces two
  // output workspaces
  // We need to group the 'IvsQ_' workspaces and the 'IvsLam_' workspaces

  plot_string << "#Group workspaces to be plotted on same axes\n";

  // Group workspaces which should be plotted on same axes
  // But first we need to determine how many groups of ws we have
  // We can do that from the processing algorithm
  size_t nGroups = processor.outputProperties();

  // A vector to store the list of groups
  std::vector<std::string> workspaceList;

  // Now we iterate through groups to get the relevant workspace
  for (size_t group = 0; group < nGroups; group++) {

    // From the reduction (processing) algorithm, get the prefix for the
    // output workspace, we'll use it to give name to this group
    std::string prefix = processor.prefix(group);

    plot_string << prefix << "groupWS = GroupWorkspaces(InputWorkspaces = '";

    // Save this group to workspaceList
    workspaceList.emplace_back(prefix + "groupWS");

    std::vector<std::string> wsNames;

    // Iterate through the elements of output_ws
    for (auto &outws : output_ws) {

      auto workspaces = splitByCommas(outws);

      // Get the workspace we need for this group
      wsNames.push_back(workspaces[group]);
    }

    plot_string << boost::algorithm::join(wsNames, ", ") << "')\n";
  }

  // Add the post-processed workspace to the list of workspaces to plot
  workspaceList.push_back(stitched_wsStr);

  // Plot I vs Q and I vs Lambda graphs
  plot_string << "#Plot workspaces\n";

  plot_string << plot1DString(workspaceList);

  return plot_string.str();
}

/**
  Create string of markdown code to display a table of data from the GUI
  @param model : tablemodel for the full table
  @param col_nums : column number for each column title
  @param rows : rows from full table to include
  @return string containing the markdown code
  */
std::string tableString(QDataProcessorTableModel_sptr model,
                        const DataProcessorWhiteList &whitelist,
                        const std::set<int> &rows) {
  std::ostringstream table_string;

  const int ncols = static_cast<int>(whitelist.size());

  for (int i = 0; i < ncols - 1; i++) {
    table_string << whitelist.colNameFromColIndex(i) << " | ";
  }
  table_string << whitelist.colNameFromColIndex(ncols - 1) << "\n";
  for (int i = 0; i < ncols - 1; i++) {
    table_string << "---"
                 << " | ";
  }
  table_string << "---"
               << "\n";

  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {

    for (int col = 0; col < ncols - 1; col++)
      table_string
          << model->data(model->index(*rowIt, col)).toString().toStdString()
          << " | ";

    table_string
        << model->data(model->index(*rowIt, ncols - 1)).toString().toStdString()
        << "\n";
  }

  return table_string.str();
}

/**
  Create string of python code to post-process workspaces in the same group
  @param rows : rows in the group
  @param instrument : name of the instrument
  @param model : table model containing details of runs and processing settings
  @param col_nums : column numbers used to find data in model
  @return tuple containing the python code string and the output workspace name
  */
boost::tuple<std::string, std::string> postprocessGroupString(
    const std::set<int> &rows, const std::string &instrument,
    QDataProcessorTableModel_sptr model,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
    const DataProcessorAlgorithm &processor,
    const DataPostprocessorAlgorithm &postprocessor) {
  std::ostringstream stitch_string;

  stitch_string << "#Post-process workspaces\n";

  // If we can get away with doing nothing, do.
  if (rows.size() < 2)
    return boost::make_tuple("", "");

  // Properties for post-processing algorithm
  // Vector containing the list of input workspaces
  std::vector<std::string> inputNames;
  // Vector containing the different bits of the output ws name
  std::vector<std::string> outputName;

  // Go through each row and prepare the input and output properties
  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {

    // The reduced ws name without prefix (for example 'TOF_13460_13462')
    auto suffix = getWorkspaceName(*rowIt, model, whitelist, preprocessMap,
                                   processor, false);

    // The reduced ws name: 'IvsQ_TOF_13460_13462'
    inputNames.emplace_back(processor.prefix(0) + suffix);
    // Add the suffix (i.e. 'TOF_13460_13462') to the output ws name
    outputName.emplace_back(suffix);
  }

  std::string outputWSName =
      postprocessor.prefix() + boost::algorithm::join(outputName, "_");
  stitch_string << outputWSName << " = ";
  stitch_string << postprocessor.name() << "(";
  stitch_string << postprocessor.inputProperty() << "='";
  stitch_string << boost::algorithm::join(inputNames, ", ") << "')";

  // TODO: Now options for post-processing

  return boost::make_tuple(stitch_string.str(), outputWSName);
}

/**
  Create string of python code to create 1D plots from workspaces
  @param ws_names : vector of workspace names to plot
  @return string  of python code to plot I vs Q
  */
std::string plot1DString(const std::vector<std::string> &ws_names) {

  std::ostringstream plot_string;

  plot_string << "fig = plots([" << vectorString(ws_names) << "], title="
              << "['";
  plot_string << boost::algorithm::join(ws_names, "', '");
  plot_string << "']"
              << ", legendLocation=[1, 1, 4])\n";

  return plot_string.str();
}

/**
 Constructs the name for the reduced workspace
 @param rowNo : the row
 @param whitelist : the whitelist
 @param preprocessMap : the pre-processing instructions as a map
 @param prefix : wheter to return the name with the prefix or not
 @return : the workspace name
*/
std::string getWorkspaceName(
    int rowNo, QDataProcessorTableModel_sptr model,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
    const DataProcessorAlgorithm &processor, bool prefix) {

  // The name of the output workspaces, e.g. 'TOF_13462', 'TRANS_13463', etc
  std::vector<std::string> workspaceNames;

  int ncols = static_cast<int>(whitelist.size());

  // Run through columns, excluding 'Group' and 'Options'
  for (int col = 0; col < ncols - 2; col++) {

    const std::string colName = whitelist.colNameFromColIndex(col);

    // We only included in the ws name pre-processed columns
    if (preprocessMap.count(colName)) {

      auto preprocessor = preprocessMap.at(colName);

      // Did we include this bit in the output ws name?
      if (preprocessor.show()) {
        // The runs
        const std::string runStr =
            model->data(model->index(rowNo, col)).toString().toStdString();

        if (!runStr.empty()) {

          std::vector<std::string> runs;
          boost::split(runs, runStr, boost::is_any_of("+"));

          workspaceNames.emplace_back(preprocessor.prefix() +
                                      boost::algorithm::join(runs, "_"));
        }
      }
    }
  }
  if (prefix)
    return processor.prefix(0) + boost::algorithm::join(workspaceNames, "_");
  else
    return boost::algorithm::join(workspaceNames, "_");
}

/**
 Create string of python code to run pre-processing and reduction algorithms on
 the specified row
 @param rowNo : the row in the model to run the pre-processing and reduction
 algorithms on
 @param instrument : name of the instrument
 @param model : table model containing details of runs and processing settings
 @param whitelist : the whitelist containing information about the model columns
 and how they relate to the algorithm properties
 @param preprocessMap : the pre-processing instructions as a map
 @param processor : the processing algorithm
 @return tuple containing the python string and the output workspace names
*/
boost::tuple<std::string, std::string> reduceRowString(
    const int rowNo, const std::string &instrument,
    QDataProcessorTableModel_sptr model,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataPreprocessorAlgorithm> &preprocessMap,
    const DataProcessorAlgorithm &processor) {

  std::ostringstream preprocess_string;

  // Vector to store the algorithm properties with values
  // For example
  // InputWorkspace = 'TOF_13460_13463'
  // ThetaIn = 0.2
  // etc
  std::vector<std::string> algProperties;

  int ncols = static_cast<int>(whitelist.size());

  // Run through columns, excluding 'Group' and 'Options'
  for (int col = 0; col < ncols - 2; col++) {

    // The column's name
    const std::string colName = whitelist.colNameFromColIndex(col);
    // The algorithm property linked to this column
    const std::string algProp = whitelist.algPropFromColIndex(col);

    if (preprocessMap.count(colName)) {
      // This column was pre-processed, we need to print pre-processing
      // instructions

      // Get the runs
      const std::string runStr =
          model->data(model->index(rowNo, col)).toString().toStdString();

      if (!runStr.empty()) {
        // Some runs were given for pre-processing

        // The pre-processing alg
        const DataPreprocessorAlgorithm preprocessor =
            preprocessMap.at(colName);
        // Python code ran to load and pre-process runs
        const boost::tuple<std::string, std::string> load_ws_string =
            loadWorkspaceString(runStr, instrument, preprocessor);
        // Populate preprocess_string
        preprocess_string << boost::get<0>(load_ws_string);

        // Add runs to reduction properties
        algProperties.push_back(algProp + " = '" +
                                boost::get<1>(load_ws_string) + "'");
      }
    } else {
      // No pre-processing

      // Just read the property value from the table
      const std::string propStr =
          model->data(model->index(rowNo, col)).toString().toStdString();
      if (!propStr.empty()) {
        // If it was not empty, we used it as an input property to the reduction
        // algorithm
        algProperties.push_back(algProp + " = " + propStr);
      }
    }
  }

  // 'Options' column
  const std::string options =
      model->data(model->index(rowNo, ncols - 1)).toString().toStdString();
  // Parse and set any user-specified options
  auto optionsMap = parseKeyValueString(options);
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    algProperties.push_back(kvp->first + " = " + kvp->second);
  }

  /* Now construct the output workspace names */

  // The output ws suffix, for example 'TOF_13460_13462'
  const std::string outputName = getWorkspaceName(
      rowNo, model, whitelist, preprocessMap, processor, false);

  // Vector containing the output ws names
  // For example
  // 'IvsQ_TOF_13460_13462',
  // 'IvsLam_TOF_13460_13462
  std::vector<std::string> output_properties;
  for (size_t prop = 0; prop < processor.outputProperties(); prop++) {
    output_properties.push_back(processor.prefix(prop) + outputName);
  }
  std::string outputPropertiesStr =
      boost::algorithm::join(output_properties, ", ");

  // Populate process_string
  std::ostringstream process_string;
  process_string << outputPropertiesStr;
  process_string << " = " << processor.name() << "(";
  process_string << boost::algorithm::join(algProperties, ", ");
  process_string << ")";

  // Populate code_string, which contains both pre-processing and processing
  // python code
  std::ostringstream code_string;
  code_string << preprocess_string.str();
  code_string << process_string.str();

  // Return the python code + the output properties
  return boost::make_tuple(code_string.str(), outputPropertiesStr);
}

/**
 Create string of python code to load workspaces
 @param runStr : string of workspaces to load
 @param instrument : name of the instrument
 @param preprocessor : the pre-processing algorithm
 @return : tuple of strings of python code and output workspace name
*/
boost::tuple<std::string, std::string>
loadWorkspaceString(const std::string &runStr, const std::string &instrument,
                    const DataPreprocessorAlgorithm &preprocessor) {

  std::vector<std::string> runs;
  boost::split(runs, runStr, boost::is_any_of("+"));

  std::ostringstream load_strings;

  // Remove leading/trailing whitespace from each run
  for (auto runIt = runs.begin(); runIt != runs.end(); ++runIt)
    boost::trim(*runIt);

  const std::string prefix = preprocessor.prefix();
  const std::string outputName = prefix + boost::algorithm::join(runs, "_");

  boost::tuple<std::string, std::string> load_string;

  load_string = loadRunString(runs[0], instrument, prefix);
  load_strings << boost::get<0>(load_string);

  // EXIT POINT if there is only one run
  if (runs.size() == 1) {
    return boost::make_tuple(load_strings.str(), boost::get<1>(load_string));
  }
  load_strings << outputName << " = " << boost::get<1>(load_string) << "\n";

  // Load each subsequent run and add it to the first run
  for (auto runIt = std::next(runs.begin()); runIt != runs.end(); ++runIt) {
    load_string = loadRunString(*runIt, instrument, prefix);
    load_strings << boost::get<0>(load_string);
    load_strings << plusString(boost::get<1>(load_string), outputName,
                               preprocessor);
  }

  return boost::make_tuple(load_strings.str(), outputName);
}

/**
 Create string of python code to run the Plus algorithm on specified workspaces
 @param input_name : name of workspace to add to the other workspace
 @param output_name : other workspace will be added to the one with this name
 @param preprocessor : the preprocessor algorithm
 @return string of python code
*/
std::string plusString(const std::string &input_name,
                       const std::string &output_name,
                       const DataPreprocessorAlgorithm &preprocessor) {
  std::ostringstream plus_string;

  plus_string << output_name << " = " << preprocessor.name() << "(";
  plus_string << preprocessor.firstInputProperty() << " = '" << output_name
              << "', ";
  plus_string << preprocessor.secondInputProperty() << " = '" << input_name;
  plus_string << "')\n";

  return plus_string.str();
}

/**
 Create string of python code to load a single workspace
 @param run : run to load
 @param instrument : name of the instrument
 @return tuple of strings of python code and output workspace name
*/
boost::tuple<std::string, std::string>
loadRunString(const std::string &run, const std::string &instrument,
              const std::string &prefix) {
  std::ostringstream load_string;
  // We do not have access to AnalysisDataService from notebook, so must load
  // run from file
  const std::string filename = instrument + run;
  const std::string ws_name = prefix + run;
  load_string << ws_name << " = ";
  load_string << "Load(";
  load_string << "Filename = '" << filename << "'";
  load_string << ")\n";

  return boost::make_tuple(load_string.str(), ws_name);
}
}
}
