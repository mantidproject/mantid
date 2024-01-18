// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "WorkspaceManipulationUtils.h"
#include "InterfaceUtils.h"
#include "MantidKernel/Logger.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidGeometry/Instrument.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
Mantid::Kernel::Logger g_log("WorkspaceManipulationUtils");

double roundToPrecision(double value, double precision) { return value - std::remainder(value, precision); }

QPair<double, double> roundRangeToPrecision(double rangeStart, double rangeEnd, double precision) {
  return QPair<double, double>(roundToPrecision(rangeStart, precision) + precision,
                               roundToPrecision(rangeEnd, precision) - precision);
}

} // namespace
namespace MantidQt {
namespace CustomInterfaces {
namespace WorkspaceManipulationUtils {

/**
 * Gets the suffix of a workspace (i.e. part after last underscore (red, sqw)).
 *
 * @param wsName Name of workspace
 * @return Suffix, or empty string if no underscore
 */
std::string getWorkspaceSuffix(const std::string &wsName) {
  auto const lastUnderscoreIndex = wsName.find_last_of("_");
  if (lastUnderscoreIndex == -1)
    return std::string();
  return wsName.substr(lastUnderscoreIndex);
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
QString getWorkspaceBasename(const QString &wsName) {
  int lastUnderscoreIndex = wsName.lastIndexOf("_");
  if (lastUnderscoreIndex == -1)
    return QString(wsName);

  return wsName.left(lastUnderscoreIndex);
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
double getEFixed(const Mantid::API::MatrixWorkspace_sptr &ws) {
  Mantid::Geometry::Instrument_const_sptr inst = ws->getInstrument();
  if (!inst)
    throw std::runtime_error("No instrument on workspace");

  // Try to get the parameter form the base instrument
  if (inst->hasParameter("Efixed"))
    return inst->getNumberParameter("Efixed")[0];

  // Try to get it form the analyser component
  if (inst->hasParameter("analyser")) {
    std::string analyserName = inst->getStringParameter("analyser")[0];
    auto analyserComp = inst->getComponentByName(analyserName);

    if (analyserComp && analyserComp->hasParameter("Efixed"))
      return analyserComp->getNumberParameter("Efixed")[0];
  }

  throw std::runtime_error("Instrument has no efixed parameter");
}

/**
 * Checks the workspace's instrument for a resolution parameter to use as
 * a default for the energy range on the mini plot
 *
 * @param workspace :: Name of the workspace to use
 * @param res :: The retrieved values for the resolution parameter (if one was
 *found)
 */
bool getResolutionRangeFromWs(const QString &workspace, QPair<double, double> &res) {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();
  auto const ws = ads.retrieveWS<const Mantid::API::MatrixWorkspace>(workspace.toStdString());
  return getResolutionRangeFromWs(ws, res);
}
/**
 * Checks the workspace's instrument for a resolution parameter to use as
 * a default for the energy range on the mini plot
 *
 * @param ws :: Name of the workspace to use
 * @param res :: The retrieved values for the resolution parameter (if one was
 *found)
 */
bool getResolutionRangeFromWs(const Mantid::API::MatrixWorkspace_const_sptr &workspace, QPair<double, double> &res) {
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
            res = qMakePair(-params[0], params[0]);
            return true;
          }
        }
      }
    }
  }
  return false;
}

QPair<double, double> getXRangeFromWorkspace(std::string const &workspaceName, double precision) {
  auto const &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    return getXRangeFromWorkspace(ads.retrieveWS<MatrixWorkspace>(workspaceName), precision);
  return QPair<double, double>(0.0, 0.0);
}

QPair<double, double> getXRangeFromWorkspace(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                                             double precision) {
  auto const xValues = workspace->x(0);
  return roundRangeToPrecision(xValues.front(), xValues.back(), precision);
}

} // namespace WorkspaceManipulationUtils
} // namespace CustomInterfaces
} // namespace MantidQt
