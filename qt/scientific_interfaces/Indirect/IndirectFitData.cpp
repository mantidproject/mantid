// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitData.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <sstream>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;
using namespace Mantid::Kernel::Strings;

/**
 * @brief Extract Q values from vertical dimension of the workspace, or compute
 * them.
 * @param workspace workspace possibly containing Q values.
 */
std::vector<double> extractQValues(const MatrixWorkspace_sptr workspace,
                                   const Spectra &spectra) {
  std::vector<double> qs;
  // Check if the vertical axis has units of momentum transfer, then extract Q
  // values...
  auto axis_ptr =
      dynamic_cast<Mantid::API::NumericAxis *>(workspace->getAxis(1));
  if (axis_ptr) {
    const boost::shared_ptr<Mantid::Kernel::Unit> &unit_ptr = axis_ptr->unit();
    if (unit_ptr->unitID() == "MomentumTransfer") {
      for (auto spectrum : spectra) {
        qs.emplace_back(axis_ptr->operator()(spectrum.value));
        // qs = axis_ptr->getValues();
      }
    }
  }
  // ...otherwise, compute the momentum transfer for each spectrum, if possible
  else {
    const auto &spectrumInfo = workspace->spectrumInfo();
    for (auto spectrum : spectra) {
      if (spectrumInfo.hasDetectors(spectrum.value)) {
        const auto detID = spectrumInfo.detector(spectrum.value).getID();
        double efixed = workspace->getEFixed(detID);
        double usignTheta = 0.5 * spectrumInfo.twoTheta(spectrum.value);
        double q = Mantid::Kernel::UnitConversion::convertToElasticQ(usignTheta,
                                                                     efixed);
        qs.emplace_back(q);
      } else {
        qs.clear();
        break;
      }
    }
  }
  return qs;
}

std::string constructSpectraString(std::vector<int> const &spectras) {
  return joinCompress(spectras.begin(), spectras.end());
}

std::vector<std::string> splitStringBy(std::string const &str,
                                       std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) {
                                    return subString.empty();
                                  }),
                   subStrings.end());
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
  std::vector<std::string> subStrings = splitStringBy(string, ",");
  for (auto it = subStrings.begin(); it < subStrings.end(); ++it) {
    spectraString += rearrangeSpectraSubString(*it);
    spectraString += it != subStrings.end() ? "," : "";
  }
  return spectraString;
}

std::string createSpectraString(std::string string) {
  string.erase(std::remove_if(string.begin(), string.end(), isspace),
               string.end());
  std::vector<int> spectras = parseRange(rearrangeSpectraRangeStrings(string));
  std::sort(spectras.begin(), spectras.end());
  // Remove duplicate entries
  spectras.erase(std::unique(spectras.begin(), spectras.end()), spectras.end());
  return constructSpectraString(spectras);
}

template <typename T>
std::string join(const std::vector<T> &values, const char *delimiter) {
  if (values.empty())
    return "";

  std::stringstream stream;
  stream << values.front();
  for (auto i = 1u; i < values.size(); ++i)
    stream << delimiter << values[i];
  return stream.str();
}

std::string cutLastOf(const std::string &str, const std::string &delimiter) {
  const auto cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}

boost::basic_format<char> &
tryPassFormatArgument(boost::basic_format<char> &formatString,
                      const std::string &arg) {
  try {
    return formatString % arg;
  } catch (const boost::io::too_many_args &) {
    return formatString;
  }
}

std::pair<double, double> getBinRange(MatrixWorkspace_sptr workspace) {
  return std::make_pair(workspace->x(0).front(), workspace->x(0).back());
}

double convertBoundToDoubleAndFormat(std::string const &str) {
  return std::round(std::stod(str) * 1000) / 1000;
}

std::string constructExcludeRegionString(std::vector<double> const &bounds) {
  std::string excludeRegion;
  for (auto it = bounds.begin(); it < bounds.end(); ++it) {
    auto const splitDouble = splitStringBy(std::to_string(*it), ".");
    excludeRegion += splitDouble[0] + "." + splitDouble[1].substr(0, 3);
    excludeRegion += it == bounds.end() - 1 ? "" : ",";
  }
  return excludeRegion;
}

std::string orderExcludeRegionString(std::vector<double> &bounds) {
  for (auto it = bounds.begin(); it < bounds.end() - 1; it += 2)
    if (*it > *(it + 1))
      std::iter_swap(it, it + 1);
  return constructExcludeRegionString(bounds);
}

std::vector<double>
getBoundsAsDoubleVector(std::vector<std::string> const &boundStrings) {
  std::vector<double> bounds;
  bounds.reserve(boundStrings.size());
  for (auto bound : boundStrings)
    bounds.emplace_back(convertBoundToDoubleAndFormat(bound));
  return bounds;
}

std::string createExcludeRegionString(std::string regionString) {
  regionString.erase(
      std::remove_if(regionString.begin(), regionString.end(), isspace),
      regionString.end());
  auto bounds = getBoundsAsDoubleVector(splitStringBy(regionString, ","));
  return orderExcludeRegionString(bounds);
}

std::vector<MantidQt::CustomInterfaces::IDA::WorkspaceIndex>
workspaceIndexVectorFromString(const std::string &listString) {
  auto const intVec = vectorFromString<int>(listString);
  std::vector<MantidQt::CustomInterfaces::IDA::WorkspaceIndex> output;
  for (auto const i : intVec) {
    output.emplace_back(MantidQt::CustomInterfaces::IDA::WorkspaceIndex{i});
  }
  return output;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

Spectra::Spectra(const std::string &str)
    : m_vec(workspaceIndexVectorFromString(str)), m_isContinuous(true) {
  checkContinuous();
}

Spectra::Spectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  if (maximum < minimum) {
    std::swap(minimum, maximum);
  }
  m_vec.resize(maximum.value - minimum.value + 1);
  std::iota(m_vec.begin(), m_vec.end(), minimum);
}

Spectra::Spectra(const Spectra &vec)
    : m_vec(vec.m_vec), m_isContinuous(vec.m_isContinuous) {}

Spectra::Spectra(Spectra &&vec)
    : m_vec(std::move(vec.m_vec)),
      m_isContinuous(std::move(vec.m_isContinuous)) {}

Spectra &Spectra::operator=(const Spectra &vec) {
  m_vec = vec.m_vec;
  m_isContinuous = vec.m_isContinuous;
  return *this;
}

Spectra &Spectra::operator=(Spectra &&vec) {
  m_vec = std::move(vec.m_vec);
  m_isContinuous = std::move(vec.m_isContinuous);
  return *this;
}

bool Spectra::empty() const { return m_vec.empty(); }

TableRowIndex Spectra::size() const {
  return TableRowIndex{static_cast<int>(m_vec.size())};
}

std::string Spectra::getString() const {
  if (empty())
    return "";
  if (m_isContinuous)
    return m_vec.size() > 1 ? std::to_string(m_vec.front().value) + "-" +
                                  std::to_string(m_vec.back().value)
                            : std::to_string(m_vec.front().value);
  std::vector<int> out(m_vec.size());
  std::transform(m_vec.begin(), m_vec.end(), out.begin(),
                 [](WorkspaceIndex i) { return i.value; });
  return Mantid::Kernel::Strings::toString(out);
}

std::pair<WorkspaceIndex, WorkspaceIndex> Spectra::getMinMax() const {
  if (empty())
    return std::make_pair(WorkspaceIndex{0}, WorkspaceIndex{0});
  return std::make_pair(m_vec.front(), m_vec.back());
}

bool Spectra::operator==(Spectra const &spec) const {
  return this->getString() == spec.getString();
}

bool Spectra::isContinuous() const { return m_isContinuous; }

TableRowIndex Spectra::indexOf(WorkspaceIndex i) const {
  auto const it = std::find(begin(), end(), i);
  if (it == end()) {
    throw std::runtime_error("Spectrum index " + std::to_string(i.value) +
                             " not found.");
  }
  return TableRowIndex{static_cast<int>(std::distance(begin(), it))};
}

Spectra Spectra::combine(const Spectra &other) const {
  std::set<WorkspaceIndex> indices(begin(), end());
  indices.insert(other.begin(), other.end());
  return Spectra(indices);
}

Spectra::Spectra(const std::set<WorkspaceIndex> &indices)
    : m_vec(indices.begin(), indices.end()) {
  checkContinuous();
}

void Spectra::checkContinuous() {
  m_isContinuous = true;
  if (m_vec.size() > 1) {
    for (size_t i = 1; i < m_vec.size(); ++i) {
      if (m_vec[i].value - m_vec[i - 1].value != 1) {
        m_isContinuous = false;
        break;
      }
    }
  }
}

IndirectFitData::IndirectFitData(MatrixWorkspace_sptr workspace,
                                 const Spectra &spectra)
    : m_workspace(workspace), m_spectra(Spectra("")) {
  setSpectra(spectra);
  auto const range =
      !spectra.empty() ? getBinRange(workspace) : std::make_pair(0.0, 0.0);
  for (auto const spectrum : spectra) {
    m_ranges[spectrum] = range;
  }
}

std::string IndirectFitData::displayName(const std::string &formatString,
                                         const std::string &) const {
  const auto workspaceName = getBasename();
  const auto spectraString = m_spectra.getString();

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, spectraString);

  auto name = formatted.str();
  std::replace(name.begin(), name.end(), ',', '+');
  return name;
}

std::string IndirectFitData::displayName(const std::string &formatString,
                                         WorkspaceIndex spectrum) const {
  const auto workspaceName = getBasename();

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, std::to_string(spectrum.value));
  return formatted.str();
}

std::string IndirectFitData::getBasename() const {
  return cutLastOf(workspace()->getName(), "_red");
}

Mantid::API::MatrixWorkspace_sptr IndirectFitData::workspace() const {
  return m_workspace;
}

const Spectra &IndirectFitData::spectra() const { return m_spectra; }

WorkspaceIndex IndirectFitData::getSpectrum(TableRowIndex index) const {
  return m_spectra[index];
}

TableRowIndex IndirectFitData::numberOfSpectra() const {
  return m_spectra.size();
}

bool IndirectFitData::zeroSpectra() const {
  if (m_workspace->getNumberHistograms())
    return m_spectra.empty();
  return true;
}

std::pair<double, double>
IndirectFitData::getRange(WorkspaceIndex spectrum) const {
  auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end()) {
    return range->second;
  }
  range = m_ranges.find(getSpectrum(TableRowIndex{0}));
  if (range != m_ranges.end()) {
    return range->second;
  }
  return getBinRange(m_workspace);
}

std::string IndirectFitData::getExcludeRegion(WorkspaceIndex spectrum) const {
  const auto region = m_excludeRegions.find(spectrum);
  if (region != m_excludeRegions.end())
    return region->second;
  return "";
}

std::vector<double>
IndirectFitData::excludeRegionsVector(WorkspaceIndex spectrum) const {
  return vectorFromString<double>(getExcludeRegion(spectrum));
}

std::vector<double> IndirectFitData::getQValues() const {
  return extractQValues(m_workspace, m_spectra);
}

void IndirectFitData::setSpectra(std::string const &spectra) {
  try {
    const Spectra spec = Spectra(createSpectraString(spectra));
    setSpectra(spec);
  } catch (std::exception &ex) {
    throw std::runtime_error("Spectra too large for cast: " +
                             std::string(ex.what()));
  }
}

void IndirectFitData::setSpectra(Spectra &&spectra) {
  validateSpectra(spectra);
  m_spectra = std::move(spectra);
}

void IndirectFitData::setSpectra(Spectra const &spectra) {
  validateSpectra(spectra);
  m_spectra = spectra;
}

void IndirectFitData::validateSpectra(Spectra const &spectra) {
  int maxValue = static_cast<int>(workspace()->getNumberHistograms()) - 1;
  std::vector<int> notInRange;
  for (auto const i : spectra) {
    if (i.value < 0 || i.value > maxValue)
      notInRange.emplace_back(i.value);
  }
  if (!notInRange.empty()) {
    if (notInRange.size() > 5)
      throw std::runtime_error(
          "Spectra out of range: " +
          join(std::vector<int>(notInRange.begin(), notInRange.begin() + 5),
               ",") +
          "...");
    throw std::runtime_error("Spectra out of range: " + join(notInRange, ","));
  }
}

void IndirectFitData::setStartX(double const &startX,
                                WorkspaceIndex const &spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end())
    range->second.first = startX;
  else if (m_workspace)
    m_ranges[spectrum] = std::make_pair(startX, m_workspace->x(0).back());
  else
    throw std::runtime_error(
        "Unable to set StartX: Workspace no longer exists.");
}

void IndirectFitData::setStartX(double const &startX) {
  for (auto const spectrum : m_spectra) {
    setStartX(startX, spectrum);
  }
}

void IndirectFitData::setEndX(double const &endX,
                              WorkspaceIndex const &spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end()) {
    range->second.second = endX;
  } else if (m_workspace) {
    m_ranges[spectrum] = std::make_pair(m_workspace->x(0).front(), endX);
  } else
    throw std::runtime_error("Unable to set EndX: Workspace no longer exists.");
}

void IndirectFitData::setEndX(double const &endX) {
  for (auto const spectrum : m_spectra) {
    setEndX(endX, spectrum);
  }
}

void IndirectFitData::setExcludeRegionString(
    std::string const &excludeRegionString, WorkspaceIndex const &spectrum) {
  if (!excludeRegionString.empty())
    m_excludeRegions[spectrum] = createExcludeRegionString(excludeRegionString);
  else
    m_excludeRegions[spectrum] = excludeRegionString;
}

IndirectFitData &IndirectFitData::combine(IndirectFitData const &fitData) {
  m_workspace = fitData.m_workspace;
  setSpectra(m_spectra.combine(fitData.m_spectra));
  m_excludeRegions.insert(std::begin(fitData.m_excludeRegions),
                          std::end(fitData.m_excludeRegions));
  for (auto rangeIt = fitData.m_ranges.begin();
       rangeIt != fitData.m_ranges.end(); ++rangeIt) {
    auto sameSpecRange = m_ranges.find(rangeIt->first);
    if (sameSpecRange != m_ranges.end()) {
      sameSpecRange->second = std::make_pair(
          std::max(sameSpecRange->second.first, rangeIt->second.first),
          std::min(sameSpecRange->second.second, rangeIt->second.second));
    }
  }
  m_ranges.insert(std::begin(fitData.m_ranges), std::end(fitData.m_ranges));
  return *this;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
