// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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

namespace Mantid::CurveFitting::Algorithms {

// Ideally this would use boost::try_lexical_cast in order to avoid too many
// exceptions
// but we do not yet have the correct version of boost.
template <class Type> Type lexCast(std::string input, const std::string &errorMessage) {
  try {
    return boost::lexical_cast<Type>(input);
  } catch (boost::bad_lexical_cast &) {
    throw std::runtime_error(errorMessage + input);
  }
}

void parseValueRange(const std::string &index, double &start, double &end, int &wi, int &spec) {
  if (index.size() > 1) { // there is some text after 'v'
    Mantid::Kernel::StringTokenizer range(
        index.substr(1), ":", Kernel::StringTokenizer::TOK_IGNORE_EMPTY | Kernel::StringTokenizer::TOK_TRIM);
    if (range.count() < 1) {
      wi = WHOLE_RANGE; // means use the whole range
    } else if (range.count() == 1) {
      try {
        start = boost::lexical_cast<double>(range[0]);
      } catch (boost::bad_lexical_cast &) {
        throw std::runtime_error(std::string("Provided incorrect range values. Range is "
                                             "specfifed by start_value:stop_value, but "
                                             "provided ") +
                                 range[0]);
      }

      end = start;
      wi = NOT_SET;
      spec = NOT_SET;
    } else {
      std::string errorMessage = std::string("Provided incorrect range values. Range is "
                                             "specfifed by start_value:stop_value, but "
                                             "provided ") +
                                 range[0] + std::string(" and ") + range[1];
      start = lexCast<double>(range[0], errorMessage);
      end = lexCast<double>(range[1], errorMessage);

      if (start > end)
        std::swap(start, end);
      wi = NOT_SET;
      spec = NOT_SET;
    }
  } else {
    wi = WHOLE_RANGE;
  }
}

void addGroupWorkspace(std::vector<InputSpectraToFit> &nameList, double start, double end, int wi, int spec, int period,
                       const std::shared_ptr<API::WorkspaceGroup> &wsg);
void addMatrixworkspace(std::vector<InputSpectraToFit> &nameList, double start, double end, std::string &name, int wi,
                        int spec, int period, const std::optional<API::Workspace_sptr> &workspaceOptional,
                        const std::shared_ptr<API::MatrixWorkspace> &wsMatrix);
/// Create a list of input workspace names
std::vector<InputSpectraToFit> makeNames(const std::string &inputList, int default_wi, int default_spec) {
  std::vector<InputSpectraToFit> nameList;

  double start = 0;
  double end = 0;

  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer names(inputList, ";", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (const auto &input : names) {
    tokenizer params(input, ",", tokenizer::TOK_TRIM);
    std::string name = params[0];
    int wi = default_wi;
    int spec = default_spec;
    if (params.count() > 1) {
      std::string index = params[1];                        // spectrum or workspace index with a prefix
      if (index.size() > 2 && index.substr(0, 2) == "sp") { // spectrum number
        spec = boost::lexical_cast<int>(index.substr(2));
        wi = SpecialIndex::NOT_SET;                     // undefined yet
      } else if (index.size() > 1 && index[0] == 'i') { // workspace index
        wi = boost::lexical_cast<int>(index.substr(1));
        spec = SpecialIndex::NOT_SET; // undefined yet
      } else if (!index.empty() && index[0] == 'v') {
        parseValueRange(index, start, end, wi, spec);
      }
    }
    int period = 1;
    if (params.count() > 2 && !params[2].empty()) {
      period = lexCast<int>(params[2], "Incorrect value for a period: " + params[2]);
    }

    auto workspaceOptional = getWorkspace(name, period);
    if (!workspaceOptional)
      continue;

    auto wsg = std::dynamic_pointer_cast<API::WorkspaceGroup>(workspaceOptional.value());
    auto wsMatrix = std::dynamic_pointer_cast<API::MatrixWorkspace>(workspaceOptional.value());
    if (wsg) {
      addGroupWorkspace(nameList, start, end, wi, spec, period, wsg);

    } else if (wsMatrix) {
      addMatrixworkspace(nameList, start, end, name, wi, spec, period, workspaceOptional, wsMatrix);
    }
  }
  return nameList;
}
void addMatrixworkspace(std::vector<InputSpectraToFit> &nameList, double start, double end, std::string &name, int wi,
                        int spec, int period, const std::optional<API::Workspace_sptr> &workspaceOptional,
                        const std::shared_ptr<API::MatrixWorkspace> &wsMatrix) {
  auto workspaceIndices = getWorkspaceIndicesFromAxes(*wsMatrix, wi, spec, start, end);

  for (auto workspaceIndex : workspaceIndices) {
    nameList.emplace_back(name, workspaceIndex, period);
    if (workspaceOptional) {
      nameList.back().ws = wsMatrix;
    }
  }
}
void addGroupWorkspace(std::vector<InputSpectraToFit> &nameList, double start, double end, int wi, int spec, int period,
                       const std::shared_ptr<API::WorkspaceGroup> &wsg) {
  const std::vector<std::string> wsNames = wsg->getNames();

  for (const auto &wsName : wsNames) {
    if (auto workspace =
            std::dynamic_pointer_cast<API::MatrixWorkspace>(API::AnalysisDataService::Instance().retrieve(wsName))) {
      auto workspaceIndices = getWorkspaceIndicesFromAxes(*workspace, wi, spec, start, end);

      for (auto workspaceIndex : workspaceIndices) {
        nameList.emplace_back(wsName, workspaceIndex, period);
        nameList.back().ws = workspace;
      }
    }
  }
}

/** Get a workspace identified by an InputSpectraToFit structure.
 * @param ws :: Workspace to fit required to work out indices
 * @param workspaceIndex :: workspace index to use
 * @param spectrumNumber :: spectrum number to use
 * @param start :: Start of range for value based spectrum range
 * @param end :: End of range for value based spectrum range
 * @return Vector of workspace indices to fit
 */
std::vector<int> getWorkspaceIndicesFromAxes(const API::MatrixWorkspace &ws, int workspaceIndex, int spectrumNumber,
                                             double start, double end) {
  if (workspaceIndex >= 0) {
    return std::vector<int>({workspaceIndex});
  }
  std::vector<int> out;
  API::Axis *axis = ws.getAxis(1);
  if (axis->isSpectra()) {
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
    if (workspaceIndex <= SpecialIndex::WHOLE_RANGE) {
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

  return out;
}

std::optional<API::Workspace_sptr> getWorkspace(const std::string &workspaceName, int period) {
  if (API::AnalysisDataService::Instance().doesExist(workspaceName)) {
    return API::AnalysisDataService::Instance().retrieve(workspaceName);
  } else {
    std::string::size_type i = workspaceName.find_last_of('.');
    if (i == std::string::npos) {
      g_log.warning() << "Cannot open file " << workspaceName << "\n";
      return {};
    }
    auto load = Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    load->setChild(true);
    load->initialize();
    load->setPropertyValue("FileName", workspaceName);
    load->setProperty("OutputWorkspace", "__NotUsed");
    load->setRethrows(false);
    load->execute();
    if (load->isExecuted()) {
      API::Workspace_sptr rws = load->getProperty("OutputWorkspace");
      if (rws) {
        if (std::dynamic_pointer_cast<DataObjects::Workspace2D>(rws)) {
          return rws;
        } else {
          API::WorkspaceGroup_sptr gws = std::dynamic_pointer_cast<API::WorkspaceGroup>(rws);
          if (gws) {
            std::string propName = "OUTPUTWORKSPACE_" + std::to_string(period);
            if (load->existsProperty(propName)) {
              API::Workspace_sptr rws1 = load->getProperty(propName);
              return rws1;
            }
          }
        }
      }
    }
  }
  return {};
}

} // namespace Mantid::CurveFitting::Algorithms
