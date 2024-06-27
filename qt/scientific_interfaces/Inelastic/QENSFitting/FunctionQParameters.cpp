// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionQParameters.h"

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionQParameters::FunctionQParameters() : m_widths(), m_widthSpectra(), m_eisf(), m_eisfSpectra() {}

FunctionQParameters::FunctionQParameters(std::pair<std::vector<std::string>, std::vector<std::size_t>> widths,
                                         std::pair<std::vector<std::string>, std::vector<std::size_t>> eisfs)
    : m_widths(widths.first), m_widthSpectra(widths.second), m_eisf(eisfs.first), m_eisfSpectra(eisfs.second) {}

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
