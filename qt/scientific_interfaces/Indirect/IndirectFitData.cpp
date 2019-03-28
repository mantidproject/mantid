// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitData.h"

#include "MantidKernel/Strings.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <sstream>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;
using namespace Mantid::Kernel::Strings;

std::string rangeToString(const std::pair<std::size_t, std::size_t> &range,
                          const std::string &delimiter = "-") {
  if (range.first != range.second)
    return std::to_string(range.first) + delimiter +
           std::to_string(range.second);
  return std::to_string(range.first);
}

template <template <typename...> class Vector, typename T, typename... Ts>
std::vector<T> outOfRange(const Vector<T, Ts...> &values, const T &minimum,
                          const T &maximum) {
  std::vector<T> result;
  for (auto &&value : values) {
    if (value < minimum || value > maximum)
      result.emplace_back(value);
  }
  return result;
}

template <typename T>
struct SpectraOutOfRange : boost::static_visitor<std::vector<T>> {
  SpectraOutOfRange(const T &minimum, const T &maximum)
      : m_minimum(minimum), m_maximum(maximum) {}

  std::vector<T> operator()(const std::pair<T, T> &range) const {
    std::vector<T> notInRange;
    if (range.first < m_minimum)
      notInRange.emplace_back(m_minimum);
    if (range.second > m_maximum)
      notInRange.emplace_back(m_maximum);
    return notInRange;
  }

  std::vector<T> operator()(const DiscontinuousSpectra<T> &spectra) const {
    return outOfRange(spectra, m_minimum, m_maximum);
  }

private:
  const T m_minimum, m_maximum;
};

struct CheckZeroSpectrum : boost::static_visitor<bool> {
  bool operator()(const std::pair<std::size_t, std::size_t> & /*unused*/) const {
    return false;
  }
  bool operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    return spectra.empty();
  }
};

struct NumberOfSpectra : boost::static_visitor<std::size_t> {
  std::size_t
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return 1 + (spectra.second - spectra.first);
  }

  std::size_t
  operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    return spectra.size();
  }
};

struct SpectraToString : boost::static_visitor<std::string> {
  explicit SpectraToString(const std::string &rangeDelimiter = "-")
      : m_rangeDelimiter(rangeDelimiter) {}

  std::string
  operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    return spectra.getString();
  }

  std::string
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return rangeToString(spectra, m_rangeDelimiter);
  }

private:
  const std::string m_rangeDelimiter;
};

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

struct CombineSpectra : boost::static_visitor<Spectra> {
  Spectra
  operator()(const std::pair<std::size_t, std::size_t> &spectra1,
             const std::pair<std::size_t, std::size_t> &spectra2) const {
    if (spectra1.second + 1 == spectra2.first)
      return std::make_pair(spectra1.first, spectra2.second);
    else if (spectra2.second + 1 == spectra1.first)
      return std::make_pair(spectra2.first, spectra1.second);
    else {
      return DiscontinuousSpectra<std::size_t>(createSpectraString(
          rangeToString(spectra1) + "," + rangeToString(spectra2)));
    }
  }

  Spectra operator()(const Spectra &spectra1, const Spectra &spectra2) const {
    return DiscontinuousSpectra<std::size_t>(createSpectraString(
        boost::apply_visitor(SpectraToString(), spectra1) + "," +
        boost::apply_visitor(SpectraToString(), spectra2)));
  }
};

struct GetSpectrum : boost::static_visitor<std::size_t> {
  explicit GetSpectrum(std::size_t index) : m_index(index) {}

  std::size_t
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return spectra.first + m_index;
  }

  std::size_t
  operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    return spectra[m_index];
  }

private:
  std::size_t m_index;
};

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

template <typename T, typename... Ts>
std::vector<T, Ts...> subvector(const std::vector<T, Ts...> vec,
                                std::size_t start, std::size_t end) {
  return std::vector<T, Ts...>(vec.begin() + start, vec.begin() + end);
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

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitData::IndirectFitData(MatrixWorkspace_sptr workspace,
                                 const Spectra &spectra)
    : m_workspace(workspace), m_spectra(DiscontinuousSpectra<std::size_t>("")) {
  setSpectra(spectra);
}

std::string
IndirectFitData::displayName(const std::string &formatString,
                             const std::string &rangeDelimiter) const {
  const auto workspaceName = getBasename();
  const auto spectraString =
      boost::apply_visitor(SpectraToString(rangeDelimiter), m_spectra);

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, spectraString);

  auto name = formatted.str();
  std::replace(name.begin(), name.end(), ',', '+');
  return name;
}

std::string IndirectFitData::displayName(const std::string &formatString,
                                         std::size_t spectrum) const {
  const auto workspaceName = getBasename();

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, std::to_string(spectrum));
  return formatted.str();
}

std::string IndirectFitData::getBasename() const {
  return cutLastOf(workspace()->getName(), "_red");
}

Mantid::API::MatrixWorkspace_sptr IndirectFitData::workspace() const {
  return m_workspace;
}

const Spectra &IndirectFitData::spectra() const { return m_spectra; }

std::size_t IndirectFitData::getSpectrum(std::size_t index) const {
  return boost::apply_visitor(GetSpectrum(index), m_spectra);
}

std::size_t IndirectFitData::numberOfSpectra() const {
  return boost::apply_visitor(NumberOfSpectra(), m_spectra);
}

bool IndirectFitData::zeroSpectra() const {
  if (m_workspace->getNumberHistograms())
    return boost::apply_visitor(CheckZeroSpectrum(), m_spectra);
  return true;
}

std::pair<double, double>
IndirectFitData::getRange(std::size_t spectrum) const {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end())
    return range->second;
  return getBinRange(m_workspace);
}

std::string IndirectFitData::getExcludeRegion(std::size_t spectrum) const {
  const auto region = m_excludeRegions.find(spectrum);
  if (region != m_excludeRegions.end())
    return region->second;
  return "";
}

std::vector<double>
IndirectFitData::excludeRegionsVector(std::size_t spectrum) const {
  return vectorFromString<double>(getExcludeRegion(spectrum));
}

void IndirectFitData::setSpectra(std::string const &spectra) {
  try {
    const Spectra spec =
        DiscontinuousSpectra<std::size_t>(createSpectraString(spectra));
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
  const auto visitor =
      SpectraOutOfRange<std::size_t>(0, workspace()->getNumberHistograms() - 1);
  auto notInRange = boost::apply_visitor(visitor, spectra);
  if (!notInRange.empty()) {
    if (notInRange.size() > 5)
      throw std::runtime_error("Spectra out of range: " +
                               join(subvector(notInRange, 0, 5), ",") + "...");
    throw std::runtime_error("Spectra out of range: " + join(notInRange, ","));
  }
}

void IndirectFitData::setStartX(double const &startX,
                                std::size_t const &spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end())
    range->second.first = startX;
  else if (m_workspace)
    m_ranges[spectrum] = std::make_pair(startX, m_workspace->x(0).back());
  else
    throw std::runtime_error(
        "Unable to set StartX: Workspace no longer exists.");
}

void IndirectFitData::setEndX(double const &endX, std::size_t const &spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end())
    range->second.second = endX;
  else if (m_workspace)
    m_ranges[spectrum] = std::make_pair(m_workspace->x(0).front(), endX);
  else
    throw std::runtime_error("Unable to set EndX: Workspace no longer exists.");
}

void IndirectFitData::setExcludeRegionString(
    std::string const &excludeRegionString, std::size_t const &spectrum) {
  if (!excludeRegionString.empty())
    m_excludeRegions[spectrum] = createExcludeRegionString(excludeRegionString);
  else
    m_excludeRegions[spectrum] = excludeRegionString;
}

IndirectFitData &IndirectFitData::combine(IndirectFitData const &fitData) {
  m_workspace = fitData.m_workspace;
  setSpectra(
      boost::apply_visitor(CombineSpectra(), m_spectra, fitData.m_spectra));
  m_excludeRegions.insert(std::begin(fitData.m_excludeRegions),
                          std::end(fitData.m_excludeRegions));
  m_ranges.insert(std::begin(fitData.m_ranges), std::end(fitData.m_ranges));
  return *this;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
