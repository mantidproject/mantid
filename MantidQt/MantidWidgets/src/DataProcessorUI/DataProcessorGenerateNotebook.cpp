#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGenerateNotebook.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorVectorString.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ParseKeyValueString.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <memory>
#include <sstream>

namespace MantidQt {
namespace MantidWidgets {

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

/**
Constructor
@param name : the name of the table workspace used for the reduction
@param instrument : the instrument
@param whitelist : the white list defining the columns
@param preprocessMap : a map indicating which columns were pre-processed and the
corresponding pre-processing algorithms
@param processor : the reduction algorithm
@param postprocessor : the post-processing algorithm
@param preprocessingOptionsMap : options to pre-processing algorithms
specified via hinting line edits in the view
@param processingOptions : options to the reduction algorithm specified via
the corresponding hinting line edit in the view
@param postprocessingOptions : options to the post-processing algorithm
specified via the corresponding hinting line edit in the view
@returns ipython notebook string
*/
DataProcessorGenerateNotebook::DataProcessorGenerateNotebook(
    std::string name, const std::string instrument,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::map<std::string, std::string> preprocessingOptionsMap,
    const std::string processingOptions,
    const std::string postprocessingOptions)
    : m_wsName(name), m_instrument(instrument), m_whitelist(whitelist),
      m_preprocessMap(preprocessMap), m_processor(processor),
      m_postprocessor(postprocessor),
      m_preprocessingOptionsMap(preprocessingOptionsMap),
      m_processingOptions(processingOptions),
      m_postprocessingOptions(postprocessingOptions) {

  // Some checks
  if (m_whitelist.size() < 2)
    throw std::invalid_argument(
        "A valid WhiteList must have at least two columns");
}

/**
  Generate an ipython notebook
  @param data : the processed data
  @returns ipython notebook string
  */
std::string
DataProcessorGenerateNotebook::generateNotebook(const TreeData &data) {

  auto notebook = Mantid::Kernel::make_unique<Mantid::API::NotebookWriter>();

  notebook->markdownCell(titleString(m_wsName));

  notebook->markdownCell(tableString(data, m_whitelist));

  for (const auto &item : data) {

    const int groupId = item.first;
    const auto rowMap = item.second;

    /** Announce the stitch group in the notebook **/

    std::ostringstream group_title_string;
    group_title_string << "Group " << groupId;
    notebook->markdownCell(group_title_string.str());

    /**  Reduce all rows **/

    // The python code
    std::ostringstream code_string;
    // A vector to store the output ws produced during the reduction process
    // In the case of Reflectometry those will be the IvsQ_ and IvsLam
    // workspaces
    std::vector<std::string> output_ws;

    for (const auto &row : rowMap) {
      code_string << "#Load and reduce\n";

      auto reduce_row_string = reduceRowString(
          row.second, m_instrument, m_whitelist, m_preprocessMap, m_processor,
          m_preprocessingOptionsMap, m_processingOptions);

      // The reduction code
      code_string << boost::get<0>(reduce_row_string);
      // The output workspace names
      output_ws.emplace_back(boost::get<1>(reduce_row_string));
    }
    notebook->codeCell(code_string.str());

    /** Post-process string **/
    boost::tuple<std::string, std::string> postprocess_string;
    if (rowMap.size() > 1) {
      // If there was only one run selected, it could not be post-processed
      postprocess_string =
          postprocessGroupString(rowMap, m_whitelist, m_processor,
                                 m_postprocessor, m_postprocessingOptions);
    }
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

  return title_string;
}

/**
  Create string of python code to call plots() with the required workspaces
  @param output_ws : vector containing all the output workspaces produced during
  the reduction
  @param stitched_wsStr : string containing the name of the stitched
  (post-processed workspace)
  @param processor : the data processor algorithm
  @return string containing the python code
  */
std::string plotsString(const std::vector<std::string> &output_ws,
                        const std::string &stitched_wsStr,
                        const DataProcessorProcessingAlgorithm &processor) {

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
  // output_ws [0] = 'IvsQ_binned_TOF_13460, IvsQ_TOF_13460, IvsLam_TOF_13460'
  // output_ws [1] = 'IvsQ_binned_TOF_13462, IvsQ_TOF_13462, IvsLam_TOF_13462'
  // output_ws [3] = 'IvsQ_binned_TOF_13463, IvsQ_TOF_13463, IvsLam_TOF_13463'
  // As the reduction algorithm, ReflectometryReductionOneAuto, produces two
  // output workspaces
  // We need to group the 'IvsQ_' workspaces and the 'IvsLam_' workspaces

  plot_string << "#Group workspaces to be plotted on same axes\n";

  // Group workspaces which should be plotted on same axes
  // But first we need to determine how many groups of ws we have
  // We can do that from the processing algorithm
  size_t nGroups = processor.numberOfOutputProperties();

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
    for (const auto &outws : output_ws) {

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
  @param treeData : the processed data
  @param whitelist : the whitelist defining the table columns
  @return string containing the markdown code
  */
std::string tableString(const TreeData &treeData,
                        const DataProcessorWhiteList &whitelist) {

  std::ostringstream table_string;

  const int ncols = static_cast<int>(whitelist.size());

  table_string << "Group | ";
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

  for (const auto &group : treeData) {

    auto groupId = group.first;
    auto rowMap = group.second;

    for (const auto &row : rowMap) {

      std::vector<std::string> values;
      values.push_back(std::to_string(groupId));

      if (row.second.size() != whitelist.size())
        throw std::invalid_argument("Can't generate table for notebook");

      for (const auto &datum : row.second)
        values.push_back(datum);

      table_string << boost::algorithm::join(values, " | ");
      table_string << "\n";
    }
  }
  return table_string.str();
}

/**
  Create string of python code to post-process rows in the same group
  @param rowMap : map where keys are row indices and values are vectors
  containing the data
  @param whitelist : the whitelist
  @param processor : the reduction algorithm
  @param postprocessor : the algorithm responsible for post-processing
  groups
  @param postprocessingOptions : options specified for post-processing via
  HintingLineEdit
  @return tuple containing the python code string and the output workspace name
  */
boost::tuple<std::string, std::string> postprocessGroupString(
    const GroupData &rowMap, const DataProcessorWhiteList &whitelist,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::string &postprocessingOptions) {
  std::ostringstream stitch_string;

  stitch_string << "#Post-process workspaces\n";

  // Properties for post-processing algorithm
  // Vector containing the list of input workspaces
  std::vector<std::string> inputNames;
  // Vector containing the different bits of the output ws name
  std::vector<std::string> outputName;

  // Go through each row and prepare the input and output properties
  for (const auto &row : rowMap) {

    // The reduced ws name without prefix (for example 'TOF_13460_13462')
    auto suffix = getReducedWorkspaceName(row.second, whitelist);

    // The reduced ws name: 'IvsQ_TOF_13460_13462'
    inputNames.emplace_back(processor.prefix(0) + suffix);
    // Add the suffix (i.e. 'TOF_13460_13462') to the output ws name
    outputName.emplace_back(suffix);
  }

  std::string outputWSName =
      postprocessor.prefix() + boost::algorithm::join(outputName, "_");
  stitch_string << outputWSName;
  stitch_string << completeOutputProperties(
                       postprocessor.name(),
                       postprocessor.numberOfOutputProperties()) << " = ";
  stitch_string << postprocessor.name() << "(";
  stitch_string << postprocessor.inputProperty() << " = '";
  stitch_string << boost::algorithm::join(inputNames, ", ") << "'";
  if (!postprocessingOptions.empty())
    stitch_string << ", " << postprocessingOptions << ")";

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
 @param data : vector containing the data used in the reduction
 @param whitelist : the whitelist
 @param prefix : wheter to return the name with the prefix or not
 @return : the workspace name
*/
std::string getReducedWorkspaceName(const RowData &data,
                                    const DataProcessorWhiteList &whitelist,
                                    const std::string &prefix) {

  if (data.size() != whitelist.size())
    throw std::invalid_argument(
        "Can't write output workspace name to notebook");

  std::vector<std::string> names;

  int ncols = static_cast<int>(whitelist.size());

  for (int col = 0; col < ncols - 1; col++) {

    // Do we want to use this column to generate the name of the output ws?
    if (whitelist.showValue(col)) {

      // Get what's in the column
      const std::string valueStr = data.at(col);

      // If it's not empty, use it
      if (!valueStr.empty()) {

        // But we may have things like '1+2' which we want to replace with '1_2'
        std::vector<std::string> value;
        boost::split(value, valueStr, boost::is_any_of("+,"));

        names.push_back(whitelist.prefix(col) +
                        boost::algorithm::join(value, "_"));
      }
    }
  } // Columns

  std::string wsname = prefix;
  wsname += boost::algorithm::join(names, "_");

  return wsname;
}

/**
 Create string of python code to run pre-processing and reduction algorithms on
 the specified row
 @param data : the data used in the reduction
 @param instrument : name of the instrument
 @param whitelist : the whitelist
 @param preprocessMap : the pre-processing instructions as a map
 @param processor : the processing algorithm
 @param preprocessingOptionsMap : a map containing the pre-processing options
 @param processingOptions : the pre-processing options specified via hinting
 line edit
 @return tuple containing the python string and the output workspace names.
 First item in the tuple is the python code that performs the reduction, and
 second item are the names of the output workspaces.
*/
boost::tuple<std::string, std::string> reduceRowString(
    const RowData &data, const std::string &instrument,
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const std::map<std::string, std::string> &preprocessingOptionsMap,
    const std::string &processingOptions) {

  if (whitelist.size() != data.size()) {
    throw std::invalid_argument("Can't generate notebook");
  }

  std::ostringstream preprocess_string;

  // Vector to store the algorithm properties with values
  // For example
  // InputWorkspace = 'TOF_13460_13463'
  // ThetaIn = 0.2
  // etc
  std::vector<std::string> algProperties;

  int ncols = static_cast<int>(whitelist.size());

  // Run through columns, excluding 'Options'
  for (int col = 0; col < ncols - 1; col++) {

    // The column's name
    const std::string colName = whitelist.colNameFromColIndex(col);
    // The algorithm property linked to this column
    const std::string algProp = whitelist.algPropFromColIndex(col);

    if (preprocessMap.count(colName)) {
      // This column was pre-processed, we need to print pre-processing
      // instructions

      // Get the runs
      const std::string runStr = data.at(col);

      if (!runStr.empty()) {
        // Some runs were given for pre-processing

        // The pre-processing alg
        const DataProcessorPreprocessingAlgorithm preprocessor =
            preprocessMap.at(colName);
        // The pre-processing options
        const std::string options = preprocessingOptionsMap.count(colName) > 0
                                        ? preprocessingOptionsMap.at(colName)
                                        : "";
        // Python code ran to load and pre-process runs
        const boost::tuple<std::string, std::string> load_ws_string =
            loadWorkspaceString(runStr, instrument, preprocessor, options);
        // Populate preprocess_string
        preprocess_string << boost::get<0>(load_ws_string);

        // Add runs to reduction properties
        algProperties.push_back(algProp + " = '" +
                                boost::get<1>(load_ws_string) + "'");
      }
    } else {
      // No pre-processing

      // Just read the property value from the table
      const std::string propStr = data.at(col);

      if (!propStr.empty()) {
        // If it was not empty, we used it as an input property to the reduction
        // algorithm
        algProperties.push_back(algProp + " = " + propStr);
      }
    }
  }

  // 'Options' specified either via 'Options' column or HintinLineEdit
  auto options = parseKeyValueString(processingOptions);
  const std::string optionsStr = data.back();
  // Parse and set any user-specified options
  auto optionsMap = parseKeyValueString(optionsStr);
  // Options specified via 'Options' column will be preferred
  optionsMap.insert(options.begin(), options.end());
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    algProperties.push_back(kvp->first + " = " + kvp->second);
  }

  /* Now construct the names of the reduced workspaces*/

  // Vector containing the output ws names
  // For example
  // 'IvsQ_TOF_13460_13462',
  // 'IvsLam_TOF_13460_13462
  std::vector<std::string> output_properties;
  for (size_t prop = 0; prop < processor.numberOfOutputProperties(); prop++) {
    output_properties.push_back(
        getReducedWorkspaceName(data, whitelist, processor.prefix(prop)));
  }

  std::string outputPropertiesStr =
      boost::algorithm::join(output_properties, ", ");

  // Populate process_string
  std::ostringstream process_string;
  process_string << outputPropertiesStr;
  process_string << completeOutputProperties(
      processor.name(), processor.numberOfOutputProperties());
  process_string << " = " << processor.name() << "(";
  process_string << boost::algorithm::join(algProperties, ", ");
  process_string << ")";

  // Populate code_string, which contains both pre-processing and processing
  // python code
  std::ostringstream code_string;
  code_string << preprocess_string.str();
  code_string << process_string.str();
  code_string << "\n";

  // Return the python code + the output properties
  return boost::make_tuple(code_string.str(), outputPropertiesStr);
}

/**
 Create string of python code to load workspaces
 @param runStr : string of workspaces to load
 @param instrument : name of the instrument
 @param preprocessor : the pre-processing algorithm
 @param options : options given to this pre-processing algorithm
 @return : tuple of strings of python code and output workspace name
*/
boost::tuple<std::string, std::string>
loadWorkspaceString(const std::string &runStr, const std::string &instrument,
                    const DataProcessorPreprocessingAlgorithm &preprocessor,
                    const std::string &options) {

  std::vector<std::string> runs;
  boost::split(runs, runStr, boost::is_any_of("+,"));

  std::ostringstream load_strings;

  // Remove leading/trailing whitespace from each run
  for (auto &run : runs)
    boost::trim(run);

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
                               preprocessor, options);
  }

  return boost::make_tuple(load_strings.str(), outputName);
}

/**
 Create string of python code to run the Plus algorithm on specified workspaces
 @param input_name : name of workspace to add to the other workspace
 @param output_name : other workspace will be added to the one with this name
 @param preprocessor : the preprocessor algorithm
 @param options : options given for pre-processing
 @return string of python code
*/
std::string plusString(const std::string &input_name,
                       const std::string &output_name,
                       const DataProcessorPreprocessingAlgorithm &preprocessor,
                       const std::string &options) {
  std::ostringstream plus_string;

  plus_string << output_name << " = " << preprocessor.name() << "(";
  plus_string << preprocessor.lhsProperty() << " = '" << output_name << "', ";
  plus_string << preprocessor.rhsProperty() << " = '" << input_name << "'";
  if (!options.empty())
    plus_string << ", " << options;
  plus_string << ")\n";

  return plus_string.str();
}

/**
 Create string of python code to load a single workspace
 @param run : run to load
 @param instrument : name of the instrument
 @param prefix : the prefix to prepend to the output workspace name
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

/** Given an algorithm's name, completes the list of output properties
* @param algName : The name of the algorithm
* @param currentProperties : The number of output properties that are workspaces
* @return : The list of output properties as a string
*/
std::string completeOutputProperties(const std::string &algName,
                                     size_t currentProperties) {

  // In addition to output ws properties, our reduction and post-processing
  // algorithms could return other types of properties, for instance,
  // ReflectometryReductionOneAuto also returns a number called 'ThetaOut'
  // We need to specify those too in our python code

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create(algName);
  auto properties = alg->getProperties();
  int totalOutputProp = 0;
  for (auto &prop : properties) {
    if (prop->direction())
      totalOutputProp++;
  }
  totalOutputProp -= static_cast<int>(currentProperties);

  std::string outString;
  for (int i = 0; i < totalOutputProp; i++)
    outString += ", _";

  return outString;
}
}
}
