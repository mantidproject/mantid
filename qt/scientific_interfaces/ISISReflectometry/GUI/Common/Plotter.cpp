// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Plotter.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
QString scaleName(AxisScale scale) { return scale == AxisScale::Log ? QString("log") : QString("linear"); }

std::string axisLabel(PlotAxis const &axis) {
  if (axis.unit.empty())
    return axis.label;
  return axis.label + " (" + axis.unit + ")";
}

QHash<QString, QVariant> axisProperties(PlotOptions const &options) {
  QHash<QString, QVariant> properties;
  properties[QString("xscale")] = QVariant(scaleName(options.xAxis.scale));
  properties[QString("yscale")] = QVariant(scaleName(options.yAxis.scale));

  auto const xLabel = axisLabel(options.xAxis);
  if (!xLabel.empty())
    properties[QString("xlabel")] = QVariant(QString::fromStdString(xLabel));

  auto const yLabel = axisLabel(options.yAxis);
  if (!yLabel.empty())
    properties[QString("ylabel")] = QVariant(QString::fromStdString(yLabel));

  return properties;
}

std::vector<std::string> expandWorkspaceGroups(std::vector<std::string> const &workspaces) {
  auto actualWorkspaces = std::vector<std::string>{};
  actualWorkspaces.reserve(workspaces.size());
  for (const auto &workspace : workspaces) {
    const Mantid::API::Workspace_sptr workspaceObject =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspace);
    if (const auto workspaceGroup = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(workspaceObject)) {
      for (auto index = 0u; index < workspaceGroup->size(); ++index) {
        actualWorkspaces.emplace_back(workspaceGroup->getItem(index)->getName());
      }
    } else {
      actualWorkspaces.emplace_back(workspace);
    }
  }
  return actualWorkspaces;
}

QStringList toQStringList(std::vector<std::string> const &workspaces) {
  auto result = QStringList{};
  for (auto const &workspace : workspaces)
    result.append(QString::fromStdString(workspace));
  return result;
}
} // namespace

void Plotter::plot(PlotRequest const &request) const {
  if (request.workspaces.empty())
    return;

  // plot(workspaces, spectrum_nums, wksp_indices, fig, plot_kwargs,
  // ax_properties, windows_title, errors, overplot)
  auto const actualWorkspaces = expandWorkspaceGroups(request.workspaces);
  auto const &options = request.options;

  if (options.plotStyle == PlotStyle::Colorfill) {
    pcolormesh(toQStringList(actualWorkspaces));
    return;
  }

  auto const axProperties = axisProperties(options);
  auto const windowTitle = options.windowTitle.empty() ? actualWorkspaces.front() : options.windowTitle;
  auto const overplot = options.layout == PlotLayout::Overplot;
  auto const tiled = options.layout == PlotLayout::Tiled;
  const std::vector<int> workspaceIndices = {0};

  MantidQt::Widgets::MplCpp::plot(actualWorkspaces, std::nullopt, workspaceIndices, std::nullopt, std::nullopt,
                                  axProperties, windowTitle, options.showErrors, overplot, tiled);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
