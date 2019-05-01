// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/GenerateNotebook.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/VectorString.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WorkspaceNameUtils.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
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
post-processing algorithms specified via hinting line edits in the view
@param preprocessingOptionsMap : options passed to the preprocessing algorithm.
the corresponding hinting line edit in the view
@returns ipython notebook string
*/
GenerateNotebook::GenerateNotebook(
    QString name, QString instrument, WhiteList whitelist,
    std::map<QString, PreprocessingAlgorithm> preprocessMap,
    ProcessingAlgorithm processor,
    boost::optional<PostprocessingStep> postprocessingStep,
    ColumnOptionsMap preprocessingOptionsMap)
    : m_wsName(std::move(name)), m_instrument(std::move(instrument)),
      m_whitelist(std::move(whitelist)),
      m_preprocessMap(std::move(preprocessMap)),
      m_processor(std::move(processor)),
      m_postprocessingStep(std::move(postprocessingStep)),
      m_preprocessingOptionsMap(std::move(preprocessingOptionsMap)) {

  if (m_whitelist.size() < 2)
    throw std::invalid_argument(
        "A valid WhiteList must have at least two columns");
}

/** Check whether post-processing is applicable
 * @return : true if there is a post-processing step
 * */
bool GenerateNotebook::hasPostprocessing() const {
  return bool(m_postprocessingStep);
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
    for (const auto &row : rowMap) {
      codeString += "#Load and reduce\n";

      auto reducedRowStr = reduceRowString(
          row.second, m_instrument, m_whitelist, m_preprocessMap, m_processor,
          m_preprocessingOptionsMap);

      // The reduction code
      codeString += reducedRowStr;
    }
    notebook->codeCell(codeString.toStdString());

    /** Post-process string **/
    boost::tuple<QString, QString> postProcessString;
    if (hasPostprocessing() && rowMap.size() > 1) {
      // If there was only one run selected, it could not be post-processed
      postProcessString =
          postprocessGroupString(rowMap, m_processor, *m_postprocessingStep);
    }
    notebook->codeCell(boost::get<0>(postProcessString).toStdString());

    /** Draw plots **/

    notebook->codeCell(
        plotsString(rowMap, boost::get<1>(postProcessString), m_processor)
            .toStdString());
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
  @param groupData : the group of rows to plot
  @param stitched_wsStr : string containing the name of the stitched
  (post-processed workspace)
  @param processor : the data processor algorithm
  @return string containing the python code
  */
QString plotsString(const GroupData &groupData, const QString &stitched_wsStr,
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

    QString propertyName = processor.outputPropertyName(group);

    // From the reduction (processing) algorithm, get the prefix for the
    // output workspace, we'll use it to give name to this group
    QString prefix = processor.prefix(group);

    // Save this group to workspaceList
    workspaceList.append(prefix + "groupWS");

    QStringList wsNames;

    // Add the output name for this property from each reduced row
    for (auto const &groupItem : groupData) {
      auto row = groupItem.second;
      if (row->hasOption(propertyName))
        wsNames.append(row->optionValue(propertyName));
    }

    plotString +=
        "GroupWorkspaces(InputWorkspaces = '" + wsNames.join(", ") + "', ";
    plotString += "OutputWorkspace = '" + prefix + "groupWS'";
    plotString += ")\n";
  }

  // Add the post-processed workspace to the list of workspaces to plot
  workspaceList.append(stitched_wsStr);

  // Plot I vs Q and I vs Lambda graphs
  plotString += "#Plot workspaces\n";

  // Remove empty values
  workspaceList.removeAll("");

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

    for (auto &row : rowMap) {
      QStringList values;
      values.append(QString::number(groupId));

      if (row.second->size() != ncols)
        throw std::invalid_argument("Can't generate table for notebook");

      for (const auto &datum : *row.second)
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
  @param processor : the reduction algorithm
  @param postprocessingStep : the algorithm responsible for post-processing
  groups and the options specified for post-processing via HintingLineEdit.
  @return tuple containing the python code string and the output workspace name
  */
boost::tuple<QString, QString>
postprocessGroupString(const GroupData &rowMap,
                       const ProcessingAlgorithm &processor,
                       const PostprocessingStep &postprocessingStep) {

  QString postprocessString;

  postprocessString += "#Post-process workspaces\n";

  // Properties for post-processing algorithm
  // Vector containing the list of input workspaces
  QStringList inputNames;
  // Vector containing the different bits of the output ws name
  QStringList outputName;

  // Go through each row and prepare the input and output properties
  for (const auto &row : rowMap) {
    // The reduced ws name without prefix (for example 'TOF_13460_13462')
    auto suffix = row.second->reducedName();
    // The reduced ws name: 'IvsQ_TOF_13460_13462'
    inputNames.append(processor.defaultOutputPrefix() + suffix);
    // Add the suffix (i.e. 'TOF_13460_13462') to the output ws name
    outputName.append(suffix);
  }

  auto &postprocessingAlgorithm = postprocessingStep.m_algorithm;

  auto outputWSName = postprocessingAlgorithm.prefix() + outputName.join("_");
  postprocessString += postprocessingAlgorithm.name() + "(";
  postprocessString += postprocessingAlgorithm.inputProperty() + " = '";
  postprocessString += inputNames.join(", ");
  postprocessString += "'";
  if (!postprocessingStep.m_options.isEmpty()) {
    postprocessString += ", ";
    postprocessString += postprocessingStep.m_options;
  }
  postprocessString += ", " + postprocessingStep.m_algorithm.outputProperty() +
                       " = '" + outputWSName + "'";
  postprocessString += ")";

  return boost::make_tuple(postprocessString, outputWSName);
}

/**
  Create string of python code to create 1D plots from workspaces
  @param ws_names : vector of workspace names to plot
  @return string  of python code to plot I vs Q
  */
QString plot1DString(const QStringList &ws_names) {

  // Edit workspace names to access workspaces from the ADS. Note
  // that we avoid creating python variables based on the workspace
  // names because they may contain invalid characters
  QStringList ads_workspaces;
  for (const auto &ws_name : ws_names) {
    ads_workspaces.push_back("mtd['" + ws_name + "']");
  }

  // Use a legend location of 1 (meaning top-right) for all
  // plots by default.
  auto legendLocations = std::string("legendLocation=[1");
  for (auto i = 1; i < ws_names.size(); ++i) {
    // For the 3rd plot (IvsLam) use location=4 (bottom-right).
    // It's not ideal to hard-code this here so longer term I'd
    // like to refactor this to store the locations alongside the
    // output properties.
    if (i == 2)
      legendLocations += ", 4";
    else
      legendLocations += ", 1";
  }
  legendLocations += "]";

  QString plotString;
  plotString += "fig = plots([";
  plotString += vectorString(ads_workspaces);
  plotString += "], title=['";
  plotString += ws_names.join("', '");
  plotString += "'], ";
  plotString += QString::fromStdString(legendLocations);
  plotString += ")\n";
  return plotString;
}

template <typename Map>
void addProperties(QStringList &algProperties, const Map &optionsMap) {
  for (auto &&kvp : optionsMap) {
    algProperties.append(kvp.first + " = '" + kvp.second + "'");
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
 @param globalPreprocessingOptionsMap : a map containing the pre-processing
 options
 @return tuple containing the python string and the output workspace names.
 First item in the tuple is the python code that performs the reduction, and
 second item are the names of the output workspaces.
*/
QString
reduceRowString(const RowData_sptr data, const QString &instrument,
                const WhiteList &whitelist,
                const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
                const ProcessingAlgorithm &processor,
                const ColumnOptionsMap &globalPreprocessingOptionsMap) {

  if (static_cast<int>(whitelist.size()) != data->size()) {
    throw std::invalid_argument("Can't generate notebook");
  }

  QString preprocessingString;

  // Create a copy of the processing options, which we'll update with
  // the results of preprocessing where applicable
  auto processingOptions = data->options();

  // Loop through all columns, excluding 'Options'  and 'Hidden Options'
  int ncols = static_cast<int>(whitelist.size());
  for (int col = 0; col < ncols - 2; col++) {
    // The column's name
    const QString colName = whitelist.name(col);
    // The algorithm property name linked to this column
    const QString algProp = whitelist.algorithmProperty(col);

    // Nothing to do if there is no value or no preprocessing
    if (!data->hasOption(algProp) || preprocessMap.count(colName) == 0)
      continue;

    // Get the column value. Note that we take this from the cached options,
    // rather than the row data, so that it includes any default values from
    // the global settings.
    const auto colValue = data->optionValue(algProp);

    // This column was pre-processed. We need to print pre-processing
    // instructions

    // The pre-processing alg
    const PreprocessingAlgorithm &preprocessor = preprocessMap.at(colName);

    // The options for the pre-processing alg
    // Only include options in the given preprocessing options map,
    // but override them if they are set in the row data
    QString options;
    if (globalPreprocessingOptionsMap.count(colName) > 0) {
      OptionsMap preprocessingOptions = getCanonicalOptions(
          data, globalPreprocessingOptionsMap.at(colName), whitelist, false);
      options = convertMapToString(preprocessingOptions);
    }

    // Python code ran to load and pre-process runs
    const boost::tuple<QString, QString> load_ws_string =
        loadWorkspaceString(colValue, instrument, preprocessor, options);
    preprocessingString += boost::get<0>(load_ws_string);

    // Update the options map with the result of preprocessing
    processingOptions[algProp] = boost::get<1>(load_ws_string);
  }

  // Vector to store the algorithm properties with values
  // For example
  // InputWorkspace = 'TOF_13460_13463'
  // ThetaIn = 0.2
  // etc
  QStringList algProperties;
  addProperties(algProperties, processingOptions);

  // Populate processString
  QString processingString;
  processingString += processor.name();
  processingString += "(";
  processingString += algProperties.join(", ");
  processingString += ")";

  // Populate codeString, which contains both pre-processing and processing
  // python code
  QString codeString;
  codeString += preprocessingString;
  codeString += processingString;
  codeString += "\n";

  // Return the python code + the output properties
  return codeString;
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

  auto runs = preprocessingStringToList(runStr);

  QString loadStrings;

  // Remove leading/trailing whitespace from each run
  for (auto &run : runs) {
    run = run.trimmed();
  }

  const QString prefix = preprocessor.prefix();
  const QString outputName =
      preprocessingListToString(runs, prefix, preprocessor.separator());

  boost::tuple<QString, QString> loadString;

  loadString = loadRunString(runs[0], instrument, prefix, outputName);
  loadStrings += boost::get<0>(loadString);

  // EXIT POINT if there is only one run
  if (runs.size() == 1) {
    return boost::make_tuple(loadStrings, boost::get<1>(loadString));
  }

  auto inputName1 = boost::get<1>(loadString);

  // Load each subsequent run and add it to the first run
  for (auto runIt = runs.begin() + 1; runIt != runs.end(); ++runIt) {
    loadString = loadRunString(*runIt, instrument, prefix);
    loadStrings += boost::get<0>(loadString);
    auto inputName2 = boost::get<1>(loadString);
    loadStrings += preprocessString(inputName1, inputName2, outputName,
                                    preprocessor, options);
  }

  return boost::make_tuple(loadStrings, outputName);
}

/**
 Create string of python code to run the preprocessing algorithm on specified
 workspaces
 @param input_name1 : the name of the 1st workspace to combine
 @param input_name2 : the name of the 2nd workspace to combine
 @param output_name : the name to give to the output workspace
 @param preprocessor : the preprocessor algorithm
 @param options : the properties to pass to the pre-processing algorithm
 @return string of python code
*/
QString preprocessString(const QString &input_name1, const QString &input_name2,
                         const QString &output_name,
                         const PreprocessingAlgorithm &preprocessor,
                         const QString &options) {
  QString preprocessString;

  preprocessString += preprocessor.name();
  preprocessString += "(";
  preprocessString += preprocessor.lhsProperty() + " = '" + input_name1 + "', ";
  preprocessString += preprocessor.rhsProperty() + " = '" + input_name2 + "'";
  if (!options.isEmpty()) {
    preprocessString += ", " + options;
  }
  preprocessString +=
      ", " + preprocessor.outputProperty() + " = '" + output_name + "'";
  preprocessString += ")\n";
  return preprocessString;
}

/**
 Create string of python code to load a single workspace
 @param run : the run to load
 @param instrument : the name of the instrument
 @param prefix : if the outputName is not given, the output name will
 be constructed using the run number and this prefix
 @param outputName : the output name to use. If empty, a default name
 will be created based on the run number and prefix
 @return tuple of strings of python code and output workspace name
*/
boost::tuple<QString, QString> loadRunString(const QString &run,
                                             const QString &instrument,
                                             const QString &prefix,
                                             const QString &outputName) {
  QString loadString;
  // We do not have access to AnalysisDataService from notebook, so must load
  // run from file
  const QString filename = instrument + run;
  // Use the given output name, if given, otherwise construct it from the run
  // number
  const QString ws_name = outputName.isEmpty() ? prefix + run : outputName;
  loadString += "Load(";
  loadString += "Filename = '" + filename + "'";
  loadString += ", OutputWorkspace = '" + ws_name + "'";
  loadString += ")\n";

  return boost::make_tuple(loadString, ws_name);
}

/** Given an algorithm's name, completes the list of output properties
 * @param algName : The name of the algorithm
 * @param currentProperties : The number of output properties that are
 * workspaces
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
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
