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
#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

#include <QHash>
#include <QString>
#include <QVariant>
#include <QWidget>
#include <QWindow>

#include <boost/python/extract.hpp>

#include <algorithm>
#include <functional>
#include <vector>
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

Mantid::API::Workspace_sptr retrieveWorkspace(std::string const &workspace) {
  return Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspace);
}

bool isWorkspaceGroup(std::string const &workspace) {
  return std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(retrieveWorkspace(workspace)) != nullptr;
}

std::vector<std::string> expandWorkspaceGroups(std::vector<std::string> const &workspaces) {
  auto actualWorkspaces = std::vector<std::string>{};
  actualWorkspaces.reserve(workspaces.size());
  for (const auto &workspace : workspaces) {
    const auto workspaceObject = retrieveWorkspace(workspace);
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

std::vector<std::vector<std::string>> groupedWorkspaceNames(std::vector<std::string> const &workspaces) {
  auto groupedWorkspaces = std::vector<std::vector<std::string>>{};
  groupedWorkspaces.reserve(workspaces.size());
  for (auto const &workspace : workspaces) {
    groupedWorkspaces.emplace_back(expandWorkspaceGroups({workspace}));
  }
  return groupedWorkspaces;
}

void forEachAxis(MantidQt::Widgets::Common::Python::Object const &figure,
                 std::function<void(MantidQt::Widgets::Common::Python::Object const &)> const &callback) {
  using namespace MantidQt::Widgets::Common;

  auto const axes = figure.attr("axes");
  auto const axesCount = Python::Len(axes);
  for (auto index = 0; index < axesCount; ++index)
    callback(Python::Object(axes[index]));
}

void forEachAxisInList(MantidQt::Widgets::Common::Python::Object const &axes,
                       std::function<void(MantidQt::Widgets::Common::Python::Object const &)> const &callback) {
  using namespace MantidQt::Widgets::Common;

  auto const axesCount = Python::Len(axes);
  for (auto index = 0; index < axesCount; ++index)
    callback(Python::Object(axes[index]));
}

void applyAxisLabelsToAxes(MantidQt::Widgets::Common::Python::Object const &axes, PlotOptions const &options) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const xLabel = axisLabel(options.xAxis);
    auto const yLabel = axisLabel(options.yAxis);
    forEachAxisInList(axes, [&xLabel, &yLabel](Python::Object const &axis) {
      if (!xLabel.empty())
        axis.attr("set_xlabel")(xLabel);
      if (!yLabel.empty())
        axis.attr("set_ylabel")(yLabel);
    });
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
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

void applyColorfillAxisLabels(MantidQt::Widgets::Common::Python::Object const &figure, PlotOptions const &options) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const xLabel = axisLabel(options.xAxis);
    auto const yLabel = axisLabel(options.yAxis);
    auto const zLabel = axisLabel(options.zAxis);

    forEachAxis(figure, [&xLabel, &yLabel, &zLabel](Python::Object const &axis) {
      auto const axisLabel = boost::python::extract<std::string>(axis.attr("get_label")())();
      if (axisLabel == "<colorbar>") {
        if (!zLabel.empty())
          axis.attr("set_ylabel")(zLabel);
        return;
      }

      if (!xLabel.empty())
        axis.attr("set_xlabel")(xLabel);
      if (!yLabel.empty())
        axis.attr("set_ylabel")(yLabel);
    });
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

void addHorizontalMarkersToAxes(MantidQt::Widgets::Common::Python::Object const &figure,
                                MantidQt::Widgets::Common::Python::Object const &axes, double position) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const canvas = Python::Object(figure.attr("canvas"));
    auto const manager = Python::Object(canvas.attr("manager"));
    if (PyObject_HasAttrString(manager.ptr(), "_fig_interaction") == 0)
      return;

    auto const figureInteraction = manager.attr("_fig_interaction");
    forEachAxisInList(axes, [&figureInteraction, position](Python::Object const &axis) {
      auto const xLimits = axis.attr("get_xlim")();
      auto const lower = PyFloat_AsDouble(Python::Object(xLimits[0]).ptr());
      auto const upper = PyFloat_AsDouble(Python::Object(xLimits[1]).ptr());
      figureInteraction.attr("_add_horizontal_marker")(position, lower, upper, axis);
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

void setPlotWindowParent(MantidQt::Widgets::Common::Python::Object const &figure, QWidget *parent) {
  using namespace MantidQt::Widgets::Common;

  if (!parent)
    return;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const canvas = Python::Object(figure.attr("canvas"));
    auto const manager = Python::Object(canvas.attr("manager"));
    if (PyObject_HasAttrString(manager.ptr(), "window") == 0)
      return;

    auto const window = Python::Object(manager.attr("window"));
    auto *plotWindow = Python::address<QWidget>(window);
    if (!plotWindow)
      return;

    auto *plotWindowHandle = plotWindow->windowHandle();
    auto *parentWindowHandle = parent->windowHandle();
    if (!plotWindowHandle || !parentWindowHandle)
      return;

    plotWindowHandle->setTransientParent(parentWindowHandle);
    plotWindow->raise();
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

MantidQt::Widgets::Common::Python::Object createSubplots(size_t const plotCount, bool vertical = false) {
  using namespace MantidQt::Widgets::Common;

  auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantid.plots.plotfunctions")));
  auto const result =
      plotFunctions.attr("create_subplots")(static_cast<int>(plotCount), Python::Object(), "tight", vertical);
  return Python::Object(result[0]);
}

std::optional<MantidQt::Widgets::Common::Python::Object> currentFigureOrNone() {
  using namespace MantidQt::Widgets::Common;

  auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions")));
  auto const figure = Python::Object(plotFunctions.attr("current_figure_or_none")());
  if (figure.ptr() == Py_None)
    return std::nullopt;
  return figure;
}

MantidQt::Widgets::Common::Python::Object addTiledAxesToFigure(MantidQt::Widgets::Common::Python::Object const &figure,
                                                               size_t const plotCount, bool vertical) {
  using namespace MantidQt::Widgets::Common;

  auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantid.plots.plotfunctions")));
  return Python::Object(plotFunctions.attr("add_tiled_axes")(figure, static_cast<int>(plotCount), "tight", vertical));
}

MantidQt::Widgets::Common::Python::Object plotColorfill(std::vector<std::string> const &workspaces, bool vertical) {
  using namespace Mantid::PythonInterface;
  using namespace MantidQt::Widgets::Common;

  auto const plottingFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions")));
  auto const workspaceList = Converters::ToPyList<std::string>()(workspaces);
  Python::Dict kwargs;
  kwargs["vertical"] = vertical;
  auto const args = Python::Object(Python::NewRef(Py_BuildValue("(O)", workspaceList.ptr())));
  return Python::Object(plottingFunctions.attr("pcolormesh")(*args, **kwargs));
}

void plotOnAxis(std::vector<std::string> const &workspaces, MantidQt::Widgets::Common::Python::Object const &,
                MantidQt::Widgets::Common::Python::Object const &axis, PlotOptions const &options) {
  using namespace Mantid::PythonInterface;
  using namespace MantidQt::Widgets::Common;

  auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantid.plots.plotfunctions")));
  auto const workspaceList = Converters::ToPyList<std::string>()(workspaces);
  Python::Dict kwargs;
  kwargs["wksp_indices"] = Converters::ToPyList<int>()({0});
  kwargs["errors"] = options.showErrors;
  kwargs["ax_properties"] = Python::qHashToDict(axisScaleProperties(options));
  if (!options.windowTitle.empty())
    kwargs["window_title"] = options.windowTitle;
  auto const args = Python::Object(Python::NewRef(Py_BuildValue("(OO)", workspaceList.ptr(), axis.ptr())));
  plotFunctions.attr("plot_on_axis")(*args, **kwargs);
}

MantidQt::Widgets::Common::Python::Object plotGroupedTiled(std::vector<std::string> const &workspaces,
                                                           PlotOptions const &options, bool vertical = false) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const groupedWorkspaces = groupedWorkspaceNames(workspaces);
    auto const figure = createSubplots(groupedWorkspaces.size(), vertical);
    auto const axes = figure.attr("axes");
    auto const axesCount = Python::Len(axes);
    for (auto index = 0; index < axesCount; ++index) {
      auto const axis = Python::Object(axes[index]);
      if (index < static_cast<int>(groupedWorkspaces.size())) {
        plotOnAxis(groupedWorkspaces[index], figure, axis, options);
      } else {
        axis.attr("axis")("off");
      }
    }
    return figure;
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

struct ExistingTiledPlot {
  MantidQt::Widgets::Common::Python::Object figure;
  MantidQt::Widgets::Common::Python::Object axes;
};

std::optional<ExistingTiledPlot> plotTiledOnExistingFigure(std::vector<std::string> const &workspaces,
                                                           PlotOptions const &options, bool vertical) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto figure = currentFigureOrNone();
    if (!figure)
      return std::nullopt;

    auto const groupedWorkspaces = groupedWorkspaceNames(workspaces);
    auto const axes = addTiledAxesToFigure(*figure, groupedWorkspaces.size(), vertical);
    auto const axesCount = Python::Len(axes);
    for (auto index = 0; index < axesCount && index < static_cast<int>(groupedWorkspaces.size()); ++index) {
      auto const axis = Python::Object(axes[index]);
      plotOnAxis(groupedWorkspaces[index], *figure, axis, options);
    }

    return ExistingTiledPlot{*figure, axes};
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}
} // namespace

bool Plotter::canOverplotActiveFigure() const {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions")));
    return boost::python::extract<bool>(plotFunctions.attr("can_overplot")())();
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

bool Plotter::hasActiveFigure() const {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    return currentFigureOrNone().has_value();
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

void Plotter::plot(PlotRequest const &request) const {
  Mantid::PythonInterface::GlobalInterpreterLock lock;

  if (request.workspaces.empty())
    return;

  // plot(workspaces, spectrum_nums, wksp_indices, fig, plot_kwargs,
  // ax_properties, windows_title, errors, overplot)
  auto const actualWorkspaces = expandWorkspaceGroups(request.workspaces);
  auto const &options = request.options;
  auto const tiled = options.layout == PlotLayout::Tiled;

  if (options.plotStyle == PlotStyle::Colorfill) {
    auto const figure = plotColorfill(actualWorkspaces, tiled && request.tiledVertically);
    applyColorfillAxisLabels(figure, options);
    setPlotWindowParent(figure, request.parentWidget);
    return;
  }

  auto const axProperties = axisScaleProperties(options);
  auto const windowTitle = options.windowTitle.empty() ? actualWorkspaces.front() : options.windowTitle;
  // PlotLayout::Overplot means overlay this request in one fresh figure.
  // The MplCpp overplot flag instead targets an existing active figure.
  auto const reuseExistingFigure = request.addToExistingPlot && options.layout == PlotLayout::Overplot;
  const std::vector<int> workspaceIndices = {0};

  auto const existingTiledPlot = request.addToExistingPlot && tiled
                                     ? plotTiledOnExistingFigure(request.workspaces, options, request.tiledVertically)
                                     : std::nullopt;
  auto const figure =
      existingTiledPlot
          ? existingTiledPlot->figure
          : (tiled && (request.tiledVertically ||
                       std::any_of(request.workspaces.cbegin(), request.workspaces.cend(), isWorkspaceGroup))
                 ? plotGroupedTiled(request.workspaces, options, request.tiledVertically)
                 : MantidQt::Widgets::MplCpp::plot(actualWorkspaces, std::nullopt, workspaceIndices, std::nullopt,
                                                   std::nullopt, axProperties, windowTitle, options.showErrors,
                                                   reuseExistingFigure, tiled));
  if (existingTiledPlot) {
    applyAxisLabelsToAxes(existingTiledPlot->axes, options);
    if (options.horizontalMarker)
      addHorizontalMarkersToAxes(figure, existingTiledPlot->axes, *options.horizontalMarker);
  } else {
    applyAxisLabels(figure, options);
    if (options.horizontalMarker)
      addHorizontalMarkers(figure, *options.horizontalMarker);
  }
  setPlotWindowParent(figure, request.parentWidget);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
