// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValueHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/StringTokenizer.h"
#include <boost/lexical_cast.hpp>
#include <fstream>

namespace {
Mantid::Kernel::Logger g_log("PlotPeakByLogValue");
}

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/// Create a list of input workspace names
std::vector<InputData> makeNames(std::string inputList, int default_wi,
                                 int default_spec) {
  std::vector<InputData> nameList;

  double start = 0;
  double end = 0;

  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer names(inputList, ";",
                  tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (const auto &input : names) {
    tokenizer params(input, ",", tokenizer::TOK_TRIM);
    std::string name = params[0];
    int wi = default_wi;
    int spec = default_spec;
    if (params.count() > 1) {
      std::string index =
          params[1]; // spectrum or workspace index with a prefix
      if (index.size() > 2 && index.substr(0, 2) == "sp") { // spectrum number
        spec = boost::lexical_cast<int>(index.substr(2));
        wi = -1;                                        // undefined yet
      } else if (index.size() > 1 && index[0] == 'i') { // workspace index
        wi = boost::lexical_cast<int>(index.substr(1));
        spec = -1; // undefined yet
      } else if (!index.empty() && index[0] == 'v') {
        if (index.size() > 1) { // there is some text after 'v'
          tokenizer range(index.substr(1), ":",
                          tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
          if (range.count() < 1) {
            wi = -2; // means use the whole range
          } else if (range.count() == 1) {
            try {
              start = boost::lexical_cast<double>(range[0]);
            } catch (boost::bad_lexical_cast &) {
              throw std::runtime_error(
                  std::string("Provided incorrect range values. Range is "
                              "specfifed by start_value:stop_value, but "
                              "provided ") +
                  range[0]);
            }

            end = start;
            wi = -1;
            spec = -1;
          } else if (range.count() > 1) {
            try {
              start = boost::lexical_cast<double>(range[0]);
              end = boost::lexical_cast<double>(range[1]);
            } catch (boost::bad_lexical_cast &) {
              throw std::runtime_error(
                  std::string("Provided incorrect range values. Range is "
                              "specfifed by start_value:stop_value, but "
                              "provided ") +
                  range[0] + std::string(" and ") + range[1]);
            }
            if (start > end)
              std::swap(start, end);
            wi = -1;
            spec = -1;
          }
        } else {
          wi = -2;
        }
      } else {
        wi = default_wi;
      }
    }
    int period = 1;
    try {
      if (params.count() > 2 && !params[2].empty()) {
        period = boost::lexical_cast<int>(params[2]);
      }
    } catch (boost::bad_lexical_cast &) {
      throw std::runtime_error("Incorrect value for a period: " + params[2]);
    }
    if (API::AnalysisDataService::Instance().doesExist(name)) {
      API::Workspace_sptr ws =
          API::AnalysisDataService::Instance().retrieve(name);
      API::WorkspaceGroup_sptr wsg =
          boost::dynamic_pointer_cast<API::WorkspaceGroup>(ws);
      if (wsg) {
        const std::vector<std::string> wsNames = wsg->getNames();
        for (const auto &wsName : wsNames) {
          nameList.emplace_back(InputData(wsName, wi, -1, period, start, end));
        }
        continue;
      }
    }
    nameList.emplace_back(name, wi, spec, period, start, end);
  }
  return nameList;
}

/** Get a workspace identified by an InputData structure.
 * @param data :: InputData with name and either spec or i fields defined.
 * @return InputData structure with the ws field set if everything was OK.
 */
InputData getWorkspace(const InputData &data, API::IAlgorithm_sptr load) {
  InputData out(data);
  if (API::AnalysisDataService::Instance().doesExist(data.name)) {
    DataObjects::Workspace2D_sptr ws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve(data.name));
    if (ws) {
      out.ws = ws;
    } else {
      return data;
    }
  } else {
    std::ifstream fil(data.name.c_str());
    if (!fil) {
      g_log.warning() << "File " << data.name << " does not exist\n";
      return data;
    }
    fil.close();
    std::string::size_type i = data.name.find_last_of('.');
    if (i == std::string::npos) {
      g_log.warning() << "Cannot open file " << data.name << "\n";
      return data;
    }
    try {
      load->initialize();
      load->setPropertyValue("FileName", data.name);
      load->execute();
      if (load->isExecuted()) {
        API::Workspace_sptr rws = load->getProperty("OutputWorkspace");
        if (rws) {
          DataObjects::Workspace2D_sptr ws =
              boost::dynamic_pointer_cast<DataObjects::Workspace2D>(rws);
          if (ws) {
            out.ws = ws;
          } else {
            API::WorkspaceGroup_sptr gws =
                boost::dynamic_pointer_cast<API::WorkspaceGroup>(rws);
            if (gws) {
              std::string propName =
                  "OUTPUTWORKSPACE_" + std::to_string(data.period);
              if (load->existsProperty(propName)) {
                API::Workspace_sptr rws1 = load->getProperty(propName);
                out.ws =
                    boost::dynamic_pointer_cast<DataObjects::Workspace2D>(rws1);
              }
            }
          }
        }
      }
    } catch (std::exception &e) {
      g_log.error(e.what());
      return data;
    }
  }

  if (!out.ws)
    return data;

  API::Axis *axis = out.ws->getAxis(1);
  if (axis->isSpectra()) { // spectra axis
    if (out.spec < 0) {
      if (out.i >= 0) {
        out.spec = axis->spectraNo(out.i);
      } else { // i < 0 && spec < 0 => use start and end
        for (size_t i = 0; i < axis->length(); ++i) {
          auto s = double(axis->spectraNo(i));
          if (s >= out.start && s <= out.end) {
            out.indx.emplace_back(static_cast<int>(i));
          }
        }
      }
    } else {
      for (size_t i = 0; i < axis->length(); ++i) {
        int j = axis->spectraNo(i);
        if (j == out.spec) {
          out.i = static_cast<int>(i);
          break;
        }
      }
    }
    if (out.i < 0 && out.indx.empty()) {
      return data;
    }
  } else { // numeric axis
    out.spec = -1;
    if (out.i >= 0) {
      out.indx.clear();
    } else {
      if (out.i < -1) {
        out.start = (*axis)(0);
        out.end = (*axis)(axis->length() - 1);
      }
      for (size_t i = 0; i < axis->length(); ++i) {
        double s = (*axis)(i);
        if (s >= out.start && s <= out.end) {
          out.indx.emplace_back(static_cast<int>(i));
        }
      }
    }
  }

  return out;
}
} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid