// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionQParameters.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"

#include <utility>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::Inelastic;

std::vector<PairNameSpectra> findAxisLabels(TextAxis const *axis, std::vector<std::string> const &parameterSuffixes) {
  std::vector<PairNameSpectra> labelAndSpectra;

  for (auto i = 0u; i < axis->length(); ++i) {
    auto const label = axis->label(i);
    if (std::any_of(parameterSuffixes.cbegin(), parameterSuffixes.cend(),
                    [&](auto const &str) { return label.rfind(str) != std::string::npos; })) {
      labelAndSpectra.emplace_back(std::make_pair(label, i));
    }
  }
  return labelAndSpectra;
}

std::vector<PairNameSpectra> findAxisLabels(MatrixWorkspace_sptr const &workspace,
                                            std::vector<std::string> const &parameterSuffixes) {
  if (auto const axis = dynamic_cast<TextAxis *>(workspace->getAxis(1)))
    return findAxisLabels(axis, parameterSuffixes);
  return {};
}

template <typename T>
std::vector<T> extract(const std::vector<PairNameSpectra> &vec, std::function<T(PairNameSpectra)> const &lambda) {
  std::vector<T> result;
  result.reserve(vec.size());
  std::transform(vec.begin(), vec.end(), std::back_inserter(result), lambda);
  return result;
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionQParameters::FunctionQParameters() : m_widths(), m_eisfs() {}

FunctionQParameters::FunctionQParameters(const MatrixWorkspace_sptr &workspace)
    : m_widths(findAxisLabels(workspace, {".Width", ".FWHM", ".HWHM"})), m_eisfs(findAxisLabels(workspace, {".EISF"})),
      m_a0s(findAxisLabels(workspace, {".A0"})) {}

std::vector<std::string> FunctionQParameters::names(std::string const &parameterType) const {
  auto const nameGetter = [](PairNameSpectra const &pair) -> std::string { return pair.first; };
  if (parameterType == "Width") {
    return extract<std::string>(m_widths, nameGetter);
  } else if (parameterType == "EISF") {
    return extract<std::string>(m_eisfs, nameGetter);
  } else if (parameterType == "A0") {
    return extract<std::string>(m_a0s, nameGetter);
  }
  return {};
}

std::vector<std::size_t> FunctionQParameters::spectra(std::string const &parameterType) const {
  auto const spectraGetter = [](PairNameSpectra const &pair) -> std::size_t { return pair.second; };
  if (parameterType == "Width") {
    return extract<std::size_t>(m_widths, spectraGetter);
  } else if (parameterType == "EISF") {
    return extract<std::size_t>(m_eisfs, spectraGetter);
  } else if (parameterType == "A0") {
    return extract<std::size_t>(m_a0s, spectraGetter);
  }
  throw std::logic_error("An unexpected parameter type '" + parameterType + "'is active.");
}

std::vector<std::string> FunctionQParameters::types() const {
  std::vector<std::string> types;
  if (!m_widths.empty())
    types.emplace_back("Width");
  if (!m_eisfs.empty())
    types.emplace_back("EISF");
  if (!m_a0s.empty())
    types.emplace_back("A0");
  return types;
}

} // namespace MantidQt::CustomInterfaces::Inelastic
