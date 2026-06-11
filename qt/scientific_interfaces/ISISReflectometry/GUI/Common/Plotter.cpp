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
#include <stdexcept>
#include <vector>
using namespace MantidQt::Widgets::MplCpp;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
QString scaleName(AxisScale scale) { return scale == AxisScale::Log ? QString("log") : QString("linear"); }

/// Combine an axis label and unit into the matplotlib label text.
std::string axisLabel(PlotAxis const &axis) {
  if (axis.unit.empty())
    return axis.label;
  return axis.label + " (" + axis.unit + ")";
}

/// Return x/y scale properties in the format expected by Mantid plotting functions.
QHash<QString, QVariant> axisScaleProperties(PlotOptions const &options) {
  QHash<QString, QVariant> properties;
  properties[QString("xscale")] = QVariant(scaleName(options.xAxis.scale));
  properties[QString("yscale")] = QVariant(scaleName(options.yAxis.scale));
  return properties;
}

/// Retrieve a workspace from the ADS by name.
Mantid::API::Workspace_sptr retrieveWorkspace(std::string const &workspace) {
  return Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>(workspace);
}

/// Return true if the named ADS workspace is a workspace group.
bool isWorkspaceGroup(std::string const &workspace) {
  return std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(retrieveWorkspace(workspace)) != nullptr;
}

/// Replace workspace-group names with their member workspace names.
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

/// Group each requested workspace name with its expanded workspace-group members.
std::vector<std::vector<std::string>> groupedWorkspaceNames(std::vector<std::string> const &workspaces) {
  auto groupedWorkspaces = std::vector<std::vector<std::string>>{};
  groupedWorkspaces.reserve(workspaces.size());
  for (auto const &workspace : workspaces) {
    groupedWorkspaces.emplace_back(expandWorkspaceGroups({workspace}));
  }
  return groupedWorkspaces;
}

/// Apply a callback to every axis in a matplotlib figure.
void forEachAxis(MantidQt::Widgets::Common::Python::Object const &figure,
                 std::function<void(MantidQt::Widgets::Common::Python::Object const &)> const &callback) {
  using namespace MantidQt::Widgets::Common;

  auto const axes = figure.attr("axes");
  auto const axesCount = Python::Len(axes);
  for (auto index = 0; index < axesCount; ++index)
    callback(Python::Object(axes[index]));
}

/// Apply a callback to every axis in a Python axes list.
void forEachAxisInList(MantidQt::Widgets::Common::Python::Object const &axes,
                       std::function<void(MantidQt::Widgets::Common::Python::Object const &)> const &callback) {
  using namespace MantidQt::Widgets::Common;

  auto const axesCount = Python::Len(axes);
  for (auto index = 0; index < axesCount; ++index)
    callback(Python::Object(axes[index]));
}

/// Apply x/y labels to a supplied list of matplotlib axes.
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
  } catch (std::runtime_error const &) {
    return;
  }
}

/// Apply x/y labels to every axis in a matplotlib figure.
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

/// Apply x/y labels to plot axes and z label to colorbar axes.
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

/// Add a horizontal marker to supplied axes when figure interaction supports it.
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

/// Add a horizontal marker to every axis in a figure when figure interaction supports it.
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

/// Make the plot window transient for the reflectometry window when possible.
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
    auto *plotWindow = Python::extract<QWidget>(window);
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

/// Create a new matplotlib figure with enough tiled axes for the requested plots.
MantidQt::Widgets::Common::Python::Object createSubplots(size_t const plotCount, bool vertical = false) {
  using namespace MantidQt::Widgets::Common;

  auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantid.plots.plotfunctions")));
  auto const result =
      plotFunctions.attr("create_subplots")(static_cast<int>(plotCount), Python::Object(), "tight", vertical);
  return Python::Object(result[0]);
}

/// Return the active matplotlib figure, if one exists.
std::optional<MantidQt::Widgets::Common::Python::Object> currentFigureOrNone() {
  using namespace MantidQt::Widgets::Common;

  auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantidqt.plotting.functions")));
  auto const figure = Python::Object(plotFunctions.attr("current_figure_or_none")());
  if (figure.ptr() == Py_None)
    return std::nullopt;
  return figure;
}

/// Add tiled axes to an existing figure and return the created axes.
MantidQt::Widgets::Common::Python::Object addTiledAxesToFigure(MantidQt::Widgets::Common::Python::Object const &figure,
                                                               size_t const plotCount, bool vertical) {
  using namespace MantidQt::Widgets::Common;

  auto const plotFunctions = Python::Object(Python::NewRef(PyImport_ImportModule("mantid.plots.plotfunctions")));
  return Python::Object(plotFunctions.attr("add_tiled_axes")(figure, static_cast<int>(plotCount), "tight", vertical));
}

/// Plot detector-map/colorfill workspaces through Mantid's Python plotting functions.
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

/// Plot one or more line workspaces on a supplied matplotlib axis.
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

/// Plot line workspaces using Mantid's tiled axes helper instead of the standard plot function.
MantidQt::Widgets::Common::Python::Object plotCustomTiled(std::vector<std::string> const &workspaces,
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

/// Existing figure plus the axes added for a tiled add-to-existing request.
struct ExistingTiledPlot {
  MantidQt::Widgets::Common::Python::Object figure;
  MantidQt::Widgets::Common::Python::Object axes;
};

/// Plot implementation path selected after evaluating a request.
enum class PlotRoute { Colorfill, ExistingTiled, CustomTiled, Standard };

/// Axes that should receive post-plot labels and markers.
enum class PostPlotTarget { AllAxes, ColorfillAxes, NewAxes };

/// Plot request after workspace expansion and route selection.
struct EvaluatedPlotRequest {
  std::vector<std::string> actualWorkspaces;
  QHash<QString, QVariant> axProperties;
  std::string windowTitle;
  MantidQt::Widgets::Common::Python::Object existingFigure;
  PlotRoute route;
  bool tiled;
  bool reuseExistingFigure;
};

/// Figure and target axes returned by a plotting route.
struct PlottedFigure {
  MantidQt::Widgets::Common::Python::Object figure;
  MantidQt::Widgets::Common::Python::Object newAxes;
  PostPlotTarget postPlotTarget;
};

/// Plot a tiled line output by adding axes to an existing figure.
ExistingTiledPlot plotTiledOnExistingFigure(MantidQt::Widgets::Common::Python::Object const &figure,
                                            std::vector<std::string> const &workspaces, PlotOptions const &options,
                                            bool vertical) {
  using namespace MantidQt::Widgets::Common;

  Mantid::PythonInterface::GlobalInterpreterLock lock;
  try {
    auto const groupedWorkspaces = groupedWorkspaceNames(workspaces);
    auto const axes = addTiledAxesToFigure(figure, groupedWorkspaces.size(), vertical);
    auto const axesCount = Python::Len(axes);
    for (auto index = 0; index < axesCount && index < static_cast<int>(groupedWorkspaces.size()); ++index) {
      auto const axis = Python::Object(axes[index]);
      plotOnAxis(groupedWorkspaces[index], figure, axis, options);
    }

    return ExistingTiledPlot{figure, axes};
  } catch (Python::ErrorAlreadySet &) {
    throw Mantid::PythonInterface::PythonException();
  }
}

/// Select the plotting route needed for the request shape and active-figure state.
PlotRoute plotRouteFor(PlotRequest const &request, bool tiled, bool hasExistingFigure) {
  if (request.options.plotStyle == PlotStyle::Colorfill)
    return PlotRoute::Colorfill;
  if (request.addToExistingPlot && tiled && hasExistingFigure)
    return PlotRoute::ExistingTiled;
  if (tiled && (request.tiledVertically ||
                std::any_of(request.workspaces.cbegin(), request.workspaces.cend(), isWorkspaceGroup)))
    return PlotRoute::CustomTiled;
  return PlotRoute::Standard;
}

/// Expand workspaces, derive axis properties and choose a plotting route.
EvaluatedPlotRequest evaluatePlotRequest(PlotRequest const &request) {
  auto const actualWorkspaces = expandWorkspaceGroups(request.workspaces);
  auto const &options = request.options;
  auto const tiled = options.layout == PlotLayout::Tiled;
  auto const reuseExistingFigure = request.addToExistingPlot && options.layout == PlotLayout::Overplot;
  auto const windowTitle = options.windowTitle.empty() ? actualWorkspaces.front() : options.windowTitle;
  auto const existingFigure = request.addToExistingPlot && tiled ? currentFigureOrNone() : std::nullopt;
  return {actualWorkspaces,
          axisScaleProperties(options),
          windowTitle,
          existingFigure.value_or(MantidQt::Widgets::Common::Python::Object()),
          plotRouteFor(request, tiled, existingFigure.has_value()),
          tiled,
          reuseExistingFigure};
}

/// Plot a detector-map/colorfill request.
PlottedFigure plotColorfillFigure(PlotRequest const &request, EvaluatedPlotRequest const &evaluated) {
  return {plotColorfill(evaluated.actualWorkspaces, evaluated.tiled && request.tiledVertically),
          MantidQt::Widgets::Common::Python::Object(), PostPlotTarget::ColorfillAxes};
}

/// Plot a tiled line request onto an existing figure.
PlottedFigure plotExistingTiledFigure(PlotRequest const &request, EvaluatedPlotRequest const &evaluated) {
  auto const existingTiledPlot =
      plotTiledOnExistingFigure(evaluated.existingFigure, request.workspaces, request.options, request.tiledVertically);
  return {existingTiledPlot.figure, existingTiledPlot.axes, PostPlotTarget::NewAxes};
}

/// Plot a tiled line request that requires grouped workspace handling or vertical tiling.
PlottedFigure plotCustomTiledFigure(PlotRequest const &request) {
  return {plotCustomTiled(request.workspaces, request.options, request.tiledVertically),
          MantidQt::Widgets::Common::Python::Object(), PostPlotTarget::AllAxes};
}

/// Plot a request through the standard Mantid MplCpp plot function.
PlottedFigure plotStandardFigure(PlotRequest const &request, EvaluatedPlotRequest const &evaluated) {
  auto const workspaceIndices = std::vector<int>{0};
  return {MantidQt::Widgets::MplCpp::plot(evaluated.actualWorkspaces, std::nullopt, workspaceIndices, std::nullopt,
                                          std::nullopt, evaluated.axProperties, evaluated.windowTitle,
                                          request.options.showErrors, evaluated.reuseExistingFigure, evaluated.tiled),
          MantidQt::Widgets::Common::Python::Object(), PostPlotTarget::AllAxes};
}

/// Dispatch to the evaluated plotting route.
PlottedFigure plotFigure(PlotRequest const &request, EvaluatedPlotRequest const &evaluated) {
  if (evaluated.route == PlotRoute::Colorfill)
    return plotColorfillFigure(request, evaluated);
  if (evaluated.route == PlotRoute::ExistingTiled)
    return plotExistingTiledFigure(request, evaluated);
  if (evaluated.route == PlotRoute::CustomTiled)
    return plotCustomTiledFigure(request);
  return plotStandardFigure(request, evaluated);
}

/// Apply axis labels, optional markers and window parenting after plotting.
void applyPostPlotting(PlotRequest const &request, PlottedFigure const &plottedFigure) {
  auto const &options = request.options;
  if (plottedFigure.postPlotTarget == PostPlotTarget::ColorfillAxes) {
    applyColorfillAxisLabels(plottedFigure.figure, options);
  } else if (plottedFigure.postPlotTarget == PostPlotTarget::NewAxes) {
    applyAxisLabelsToAxes(plottedFigure.newAxes, options);
    if (options.horizontalMarker)
      addHorizontalMarkersToAxes(plottedFigure.figure, plottedFigure.newAxes, *options.horizontalMarker);
  } else {
    applyAxisLabels(plottedFigure.figure, options);
    if (options.horizontalMarker)
      addHorizontalMarkers(plottedFigure.figure, *options.horizontalMarker);
  }
  setPlotWindowParent(plottedFigure.figure, request.parentWidget);
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

  auto const evaluated = evaluatePlotRequest(request);
  auto const plottedFigure = plotFigure(request, evaluated);
  applyPostPlotting(request, plottedFigure);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
