#include "MantidQtWidgets/Common/DataProcessorUI/GenerateNotebook.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/VectorString.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/*
 * Split the input string on commas and trim leading and trailing whitespace
 * from the results
 */
QStringList splitByCommas(const QString &namesString) {
  auto splitByCommas = namesString.split(',');
  // Remove leading/trailing whitespace from each resulting string
  for (auto &name : splitByCommas) {
    name = name.trimmed();
  }
  return splitByCommas;
}

/**
Constructor
@param name : the name of the table workspace used for the reduction
@param instrument : the instrument
@param whitelist : the white list defining the columns
@param preprocessMap : a map indicating which columns were pre-processed and the
corresponding pre-processing algorithms
@param processor : the reduction algorithm
@param postprocessingStep : the post-processing algorithm and options for the
pre-processing algorithms specified via hinting line edits in the view
@param preprocessingOptionsMap : options passed to the preprocessing algorithm.
@param processingOptions : options to the reduction algorithm specified via
the corresponding hinting line edit in the view
@returns ipython notebook string
*/
GenerateNotebook::GenerateNotebook(
    QString name, const QString instrument, const WhiteList &whitelist,
    const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
    const ProcessingAlgorithm &processor,
    const PostprocessingStep &postprocessingStep,
    const std::map<QString, QString> preprocessingOptionsMap,
    const QString processingOptions)
    : m_wsName(name), m_instrument(instrument), m_whitelist(whitelist),
      m_preprocessMap(preprocessMap), m_processor(processor),
      m_postprocessingStep(postprocessingStep),
      m_preprocessingOptionsMap(preprocessingOptionsMap),
      m_processingOptions(processingOptions) {

  if (m_whitelist.size() < 2)
    throw std::invalid_argument(
        "A valid WhiteList must have at least two columns");
}

/**
  Generate an ipython notebook
  @param data : the processed data
  @returns ipython notebook string
  */
QString GenerateNotebook::generateNotebook(const TreeData &data) {

  auto notebook = Mantid::Kernel::make_unique<Mantid::API::NotebookWriter>();

  notebook->markdownCell(titleString(m_wsName).toStdString());

  notebook->markdownCell(tableString(data, m_whitelist).toStdString());

  for (const auto &item : data) {

    const int groupId = item.first;
    const auto rowMap = item.second;

    /** Announce the stitch group in the notebook **/

    QString groupTitle = "Group " + QString::number(groupId);
    notebook->markdownCell(groupTitle.toStdString());

    /**  Reduce all rows **/

    // The python code
    QString codeString;
    // A vector to store the output ws produced during the reduction process
    // In the case of Reflectometry those will be the IvsQ_ and IvsLam
    // workspaces
    QStringList output_ws;

    for (const auto &row : rowMap) {
      codeString += "#Load and reduce\n";

      auto reducedRowStr = reduceRowString(
          row.second, m_instrument, m_whitelist, m_preprocessMap, m_processor,
          m_preprocessingOptionsMap, m_processingOptions);

      // The reduction code
      codeString += boost::get<0>(reducedRowStr);
      // The output workspace names
      output_ws.append(boost::get<1>(reducedRowStr));
    }
    notebook->codeCell(codeString.toStdString());

    /** Post-process string **/
    boost::tuple<QString, QString> postProcessString;
    if (rowMap.size() > 1) {
      // If there was only one run selected, it could not be post-processed
      postProcessString = postprocessGroupString(
          rowMap, m_whitelist, m_processor, m_postprocessingStep);
    }
    notebook->codeCell(boost::get<0>(postProcessString).toStdString());

    /** Draw plots **/

    notebook->codeCell(plotsString(output_ws, boost::get<1>(postProcessString),
                                   m_processor).toStdString());
  }

  return QString::fromStdString(notebook->writeNotebook());
}

/**
  Create string of markdown code for title of the data processing part of the
  notebook
  @param wsName : name of the table workspace
  @return string containing markdown code
  */
QString titleString(const QString &wsName) {
  QString title_string;

  if (!wsName.isEmpty()) {
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
QString plotsString(const QStringList &output_ws, const QString &stitched_wsStr,
                    const ProcessingAlgorithm &processor) {

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

  auto plotString = QString("#Group workspaces to be plotted on same axes\n");

  // Group workspaces which should be plotted on same axes
  // But first we need to determine how many groups of ws we have
  // We can do that from the processing algorithm
  size_t nGroups = processor.numberOfOutputProperties();

  // A vector to store the list of groups
  QStringList workspaceList;

  // Now we iterate through groups to get the relevant workspace
  for (auto group = 0u; group < nGroups; group++) {

    // From the reduction (processing) algorithm, get the prefix for the
    // output workspace, we'll use it to give name to this group
    QString prefix = processor.prefix(group);

    plotString += prefix + "groupWS = GroupWorkspaces(InputWorkspaces = '";

    // Save this group to workspaceList
    workspaceList.append(prefix + "groupWS");

    QStringList wsNames;

    // Iterate through the elements of output_ws
    for (const auto &outws : output_ws) {
      auto workspaces = splitByCommas(outws);
      // Get the workspace we need for this group
      wsNames.append(workspaces[group]);
    }

    plotString += wsNames.join(", ");
    plotString += "')\n";
  }

  // Add the post-processed workspace to the list of workspaces to plot
  workspaceList.append(stitched_wsStr);

  // Plot I vs Q and I vs Lambda graphs
  plotString += "#Plot workspaces\n";

  plotString += plot1DString(workspaceList);

  return plotString;
}

/**
  Create string of markdown code to display a table of data from the GUI
  @param treeData : the processed data
  @param whitelist : the whitelist defining the table columns
  @return string containing the markdown code
  */
QString tableString(const TreeData &treeData, const WhiteList &whitelist) {

  QString tableString;

  const int ncols = static_cast<int>(whitelist.size());

  tableString += "Group | ";
  for (int i = 0; i < ncols - 1; i++) {
    tableString += whitelist.name(i);
    tableString += " | ";
  }
  tableString += whitelist.name(ncols - 1);
  tableString += "\n";
  for (int i = 0; i < ncols - 1; i++) {
    tableString += "---";
    tableString += " | ";
  }
  tableString += "---";
  tableString += "\n";

  for (const auto &group : treeData) {
    auto groupId = group.first;
    auto rowMap = group.second;

    for (const auto &row : rowMap) {
      QStringList values;
      values.append(QString::number(groupId));

      if (row.second.size() != ncols)
        throw std::invalid_argument("Can't generate table for notebook");

      for (const auto &datum : row.second)
        values.append(datum);

      tableString += values.join(QString(" | "));
      tableString += "\n";
    }
  }
  return tableString;
}

/**
  Create string of python code to post-process rows in the same group
  @param rowMap : map where keys are row indices and values are vectors
  containing the data
  @param whitelist : the whitelist
  @param processor : the reduction algorithm
  @param postprocessingStep : the algorithm responsible for post-processing
  groups and the options specified for post-processing via HintingLineEdit.
  @return tuple containing the python code string and the output workspace name
  */
boost::tuple<QString, QString>
postprocessGroupString(const GroupData &rowMap, const WhiteList &whitelist,
                       const ProcessingAlgorithm &processor,
                       const PostprocessingStep &postprocessingStep) {

  QString stitchString;

  stitchString += "#Post-process workspaces\n";

  // Properties for post-processing algorithm
  // Vector containing the list of input workspaces
  QStringList inputNames;
  // Vector containing the different bits of the output ws name
  QStringList outputName;

  // Go through each row and prepare the input and output properties
  for (const auto &row : rowMap) {
    // The reduced ws name without prefix (for example 'TOF_13460_13462')
    auto suffix = getReducedWorkspaceName(row.second, whitelist);
    // The reduced ws name: 'IvsQ_TOF_13460_13462'
    inputNames.append(processor.prefix(0) + suffix);
    // Add the suffix (i.e. 'TOF_13460_13462') to the output ws name
    outputName.append(suffix);
  }

  auto postprocessingAlgorithm = postprocessingStep.m_algorithm;

  QString outputWSName =
      postprocessingStep.m_algorithm.prefix() + outputName.join("_");
  stitchString += outputWSName;
  stitchString += completeOutputProperties(
      postprocessingAlgorithm.name(),
      postprocessingAlgorithm.numberOfOutputProperties());
  stitchString += " = ";
  stitchString += postprocessingAlgorithm.name() + "(";
  stitchString += postprocessingAlgorithm.inputProperty() + " = '";
  stitchString += inputNames.join(", ");
  stitchString += "'";
  if (!postprocessingStep.m_options.isEmpty()) {
    stitchString += ", ";
    stitchString += postprocessingStep.m_options;
    stitchString += ")";
  }

  return boost::make_tuple(stitchString, outputWSName);
}

/**
  Create string of python code to create 1D plots from workspaces
  @param ws_names : vector of workspace names to plot
  @return string  of python code to plot I vs Q
  */
QString plot1DString(const QStringList &ws_names) {
  QString plotString;
  plotString += "fig = plots([";
  plotString += vectorString(ws_names);
  plotString += "], title=['";
  plotString += ws_names.join("', '");
  plotString += "'], legendLocation=[1, 1, 4])\n";
  return plotString;
}

/**
 Constructs the name for the reduced workspace
 @param data : vector containing the data used in the reduction
 @param whitelist : the whitelist
 @param prefix : wheter to return the name with the prefix or not
 @return : the workspace name
*/
QString getReducedWorkspaceName(const RowData &data, const WhiteList &whitelist,
                                const QString &prefix) {

  int ncols = static_cast<int>(whitelist.size());
  if (data.size() != ncols)
    throw std::invalid_argument(
        "Can't write output workspace name to notebook");

  auto names = QStringList();

  for (int col = 0; col < ncols - 1; col++) {
    // Do we want to use this column to generate the name of the output ws?
    if (whitelist.isShown(col)) {
      // Get what's in the column
      const QString valueStr = data.at(col);
      if (!valueStr.isEmpty()) {
        // But we may have things like '1+2' which we want to replace with '1_2'
        auto value = valueStr.split(QRegExp("[+,]"), QString::SkipEmptyParts);
        names.append(whitelist.prefix(col) + value.join("_"));
      }
    }
  } // Columns

  return prefix + names.join("_");
}

template <typename Map>
void addProperties(QStringList &algProperties, const Map &optionsMap) {
  for (auto &&kvp : optionsMap) {
    algProperties.append(
        QString::fromStdString(kvp.first + " = " + kvp.second));
  }
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
boost::tuple<QString, QString>
reduceRowString(const RowData &data, const QString &instrument,
                const WhiteList &whitelist,
                const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
                const ProcessingAlgorithm &processor,
                const std::map<QString, QString> &preprocessingOptionsMap,
                const QString &processingOptions) {

  if (static_cast<int>(whitelist.size()) != data.size()) {
    throw std::invalid_argument("Can't generate notebook");
  }

  QString preprocessString;

  // Vector to store the algorithm properties with values
  // For example
  // InputWorkspace = 'TOF_13460_13463'
  // ThetaIn = 0.2
  // etc
  QStringList algProperties;

  int ncols = static_cast<int>(whitelist.size());

  // Run through columns, excluding 'Options'
  for (int col = 0; col < ncols - 2; col++) {
    // The column's name
    const QString colName = whitelist.name(col);
    // The algorithm property linked to this column
    const QString algProp = whitelist.algorithmProperty(col);

    if (preprocessMap.count(colName)) {
      // This column was pre-processed, we need to print pre-processing
      // instructions

      // Get the runs
      const QString runStr = data.at(col);

      if (!runStr.isEmpty()) {
        // Some runs were given for pre-processing

        // The pre-processing alg
        const PreprocessingAlgorithm preprocessor = preprocessMap.at(colName);
        // The pre-processing options
        const QString options = preprocessingOptionsMap.count(colName) > 0
                                    ? preprocessingOptionsMap.at(colName)
                                    : "";
        // Python code ran to load and pre-process runs
        const boost::tuple<QString, QString> load_ws_string =
            loadWorkspaceString(runStr, instrument, preprocessor, options);
        preprocessString += boost::get<0>(load_ws_string);

        // Add runs to reduction properties
        algProperties.append(algProp + " = '" + boost::get<1>(load_ws_string) +
                             "'");
      }
    } else {
      // No pre-processing

      // Just read the property value from the table
      const QString propStr = data.at(col);

      if (!propStr.isEmpty()) {
        // If it was not empty, we used it as an input property to the reduction
        // algorithm
        algProperties.append(algProp + " = " + propStr);
      }
    }
  }

  auto options = parseKeyValueString(processingOptions.toStdString());

  const auto hiddenOptionsStr = data.back();
  // Parse and set any user-specified options
  auto hiddenOptionsMap = parseKeyValueString(hiddenOptionsStr.toStdString());
  // Options specified via 'Hidden Options' column will be preferred
  addProperties(algProperties, hiddenOptionsMap);

  // 'Options' specified either via 'Options' column or HintinLineEdit
  const auto optionsStr = data.at(ncols - 2);
  // Parse and set any user-specified options
  auto optionsMap = parseKeyValueString(optionsStr.toStdString());
  // Options specified via 'Options' column will be preferred
  optionsMap.insert(options.begin(), options.end());
  addProperties(algProperties, optionsMap);

  /* Now construct the names of the reduced workspaces*/

  // Vector containing the output ws names
  // For example
  // 'IvsQ_TOF_13460_13462',
  // 'IvsLam_TOF_13460_13462
  QStringList outputProperties;
  for (auto prop = 0u; prop < processor.numberOfOutputProperties(); prop++) {
    outputProperties.append(
        getReducedWorkspaceName(data, whitelist, processor.prefix(prop)));
  }

  QString outputPropertiesStr = outputProperties.join(", ");

  // Populate processString
  QString processString;
  processString += outputPropertiesStr;
  processString += completeOutputProperties(
      processor.name(), processor.numberOfOutputProperties());
  processString += " = ";
  processString += processor.name();
  processString += "(";
  processString += algProperties.join(", ");
  processString += ")";

  // Populate codeString, which contains both pre-processing and processing
  // python code
  QString codeString;
  codeString += preprocessString;
  codeString += processString;
  codeString += "\n";

  // Return the python code + the output properties
  return boost::make_tuple(codeString, outputPropertiesStr);
}

/**
 Create string of python code to load workspaces
 @param runStr : string of workspaces to load
 @param instrument : name of the instrument
 @param preprocessor : the pre-processing algorithm
 @param options : options given to this pre-processing algorithm
 @return : tuple of strings of python code and output workspace name
*/
boost::tuple<QString, QString>
loadWorkspaceString(const QString &runStr, const QString &instrument,
                    const PreprocessingAlgorithm &preprocessor,
                    const QString &options) {

  auto runs = runStr.split(QRegExp("[+,]"));

  QString loadStrings;

  // Remove leading/trailing whitespace from each run
  for (auto &run : runs) {
    run = run.trimmed();
  }

  const QString prefix = preprocessor.prefix();
  const QString outputName = prefix + runs.join("_");

  boost::tuple<QString, QString> loadString;

  loadString = loadRunString(runs[0], instrument, prefix);
  loadStrings += boost::get<0>(loadString);

  // EXIT POINT if there is only one run
  if (runs.size() == 1) {
    return boost::make_tuple(loadStrings, boost::get<1>(loadString));
  }
  loadStrings += outputName;
  loadStrings += " = ";
  loadStrings += boost::get<1>(loadString);
  loadStrings += "\n";

  // Load each subsequent run and add it to the first run
  for (auto runIt = runs.begin() + 1; runIt != runs.end(); ++runIt) {
    loadString = loadRunString(*runIt, instrument, prefix);
    loadStrings += boost::get<0>(loadString);
    loadStrings += plusString(boost::get<1>(loadString), outputName,
                              preprocessor, options);
  }

  return boost::make_tuple(loadStrings, outputName);
}

/**
 Create string of python code to run the Plus algorithm on specified workspaces
 @param input_name : name of workspace to add to the other workspace
 @param output_name : other workspace will be added to the one with this name
 @param preprocessor : the preprocessor algorithm
 @param options : options given for pre-processing
 @return string of python code
*/
QString plusString(const QString &input_name, const QString &output_name,
                   const PreprocessingAlgorithm &preprocessor,
                   const QString &options) {
  QString plusString;

  plusString += output_name + " = " + preprocessor.name();
  plusString += "(";
  plusString += preprocessor.lhsProperty() + " = '" + output_name + "', ";
  plusString += preprocessor.rhsProperty() + " = '" + input_name + "'";
  if (!options.isEmpty()) {
    plusString += ", " + options;
  }
  plusString += ")\n";
  return plusString;
}

/**
 Create string of python code to load a single workspace
 @param run : run to load
 @param instrument : name of the instrument
 @param prefix : the prefix to prepend to the output workspace name
 @return tuple of strings of python code and output workspace name
*/
boost::tuple<QString, QString> loadRunString(const QString &run,
                                             const QString &instrument,
                                             const QString &prefix) {
  QString loadString;
  // We do not have access to AnalysisDataService from notebook, so must load
  // run from file
  const QString filename = instrument + run;
  const QString ws_name = prefix + run;
  loadString += ws_name + " = ";
  loadString += "Load(";
  loadString += "Filename = '" + filename + "'";
  loadString += ")\n";

  return boost::make_tuple(loadString, ws_name);
}

/** Given an algorithm's name, completes the list of output properties
* @param algName : The name of the algorithm
* @param currentProperties : The number of output properties that are workspaces
* @return : The list of output properties as a string
*/
QString completeOutputProperties(const QString &algName,
                                 size_t currentProperties) {

  // In addition to output ws properties, our reduction and post-processing
  // algorithms could return other types of properties, for instance,
  // ReflectometryReductionOneAuto also returns a number called 'ThetaOut'
  // We need to specify those too in our python code

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create(algName.toStdString());
  auto properties = alg->getProperties();
  int totalOutputProp = 0;
  for (auto &prop : properties) {
    if (prop->direction())
      totalOutputProp++;
  }
  totalOutputProp -= static_cast<int>(currentProperties);

  QString outString;
  for (int i = 0; i < totalOutputProp; i++)
    outString += ", _";

  return outString;
}
}
}
}
