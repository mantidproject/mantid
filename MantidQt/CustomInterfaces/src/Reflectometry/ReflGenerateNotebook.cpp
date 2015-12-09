#include "MantidQtCustomInterfaces/Reflectometry/ReflGenerateNotebook.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidQtCustomInterfaces/ParseKeyValueString.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflVectorString.h"

#include <sstream>
#include <fstream>
#include <memory>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

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

ReflGenerateNotebook::ReflGenerateNotebook(
    std::string name, QReflTableModel_sptr model, const std::string instrument,
    const int runs_column, const int transmission_column,
    const int options_column, const int angle_column, const int min_q,
    const int max_q, const int d_qq, const int scale_column,
    const int group_column)
    : m_wsName(name), m_model(model), m_instrument(instrument),
      col_nums(runs_column, transmission_column, options_column, angle_column,
               min_q, max_q, d_qq, scale_column, group_column) {}

/**
  Generate an ipython notebook
  @param groups : groups of rows which were stitched
  @param rows : rows which were processed
  @returns ipython notebook string
  */
std::string
ReflGenerateNotebook::generateNotebook(std::map<int, std::set<int>> groups,
                                       std::set<int> rows) {

  std::unique_ptr<Mantid::API::NotebookWriter> notebook(
      new Mantid::API::NotebookWriter());

  notebook->codeCell(plotsFunctionString());

  notebook->markdownCell(titleString(m_wsName));

  notebook->markdownCell(tableString(m_model, col_nums, rows));

  int groupNo = 1;
  for (auto gIt = groups.begin(); gIt != groups.end(); ++gIt, ++groupNo) {
    const std::set<int> groupRows = gIt->second;

    // Announce the stitch group in the notebook
    std::ostringstream stitch_title_string;
    stitch_title_string << "Stitch group " << groupNo;
    notebook->markdownCell(stitch_title_string.str());

    // Reduce each row
    std::ostringstream code_string;
    boost::tuple<std::string, std::string, std::string> reduce_row_string;
    std::vector<std::string> unstitched_ws;
    std::vector<std::string> IvsLam_ws;
    code_string << "#Load and reduce\n";
    for (auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt) {
      reduce_row_string =
          reduceRowString(*rIt, m_instrument, m_model, col_nums);
      code_string << boost::get<0>(reduce_row_string);
      unstitched_ws.push_back(boost::get<1>(reduce_row_string));
      IvsLam_ws.push_back(boost::get<2>(reduce_row_string));
    }
    notebook->codeCell(code_string.str());

    // Stitch group
    boost::tuple<std::string, std::string> stitch_string =
        stitchGroupString(groupRows, m_instrument, m_model, col_nums);
    notebook->codeCell(boost::get<0>(stitch_string));

    // Draw plots
    notebook->codeCell(
        plotsString(unstitched_ws, IvsLam_ws, boost::get<1>(stitch_string)));
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
  @param unstitched_ws : vector of unstitched data workspace names to be plotted
  together
  @param IvsLam_ws : vector of I vs lambda data workspace names to be plotted
  together
  @param stitched_wsStr : name of stitched data workspace
  @return string containing the python code
  */
std::string plotsString(const std::vector<std::string> &unstitched_ws,
                        const std::vector<std::string> &IvsLam_ws,
                        const std::string &stitched_wsStr) {
  // Group workspaces which should be plotted on same axes
  std::ostringstream plot_string;
  plot_string << "#Group workspaces to be plotted on same axes\n";
  plot_string << "unstitchedGroupWS = GroupWorkspaces("
              << vectorParamString("InputWorkspaces", unstitched_ws) << ")\n";
  plot_string << "IvsLamGroupWS = GroupWorkspaces("
              << vectorParamString("InputWorkspaces", IvsLam_ws) << ")\n";

  // Plot I vs Q and I vs Lambda graphs
  plot_string << "#Plot workspaces\n";
  std::vector<std::string> workspaceList;
  workspaceList.push_back("unstitchedGroupWS");
  workspaceList.push_back(stitched_wsStr);
  workspaceList.push_back("IvsLamGroupWS");

  plot_string << plot1DString(
      workspaceList,
      "['I vs Q Unstitched', 'I vs Q Stitiched', 'I vs Lambda']");
  return plot_string.str();
}

/**
  Create string of markdown code to display a table of data from the GUI
  @param model : tablemodel for the full table
  @param col_nums : column number for each column title
  @param rows : rows from full table to include
  @return string containing the markdown code
  */
std::string tableString(QReflTableModel_sptr model, ColNumbers col_nums,
                        const std::set<int> &rows) {
  std::ostringstream table_string;

  table_string << "Run(s) | Angle | Transmission Run(s) | Q min | Q max | dQ/Q "
                  "| Scale | Group | Options\n";
  table_string << "------ | ----- | ------------------- | ----- | ----- | ---- "
                  "| ----- | ----- | -------\n";

  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {
    table_string << model->data(model->index(*rowIt, col_nums.runs))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.angle))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.transmission))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.qmin))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.qmax))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.dqq))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.scale))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.group))
                        .toString()
                        .toStdString() << " | ";
    table_string << model->data(model->index(*rowIt, col_nums.options))
                        .toString()
                        .toStdString() << "\n";
  }

  return table_string.str();
}

/**
  Create string of python code for plotting functions
  @return string containing the python code
  */
std::string plotsFunctionString() {
  return "#Import some useful tools for plotting\n"
         "from MantidIPython import *";
}

/**
  Create string of python code to stitch workspaces in the same group
  @param rows : rows in the stitch group
  @param instrument : name of the instrument
  @param model : table model containing details of runs and processing settings
  @param col_nums : column numbers used to find data in model
  @return tuple containing the python code string and the output workspace name
  */
boost::tuple<std::string, std::string>
stitchGroupString(const std::set<int> &rows, const std::string &instrument,
                  QReflTableModel_sptr model, ColNumbers col_nums) {
  std::ostringstream stitch_string;

  stitch_string << "#Stitch workspaces\n";

  // If we can get away with doing nothing, do.
  if (rows.size() < 2)
    return boost::make_tuple("", "");

  // Properties for Stitch1DMany
  std::vector<std::string> workspaceNames;
  std::vector<std::string> runs;

  std::vector<double> params;
  std::vector<double> startOverlaps;
  std::vector<double> endOverlaps;

  // Go through each row and prepare the properties
  for (auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt) {
    const std::string runStr = model->data(model->index(*rowIt, col_nums.runs))
                                   .toString()
                                   .toStdString();
    const double qmin =
        model->data(model->index(*rowIt, col_nums.qmin)).toDouble();
    const double qmax =
        model->data(model->index(*rowIt, col_nums.qmax)).toDouble();

    const boost::tuple<std::string, std::string> load_ws_string =
        loadWorkspaceString(runStr, instrument);

    const std::string runNo = getRunNumber(boost::get<1>(load_ws_string));
    runs.push_back(runNo);
    workspaceNames.push_back("IvsQ_" + runNo);

    startOverlaps.push_back(qmin);
    endOverlaps.push_back(qmax);
  }

  double dqq =
      model->data(model->index(*(rows.begin()), col_nums.dqq)).toDouble();

  // params are qmin, -dqq, qmax for the final output
  params.push_back(
      *std::min_element(startOverlaps.begin(), startOverlaps.end()));
  params.push_back(-dqq);
  params.push_back(*std::max_element(endOverlaps.begin(), endOverlaps.end()));

  // startOverlaps and endOverlaps need to be slightly offset from each other
  // See usage examples of Stitch1DMany to see why we discard first qmin and
  // last qmax
  startOverlaps.erase(startOverlaps.begin());
  endOverlaps.pop_back();

  std::string outputWSName = "IvsQ_" + boost::algorithm::join(runs, "_");

  stitch_string << outputWSName << ", _ = Stitch1DMany(";
  stitch_string << vectorParamString("InputWorkspaces", workspaceNames);
  stitch_string << ", ";
  stitch_string << vectorParamString("Params", params);
  stitch_string << ", ";
  stitch_string << vectorParamString("StartOverlaps", startOverlaps);
  stitch_string << ", ";
  stitch_string << vectorParamString("EndOverlaps", endOverlaps);
  stitch_string << ")\n";

  return boost::make_tuple(stitch_string.str(), outputWSName);
}

/**
  Create string of python code to create 1D plots from workspaces
  @param ws_names : vector of workspace names to plot
  @param title : title for figure
  @return string  of python code to plot I vs Q
  */
std::string plot1DString(const std::vector<std::string> &ws_names,
                         const std::string &title) {

  std::ostringstream plot_string;

  plot_string << "fig = plots([" << vectorString(ws_names)
              << "], title=" << title << ", legendLocation=[1, 1, 4])\n";

  return plot_string.str();
}

/**
 Create string of python code to run reduction algorithm on the specified row
 @param rowNo : the row in the model to run the reduction algorithm on
 @param instrument : name of the instrument
 @param model : table model containing details of runs and processing settings
 @param col_nums : column numbers used to find data in model
 @return tuple containing the python string and the output workspace name
*/
boost::tuple<std::string, std::string, std::string>
reduceRowString(const int rowNo, const std::string &instrument,
                QReflTableModel_sptr model, ColNumbers col_nums) {
  std::ostringstream code_string;

  const std::string runStr =
      model->data(model->index(rowNo, col_nums.runs)).toString().toStdString();
  const std::string transStr =
      model->data(model->index(rowNo, col_nums.transmission))
          .toString()
          .toStdString();
  const std::string options = model->data(model->index(rowNo, col_nums.options))
                                  .toString()
                                  .toStdString();

  double theta = 0;

  const bool thetaGiven =
      !model->data(model->index(rowNo, col_nums.angle)).toString().isEmpty();

  if (thetaGiven)
    theta = model->data(model->index(rowNo, col_nums.angle)).toDouble();

  const boost::tuple<std::string, std::string> load_ws_string =
      loadWorkspaceString(runStr, instrument);
  code_string << boost::get<0>(load_ws_string);

  const std::string runNo = getRunNumber(boost::get<1>(load_ws_string));
  const std::string IvsLamName = "IvsLam_" + runNo;
  const std::string thetaName = "theta_" + runNo;

  if (!transStr.empty()) {
    const boost::tuple<std::string, std::string> trans_string =
        transWSString(transStr, instrument);
    code_string << boost::get<0>(trans_string);
    code_string << "IvsQ_" << runNo << ", " << IvsLamName << ", " << thetaName
                << " = ";
    code_string << "ReflectometryReductionOneAuto(InputWorkspace = '"
                << boost::get<1>(load_ws_string) << "'";
    code_string << ", "
                << "FirstTransmissionRun = '" << boost::get<1>(trans_string)
                << "'";
  } else {
    code_string << "IvsQ_" << runNo << ", " << IvsLamName << ", " << thetaName
                << " = ";
    code_string << "ReflectometryReductionOneAuto(InputWorkspace = '"
                << boost::get<1>(load_ws_string) << "'";
  }

  if (thetaGiven) {
    code_string << ", "
                << "ThetaIn = " << theta;
  }

  // Parse and set any user-specified options
  auto optionsMap = parseKeyValueString(options);
  for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
    code_string << ", " << kvp->first << " = " << kvp->second;
  }
  code_string << ")\n";

  const double scale =
      model->data(model->index(rowNo, col_nums.scale)).toDouble();
  if (scale != 1.0) {
    const boost::tuple<std::string, std::string> scale_string =
        scaleString(runNo, scale);
    code_string << boost::get<0>(scale_string);
  }

  const boost::tuple<std::string, std::string> rebin_string =
      rebinString(rowNo, runNo, model, col_nums);
  code_string << boost::get<0>(rebin_string);

  return boost::make_tuple(code_string.str(), boost::get<1>(rebin_string),
                           IvsLamName);
}

/**
 Create string of python code to run the scale algorithm
 @param runNo : the number of the run to scale
 @param scale : value of scaling factor to use
 @return tuple of strings of python code and output workspace name
 */
boost::tuple<std::string, std::string> scaleString(const std::string &runNo,
                                                   const double scale) {
  std::ostringstream scale_string;

  scale_string << "IvsQ_" << runNo << " = Scale(";
  scale_string << "InputWorkspace = IvsQ_" << runNo;
  scale_string << ", Factor = " << 1.0 / scale;
  scale_string << ")\n";

  return boost::make_tuple(scale_string.str(), "IvsQ" + runNo);
}

/**
 Create string of python code to rebin data in a workspace
 @param rowNo : the number of the row to rebin
 @param runNo : the number of the run to rebin
 @param model : table model containing details of runs and processing settings
 @param col_nums : column numbers used to find data in model
 @return tuple of strings of python code and output workspace name
*/
boost::tuple<std::string, std::string> rebinString(const int rowNo,
                                                   const std::string &runNo,
                                                   QReflTableModel_sptr model,
                                                   ColNumbers col_nums) {
  // We need to make sure that qmin and qmax are respected, so we rebin to
  // those limits here.
  std::ostringstream rebin_string;
  rebin_string << "IvsQ_" << runNo << " = ";
  rebin_string << "Rebin(";
  rebin_string << "IvsQ_" << runNo;

  const double qmin =
      model->data(model->index(rowNo, col_nums.qmin)).toDouble();
  const double qmax =
      model->data(model->index(rowNo, col_nums.qmax)).toDouble();
  const double dqq = model->data(model->index(rowNo, col_nums.dqq)).toDouble();

  rebin_string << ", "
               << "Params = ";
  rebin_string << "'" << qmin << ", " << -dqq << ", " << qmax << "'";
  rebin_string << ")\n";

  return boost::make_tuple(rebin_string.str(), "IvsQ_" + runNo);
}

/**
 Create string of python code to create a transmission workspace
 @param trans_ws_str : string of workspaces to create transmission workspace
 from
 @param instrument : name of the instrument
 @return tuple of strings of python code and output workspace name
*/
boost::tuple<std::string, std::string>
transWSString(const std::string &trans_ws_str, const std::string &instrument) {
  const size_t maxTransWS = 2;

  std::ostringstream trans_string;
  std::vector<std::string> trans_ws_name;

  std::vector<std::string> trans_vector = splitByCommas(trans_ws_str);
  if (trans_vector.size() > maxTransWS)
    trans_vector.resize(maxTransWS);

  // Load the transmission runs
  boost::tuple<std::string, std::string> load_tuple;
  for (const auto &trans_name : trans_vector) {
    load_tuple = loadWorkspaceString(trans_name, instrument);
    trans_ws_name.push_back(boost::get<1>(load_tuple));
    trans_string << boost::get<0>(load_tuple);
  }

  // The runs are loaded, so we can create a TransWS
  std::string wsName = "TRANS_" + getRunNumber(trans_ws_name[0]);
  if (trans_ws_name.size() > 1)
    wsName += "_" + getRunNumber(trans_ws_name[1]);
  trans_string << wsName << " = ";
  trans_string << "CreateTransmissionWorkspaceAuto(";
  trans_string << "FirstTransmissionRun = '" << trans_ws_name[0] << "'";
  if (trans_ws_name.size() > 1)
    trans_string << ", SecondTransmissionRun = '" << trans_ws_name[1] << "'";
  trans_string << ")\n";

  return boost::make_tuple(trans_string.str(), wsName);
}

/**
 Get run number from workspace name
 @param ws_name : workspace name
 @return run number, as a string
*/
std::string getRunNumber(const std::string &ws_name) {
  // Matches TOF_13460 -> 13460
  boost::regex outputRegex("(TOF|IvsQ|IvsLam)_([0-9]+)");

  // Matches INTER13460 -> 13460
  boost::regex instrumentRegex("[a-zA-Z]{3,}([0-9]{3,})");

  boost::smatch matches;

  if (boost::regex_match(ws_name, matches, outputRegex)) {
    return matches[2].str();
  } else if (boost::regex_match(ws_name, matches, instrumentRegex)) {
    return matches[1].str();
  }

  // Resort to using the workspace name
  return ws_name;
}

/**
 Create string of python code to load workspaces
 @param runStr : string of workspaces to load
 @param instrument : name of the instrument
 @return tuple of strings of python code and output workspace name
*/
boost::tuple<std::string, std::string>
loadWorkspaceString(const std::string &runStr, const std::string &instrument) {
  std::vector<std::string> runs;
  boost::split(runs, runStr, boost::is_any_of("+"));

  std::ostringstream load_strings;

  // Remove leading/trailing whitespace from each run
  for (auto runIt = runs.begin(); runIt != runs.end(); ++runIt)
    boost::trim(*runIt);

  const std::string outputName = "TOF_" + boost::algorithm::join(runs, "_");

  boost::tuple<std::string, std::string> load_string;

  load_string = loadRunString(runs[0], instrument);
  load_strings << boost::get<0>(load_string);

  // EXIT POINT if there is only one run
  if (runs.size() == 1) {
    return boost::make_tuple(load_strings.str(), boost::get<1>(load_string));
  }
  load_strings << outputName << " = " << boost::get<1>(load_string) << "\n";

  // Load each subsequent run and add it to the first run
  for (auto runIt = std::next(runs.begin()); runIt != runs.end(); ++runIt) {
    load_string = loadRunString(*runIt, instrument);
    load_strings << boost::get<0>(load_string);
    load_strings << plusString(boost::get<1>(load_string), outputName);
  }

  return boost::make_tuple(load_strings.str(), outputName);
}

/**
 Create string of python code to run the Plus algorithm on specified workspaces
 @param input_name : name of workspace to add to the other workspace
 @param output_name : other workspace will be added to the one with this name
 @return string of python code
*/
std::string plusString(const std::string &input_name,
                       const std::string &output_name) {
  std::ostringstream plus_string;

  plus_string << output_name << " = Plus('LHSWorkspace' = " << output_name;
  plus_string << ", 'RHSWorkspace' = " << input_name;
  plus_string << ")\n";

  return plus_string.str();
}

/**
 Create string of python code to load a single workspace
 @param run : run to load
 @param instrument : name of the instrument
 @return tuple of strings of python code and output workspace name
*/
boost::tuple<std::string, std::string>
loadRunString(const std::string &run, const std::string &instrument) {
  std::ostringstream load_string;
  // We do not have access to AnalysisDataService from notebook, so must load
  // run from file
  const std::string filename = instrument + run;
  const std::string ws_name = "TOF_" + run;
  load_string << ws_name << " = ";
  load_string << "Load(";
  load_string << "Filename = '" << filename << "'";
  load_string << ")\n";

  return boost::make_tuple(load_string.str(), ws_name);
}
}
}
