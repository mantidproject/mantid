// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/TableWidgetValidators.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument.h"
#include <boost/algorithm/string.hpp>
#include <regex>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("WorkspaceUtils");

double roundToPrecision(double value, double precision) { return value - std::remainder(value, precision); }

std::pair<double, double> roundRangeToPrecision(double rangeStart, double rangeEnd, double precision) {
  return std::pair<double, double>(roundToPrecision(rangeStart, precision) + precision,
                                   roundToPrecision(rangeEnd, precision) - precision);
}

auto const regDigits = std::regex("\\d+");
} // namespace
namespace MantidQt {
namespace MantidWidgets {
namespace WorkspaceUtils {

/**
 * Gets the maximum number of histograms for a 2D Workspace
 *
 * @param workspace Workspace pointer
 * @return size_t if index is found or null pointer if not found.
 */

std::optional<std::size_t> maximumIndex(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  if (workspace) {
    const auto numberOfHistograms = workspace->getNumberHistograms();
    if (numberOfHistograms > 0)
      return numberOfHistograms - 1;
  }
  return std::nullopt;
}

std::string getIndexString(const MatrixWorkspace_sptr &workspace) {
  const auto maximum = maximumIndex(workspace);
  if (maximum)
    return "0-" + std::to_string(*maximum);
  return "";
}

std::string getIndexString(const std::string &workspaceName) { return getIndexString(getADSWorkspace(workspaceName)); }

/**
 * Gets the suffix of a workspace (i.e. part after last underscore (red, sqw)).
 *
 * @param wsName Name of workspace
 * @return Suffix, or empty string if no underscore
 */
std::string getWorkspaceSuffix(const std::string &wsName) {
  auto const lastUnderscoreIndex = wsName.find_last_of("_");
  if (lastUnderscoreIndex == wsName.npos)
    return std::string();
  return wsName.substr(lastUnderscoreIndex + 1);
}

/**
 * Returns the basename of a workspace (i.e. the part before the last
 *underscore)
 *
 * e.g. basename of irs26176_graphite002_red is irs26176_graphite002
 *
 * @param wsName Name of workspace
 * @return Base name, or wsName if no underscore
 */
std::string getWorkspaceBasename(const std::string &wsName) {
  auto lastUnderscoreIndex = wsName.find_last_of("_");
  if (lastUnderscoreIndex == wsName.npos)
    return wsName;

  return wsName.substr(0, lastUnderscoreIndex);
}

/* Extracts the labels from the axis at the specified index in the
 * specified workspace.
 *
 * @param workspace Constant reference to the matrix workspace
 * @param axisIndex  Index of the selected axis
 * @return map of label, index pairs of labels of selected axis.
 */
std::unordered_map<std::string, size_t> extractAxisLabels(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                                                          const size_t &axisIndex) {
  Axis *axis = workspace->getAxis(axisIndex);
  if (!axis->isText())
    return std::unordered_map<std::string, size_t>();

  auto *textAxis = static_cast<TextAxis *>(axis);
  std::unordered_map<std::string, size_t> labels;

  for (size_t i = 0; i < textAxis->length(); ++i)
    labels[textAxis->label(i)] = i;
  return labels;
}

/**
 * Gets the energy mode from a workspace based on the X unit.
 *
 * Units of dSpacing typically denote diffraction, hence Elastic.
 * All other units default to spectroscopy, therefore Indirect.
 *
 * @param ws Pointer to the workspace
 * @return Energy mode
 */
std::string getEMode(const Mantid::API::MatrixWorkspace_sptr &ws) {
  Mantid::Kernel::Unit_sptr xUnit = ws->getAxis(0)->unit();
  std::string xUnitName = xUnit->caption();

  g_log.debug() << "X unit name is: " << xUnitName << '\n';
  if (boost::algorithm::find_first(xUnitName, "d-Spacing"))
    return "Elastic";

  return "Indirect";
}

/**
 * Gets the eFixed value from the workspace using the instrument parameters.
 *
 * @param ws Pointer to the workspace
 * @return eFixed value
 */
std::optional<double> getEFixed(const Mantid::API::MatrixWorkspace_sptr &ws) {
  Mantid::Geometry::Instrument_const_sptr inst = ws->getInstrument();
  if (!inst)
    return std::nullopt;

  // Try to get it from the analyser component
  if (inst->hasParameter("analyser")) {
    auto const analyserName = inst->getStringParameter("analyser")[0];
    auto const analyserComp = inst->getComponentByName(analyserName != "fmica" ? analyserName : "mica");

    if (analyserComp && analyserComp->hasParameter("Efixed"))
      return analyserComp->getNumberParameter("Efixed")[0];
  }

  // Try to get the parameter form the base instrument
  if (inst->hasParameter("Efixed"))
    return inst->getNumberParameter("Efixed")[0];
  return std::nullopt;
}

/**
 * Checks the workspace's instrument for a resolution parameter to use as
 * a default for the energy range on the mini plot
 *
 * @param workspace :: Name of the workspace to use
 * @param res :: The retrieved values for the resolution parameter (if one was
 *found)
 */
bool getResolutionRangeFromWs(const std::string &workspace, std::pair<double, double> &res) {
  auto const ws =
      Mantid::API::AnalysisDataService::Instance().retrieveWS<const Mantid::API::MatrixWorkspace>(workspace);
  return getResolutionRangeFromWs(ws, res);
}
/**
 * Checks the workspace's instrument for a resolution parameter to use as
 * a default for the energy range on the mini plot
 *
 * @param workspace :: Name of the workspace to use
 * @param res :: The retrieved values for the resolution parameter (if one was
 *found)
 */
bool getResolutionRangeFromWs(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                              std::pair<double, double> &res) {
  if (workspace) {
    auto const instrument = workspace->getInstrument();
    if (instrument && instrument->hasParameter("analyser")) {
      auto const analyser = instrument->getStringParameter("analyser");
      if (analyser.size() > 0) {
        auto comp = instrument->getComponentByName(analyser[0]);
        if (comp) {
          auto params = comp->getNumberParameter("resolution", true);

          // set the default instrument resolution
          if (params.size() > 0) {
            res = std::pair(-params[0], params[0]);
            return true;
          }
        }
      }
    }
  }
  return false;
}

std::pair<double, double> getXRangeFromWorkspace(std::string const &workspaceName, double precision) {
  auto const &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    return getXRangeFromWorkspace(ads.retrieveWS<MatrixWorkspace>(workspaceName), precision);
  return std::pair<double, double>(0.0, 0.0);
}

std::pair<double, double> getXRangeFromWorkspace(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                                                 double precision) {
  assert(workspace != nullptr);
  auto const &xValues = workspace->x(0);
  return roundRangeToPrecision(xValues.front(), xValues.back(), precision);
}

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

bool doAllWsExistInADS(std::vector<std::string> const &workspaceNames) {
  return AnalysisDataService::Instance().doAllWsExist(workspaceNames);
}

std::vector<std::string> attachPrefix(std::vector<std::string> const &strings, std::string const &prefix) {
  return transformElements(strings.begin(), strings.end(), [&prefix](std::string const &str) { return prefix + str; });
}

/**
 * Checks the name of the input workspace against a regexp for prefixes in the form `instrName\#runNumber_...`, where
 * \#runNumber is a number indicating the specific instrument run number that the workspace refers to.
 * As output, it returns a single string with format instrName\#firstRunNumber-\#finalRunNumber_..., if run numbers are
 * found.
 *
 * @param workspaceNames :: a vector of strings containing a series of workspace names
 */

std::string parseRunNumbers(std::vector<std::string> const &workspaceNames) {
  auto names = WorkspaceUtils::transformElements(workspaceNames.begin(), workspaceNames.end(),
                                                 [](auto &name) { return name.substr(0, name.find_first_of('_')); });
  std::smatch match;
  std::vector<int> runNumbers;
  std::string prefix;
  if (workspaceNames.empty()) {
    return std::string("");
  }
  std::string suffix = workspaceNames[0].substr(workspaceNames[0].find_first_of('_'));
  for (auto const &name : names)
    if (std::regex_search(name, match, regDigits)) {
      runNumbers.push_back(std::stoi(match.str(0)));
      if (prefix.empty())
        prefix = match.prefix().str();
    }
  if (runNumbers.empty() || runNumbers.size() == 1)
    return workspaceNames[0];
  else {
    auto [min, max] = std::minmax_element(runNumbers.cbegin(), runNumbers.cend());
    return prefix + std::to_string(*min) + "-" + std::to_string(*max) + suffix;
  }
}

} // namespace WorkspaceUtils
} // namespace MantidWidgets
} // namespace MantidQt
