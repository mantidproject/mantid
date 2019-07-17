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

std::string constructSpectraString(std::vector<int> const &spectras) {
  return joinCompress(spectras.begin(), spectras.end());
}

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

std::string getSpectraRange(std::string const &string) {
  auto const bounds = splitStringBy(string, "-");
  return std::stoull(bounds[0]) > std::stoull(bounds[1])
             ? bounds[1] + "-" + bounds[0]
             : string;
}

std::string rearrangeSpectraSubString(std::string const &string) {
  return string.find("-") != std::string::npos ? getSpectraRange(string)
                                               : string;
}

// Swaps the two numbers in a spectra range if they go from large to small
std::string rearrangeSpectraRangeStrings(std::string const &string) {
  std::string spectraString;
  auto subStrings = splitStringBy(string, ",");
  for (auto it = subStrings.begin(); it < subStrings.end(); ++it) {
    spectraString += rearrangeSpectraSubString(*it);
    spectraString += it != subStrings.end() ? "," : "";
  }
  return spectraString;
}

std::string formatSpectraString(std::string &string) {
  // Remove spaces
  removeFromIterable(std::remove_if(string.begin(), string.end(), isspace),
                     string);
  // Rearrange range strings
  auto spectras = parseRange(rearrangeSpectraRangeStrings(string));
  std::sort(spectras.begin(), spectras.end());
  // Remove duplicate entries
  removeFromIterable(std::unique(spectras.begin(), spectras.end()), spectras);
  return constructSpectraString(spectras);
}

template <typename T = std::size_t>
void addToSpectraVector(std::vector<T> &spectraVec,
                        std::size_t const &startSpec,
                        std::size_t const &startEnd) {
  for (auto spec = startSpec; spec <= startEnd; ++spec)
    spectraVec.emplace_back(spec);
}

template <typename T = std::size_t>
void addToSpectraVector(std::vector<T> &spectraVec,
                        std::string const &spectraString) {
  auto const range = splitStringBy(spectraString, "-");
  if (range.size() > 1)
    addToSpectraVector<T>(spectraVec, std::stoul(range[0]),
                          std::stoul(range[1]));
  else
    spectraVec.emplace_back(std::stoul(range[0]));
}

template <typename T = std::size_t>
std::vector<T> createSpectraVector(std::string const &spectra) {
  std::vector<T> spectraVec;
  for (auto subString : splitStringBy(spectra, ","))
    addToSpectraVector<T>(spectraVec, subString);
  return spectraVec;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
std::string expandSpectraRange(std::size_t const &startSpec,
                               std::size_t const &endSpec,
                               std::string const &separator) {
  std::string expandedRange;
  for (auto spec = startSpec; spec <= endSpec; ++spec) {
    expandedRange += std::to_string(spec);
    expandedRange += spec != endSpec ? separator : "";
  }
  return expandedRange;
}

std::string expandSpectraRange(std::string const &startSpec,
                               std::string const &endSpec,
                               std::string const &separator) {
  return expandSpectraRange(std::stoul(startSpec), std::stoul(endSpec),
                            separator);
}

void addToSpectraList(std::string &spectraList,
                      std::string const &spectraString) {
  auto const range = splitStringBy(spectraString, "-");
  auto const expandedSpectra =
      range.size() > 1 ? expandSpectraRange(range[0], range[1], ",") : range[0];
  spectraList += expandedSpectra;
}

std::string createSpectraList(std::string const &spectra) {
  std::string spectraList;
  auto const subStrings = splitStringBy(spectra, ",");
  for (auto iter = subStrings.begin(); iter < subStrings.end(); ++iter) {
    addToSpectraList(spectraList, *iter);
    spectraList += iter < subStrings.end() - 1 ? "," : "";
  }
  return "[" + spectraList + "]";
}

std::string createPlotSpectraString(std::string const &workspaceName,
                                    std::string const &spectra,
                                    bool errorbars) {
  auto const errors = errorbars ? "True" : "False";
  std::string plotString = "from mantidplot import plotSpectrum\n";
  return plotString + "plotSpectrum(['" + workspaceName + "'], " + spectra +
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
namespace IDA {

IndirectPlotOptionsModel::IndirectPlotOptionsModel()
    : m_spectra(boost::none), m_workspaceName(boost::none) {}

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
IndirectPlotOptionsModel::formatSpectra(std::string const &spectra) const {
  auto spectraString = spectra;
  return formatSpectraString(spectraString);
}

bool IndirectPlotOptionsModel::setSpectra(std::string const &spectra) {
  bool valid = validateSpectra(spectra);
  if (valid)
    m_spectra = spectra;
  else
    m_spectra = boost::none;
  return valid;
}

boost::optional<std::string> IndirectPlotOptionsModel::spectra() const {
  return m_spectra;
}

bool IndirectPlotOptionsModel::validateSpectra(
    std::string const &spectra) const {
  auto &ads = AnalysisDataService::Instance();
  if (!spectra.empty() && m_workspaceName)
    if (ads.doesExist(m_workspaceName.get()))
      return validateSpectra(
          ads.retrieveWS<MatrixWorkspace>(m_workspaceName.get()), spectra);
  return false;
}

bool IndirectPlotOptionsModel::validateSpectra(
    MatrixWorkspace_sptr workspace, std::string const &spectra) const {
  auto const numberOfHistograms = workspace->getNumberHistograms();
  auto const lastIndex = std::stoul(splitStringBy(spectra, ",-").back());
  return lastIndex < numberOfHistograms;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
boost::optional<std::string>
IndirectPlotOptionsModel::getPlotSpectraString(bool errorBars) const {
  auto const workspaceName = workspace();
  auto const spectraString = spectra();
  if (workspaceName && spectraString)
    return createPlotSpectraString(
        workspaceName.get(), createSpectraList(spectraString.get()), errorBars);
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
  auto const spectraString = spectra();
  if (workspaceName && spectraString)
    return createPlotTiledString(workspaceName.get(),
                                 createSpectraVector(spectraString.get()));
  return boost::none;
}

#else
void IndirectPlotOptionsModel::plotSpectra(bool errorBars) {
  auto const workspaceName = workspace();
  auto const spectraString = spectra();
  if (workspaceName && spectraString) {
    QHash<QString, QVariant> plotKwargs;
    if (errorBars)
      plotKwargs["capsize"] = 3;
    using MantidQt::Widgets::MplCpp::plot;
    plot(QStringList(QString::fromStdString(workspaceName.get())), boost::none,
         createSpectraVector<int>(spectraString.get()), boost::none, plotKwargs,
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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
