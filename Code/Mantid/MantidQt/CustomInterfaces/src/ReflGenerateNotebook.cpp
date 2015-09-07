#include "MantidQtCustomInterfaces/ReflGenerateNotebook.h"
#include "MantidAPI/NotebookWriter.h"
#include "MantidQtCustomInterfaces/ParseKeyValueString.h"

#include <sstream>
#include <fstream>
#include <memory>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

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
      @param rows : rows in the model which were processed
      @param groups : groups of rows which were stitched
      */
    void ReflGenerateNotebook::generateNotebook(std::map<int, std::set<int>> groups, std::set<int> rows, std::string filename) {
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
        std::tuple<std::string, std::string> reduce_row_string;
        std::vector<std::string> ws_names;
        code_string << "#Load and reduce\n";
        for (auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt) {
          reduce_row_string = reduceRowString(*rIt);
          code_string << std::get<0>(reduce_row_string);
          ws_names.push_back(std::get<1>(reduce_row_string));
        }
        notebook->codeCell(code_string.str());

        // Plot the unstitched I vs Q
        notebook->codeCell(plotIvsQ(ws_names));

        // Stitch group
        std::tuple<std::string, std::string> stitch_string = stitchGroupString(groupRows);
        notebook->codeCell(std::get<0>(stitch_string));

        // Plot the stitched I vs Q
        std::vector<std::string> stitched_ws;
        stitched_ws.push_back(std::get<1>(stitch_string));
        notebook->codeCell(plotIvsQ(stitched_ws));
      }

      std::string generatedNotebook = notebook->writeNotebook();
      std::ofstream file(filename.c_str(), std::ofstream::trunc);
      file << generatedNotebook;
      file.flush();
      file.close();
    }

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

    std::string ReflGenerateNotebook::plotIvsQ(std::vector<std::string> ws_names) {

      std::ostringstream plot_string;
      plot_string << "#Plot I vs Q\n";
      for (auto it = ws_names.begin(); it != ws_names.end(); ++it) {
        std::tuple<std::string, std::string> convert_point_string = convertToPointString(*it);
        plot_string << std::get<0>(convert_point_string);

        plot_string << "plt.loglog(" << std::get<1>(convert_point_string) << ".readX(0), "
                    << std::get<1>(convert_point_string) << ".readY(0), "
                    << "basex=10, label='" << *it << "')\n";
      }
      plot_string << "plt.title('Unstitched I vs Q')\n";
      plot_string << "plt.grid() #Show a grid\n";
      plot_string << "plt.legend() #Show a legend\n";
      plot_string << "plt.show() #Draw the plot\n";

      return plot_string.str();
    }

    /**
    Add a code cell to the notebook which runs reduce algorithm on the row specified
    @param notebook : the notebook to add the cell to
    @param rowNo : the row in the model to run the reduction algorithm on
    */
    std::tuple<std::string, std::string> ReflGenerateNotebook::reduceRowString(int rowNo) {
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

      if (!transStr.empty()) {
        const std::tuple<std::string, std::string> trans_string = transWSString(transStr);
        code_string << std::get<0>(trans_string);
        code_string << "IvsQ_" << runNo << ", " << "IvsLam_" << runNo << ", _ = ";
        code_string << "ReflectometryReductionOneAuto(InputWorkspace = '" << std::get<1>(load_ws_string) << "'";
        code_string << ", " << "FirstTransmissionRun = '" << std::get<1>(trans_string) << "'";
      }
      else {
        code_string << "IvsQ_" << runNo << ", " << "IvsLam_" << runNo << ", _ = ";
        code_string << "ReflectometryReductionOneAuto(InputWorkspace = '" << std::get<1>(load_ws_string) << "'";
      }

      if (thetaGiven)
        code_string << ", " << "ThetaIn = " << theta;

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

      return std::make_tuple(code_string.str(), std::get<1>(rebin_string));
    }

    std::tuple<std::string, std::string> ReflGenerateNotebook::scaleString(std::string runNo, double scale)
    {
      std::ostringstream scale_string;

      scale_string << "IvsQ_" << runNo << " = Scale(";
      scale_string << "InputWorkspace = IvsQ_" << runNo;
      scale_string << ", Factor = " << 1.0 / scale;
      scale_string << ")\n";

      return std::make_tuple(scale_string.str(), "IvsQ" + runNo);
    }

    std::tuple<std::string, std::string> ReflGenerateNotebook::convertToPointString(std::string wsName)
    {
      const std::string output_name = wsName + "_plot";
      std::ostringstream convert_string;
      convert_string << output_name << " = ConvertToPointData(" << wsName << ")\n";

      return std::make_tuple(convert_string.str(), output_name);
    }

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

    std::string ReflGenerateNotebook::plusString(std::string input_name, std::string output_name)
    {
      std::ostringstream plus_string;

      plus_string << output_name << " = Plus('LHSWorkspace' = " << output_name;
      plus_string << ", 'RHSWorkspace' = " << input_name;
      plus_string << ")\n";

      return plus_string.str();
    }

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