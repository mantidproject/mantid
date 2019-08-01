// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotter.h"
#include "IndirectSettingsHelper.h"

#include "MantidAPI/AnalysisDataService.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidKernel/Strings.h"
#else
using namespace MantidQt::Widgets::MplCpp;
#endif

using namespace Mantid::API;

namespace {

template <typename BeginIter, typename Iterable>
void removeFromIterable(BeginIter const &beginIter, Iterable &iterable) {
  iterable.erase(beginIter, iterable.end());
}

std::vector<std::string> splitStringBy(std::string const &str,
                                       std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  removeFromIterable(std::remove_if(subStrings.begin(), subStrings.end(),
                                    [](std::string const &subString) {
                                      return subString.empty();
                                    }),
                     subStrings);
  return subStrings;
}

template <typename T> T convertToT(std::string const &num) {
  if (std::is_same<T, std::size_t>::value)
    return static_cast<std::size_t>(std::stoi(num));
  else if (std::is_same<T, int>::value)
    return std::stoi(num);
  std::runtime_error(
      "Could not convert std::string to std::size_t or int type.");
}

template <typename T>
void addToIndicesVector(std::vector<T> &indicesVec, T const &startIndex,
                        T const &endIndex) {
  for (auto index = startIndex; index <= endIndex; ++index)
    indicesVec.emplace_back(index);
}

template <typename T>
void addToIndicesVector(std::vector<T> &indicesVec,
                        std::string const &indicesString) {
  auto const range = splitStringBy(indicesString, "-");
  if (range.size() > 1)
    addToIndicesVector<T>(indicesVec, convertToT<T>(range[0]),
                          convertToT<T>(range[1]));
  else
    indicesVec.emplace_back(convertToT<T>(range[0]));
}

template <typename T>
std::vector<T> createIndicesVector(std::string const &indices) {
  std::vector<T> indicesVec;
  for (auto subString : splitStringBy(indices, ","))
    addToIndicesVector<T>(indicesVec, subString);
  return indicesVec;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
std::string expandIndicesRange(std::size_t const &startIndex,
                               std::size_t const &endIndex,
                               std::string const &separator) {
  std::string expandedRange;
  for (auto index = startIndex; index <= endIndex; ++index) {
    expandedRange += std::to_string(index);
    expandedRange += index != endIndex ? separator : "";
  }
  return expandedRange;
}

std::string expandIndicesRange(std::string const &startIndex,
                               std::string const &endIndex,
                               std::string const &separator) {
  return expandIndicesRange(std::stoul(startIndex), std::stoul(endIndex),
                            separator);
}

void addToIndicesList(std::string &indicesList,
                      std::string const &indicesString) {
  auto const range = splitStringBy(indicesString, "-");
  auto const expandedIndices =
      range.size() > 1 ? expandIndicesRange(range[0], range[1], ",") : range[0];
  indicesList += expandedIndices;
}

std::string createIndicesList(std::string const &indices) {
  std::string indicesList;
  auto const subStrings = splitStringBy(indices, ",");
  for (auto iter = subStrings.begin(); iter < subStrings.end(); ++iter) {
    addToIndicesList(indicesList, *iter);
    indicesList += iter < subStrings.end() - 1 ? "," : "";
  }
  return "[" + indicesList + "]";
}

std::string createPlotSpectraString(std::string const &workspaceName,
                                    std::string const &spectra,
                                    bool errorbars) {
  auto const errors = errorbars ? "True" : "False";
  std::string plotString = "from mantidplot import plotSpectrum\n";
  return plotString + "plotSpectrum(['" + workspaceName + "'], " + spectra +
         ", error_bars=" + errors + ")\n";
}

std::string createPlotBinsString(std::string const &workspaceName,
                                 std::string const &bins, bool errorbars) {
  auto const errors = errorbars ? "True" : "False";
  std::string plotString = "from mantidplot import plotTimeBin\n";
  return plotString + "plotTimeBin(['" + workspaceName + "'], " + bins +
         ", error_bars=" + errors + ")\n";
}

std::string createPlotContourString(std::string const &workspaceName) {
  std::string plotString = "from mantidplot import plot2D\n";
  return plotString + "plot2D('" + workspaceName + "')\n";
}

std::string createPlotTiledString(std::string const &workspaceName,
                                  std::vector<std::size_t> const &spectra) {
  std::string plotString = "from mantidplot import newTiledWindow\n";
  plotString += "newTiledWindow(sources=[";
  for (auto spectrum : spectra) {
    plotString +=
        "(['" + workspaceName + "'], " + std::to_string(spectrum) + ")";
    plotString += spectrum < spectra.back() ? "," : "";
  }
  plotString += "])\n";
  return plotString;
}

//#else
// QHash<QString, QVariant> constructKwargs(
//    bool errorBars,
//    QHash<QString, QVariant> otherKwargs = QHash<QString, QVariant>()) {
//  if (errorBars)
//    otherKwargs["capsize"] = 3;
//  return otherKwargs;
//}
#endif

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
IndirectPlotter::IndirectPlotter(IPyRunner *pythonRunner)
    : QObject(nullptr), m_pyRunner(pythonRunner) {}

#else
IndirectPlotter::IndirectPlotter(IPyRunner *pythonRunner) : QObject(nullptr) {
  UNUSED_ARG(pythonRunner);
}
#endif

IndirectPlotter::~IndirectPlotter() {}

/**
 * Produces an external plot of workspace spectra
 *
 * @param workspaceName The name of the workspace to plot
 * @param workspaceIndices The indices within the workspace to plot (e.g.
 * '0-2,5,7-10')
 */
void IndirectPlotter::plotSpectra(std::string const &workspaceName,
                                  std::string const &workspaceIndices) {
  if (validate(workspaceName, workspaceIndices, MantidAxis::Spectrum)) {
    auto const errorBars = IndirectSettingsHelper::externalPlotErrorBars();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    runPythonCode(createPlotSpectraString(
        workspaceName, createIndicesList(workspaceIndices), errorBars));
#else
    workbenchPlot(QStringList(QString::fromStdString(workspaceName)),
                  createIndicesVector<int>(workspaceIndices), errorBars,
                  boost::none);
    // plot(QStringList(QString::fromStdString(workspaceName)), boost::none,
    //     createIndicesVector<int>(workspaceIndices), boost::none,
    //     constructKwargs(errorBars), boost::none, boost::none, errorBars);
#endif
  }
}

/**
 * Plots different spectra for multiple workspaces on the same plot.
 * The size of workspaceNames and workspaceIndices must be equal.
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param workspaceIndices List of indices to plot
 */
void IndirectPlotter::plotCorrespondingSpectra(
    std::vector<std::string> const &workspaceNames,
    std::vector<int> const &workspaceIndices) {
  if (workspaceNames.size() != workspaceIndices.size())
    return;
  auto const errorBars = IndirectSettingsHelper::externalPlotErrorBars();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  std::string pyInput = "from mantidplot import plotSpectrum\n";
  pyInput += "current_window = plotSpectrum('";
  pyInput += workspaceNames[0];
  pyInput += "', ";
  pyInput += std::to_string(workspaceIndices[0]);
  pyInput += ")\n";

  for (auto i = 1u; i < workspaceNames.size(); ++i) {
    pyInput += "plotSpectrum('";
    pyInput += workspaceNames[i];
    pyInput += "', ";
    pyInput += std::to_string(workspaceIndices[i]);
    pyInput += ", window=current_window)\n";
  }
  runPythonCode(pyInput);
#else
  auto figure =
      workbenchPlot(QStringList(QString::fromStdString(workspaceNames[0])),
                    {workspaceIndices[0]}, errorBars);
  for (auto i = 1; i < workspaceNames.size(); ++i) {
    figure =
        workbenchPlot(QStringList(QString::fromStdString(workspaceNames[i])),
                      {workspaceIndices[i]}, errorBars, boost::none, figure);
  }
  // workbenchPlot(workspaceNames, workspaceIndices, m_plotErrorBars,
  // boost::none);
#endif
}

/**
 * Produces an external plot of workspace bins
 *
 * @param workspaceName The name of the workspace to plot
 * @param binIndices The indices within the workspace to plot (e.g.
 * '0-2,5,7-10')
 */
void IndirectPlotter::plotBins(std::string const &workspaceName,
                               std::string const &binIndices) {
  if (validate(workspaceName, binIndices, MantidAxis::Bin)) {
    auto const errorBars = IndirectSettingsHelper::externalPlotErrorBars();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    runPythonCode(createPlotBinsString(
        workspaceName, createIndicesList(binIndices), errorBars));
#else
    QHash<QString, QVariant> plotKwargs;
    plotKwargs["axis"] = static_cast<int>(MantidAxType::Bin);
    // plot(QStringList(QString::fromStdString(workspaceName)), boost::none,
    //     createIndicesVector<int>(binIndices), boost::none,
    //     constructKwargs(errorBars, plotKwargs), boost::none, boost::none,
    //     errorBars);
    workbenchPlot(QStringList(QString::fromStdString(workspaceName)),
                  createIndicesVector<int>(binIndices), errorBars, plotKwargs);
#endif
  }
}

/**
 * Produces an external contour plot of a workspace
 *
 * @param workspaceName The name of the workspace to plot
 */
void IndirectPlotter::plotContour(std::string const &workspaceName) {
  if (validate(workspaceName)) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    runPythonCode(createPlotContourString(workspaceName));
#else
    pcolormesh(QStringList(QString::fromStdString(workspaceName)));
#endif
  }
}

/**
 * Produces an external tiled plot of spectra within a workspace
 *
 * @param workspaceName The name of the workspace to plot
 * @param workspaceIndices The indices within the workspace to tile plot (e.g.
 * '0-2,5,7-10')
 */
void IndirectPlotter::plotTiled(std::string const &workspaceName,
                                std::string const &workspaceIndices) {
  if (validate(workspaceName, workspaceIndices, MantidAxis::Spectrum)) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    runPythonCode(createPlotTiledString(
        workspaceName, createIndicesVector<std::size_t>(workspaceIndices)));
#else
    UNUSED_ARG(workspaceName);
    UNUSED_ARG(workspaceIndices);
    std::runtime_error(
        "Tiled plotting for the Workbench has not been implemented.");
#endif
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
bool IndirectPlotter::validate(
    std::string const &workspaceName,
    boost::optional<std::string> const &workspaceIndices,
    boost::optional<MantidAxis> const &axisType) const {
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
bool IndirectPlotter::validate(
    MatrixWorkspace_const_sptr workspace,
    boost::optional<std::string> const &workspaceIndices,
    boost::optional<MantidAxis> const &axisType) const {
  if (workspaceIndices && axisType && axisType.get() == MantidAxis::Spectrum)
    return validateSpectra(workspace, workspaceIndices.get());
  else if (workspaceIndices && axisType && axisType.get() == MantidAxis::Bin)
    return validateBins(workspace, workspaceIndices.get());
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
bool IndirectPlotter::validateSpectra(
    MatrixWorkspace_const_sptr workspace,
    std::string const &workspaceIndices) const {
  auto const numberOfHistograms = workspace->getNumberHistograms();
  auto const lastIndex =
      std::stoul(splitStringBy(workspaceIndices, ",-").back());
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
bool IndirectPlotter::validateBins(MatrixWorkspace_const_sptr workspace,
                                   std::string const &binIndices) const {
  auto const numberOfBins = workspace->y(0).size();
  auto const lastIndex = std::stoul(splitStringBy(binIndices, ",-").back());
  return lastIndex < numberOfBins;
}

/**
 * Runs python code (mantidplot only)
 *
 * @param pythonCode The python code to run
 */
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
void IndirectPlotter::runPythonCode(std::string const &pythonCode) {
  m_pyRunner->runPythonCode(pythonCode);
}
#else

/**
 * Used for plotting spectra or bins on the workbench
 *
 * @param workspaceNames List of names of workspaces to plot
 * @param indices The workspace indices to plot
 * @param errorBars True if error bars are enabled
 * @param kwargs Other arguments for plotting
 * @param figure The figure to plot on top of
 */
using namespace MantidQt::Widgets::Common;

Python::Object
IndirectPlotter::workbenchPlot(QStringList const &workspaceNames,
                               std::vector<int> const &indices, bool errorBars,
                               boost::optional<QHash<QString, QVariant>> kwargs,
                               boost::optional<Python::Object> figure) {
  QHash<QString, QVariant> plotKwargs;
  if (kwargs)
    plotKwargs = kwargs.get();
  if (errorBars)
    plotKwargs["capsize"] = 3;

  using MantidQt::Widgets::MplCpp::plot;
  return plot(workspaceNames, boost::none, indices, figure, plotKwargs,
              boost::none, boost::none, errorBars);
}
#endif

} // namespace CustomInterfaces
} // namespace MantidQt
