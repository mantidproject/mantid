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

namespace {
Mantid::Kernel::Logger g_log("PlotPeakByLogValue");
}

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/// Create a list of input workspace names
std::vector<InputSpectraToFit> makeNames(std::string inputList, int default_wi,
                                         int default_spec) {
  std::vector<InputSpectraToFit> nameList;

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
                              "provided ") + //#include "MantidAPI/FileFinder.h"

                  range[0] +
                  std::string(" and ") + range[1]);
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

    auto workspaceOptional = getWorkspace(name, period);
    if (!workspaceOptional)
      continue;

    auto wsg = boost::dynamic_pointer_cast<API::WorkspaceGroup>(
        workspaceOptional.value());
    if (wsg) {
      const std::vector<std::string> wsNames = wsg->getNames();

      for (const auto &wsName : wsNames) {
        auto workspace = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
            API::AnalysisDataService::Instance().retrieve(wsName));
        if (!workspace)
          continue;
        auto workspaceIndices =
            getWorkspaceIndicesFromAxes(*workspace, wi, spec, start, end);

        for (auto workspaceIndex : workspaceIndices) {
          nameList.emplace_back(wsName, workspaceIndex, period);
          if (workspace) {
            nameList.back().ws = workspace;
          }
        }
      }
      continue;
    }

    auto wsMatrix = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
        workspaceOptional.value());
    if (!wsMatrix)
      continue;
    auto workspaceIndices =
        getWorkspaceIndicesFromAxes(*wsMatrix, wi, spec, start, end);

    for (auto workspaceIndex : workspaceIndices) {
      nameList.emplace_back(name, workspaceIndex, period);
      if (workspaceOptional) {
        nameList.back().ws = wsMatrix;
      }
    }
  }
  return nameList;
}

/** Get a workspace identified by an InputSpectraToFit structure.
 * @param ws :: Workspace to fit required to work out indices
 * @param workspaceIndex :: workspace index to use
 * @param spectrumNumber :: spectrum number to use
 * @param start :: Start of range for value based spectrum range
 * @param end :: End of range for value based spectrum range
 * @return Vector of workspace indices to fit
 */
std::vector<int> getWorkspaceIndicesFromAxes(API::MatrixWorkspace &ws,
                                             int workspaceIndex,
                                             int spectrumNumber, double start,
                                             double end) {
  if (workspaceIndex >= 0) {
    return std::vector<int>({workspaceIndex});
  }
  std::vector<int> out;
  API::Axis *axis = ws.getAxis(1);
  if (axis->isSpectra()) { // spectra axis
    if (spectrumNumber < 0) {
      for (size_t i = 0; i < axis->length(); ++i) {
        auto s = double(axis->spectraNo(i));
        if (s >= start && s <= end) {
          out.emplace_back(static_cast<int>(i));
        }
      }

    } else {
      for (size_t i = 0; i < axis->length(); ++i) {
        int j = axis->spectraNo(i);
        if (j == spectrumNumber) {
          out.emplace_back(static_cast<int>(i));
          break;
        }
      }
    }
  } else { // numeric axis
    spectrumNumber = -1;
    if (workspaceIndex >= 0) {
      out.clear();
    } else {
      if (workspaceIndex < -1) {
        start = (*axis)(0);
        end = (*axis)(axis->length() - 1);
      }
      for (size_t i = 0; i < axis->length(); ++i) {
        double s = (*axis)(i);
        if (s >= start && s <= end) {
          out.emplace_back(static_cast<int>(i));
        }
      }
    }
  }

  return out;
}

boost::optional<API::Workspace_sptr>
getWorkspace(const std::string &workspaceName, int period) {
  if (API::AnalysisDataService::Instance().doesExist(workspaceName)) {
    return API::AnalysisDataService::Instance().retrieve(workspaceName);
  } else {
    std::string::size_type i = workspaceName.find_last_of('.');
    if (i == std::string::npos) {
      g_log.warning() << "Cannot open file " << workspaceName << "\n";
      return {};
    }
    try {
      auto load =
          Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
      load->setChild(true);
      load->initialize();
      load->setPropertyValue("FileName", workspaceName);
      load->setProperty("OutputWorkspace", "__NotUsed");
      load->execute();
      if (load->isExecuted()) {
        API::Workspace_sptr rws = load->getProperty("OutputWorkspace");
        if (rws) {
          if (boost::dynamic_pointer_cast<DataObjects::Workspace2D>(rws)) {
            return rws;
          } else {
            API::WorkspaceGroup_sptr gws =
                boost::dynamic_pointer_cast<API::WorkspaceGroup>(rws);
            if (gws) {
              std::string propName =
                  "OUTPUTWORKSPACE_" + std::to_string(period);
              if (load->existsProperty(propName)) {
                API::Workspace_sptr rws1 = load->getProperty(propName);
                return rws1;
              }
            }
          }
        }
      }
    } catch (std::exception &e) {
      g_log.error(e.what());
      return {};
    }
  }
  return {};
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid