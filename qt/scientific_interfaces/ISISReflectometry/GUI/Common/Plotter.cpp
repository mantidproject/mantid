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
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <functional>
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
QString scaleName(AxisScale scale) { return scale == AxisScale::Log ? QString("log") : QString("linear"); }

std::string axisLabel(PlotAxis const &axis) {
  if (axis.unit.empty())
    return axis.label;
  return axis.label + " (" + axis.unit + ")";
}

QHash<QString, QVariant> axisScaleProperties(PlotOptions const &options) {
  QHash<QString, QVariant> properties;
  properties[QString("xscale")] = QVariant(scaleName(options.xAxis.scale));
  properties[QString("yscale")] = QVariant(scaleName(options.yAxis.scale));
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

void forEachAxis(MantidQt::Widgets::Common::Python::Object const &figure,
                 std::function<void(MantidQt::Widgets::Common::Python::Object const &)> const &callback) {
  using namespace MantidQt::Widgets::Common;

  auto const axes = figure.attr("axes");
  auto const axesCount = Python::Len(axes);
  for (auto index = 0; index < axesCount; ++index)
    callback(Python::Object(axes[index]));
}

void applyAxisLabels(MantidQt::Widgets::Common::Python::Object const &figure, PlotOptions const &options) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const xLabel = axisLabel(options.xAxis);
    auto const yLabel = axisLabel(options.yAxis);
    forEachAxis(figure, [&xLabel, &yLabel](Python::Object const &axis) {
      if (!xLabel.empty())
        axis.attr("set_xlabel")(xLabel);
      if (!yLabel.empty())
        axis.attr("set_ylabel")(yLabel);
    });
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

void addHorizontalMarkers(MantidQt::Widgets::Common::Python::Object const &figure, double position) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const canvas = Python::Object(figure.attr("canvas"));
    auto const manager = Python::Object(canvas.attr("manager"));
    if (PyObject_HasAttrString(manager.ptr(), "_fig_interaction") == 0)
      return;

    auto const figureInteraction = manager.attr("_fig_interaction");
    forEachAxis(figure, [&figureInteraction, position](Python::Object const &axis) {
      auto const xLimits = axis.attr("get_xlim")();
      auto const lower = PyFloat_AsDouble(Python::Object(xLimits[0]).ptr());
      auto const upper = PyFloat_AsDouble(Python::Object(xLimits[1]).ptr());
      figureInteraction.attr("_add_horizontal_marker")(position, lower, upper, axis);
    });
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
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

  auto const axProperties = axisScaleProperties(options);
  auto const windowTitle = options.windowTitle.empty() ? actualWorkspaces.front() : options.windowTitle;
  // PlotLayout::Overplot means overlay this request in one fresh figure.
  // The MplCpp overplot flag instead targets an existing active figure.
  auto constexpr reuseExistingFigure = false;
  auto const tiled = options.layout == PlotLayout::Tiled;
  const std::vector<int> workspaceIndices = {0};

  auto const figure =
      MantidQt::Widgets::MplCpp::plot(actualWorkspaces, std::nullopt, workspaceIndices, std::nullopt, std::nullopt,
                                      axProperties, windowTitle, options.showErrors, reuseExistingFigure, tiled);
  applyAxisLabels(figure, options);
  if (options.horizontalMarker)
    addHorizontalMarkers(figure, *options.horizontalMarker);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
