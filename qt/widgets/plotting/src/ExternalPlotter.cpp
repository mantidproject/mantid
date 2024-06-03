// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/MplCpp/Plot.h"
#include "MantidQtWidgets/Plotting/ExternalPlotter.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::Widgets::MplCpp;

namespace {
Mantid::Kernel::Logger g_log("ExternalPlotter");

auto constexpr ERROR_CAPSIZE = 3;

template <typename BeginIter, typename Iterable>
void removeFromIterable(BeginIter const &beginIter, Iterable &iterable) {
  iterable.erase(beginIter, iterable.end());
}

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  removeFromIterable(std::remove_if(subStrings.begin(), subStrings.end(),
                                    [](std::string const &subString) { return subString.empty(); }),
                     subStrings);
  return subStrings;
}

template <typename T> T convertToT(std::string const &num) {
  if (std::is_same<T, std::size_t>::value) {
    return static_cast<std::size_t>(std::stoi(num));
  } else if (std::is_same<T, int>::value) {
    return std::stoi(num);
  } else {
    throw std::runtime_error("Could not convert std::string to std::size_t or int type.");
  }
}

template <typename T> void addToIndicesVector(std::vector<T> &indicesVec, T const &startIndex, T const &endIndex) {
  for (auto index = startIndex; index <= endIndex; ++index)
    indicesVec.emplace_back(index);
}

template <typename T> void addToIndicesVector(std::vector<T> &indicesVec, std::string const &indicesString) {
  auto const range = splitStringBy(indicesString, "-");
  if (range.size() > 1)
    addToIndicesVector<T>(indicesVec, convertToT<T>(range[0]), convertToT<T>(range[1]));
  else
    indicesVec.emplace_back(convertToT<T>(range[0]));
}

template <typename T> std::vector<T> createIndicesVector(std::string const &indices) {
  std::vector<T> indicesVec;
  for (auto subString : splitStringBy(indices, ","))
    addToIndicesVector<T>(indicesVec, subString);
  return indicesVec;
}

/**
 * Used for plotting spectra or bins on the workbench
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param indices The workspace indices to plot
 * @param errorBars True if error bars are enabled
 * @param kwargs Other arguments for plotting
 * @param figure The figure to plot on top of
 */
using namespace Mantid::PythonInterface;
using namespace MantidQt::Widgets::Common;

std::optional<Python::Object> workbenchPlot(QStringList const &workspaceNames, std::vector<int> const &indices,
                                            bool errorBars, bool overplot = false,
                                            std::optional<QHash<QString, QVariant>> kwargs = std::nullopt,
                                            std::optional<Python::Object> figure = std::nullopt) {
  QHash<QString, QVariant> plotKwargs;
  if (kwargs)
    plotKwargs = *kwargs;
  if (errorBars)
    plotKwargs["capsize"] = ERROR_CAPSIZE;

  using MantidQt::Widgets::MplCpp::plot;
  try {
    return plot(workspaceNames, std::nullopt, indices, std::move(figure), plotKwargs, std::nullopt, std::nullopt,
                errorBars, overplot);
  } catch (PythonException const &ex) {
    g_log.error() << ex.what();
    return std::nullopt;
  }
}

} // namespace

namespace MantidQt::Widgets::MplCpp {

/**
 * Calls plotSpectra with no kwargs
 */
void ExternalPlotter::plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices,
                                  bool errorBars) {
  return plotSpectra(workspaceName, workspaceIndices, errorBars, std::nullopt);
}

/**
 * Produces an external plot of workspace spectra
 *
 * @param workspaceName The name of the workspace to plot
 * @param workspaceIndices The indices within the workspace to plot (e.g.
 * '0-2,5,7-10')
 * @param errorBars Boolean value to add/remove erorr bars
 * @param kwargs The kwargs to be used when plotting the workspace
 */
void ExternalPlotter::plotSpectra(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars,
                                  std::optional<QHash<QString, QVariant>> const &kwargs) {
  if (validate(workspaceName, workspaceIndices, MantidAxis::Spectrum)) {
    workbenchPlot(QStringList(QString::fromStdString(workspaceName)), createIndicesVector<int>(workspaceIndices),
                  errorBars, false, kwargs);
  }
}

/**
 * Calls plotCorrespondingSpectra with no kwargs
 */
void ExternalPlotter::plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                               std::vector<int> const &workspaceIndices,
                                               std::vector<bool> const &errorBars) {
  return plotCorrespondingSpectra(
      workspaceNames, workspaceIndices, errorBars,
      std::vector<std::optional<QHash<QString, QVariant>>>(workspaceNames.size(), std::nullopt));
}

/**
 * Plots different spectra for multiple workspaces on the same plot.
 * The size of workspaceNames and workspaceIndices must be equal.
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param workspaceIndices List of indices to plot
 * @param errorBars List of booleans to add/remove error bars to each workspace individually
 * @param kwargs The kwargs to be used when plotting each workspace
 */
void ExternalPlotter::plotCorrespondingSpectra(std::vector<std::string> const &workspaceNames,
                                               std::vector<int> const &workspaceIndices,
                                               std::vector<bool> const &errorBars,
                                               std::vector<std::optional<QHash<QString, QVariant>>> const &kwargs) {
  if (workspaceNames.empty() || workspaceIndices.empty() || errorBars.empty() || kwargs.empty())
    return;
  auto const numberOfPlots = workspaceNames.size();
  if (numberOfPlots > 1 &&
      (workspaceIndices.size() != numberOfPlots || errorBars.size() != numberOfPlots || kwargs.size() != numberOfPlots))
    return;
  auto figure = workbenchPlot(QStringList(QString::fromStdString(workspaceNames[0])), {workspaceIndices[0]},
                              errorBars[0], false, kwargs[0]);
  for (auto i = 1u; i < workspaceNames.size(); ++i) {
    if (figure)
      figure = workbenchPlot(QStringList(QString::fromStdString(workspaceNames[i])), {workspaceIndices[i]},
                             errorBars[i], true, kwargs[i], *figure);
  }
}

/**
 * Produces an external plot of workspace bins
 *
 * @param workspaceName The name of the workspace to plot
 * @param binIndices The indices within the workspace to plot (e.g.
 * '0-2,5,7-10')
 */
void ExternalPlotter::plotBins(std::string const &workspaceName, std::string const &binIndices, bool errorBars) {
  if (validate(workspaceName, binIndices, MantidAxis::Bin)) {
    QHash<QString, QVariant> plotKwargs;
    plotKwargs["axis"] = static_cast<int>(MantidAxType::Bin);
    workbenchPlot(QStringList(QString::fromStdString(workspaceName)), createIndicesVector<int>(binIndices), errorBars,
                  false, plotKwargs);
  }
}

/**
 * Produces an external contour plot of a workspace
 *
 * @param workspaceName The name of the workspace to plot
 */
void ExternalPlotter::plotContour(std::string const &workspaceName) {
  if (validate(workspaceName))
    pcolormesh(QStringList(QString::fromStdString(workspaceName)));
}

/**
 * Produces an external call to 3D Surface Plot on the target workspace
 *
 * @param workspaceName The name of the workspace to use in slice viewer
 */
void ExternalPlotter::plot3DSurface(std::string const &workspaceName) {
  if (validate(workspaceName)) {
    surface(QStringList(QString::fromStdString(workspaceName)));
  }
}

/**
 * Produces an external call to slice viewer on the target workspace
 *
 * @param workspaceName The name of the workspace to use in slice viewer
 */
void ExternalPlotter::showSliceViewer(std::string const &workspaceName) {
  if (validate(workspaceName)) {
    auto workspace = AnalysisDataService::Instance().retrieveWS<Workspace>(workspaceName);
    sliceviewer(workspace);
  }
}

/**
 * Produces an external tiled plot of spectra within a workspace
 *
 * @param workspaceName The name of the workspace to plot
 * @param workspaceIndices The indices within the workspace to tile plot (e.g.
 * '0-2,5,7-10')
 */
void ExternalPlotter::plotTiled(std::string const &workspaceName, std::string const &workspaceIndices, bool errorBars) {
  if (validate(workspaceName, workspaceIndices, MantidAxis::Spectrum)) {
    QHash<QString, QVariant> plotKwargs;
    if (errorBars)
      plotKwargs["capsize"] = ERROR_CAPSIZE;
    plot(QStringList(QString::fromStdString(workspaceName)), std::nullopt, createIndicesVector<int>(workspaceIndices),
         std::nullopt, plotKwargs, std::nullopt, "Tiled Plot: " + workspaceName, errorBars, false, true);
  }
}

/**
 * Validates that the workspace exists as a matrix workspace, and that the
 * indices specified exist in the workspace
 *
 * @param workspaceName The name of the workspace to plot
 * @param workspaceIndices The indices within the workspace to plot (e.g.
 * '0-2,5,7-10')
 * @param axisType The axis to validate (i.e. Spectrum or Bin)
 * @return True if the data is valid
 */
bool ExternalPlotter::validate(std::string const &workspaceName, std::optional<std::string> const &workspaceIndices,
                               std::optional<MantidAxis> const &axisType) const {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    if (auto const workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName))
      return validate(workspace, workspaceIndices, axisType);
  return false;
}

/**
 * Validates that the indices specified exist in the workspace
 *
 * @param workspace The matrix workspace
 * @param workspaceIndices The indices within the workspace to plot (e.g.
 * '0-2,5,7-10')
 * @param axisType The axis to validate (i.e. Spectrum or Bin)
 * @return True if the data is valid
 */
bool ExternalPlotter::validate(const MatrixWorkspace_const_sptr &workspace,
                               std::optional<std::string> const &workspaceIndices,
                               std::optional<MantidAxis> const &axisType) const {
  if (workspaceIndices && axisType && *axisType == MantidAxis::Spectrum)
    return validateSpectra(workspace, *workspaceIndices);
  else if (workspaceIndices && axisType && *axisType == MantidAxis::Bin)
    return validateBins(workspace, *workspaceIndices);
  return true;
}

/**
 * Validates that the workspace indices specified exist in the workspace
 *
 * @param workspace The matrix workspace
 * @param workspaceIndices The indices within the workspace to check (e.g.
 * '0-2,5,7-10')
 * @return True if the indices exist
 */
bool ExternalPlotter::validateSpectra(const MatrixWorkspace_const_sptr &workspace,
                                      std::string const &workspaceIndices) const {
  auto const numberOfHistograms = workspace->getNumberHistograms();
  auto const lastIndex = std::stoul(splitStringBy(workspaceIndices, ",-").back());
  return lastIndex < numberOfHistograms;
}

/**
 * Validates that the bin indices specified exist in the workspace
 *
 * @param workspace The matrix workspace
 * @param binIndices The bin indices within the workspace to check (e.g.
 * '0-2,5,7-10')
 * @return True if the bin indices exist
 */
bool ExternalPlotter::validateBins(const MatrixWorkspace_const_sptr &workspace, std::string const &binIndices) const {
  auto const numberOfBins = workspace->y(0).size();
  auto const lastIndex = std::stoul(splitStringBy(binIndices, ",-").back());
  return lastIndex < numberOfBins;
}
} // namespace MantidQt::Widgets::MplCpp
