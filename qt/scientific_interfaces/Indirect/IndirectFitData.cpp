#include "IndirectFitData.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

std::string rangeToString(const std::pair<std::size_t, std::size_t> &range,
                          const std::string &delimiter = "-") {
  if (range.first != range.second)
    return std::to_string(range.first) + delimiter +
           std::to_string(range.second);
  return std::to_string(range.first);
}

std::vector<double>
excludeRegionsStringToVector(const std::string &excludeRegions) {
  std::vector<std::string> regionStrings;
  boost::split(regionStrings, excludeRegions, boost::is_any_of("\,,-"));
  std::vector<double> regions;
  std::transform(
      regionStrings.begin(), regionStrings.end(), std::back_inserter(regions),
      [](const std::string &str) { return boost::lexical_cast<double>(str); });
  return regions;
}

template <typename T, typename... Ts>
std::vector<T, Ts...> outOfRange(const std::vector<T, Ts...> &values,
                                 const T &minimum, const T &maximum) {
  std::vector<T, Ts...> result;
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
    if (range.first < minimum)
      notInRange.emplace_back(m_minimum);
    else if (range.second > maximum)
      notInRange.emplace_back(m_maximum);
    return notInRange;
  }

  std::vector<T> operator()(const std::string &list) const {
    return outOfRange(vectorFromString<T>(list), m_minimum, m_maximum);
  }

private:
  const T m_minimum, m_maximum;
};

struct ExtractFirstSpectrum : boost::static_visitor<std::size_t> {
  std::size_t
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return spectra.first;
  }

  std::size_t operator()(const std::string &spectra) const {
    auto index = spectra.find_first_of(",-");
    auto spectrumString = spectra.substr(0, index);
    return boost::lexical_cast<std::size_t>(spectrumString);
  }
};

struct CheckZeroSpectrum : boost::static_visitor<bool> {
  bool operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return false;
  }
  bool operator()(const std::string &spectra) const { return spectra.empty(); }
};

struct NumberOfSpectra : boost::static_visitor<std::size_t> {
  std::size_t
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return spectra.second - spectra.first;
  }

  std::size_t operator()(const std::string &spectra) const {
    return vectorFromString<std::size_t>(spectra).size();
  }
};

struct SpectraToString : boost::static_visitor<std::string> {
  SpectraToString(const std::string &rangeDelimiter = "-")
      : m_rangeDelimiter(rangeDelimiter) {}

  std::string operator()(const std::string &spectra) const { return spectra; }

  std::string
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return rangeToString(spectra, m_rangeDelimiter);
  }

private:
  const std::string m_rangeDelimiter;
};

struct CombineSpectra : boost::static_visitor<Spectra> {
  Spectra
  operator()(const std::pair<std::size_t, std::size_t> &spectra1,
             const std::pair<std::size_t, std::size_t> &spectra2) const {
    if (spectra1.second + 1 == spectra2.first)
      return std::make_pair(spectra1.first, spectra2.second);
    else if (spectra2.second + 1 == spectra1.first)
      return std::make_pair(spectra2.first, spectra1.second);
    else
      return rangeToString(spectra1) + "," + rangeToString(spectra2);
  }

  template <typename T, typename U>
  Spectra operator()(const T &spectra1, const U &spectra2) const {
    return boost::apply_visitor(SpectraToString(), spectra1) + "," +
           boost::apply_visitor(SpectraToString(), spectra2);
  }
};

struct GetSpectrum : boost::static_visitor<std::size_t> {
  GetSpectrum(std::size_t index) : m_index(index) {}

  std::size_t operator()(const std::pair<std::size_t, std::size_t> &spectra) {
    return spectra.first + m_index;
  }

  std::size_t operator()(const std::string &spectra) {
    return vectorFromString<std::size_t>(spectra)[m_index];
  }

private:
  std::size_t m_index;
};

template <typename Map> Map combineMaps(const Map &map1, const Map &map2) {
  Map map(map1);
  map.insert(map2.begin(), map2.end());
  return map;
}

template <typename T>
std::string join(const std::vector<T> &values, const char *delimiter) {
  std::stringstream stream;
  std::copy(values.begin(), values.end(),
            std::ostream_iterator<T>(stream, delimiter));
  return stream.str();
}

std::string cutLastOf(const std::string &str, const std::string &delimiter) {
  const auto cutIndex = str.find_last_of(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}
}; // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitData::IndirectFitData(MatrixWorkspace_sptr workspace,
                                 const Spectra &spectra)
    : m_workspace(workspace), m_spectra(spectra) {}

std::string
IndirectFitData::displayName(const std::string &formatString,
                             const std::string &rangeDelimiter) const {
  const auto workspaceName = cutLastOf(workspace()->getName(), "_red");
  const auto formatted = boost::format(formatString) % workspaceName %
                         boost::apply_visitor(SpectraToString(), m_spectra);
  return formatted.str();
}

Mantid::API::MatrixWorkspace_sptr IndirectFitData::workspace() const {
  return m_workspace.lock();
}

const Spectra &IndirectFitData::spectra() const { return m_spectra; }

std::size_t IndirectFitData::getSpectrum(std::size_t index) const {
  return boost::apply_visitor(GetSpectrum(index), m_spectra);
}

std::size_t IndirectFitData::numberOfSpectra() const {
  return boost::apply_visitor(NumberOfSpectra(), m_spectra);
}

bool IndirectFitData::zeroSpectra() const {
  return boost::apply_visitor(CheckZeroSpectrum(), m_spectra);
}

std::size_t IndirectFitData::firstSpectrum() const {
  return boost::apply_visitor(ExtractFirstSpectrum(), m_spectra);
}

std::string IndirectFitData::excludeRegionString(std::size_t spectrum) const {
  auto excludeRegion = m_excludeRegions.find(spectrum);
  if (excludeRegion != m_excludeRegions.end())
    return excludeRegion->second;
  return "";
}

const std::pair<double, double> &
IndirectFitData::range(std::size_t spectrum) const {
  auto it = m_ranges.find(spectrum);
  if (it != m_ranges.end())
    return it->second;
  return std::make_pair(0., 0.);
}

std::vector<double>
IndirectFitData::excludeRegionsVector(std::size_t spectrum) const {
  auto excludeRegion = m_excludeRegions.find(spectrum);
  if (excludeRegion != m_excludeRegions.end())
    return excludeRegionsStringToVector(excludeRegion->second);
  return std::vector<double>();
}

void IndirectFitData::setSpectra(const Spectra &spectra) {
  const auto visitor =
      SpectraOutOfRange<std::size_t>(0, workspace()->getNumberHistograms() - 1);
  const auto notInRange = boost::apply_visitor(visitor, m_spectra);

  if (notInRange.empty())
    m_spectra = spectra;
  else
    throw std::runtime_error("Spectra out of range: " + join(notInRange, ","));
}

void IndirectFitData::setExcludeRegionString(std::size_t spectrum,
                                             const std::string &excludeRegion) {
  m_excludeRegions[spectrum] = excludeRegion;
}

IndirectFitData &IndirectFitData::combine(const IndirectFitData &fitData) {
  m_spectra =
      boost::apply_visitor(CombineSpectra(), m_spectra, fitData.m_spectra);
  m_excludeRegions = combineMaps(fitData.m_excludeRegions, m_excludeRegions);
  m_ranges = combineMaps(fitData.m_ranges, m_ranges);
  return *this;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
