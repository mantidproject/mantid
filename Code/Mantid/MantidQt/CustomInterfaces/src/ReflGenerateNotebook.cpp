#include "MantidQtCustomInterfaces/ReflGenerateNotebook.h"
#include "MantidAPI/NotebookWriter.h"

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
                                               const int d_qq) :
      m_wsName(name), m_model(model), m_instrument(instrument),
      COL_RUNS(runs_column), COL_TRANSMISSION(transmission_column),
      COL_OPTIONS(options_column), COL_ANGLE(angle_column),
      COL_QMIN(min_q), COL_QMAX(max_q), COL_DQQ(d_qq){ }

    /**
      Generate an ipython notebook
      @param rows : rows in the model which were processed
      @param groups : groups of rows which were stitched
      */
    void ReflGenerateNotebook::generateNotebook(std::map<int, std::set<int>> groups, std::set<int> rows) {
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
        for (auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt) {
          reduce_row_string = reduceRowString(*rIt);
          code_string << std::get<0>(reduce_row_string);
          ws_names.push_back(std::get<1>(reduce_row_string));
        }
        notebook->codeCell(code_string.str());

        //todo plot the unstitched I vs Q
        notebook->codeCell(plotIvsQ(ws_names));

        //todo stitch group
        //notebook->codeCell(stitchGroupString());

        //todo plot the stitched I vs Q
        //notebook->codeCell(plotIvsQ());
      }

      //TODO prompt for filename to save notebook
      const std::string filename = "/home/jonmd/refl_notebook.ipynb";

      std::string generatedNotebook = notebook->writeNotebook();
      std::ofstream file(filename.c_str(), std::ofstream::trunc);
      file << generatedNotebook;
      file.flush();
      file.close();
    }

    std::string ReflGenerateNotebook::plotIvsQ(std::vector<std::string> ws_names) {

      std::ostringstream plot_string;
      plot_string << "#Plot unstitched I vs Q\n";
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

    std::string ReflGenerateNotebook::stitchGroupString() {
      return "something";
    }

    /**
    Add a code cell to the notebook which runs reduce algorithm on the row specified
    @param notebook : the notebook to add the cell to
    @param rowNo : the row in the model to run the reduction algorithm on
    */
    std::tuple<std::string, std::string> ReflGenerateNotebook::reduceRowString(int rowNo) {
      std::ostringstream code_string;
      std::vector<std::string> workspace_names;

      const std::string runStr = m_model->data(m_model->index(rowNo, COL_RUNS)).toString().toStdString();
      const std::string transStr = m_model->data(m_model->index(rowNo, COL_TRANSMISSION)).toString().toStdString();
      const std::string options = m_model->data(m_model->index(rowNo, COL_OPTIONS)).toString().toStdString();

      double theta = 0;

      bool thetaGiven = !m_model->data(m_model->index(rowNo, COL_ANGLE)).toString().isEmpty();

      if (thetaGiven)
        theta = m_model->data(m_model->index(rowNo, COL_ANGLE)).toDouble();

      const std::tuple<std::string, std::string> load_ws_string = loadWorkspaceString(runStr);
      code_string << std::get<0>(load_ws_string);

      const std::string runNo = getRunNumber(std::get<1>(load_ws_string));

      if (!transStr.empty()) {
        std::tuple<std::string, std::string> trans_string = transWSString(transStr);
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

      std::tuple<std::string, std::string> reduce_string = reductionString(rowNo, runNo);
      code_string << std::get<0>(reduce_string);

      return std::make_tuple(code_string.str(), std::get<1>(reduce_string));
    }

    std::tuple<std::string, std::string> ReflGenerateNotebook::convertToPointString(std::string wsName)
    {
      const std::string output_name = wsName + "_plot";
      std::ostringstream convert_string;
      convert_string << output_name << " = ConvertToPointData(" << wsName << ")\n";

      return std::make_tuple(convert_string.str(), output_name);
    }

    std::tuple<std::string, std::string> ReflGenerateNotebook::reductionString(int rowNo, std::string runNo)
    {
      //We need to make sure that qmin and qmax are respected, so we rebin to
      //those limits here.
      std::ostringstream reduce_string;
      reduce_string << "IvsQ_" << runNo << " = ";
      reduce_string << "Rebin(";
      reduce_string << "IvsQ_" << runNo;

      const double qmin = m_model->data(m_model->index(rowNo, COL_QMIN)).toDouble();
      const double qmax = m_model->data(m_model->index(rowNo, COL_QMAX)).toDouble();
      const double dqq = m_model->data(m_model->index(rowNo, COL_DQQ)).toDouble();

      reduce_string << ", " << "Params = ";
      reduce_string << "'" << qmin << ", " << -dqq << ", " << qmax << "'";
      reduce_string << ")\n";

      return std::make_tuple(reduce_string.str(), "IvsQ_" + runNo);
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

      //Remove leading/trailing whitespace from each run
      for (auto runIt = runs.begin(); runIt != runs.end(); ++runIt)
        boost::trim(*runIt);

      //If it is only one run, just load that
      if (runs.size() == 1)
        return loadRunString(runs[0]);

      // todo deal with case of more than one run to load
      //const std::string outputName = "TOF_" + boost::algorithm::join(runs, "_");
      return std::make_tuple("temp", "temp");
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

    /**
    Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'` into a map of key/value pairs
    @param str The input string
    @throws std::runtime_error on an invalid input string
    */
    std::map<std::string, std::string> ReflGenerateNotebook::parseKeyValueString(const std::string &str) {
      //Tokenise, using '\' as an escape character, ',' as a delimiter and " and ' as quote characters
      boost::tokenizer<boost::escaped_list_separator<char> > tok(str,
                                                                 boost::escaped_list_separator<char>("\\", ",", "\"'"));

      std::map<std::string, std::string> kvp;

      for (auto it = tok.begin(); it != tok.end(); ++it) {
        std::vector<std::string> valVec;
        boost::split(valVec, *it, boost::is_any_of("="));

        if (valVec.size() > 1) {
          //We split on all '='s. The first delimits the key, the rest are assumed to be part of the value
          std::string key = valVec[0];
          //Drop the key from the values vector
          valVec.erase(valVec.begin());
          //Join the remaining sections,
          std::string value = boost::algorithm::join(valVec, "=");

          //Remove any unwanted whitespace
          boost::trim(key);
          boost::trim(value);

          if (key.empty() || value.empty())
            throw std::runtime_error("Invalid key value pair, '" + *it + "'");


          kvp[key] = value;
        }
        else {
          throw std::runtime_error("Invalid key value pair, '" + *it + "'");
        }
      }
      return kvp;
    }

  }
}