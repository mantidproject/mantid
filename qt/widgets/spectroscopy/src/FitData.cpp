// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/FitData.h"

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
using namespace MantidQt::MantidWidgets;

namespace {
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace Mantid::Kernel::Strings;

/**
 * @brief Extract Q values from vertical dimension of the workspace, or compute
 * them.
 * @param workspace workspace possibly containing Q values.
 */
std::vector<double> extractQValues(const MatrixWorkspace_sptr &workspace, const FunctionModelSpectra &spectra) {
  std::vector<double> qs;
  // Check if the vertical axis has units of momentum transfer, then extract Q
  // values...
  auto axis_ptr = dynamic_cast<Mantid::API::NumericAxis *>(workspace->getAxis(1));
  if (axis_ptr) {
    const std::shared_ptr<Mantid::Kernel::Unit> &unit_ptr = axis_ptr->unit();
    if (unit_ptr->unitID() == "MomentumTransfer") {
      qs.reserve(spectra.size().value);
      std::transform(spectra.begin(), spectra.end(), std::back_inserter(qs),
                     [&axis_ptr](const auto &spectrum) { return axis_ptr->operator()(spectrum.value); });
    }
  }
  // ...otherwise, compute the momentum transfer for each spectrum, if possible
  else {
    const auto &spectrumInfo = workspace->spectrumInfo();
    for (const auto &spectrum : spectra) {
      if (spectrumInfo.hasDetectors(spectrum.value)) {
        const auto detID = spectrumInfo.detector(spectrum.value).getID();
        double efixed = workspace->getEFixed(detID);
        double usignTheta = 0.5 * spectrumInfo.twoTheta(spectrum.value);
        double q = Mantid::Kernel::UnitConversion::convertToElasticQ(usignTheta, efixed);
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

std::vector<std::string> splitStringBy(std::string const &str, std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) { return subString.empty(); }),
                   subStrings.end());
  return subStrings;
}

std::string getSpectraRange(std::string const &string) {
  auto const bounds = splitStringBy(string, "-");
  return std::stoull(bounds[0]) > std::stoull(bounds[1]) ? bounds[1] + "-" + bounds[0] : string;
}

std::string rearrangeSpectraSubString(std::string const &string) {
  return string.find("-") != std::string::npos ? getSpectraRange(string) : string;
}

// Swaps the two numbers in a spectra range if they go from large to small
std::string rearrangeSpectraRangeStrings(std::string const &string) {
  std::string spectraString;
  std::vector<std::string> subStrings = splitStringBy(string, ",");
  for (auto it = subStrings.begin(); it < subStrings.end(); ++it) {
    spectraString += rearrangeSpectraSubString(*it);
    spectraString += it != subStrings.end() - 1 ? "," : "";
  }
  return spectraString;
}

std::string createSpectraString(std::string string) {
  string.erase(std::remove_if(string.begin(), string.end(), isspace), string.end());
  std::vector<int> spectras = parseRange(rearrangeSpectraRangeStrings(string));
  std::sort(spectras.begin(), spectras.end());
  // Remove duplicate entries
  spectras.erase(std::unique(spectras.begin(), spectras.end()), spectras.end());
  return constructSpectraString(spectras);
}

template <typename T> std::string join(const std::vector<T> &values, const char *delimiter) {
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

boost::basic_format<char> &tryPassFormatArgument(boost::basic_format<char> &formatString, const std::string &arg) {
  try {
    auto &tmpString = formatString % arg;
    return tmpString;
  } catch (const boost::io::too_many_args &) {
    return formatString;
  }
}

std::pair<double, double> getBinRange(const MatrixWorkspace_sptr &workspace) {
  return std::make_pair(workspace->x(0).front(), workspace->x(0).back());
}

double convertBoundToDoubleAndFormat(std::string const &str) { return std::round(std::stod(str) * 1000) / 1000; }

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

std::vector<double> getBoundsAsDoubleVector(std::vector<std::string> const &boundStrings) {
  std::vector<double> bounds;
  bounds.reserve(boundStrings.size());
  std::transform(boundStrings.cbegin(), boundStrings.cend(), std::back_inserter(bounds),
                 [](const auto &bound) { return convertBoundToDoubleAndFormat(bound); });
  return bounds;
}

std::string createExcludeRegionString(std::string regionString) {
  regionString.erase(std::remove_if(regionString.begin(), regionString.end(), isspace), regionString.end());
  auto bounds = getBoundsAsDoubleVector(splitStringBy(regionString, ","));
  return orderExcludeRegionString(bounds);
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FitData::FitData(const MatrixWorkspace_sptr &workspace, const FunctionModelSpectra &spectra)
    : m_workspace(workspace), m_spectra(FunctionModelSpectra("")) {
  setSpectra(spectra);
  auto const range = !spectra.empty() ? getBinRange(workspace) : std::make_pair(0.0, 0.0);
  for (auto const &spectrum : spectra) {
    m_ranges[spectrum] = range;
  }
}

std::string FitData::displayName(const std::string &formatString, const std::string &) const {
  const auto workspaceName = getBasename();
  const auto spectraString = m_spectra.getString();

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, spectraString);

  auto name = formatted.str();
  std::replace(name.begin(), name.end(), ',', '+');
  return name;
}

std::string FitData::displayName(const std::string &formatString, WorkspaceIndex spectrum) const {
  const auto workspaceName = getBasename();

  auto formatted = boost::format(formatString);
  formatted = tryPassFormatArgument(formatted, workspaceName);
  formatted = tryPassFormatArgument(formatted, std::to_string(spectrum.value));
  return formatted.str();
}

std::string FitData::getBasename() const { return cutLastOf(workspace()->getName(), "_red"); }

Mantid::API::MatrixWorkspace_sptr FitData::workspace() const { return m_workspace; }

const FunctionModelSpectra &FitData::spectra() const { return m_spectra; }

FunctionModelSpectra &FitData::getMutableSpectra() { return m_spectra; }

WorkspaceIndex FitData::getSpectrum(FitDomainIndex index) const { return m_spectra[index]; }

FitDomainIndex FitData::numberOfSpectra() const { return m_spectra.size(); }

bool FitData::zeroSpectra() const {
  if (m_workspace->getNumberHistograms())
    return m_spectra.empty();
  return true;
}

std::pair<double, double> FitData::getRange(WorkspaceIndex spectrum) const {
  auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end()) {
    return range->second;
  }
  range = m_ranges.find(getSpectrum(FitDomainIndex{0}));
  if (range != m_ranges.end()) {
    return range->second;
  }
  return getBinRange(m_workspace);
}

std::string FitData::getExcludeRegion(WorkspaceIndex spectrum) const {
  const auto region = m_excludeRegions.find(spectrum);
  if (region != m_excludeRegions.end())
    return region->second;
  return "";
}

std::vector<double> FitData::excludeRegionsVector(WorkspaceIndex spectrum) const {
  return vectorFromString<double>(getExcludeRegion(spectrum));
}

std::vector<double> FitData::getQValues() const { return extractQValues(m_workspace, m_spectra); }

void FitData::setSpectra(std::string const &spectra) {
  try {
    const FunctionModelSpectra spec = FunctionModelSpectra(createSpectraString(spectra));
    setSpectra(spec);
  } catch (std::exception &ex) {
    throw std::runtime_error("Spectra too large for cast: " + std::string(ex.what()));
  }
}

void FitData::setSpectra(FunctionModelSpectra &&spectra) {
  validateSpectra(spectra);
  m_spectra = std::move(spectra);
}

void FitData::setSpectra(FunctionModelSpectra const &spectra) {
  validateSpectra(spectra);
  m_spectra = spectra;
}

void FitData::validateSpectra(FunctionModelSpectra const &spectra) {
  size_t maxValue = workspace()->getNumberHistograms() - 1;
  std::vector<size_t> notInRange;
  for (auto const &i : spectra) {
    if (i.value > maxValue)
      notInRange.emplace_back(i.value);
  }
  if (!notInRange.empty()) {
    if (notInRange.size() > 5)
      throw std::runtime_error("Spectra out of range: " +
                               join(std::vector<size_t>(notInRange.begin(), notInRange.begin() + 5), ",") + "...");
    throw std::runtime_error("Spectra out of range: " + join(notInRange, ","));
  }
}

void FitData::setStartX(double const &startX, WorkspaceIndex const &spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end()) {
    if (range->second.second < startX) {
      range->second.first = range->second.second;
    } else {
      range->second.first = startX;
    }
  } else if (m_workspace) {
    m_ranges[spectrum] = std::make_pair(startX, m_workspace->x(0).back());
  } else {
    throw std::runtime_error("Unable to set StartX: Workspace no longer exists.");
  }
}

void FitData::setStartX(double const &startX) {
  for (auto const &spectrum : m_spectra) {
    setStartX(startX, spectrum);
  }
}

void FitData::setEndX(double const &endX, WorkspaceIndex const &spectrum) {
  const auto range = m_ranges.find(spectrum);
  if (range != m_ranges.end()) {
    if (range->second.first > endX) {
      range->second.second = range->second.first;
    } else {
      range->second.second = endX;
    }
  } else if (m_workspace) {
    m_ranges[spectrum] = std::make_pair(m_workspace->x(0).front(), endX);
  } else {
    throw std::runtime_error("Unable to set EndX: Workspace no longer exists.");
  }
}

void FitData::setEndX(double const &endX) {
  for (auto const &spectrum : m_spectra) {
    setEndX(endX, spectrum);
  }
}

void FitData::setExcludeRegionString(std::string const &excludeRegionString, WorkspaceIndex const &spectrum) {
  if (!excludeRegionString.empty())
    m_excludeRegions[spectrum] = createExcludeRegionString(excludeRegionString);
  else
    m_excludeRegions[spectrum] = excludeRegionString;
}

FitData &FitData::combine(FitData const &fitData) {
  m_workspace = fitData.m_workspace;
  setSpectra(m_spectra.combine(fitData.m_spectra));
  m_excludeRegions.insert(std::begin(fitData.m_excludeRegions), std::end(fitData.m_excludeRegions));
  for (auto rangeIt = fitData.m_ranges.begin(); rangeIt != fitData.m_ranges.end(); ++rangeIt) {
    auto sameSpecRange = m_ranges.find(rangeIt->first);
    if (sameSpecRange != m_ranges.end()) {
      sameSpecRange->second = std::make_pair(std::max(sameSpecRange->second.first, rangeIt->second.first),
                                             std::min(sameSpecRange->second.second, rangeIt->second.second));
    }
  }
  m_ranges.insert(std::begin(fitData.m_ranges), std::end(fitData.m_ranges));
  return *this;
}

} // namespace MantidQt::CustomInterfaces::Inelastic
