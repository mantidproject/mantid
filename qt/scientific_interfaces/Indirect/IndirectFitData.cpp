#include "IndirectFitData.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <sstream>

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
  bool operator()(const std::pair<std::size_t, std::size_t> &) const {
    return false;
  }
  bool operator()(const DiscontinuousSpectra<std::size_t> &spectra) const {
    return spectra.empty();
  }
};

struct NumberOfSpectra : boost::static_visitor<std::size_t> {
  std::size_t
  operator()(const std::pair<std::size_t, std::size_t> &spectra) const {
    return spectra.second - spectra.first;
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

struct CombineSpectra : boost::static_visitor<Spectra> {
  Spectra
  operator()(const std::pair<std::size_t, std::size_t> &spectra1,
             const std::pair<std::size_t, std::size_t> &spectra2) const {
    if (spectra1.second + 1 == spectra2.first)
      return std::make_pair(spectra1.first, spectra2.second);
    else if (spectra2.second + 1 == spectra1.first)
      return std::make_pair(spectra2.first, spectra1.second);
    else
      return DiscontinuousSpectra<std::size_t>(rangeToString(spectra1) + "," +
                                               rangeToString(spectra2));
  }

  Spectra operator()(const Spectra &spectra1, const Spectra &spectra2) const {
    return DiscontinuousSpectra<std::size_t>(
        boost::apply_visitor(SpectraToString(), spectra1) + "," +
        boost::apply_visitor(SpectraToString(), spectra2));
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
} // namespace

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
  const auto spectraString =
      boost::apply_visitor(SpectraToString(rangeDelimiter), m_spectra);

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, spectraString);
  return formatted.str();
}

std::string IndirectFitData::displayName(const std::string &formatString,
                                         std::size_t spectrum) const {
  const auto workspaceName = cutLastOf(workspace()->getName(), "_red");

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, std::to_string(spectrum));
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

std::pair<double, double>
IndirectFitData::getRange(std::size_t spectrum) const {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end())
    return range->second;
  return getBinRange(m_workspace.lock());
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

void IndirectFitData::setSpectra(const std::string &spectra) {
  setSpectra(DiscontinuousSpectra<std::size_t>(spectra));
}

void IndirectFitData::setSpectra(Spectra &&spectra) {
  validateSpectra(spectra);
  m_spectra = std::move(spectra);
}

void IndirectFitData::setSpectra(const Spectra &spectra) {
  validateSpectra(spectra);
  m_spectra = spectra;
}

void IndirectFitData::validateSpectra(const Spectra &spectra) {
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

void IndirectFitData::setStartX(double startX, std::size_t spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end())
    range->second.first = startX;
  else if (const auto workspace = m_workspace.lock())
    m_ranges[spectrum] = std::make_pair(startX, workspace->x(0).back());
  else
    throw std::runtime_error("Unable to set StartX: Workspace no longer exists.");
}

void IndirectFitData::setEndX(double endX, std::size_t spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end())
    range->second.second = endX;
  else if (const auto workspace = m_workspace.lock())
    m_ranges[spectrum] = std::make_pair(workspace->x(0).front(), endX);
  else
    throw std::runtime_error("Unable to set EndX: Workspace no longer exists.");
}

void IndirectFitData::setExcludeRegionString(const std::string &excludeRegion,
                                             std::size_t spectrum) {
  m_excludeRegions[spectrum] = excludeRegion;
}

IndirectFitData &IndirectFitData::combine(const IndirectFitData &fitData) {
  m_workspace = fitData.m_workspace;
  m_spectra =
      boost::apply_visitor(CombineSpectra(), m_spectra, fitData.m_spectra);
  m_excludeRegions.insert(std::begin(fitData.m_excludeRegions),
                          std::end(fitData.m_excludeRegions));
  m_ranges.insert(std::begin(fitData.m_ranges), std::end(fitData.m_ranges));
  return *this;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
