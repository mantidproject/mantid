#include "MantidQtCustomInterfaces/ReflGenerateNotebook.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidQtCustomInterfaces/ParseKeyValueString.h"

#include <sstream>
#include <fstream>
#include <memory>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>

namespace MantidQt {
  namespace CustomInterfaces {
    ReflGenerateNotebook::ReflGenerateNotebook(std::string name, QReflTableModel_sptr model,
                                               const std::string instrument, const int runs_column,
                                               const int transmission_column, const int options_column,
                                               const int angle_column, const int min_q, const int max_q,
                                               const int d_qq, const int scale_column) :
      m_wsName(name), m_model(model), m_instrument(instrument),
      COL_RUNS(runs_column), COL_TRANSMISSION(transmission_column),
      COL_OPTIONS(options_column), COL_ANGLE(angle_column),
      COL_QMIN(min_q), COL_QMAX(max_q), COL_DQQ(d_qq), COL_SCALE(scale_column){ }

    /**
      Generate an ipython notebook
      @param groups : groups of rows which were stitched
      @returns ipython notebook string
      */
    std::string ReflGenerateNotebook::generateNotebook(std::map<int, std::set<int>> groups) {
      std::unique_ptr<Mantid::API::NotebookWriter> notebook(new Mantid::API::NotebookWriter());

      std::string title_string;
      if (!m_wsName.empty()) {
        title_string = "Processed data from workspace: " + m_wsName + "\n---------------------";
      }
      else {
        title_string = "Processed data\n---------------------";
      }
      title_string += "\nNotebook generated from the ISIS Reflectometry (Polref) Interface";
      notebook->markdownCell(title_string);

      int groupNo = 1;
      for (auto gIt = groups.begin(); gIt != groups.end(); ++gIt, ++groupNo) {
        const std::set<int> groupRows = gIt->second;

        // Announce the stitch group in the notebook
        notebook->markdownCell(
          "Stitch group " + static_cast<std::ostringstream*>( &(std::ostringstream() << groupNo) )->str());

        //Reduce each row
        std::ostringstream code_string;
        std::tuple<std::string, std::string, std::string, std::string, std::string> reduce_row_string;
        std::vector<std::string> unstitched_ws;
        std::vector<std::string> IvsLam_ws;
        std::vector<std::string> theta;
        std::vector<std::string> runNos;
        code_string << "#Load and reduce\n";
        for (auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt) {
          reduce_row_string = reduceRowString(*rIt);
          code_string << std::get<0>(reduce_row_string);
          unstitched_ws.push_back(std::get<1>(reduce_row_string));
          IvsLam_ws.push_back(std::get<2>(reduce_row_string));
          theta.push_back(std::get<3>(reduce_row_string));
          runNos.push_back(std::get<4>(reduce_row_string));
        }
        notebook->codeCell(code_string.str());

        // Stitch group
        std::tuple<std::string, std::string> stitch_string = stitchGroupString(groupRows);
        notebook->codeCell(std::get<0>(stitch_string));

        // Plot I vs Q and I vs Lambda graphs
        std::ostringstream plot_string;
        plot_string << "f, (ax1, ax2, ax3) = plt.subplots(1, 3, sharey=True, figsize=(18,4))\n";
        std::vector<std::string> stitched_ws;
        stitched_ws.push_back(std::get<1>(stitch_string));
        plot_string << plot1D(unstitched_ws, "ax1", "I vs Q Unstitched", 1) << "\n";
        plot_string << plot1D(stitched_ws, "ax2", "I vs Q Stitiched", 1) << "\n";
        plot_string << plot1D(IvsLam_ws, "ax3", "I vs Lambda", 4);
        plot_string << "plt.show() #Draw the plot\n";
        notebook->codeCell(plot_string.str());

        notebook->markdownCell(printThetaString(runNos, theta));
      }

      return notebook->writeNotebook();
    }

    std::string ReflGenerateNotebook::printThetaString(std::vector<std::string> runNos,
                                                       std::vector<std::string> theta)
    {
      std::ostringstream theta_string;

      theta_string << "Run | $\\theta$ Value\n------------- | -------------\n";

      int run_i = 0;
      for(auto runIt = runNos.begin(); runIt != runNos.end(); ++runIt) {
        theta_string << *runIt << " | " << theta[run_i] << "\n";
        run_i++;
      }

      return theta_string.str();
    }

    /**
      Create string of python code to stitch workspaces in the same group
      @param rows : rows in the stitch group
      @return tuple containing the python code string and the output workspace name
      */
    std::tuple<std::string, std::string> ReflGenerateNotebook::stitchGroupString(std::set<int> rows)
    {
      std::ostringstream stitch_string;

      stitch_string << "#Stitch workspaces\n";

      //If we can get away with doing nothing, do.
      if(rows.size() < 2)
        return std::make_tuple("", "");

      //Properties for Stitch1DMany
      std::vector<std::string> workspaceNames;
      std::vector<std::string> runs;

      std::vector<double> params;
      std::vector<double> startOverlaps;
      std::vector<double> endOverlaps;

      //Go through each row and prepare the properties
      for(auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt)
      {
        const std::string  runStr = m_model->data(m_model->index(*rowIt, COL_RUNS)).toString().toStdString();
        const double         qmin = m_model->data(m_model->index(*rowIt, COL_QMIN)).toDouble();
        const double         qmax = m_model->data(m_model->index(*rowIt, COL_QMAX)).toDouble();

        const std::tuple<std::string, std::string> load_ws_string = loadWorkspaceString(runStr);

        const std::string runNo = getRunNumber(std::get<1>(load_ws_string));
        runs.push_back(runNo);
        workspaceNames.push_back("IvsQ_" + runNo);

        startOverlaps.push_back(qmin);
        endOverlaps.push_back(qmax);
      }

      double dqq = m_model->data(m_model->index(*(rows.begin()), COL_DQQ)).toDouble();

      //params are qmin, -dqq, qmax for the final output
      params.push_back(*std::min_element(startOverlaps.begin(), startOverlaps.end()));
      params.push_back(-dqq);
      params.push_back(*std::max_element(endOverlaps.begin(), endOverlaps.end()));

      //startOverlaps and endOverlaps need to be slightly offset from each other
      //See usage examples of Stitch1DMany to see why we discard first qmin and last qmax
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

      return std::make_tuple(stitch_string.str(), outputWSName);
    }

    /**
      Create string of comma separated list of parameter values from a vector
      @param param_name : name of the parameter we are creating a list of
      @param param_vec : vector of parameter values
      @return string of comma separated list of parameter values
      */
    template<typename T, typename A>
    std::string ReflGenerateNotebook::vectorParamString(std::string param_name, std::vector<T,A> &param_vec)
    {
      std::ostringstream vector_string;
      const char* separator = "";
      vector_string << param_name << " = '";
      for(auto paramIt = param_vec.begin(); paramIt != param_vec.end(); ++paramIt)
      {
        vector_string << separator << *paramIt;
        separator = ", ";
      }
      vector_string << "'";

      return vector_string.str();
    }

    /**
      Create string of python code to plot I vs Q from workspaces
      @param ws_names : vector of workspace names to plot on the same axes
      @param axes : handle of axes to plot in
      @return string  of python code to plot I vs Q
      */
    std::string ReflGenerateNotebook::plot1D(std::vector<std::string> ws_names, std::string axes, std::string title,
                                             int legendLocation) {

      std::ostringstream plot_string;

      for (auto it = ws_names.begin(); it != ws_names.end(); ++it) {
        std::tuple<std::string, std::string> convert_point_string = convertToPointString(*it);
        plot_string << std::get<0>(convert_point_string);

        plot_string << "#" << axes << ".plot(" << std::get<1>(convert_point_string) << ".readX(0), "
                    << std::get<1>(convert_point_string) << ".readY(0), "
                    << "label='" << *it << "')\n";
        plot_string << axes << ".errorbar(" << std::get<1>(convert_point_string) << ".readX(0), "
                    << std::get<1>(convert_point_string) << ".readY(0), "
                    << "yerr=" << std::get<1>(convert_point_string) << ".readE(0), "
                    << "label='" << *it << "')\n";

        plot_string << axes << ".set_yscale('log'); ";
        plot_string << axes << ".set_xscale('log')\n";
      }
      plot_string << axes << ".set_title('" << title << "')\n";
      plot_string << axes << ".grid() #Show a grid\n";
      plot_string << axes << ".legend(loc=" << legendLocation << ") #Show a legend\n";

      return plot_string.str();
    }

    /**
     Create string of python code to run reduction algorithm on the specified row
     @param rowNo : the row in the model to run the reduction algorithm on
     @return tuple containing the python string and the output workspace name
    */
    std::tuple<std::string, std::string, std::string, std::string, std::string> ReflGenerateNotebook::reduceRowString(int rowNo) {
      std::ostringstream code_string;

      const std::string runStr = m_model->data(m_model->index(rowNo, COL_RUNS)).toString().toStdString();
      const std::string transStr = m_model->data(m_model->index(rowNo, COL_TRANSMISSION)).toString().toStdString();
      const std::string options = m_model->data(m_model->index(rowNo, COL_OPTIONS)).toString().toStdString();

      double theta = 0;

      const bool thetaGiven = !m_model->data(m_model->index(rowNo, COL_ANGLE)).toString().isEmpty();

      if (thetaGiven)
        theta = m_model->data(m_model->index(rowNo, COL_ANGLE)).toDouble();

      const std::tuple<std::string, std::string> load_ws_string = loadWorkspaceString(runStr);
      code_string << std::get<0>(load_ws_string);

      const std::string runNo = getRunNumber(std::get<1>(load_ws_string));
      const std::string IvsLamName = "IvsLam_" + runNo;
      const std::string thetaName = "theta_" + runNo;

      if (!transStr.empty()) {
        const std::tuple<std::string, std::string> trans_string = transWSString(transStr);
        code_string << std::get<0>(trans_string);
        code_string << "IvsQ_" << runNo << ", " << IvsLamName << ", " << thetaName << " = ";
        code_string << "ReflectometryReductionOneAuto(InputWorkspace = '" << std::get<1>(load_ws_string) << "'";
        code_string << ", " << "FirstTransmissionRun = '" << std::get<1>(trans_string) << "'";
      }
      else {
        code_string << "IvsQ_" << runNo << ", " << IvsLamName << ", " << thetaName << " = ";
        code_string << "ReflectometryReductionOneAuto(InputWorkspace = '" << std::get<1>(load_ws_string) << "'";
      }

      std::string thetaStr;
      if (thetaGiven) {
        code_string << ", " << "ThetaIn = " << theta;
        thetaStr = static_cast<std::ostringstream *>( &(std::ostringstream() << theta))->str();
      }
      else {
        thetaStr = "theta_" + runNo; // Use variable name if we don't have the value
      }

      //Parse and set any user-specified options
      auto optionsMap = parseKeyValueString(options);
      for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
        code_string << ", " << kvp->first << " = " << kvp->second;
      }
      code_string << ")\n";

      const double scale = m_model->data(m_model->index(rowNo, COL_SCALE)).toDouble();
      if(scale != 1.0) {
        const std::tuple<std::string, std::string> scale_string = scaleString(runNo, scale);
        code_string << std::get<0>(scale_string);
      }

      const std::tuple<std::string, std::string> rebin_string = rebinString(rowNo, runNo);
      code_string << std::get<0>(rebin_string);

      return std::make_tuple(code_string.str(), std::get<1>(rebin_string), IvsLamName, thetaStr, runNo);
    }

     /**
      Create string of python code to run the scale algorithm
      @param runNo : the number of the run to scale
      @param scale : value of scaling factor to use
      @return tuple of strings of python code and output workspace name
      */
    std::tuple<std::string, std::string> ReflGenerateNotebook::scaleString(std::string runNo, double scale)
    {
      std::ostringstream scale_string;

      scale_string << "IvsQ_" << runNo << " = Scale(";
      scale_string << "InputWorkspace = IvsQ_" << runNo;
      scale_string << ", Factor = " << 1.0 / scale;
      scale_string << ")\n";

      return std::make_tuple(scale_string.str(), "IvsQ" + runNo);
    }

    /**
      Create string of python code to convert to point data, which can be plotted
      @param wsName : name of workspace to convert to point data
      @return tuple of strings of python code and output workspace name
      */
    std::tuple<std::string, std::string> ReflGenerateNotebook::convertToPointString(std::string wsName)
    {
      const std::string output_name = wsName + "_plot";
      std::ostringstream convert_string;
      convert_string << output_name << " = ConvertToPointData(" << wsName << ")\n";

      return std::make_tuple(convert_string.str(), output_name);
    }

    /**
     Create string of python code to rebin data in a workspace
     @param rowNo : the number of the row to rebin
     @param runNo : the number of the run to rebin
     @return tuple of strings of python code and output workspace name
    */
    std::tuple<std::string, std::string> ReflGenerateNotebook::rebinString(int rowNo, std::string runNo)
    {
      //We need to make sure that qmin and qmax are respected, so we rebin to
      //those limits here.
      std::ostringstream rebin_string;
      rebin_string << "IvsQ_" << runNo << " = ";
      rebin_string << "Rebin(";
      rebin_string << "IvsQ_" << runNo;

      const double qmin = m_model->data(m_model->index(rowNo, COL_QMIN)).toDouble();
      const double qmax = m_model->data(m_model->index(rowNo, COL_QMAX)).toDouble();
      const double dqq = m_model->data(m_model->index(rowNo, COL_DQQ)).toDouble();

      rebin_string << ", " << "Params = ";
      rebin_string << "'" << qmin << ", " << -dqq << ", " << qmax << "'";
      rebin_string << ")\n";

      return std::make_tuple(rebin_string.str(), "IvsQ_" + runNo);
    }

    /**
     Create string of python code to create a transmission workspace
     @param trans_ws_str : string of workspaces to create transmission workspace from
     @return tuple of strings of python code and output workspace name
    */
    std::tuple<std::string, std::string> ReflGenerateNotebook::transWSString(std::string trans_ws_str)
    {
      const size_t maxTransWS = 2;

      std::vector<std::string> transVec;
      std::ostringstream trans_string;
      std::vector<std::string> trans_ws_name;

      //Take the first two run numbers
      boost::split(transVec, trans_ws_str, boost::is_any_of(","));
      if(transVec.size() > maxTransWS)
        transVec.resize(maxTransWS);

      std::tuple<std::string, std::string> load_tuple;
      for(auto it = transVec.begin(); it != transVec.end(); ++it)
        load_tuple = loadWorkspaceString(*it);
        trans_ws_name.push_back(std::get<1>(load_tuple));
        trans_string << std::get<0>(load_tuple);

      //The runs are loaded, so we can create a TransWS
      std::string wsName = "TRANS_" + getRunNumber(trans_ws_name[0]);
      if(trans_ws_name.size() > 1)
        wsName += "_" + getRunNumber(trans_ws_name[1]);
      trans_string << wsName << " = ";
      trans_string << "CreateTransmissionWorkspaceAuto(";
      trans_string << "FirstTransmissionRun = '" << trans_ws_name[0] << "'";
      if(trans_ws_name.size() > 1)
        trans_string << ", SecondTransmissionRun = '" << trans_ws_name[1] << "'";
      trans_string << ")\n";

      return std::make_tuple(trans_string.str(), wsName);
    }

    /**
     Get run number from workspace name
     @param ws_name : workspace name
     @return run number, as a string
    */
    std::string ReflGenerateNotebook::getRunNumber(std::string ws_name) {
      //Matches TOF_13460 -> 13460
      boost::regex outputRegex("(TOF|IvsQ|IvsLam)_([0-9]+)");

      //Matches INTER13460 -> 13460
      boost::regex instrumentRegex("[a-zA-Z]{3,}([0-9]{3,})");

      boost::smatch matches;

      if (boost::regex_match(ws_name, matches, outputRegex)) {
        return matches[2].str();
      }
      else if (boost::regex_match(ws_name, matches, instrumentRegex)) {
        return matches[1].str();
      }

      //Resort to using the workspace name
      return ws_name;
    }

    /**
     Create string of python code to load workspaces
     @param runStr : string of workspaces to load
     @return tuple of strings of python code and output workspace name
    */
    std::tuple<std::string, std::string> ReflGenerateNotebook::loadWorkspaceString(std::string runStr) {
      std::vector<std::string> runs;
      boost::split(runs, runStr, boost::is_any_of("+"));

      std::ostringstream load_strings;

      //Remove leading/trailing whitespace from each run
      for (auto runIt = runs.begin(); runIt != runs.end(); ++runIt)
        boost::trim(*runIt);

      const std::string outputName = "TOF_" + boost::algorithm::join(runs, "_");

      std::tuple<std::string, std::string> load_string;

      load_string = loadRunString(runs[0]);
      load_strings << std::get<0>(load_string);

      // EXIT POINT if there is only one run
      if(runs.size() == 1) {
        return std::make_tuple(load_strings.str(), std::get<1>(load_string));
      }
      load_strings << outputName << " = " << std::get<1>(load_string) << "\n";

      // Load each subsequent run and add it to the first run
      for(auto runIt = std::next(runs.begin()); runIt != runs.end(); ++runIt)
      {
        load_string = loadRunString(*runIt);
        load_strings << std::get<0>(load_string);
        load_strings << plusString(std::get<1>(load_string), outputName);
      }

      return std::make_tuple(load_strings.str(), outputName);
    }

    /**
     Create string of python code to run the Plus algorithm on specified workspaces
     @param input_name : name of workspace to add to the other workspace
     @param output_name : other workspace will be added to the one with this name
     @return string of python code
    */
    std::string ReflGenerateNotebook::plusString(std::string input_name, std::string output_name)
    {
      std::ostringstream plus_string;

      plus_string << output_name << " = Plus('LHSWorkspace' = " << output_name;
      plus_string << ", 'RHSWorkspace' = " << input_name;
      plus_string << ")\n";

      return plus_string.str();
    }

    /**
     Create string of python code to load a single workspace
     @param run : run to load
     @return tuple of strings of python code and output workspace name
    */
    std::tuple<std::string, std::string> ReflGenerateNotebook::loadRunString(std::string run) {
      std::ostringstream load_string;
      // We do not have access to AnalysisDataService from notebook, so must load run from file
      const std::string filename = m_instrument + run;
      const std::string ws_name = "TOF_" + run;
      load_string << ws_name << " = ";
      load_string << "Load(";
      load_string << "Filename = '" << filename << "'";
      load_string << ")\n";

      return std::make_tuple(load_string.str(), ws_name);
    }

  }
}