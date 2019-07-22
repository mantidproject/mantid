// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectPlotOptionsModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Strings.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/MplCpp/Plot.h"
#endif

using namespace Mantid::API;
using namespace Mantid::Kernel::Strings;

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

std::string getIndicesRange(std::string const &str) {
  auto const bounds = splitStringBy(str, "-");
  return std::stoull(bounds[0]) > std::stoull(bounds[1])
             ? bounds[1] + "-" + bounds[0]
             : str;
}

std::string rearrangeIndicesSubString(std::string const &str) {
  return str.find("-") != std::string::npos ? getIndicesRange(str) : str;
}

// Swaps the two numbers in a spectra range if they go from large to small
std::string rearrangeIndicesRangeStrings(std::string const &str) {
  std::string indicesString;
  auto subStrings = splitStringBy(str, ",");
  for (auto it = subStrings.begin(); it < subStrings.end(); ++it) {
    indicesString += rearrangeIndicesSubString(*it);
    indicesString += it != subStrings.end() ? "," : "";
  }
  return indicesString;
}

std::string formatIndicesString(std::string &str) {
  // Remove spaces
  removeFromIterable(std::remove_if(str.begin(), str.end(), isspace), str);
  // Rearrange range strings
  auto indices = parseRange(rearrangeIndicesRangeStrings(str));
  std::sort(indices.begin(), indices.end());
  // Remove duplicate entries
  removeFromIterable(std::unique(indices.begin(), indices.end()), indices);
  return joinCompress(indices.begin(), indices.end());
}

template <typename T = std::size_t>
void addToIndicesVector(std::vector<T> &indicesVec,
                        std::size_t const &startIndex,
                        std::size_t const &endIndex) {
  for (auto index = startIndex; index <= endIndex; ++index)
    indicesVec.emplace_back(index);
}

template <typename T = std::size_t>
void addToIndicesVector(std::vector<T> &indicesVec,
                        std::string const &indicesString) {
  auto const range = splitStringBy(indicesString, "-");
  if (range.size() > 1)
    addToIndicesVector<T>(indicesVec, std::stoul(range[0]),
                          std::stoul(range[1]));
  else
    indicesVec.emplace_back(std::stoul(range[0]));
}

template <typename T = std::size_t>
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
#endif

} // namespace

namespace MantidQt {
namespace CustomInterfaces {

IndirectPlotOptionsModel::IndirectPlotOptionsModel()
    : m_fixedIndices(false), m_workspaceIndices(boost::none),
      m_workspaceName(boost::none) {}

IndirectPlotOptionsModel::~IndirectPlotOptionsModel() {}

bool IndirectPlotOptionsModel::setWorkspace(std::string const &workspaceName) {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName) &&
      ads.retrieveWS<MatrixWorkspace>(workspaceName)) {
    m_workspaceName = workspaceName;
    return true;
  }
  return false;
}

boost::optional<std::string> IndirectPlotOptionsModel::workspace() const {
  return m_workspaceName;
}

void IndirectPlotOptionsModel::removeWorkspace() {
  m_workspaceName = boost::none;
}

std::string
IndirectPlotOptionsModel::formatIndices(std::string const &indices) const {
  auto indicesString = indices;
  return formatIndicesString(indicesString);
}

void IndirectPlotOptionsModel::setFixedIndices(std::string const &indices) {
  if (m_fixedIndices = !indices.empty())
    m_workspaceIndices = indices;
}

bool IndirectPlotOptionsModel::indicesFixed() const { return m_fixedIndices; }

bool IndirectPlotOptionsModel::setIndices(std::string const &indices) {
  bool valid = validateIndices(indices);
  if (valid)
    m_workspaceIndices = indices;
  else
    m_workspaceIndices = boost::none;
  return valid;
}

boost::optional<std::string> IndirectPlotOptionsModel::indices() const {
  return m_workspaceIndices;
}

bool IndirectPlotOptionsModel::validateIndices(
    std::string const &indices, MantidAxis const &axisType) const {
  auto &ads = AnalysisDataService::Instance();
  if (!indices.empty() && m_workspaceName &&
      ads.doesExist(m_workspaceName.get())) {
    if (auto const workspace =
            ads.retrieveWS<MatrixWorkspace>(m_workspaceName.get())) {
      if (axisType == MantidAxis::Spectrum)
        return validateSpectra(workspace, indices);
      return validateBins(workspace, indices);
    }
  }
  return false;
}

bool IndirectPlotOptionsModel::validateSpectra(
    MatrixWorkspace_sptr workspace, std::string const &spectra) const {
  auto const numberOfHistograms = workspace->getNumberHistograms();
  auto const lastIndex = std::stoul(splitStringBy(spectra, ",-").back());
  return lastIndex < numberOfHistograms;
}

bool IndirectPlotOptionsModel::validateBins(MatrixWorkspace_sptr workspace,
                                            std::string const &bins) const {
  auto const numberOfBins = workspace->y(0).size();
  auto const lastIndex = std::stoul(splitStringBy(bins, ",-").back());
  return lastIndex < numberOfBins;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
boost::optional<std::string>
IndirectPlotOptionsModel::getPlotSpectraString(bool errorBars) const {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString)
    return createPlotSpectraString(
        workspaceName.get(), createIndicesList(indicesString.get()), errorBars);
  return boost::none;
}

boost::optional<std::string>
IndirectPlotOptionsModel::getPlotBinsString(bool errorBars) const {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString)
    return createPlotBinsString(
        workspaceName.get(), createIndicesList(indicesString.get()), errorBars);
  return boost::none;
}

boost::optional<std::string>
IndirectPlotOptionsModel::getPlotContourString() const {
  auto const workspaceName = workspace();
  if (workspaceName)
    return createPlotContourString(workspaceName.get());
  return boost::none;
}

boost::optional<std::string>
IndirectPlotOptionsModel::getPlotTiledString() const {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString)
    return createPlotTiledString(workspaceName.get(),
                                 createIndicesVector(indicesString.get()));
  return boost::none;
}

#else
void IndirectPlotOptionsModel::plotSpectra(bool errorBars) {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString) {
    QHash<QString, QVariant> plotKwargs;
    if (errorBars)
      plotKwargs["capsize"] = 3;
    using MantidQt::Widgets::MplCpp::plot; 
    plot(QStringList(QString::fromStdString(workspaceName.get())), boost::none,
         createIndicesVector<int>(indicesString.get()), boost::none, plotKwargs,
         boost::none, boost::none, errorBars);
  }
}

void IndirectPlotOptionsModel::plotBins(bool errorBars) {
  auto const workspaceName = workspace();
  auto const indicesString = indices();
  if (workspaceName && indicesString) {
    using MantidQt::Widgets::MplCpp::MantidAxType;
    QHash<QString, QVariant> plotKwargs;
    plotKwargs["axis"] = static_cast<int>(MantidAxType::Bin);
    if (errorBars)
      plotKwargs["capsize"] = 3;
    using MantidQt::Widgets::MplCpp::plot;
    plot(QStringList(QString::fromStdString(workspaceName.get())), boost::none,
         createIndicesVector<int>(indicesString.get()), boost::none, plotKwargs,
         boost::none, boost::none, errorBars);
  }
}

void IndirectPlotOptionsModel::plotContour() {
  if (auto const workspaceName = workspace()) {
    using MantidQt::Widgets::MplCpp::pcolormesh;
    pcolormesh(QStringList(QString::fromStdString(workspaceName.get())));
  }
}

void IndirectPlotOptionsModel::plotTiled() {
  std::runtime_error(
      "Tiled plotting for the Workbench has not been implemented.");
}
#endif

} // namespace CustomInterfaces
} // namespace MantidQt
