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

struct ContainsOneOrMore {
  explicit ContainsOneOrMore(std::vector<std::string> &&substrings) : m_substrings(std::move(substrings)) {}

  bool operator()(const std::string &str) const {
    return std::any_of(m_substrings.cbegin(), m_substrings.cend(),
                       [&str](std::string const &substring) { return str.rfind(substring) != std::string::npos; });
  }

private:
  std::vector<std::string> m_substrings;
};

template <typename Predicate>
std::pair<std::vector<std::string>, std::vector<std::size_t>> findAxisLabels(TextAxis const *axis,
                                                                             Predicate const &predicate) {
  std::vector<std::string> labels;
  std::vector<std::size_t> spectra;

  for (auto i = 0u; i < axis->length(); ++i) {
    auto label = axis->label(i);
    if (predicate(label)) {
      labels.emplace_back(label);
      spectra.emplace_back(i);
    }
  }
  return std::make_pair(labels, spectra);
}

template <typename Predicate>
std::pair<std::vector<std::string>, std::vector<std::size_t>> findAxisLabels(MatrixWorkspace_sptr const &workspace,
                                                                             Predicate const &predicate) {
  if (auto const axis = dynamic_cast<TextAxis *>(workspace->getAxis(1)))
    return findAxisLabels(axis, predicate);
  return std::make_pair(std::vector<std::string>(), std::vector<std::size_t>());
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionQParameters::FunctionQParameters() : m_widths(), m_widthSpectra(), m_eisf(), m_eisfSpectra() {}

FunctionQParameters::FunctionQParameters(const MatrixWorkspace_sptr &workspace) {
  auto const foundWidths = findAxisLabels(workspace, ContainsOneOrMore({".Width", ".FWHM"}));
  auto const foundEISF = findAxisLabels(workspace, ContainsOneOrMore({".EISF"}));

  m_widths = foundWidths.first;
  m_widthSpectra = foundWidths.second;
  m_eisf = foundEISF.first;
  m_eisfSpectra = foundEISF.second;
}

std::vector<std::string> FunctionQParameters::names(std::string const &parameterType) const {
  if (parameterType == "Width") {
    return m_widths;
  } else if (parameterType == "EISF") {
    return m_eisf;
  }
  return {};
}

std::vector<std::size_t> FunctionQParameters::spectra(std::string const &parameterType) const {
  if (parameterType == "Width") {
    return m_widthSpectra;
  } else if (parameterType == "EISF") {
    return m_eisfSpectra;
  }
  throw std::logic_error("An unexpected parameter type '" + parameterType + "'is active.");
}

std::vector<std::string> FunctionQParameters::types() const {
  std::vector<std::string> types;
  if (!m_widths.empty())
    types.emplace_back("Width");
  if (!m_eisf.empty())
    types.emplace_back("EISF");
  return types;
}

} // namespace MantidQt::CustomInterfaces::Inelastic
