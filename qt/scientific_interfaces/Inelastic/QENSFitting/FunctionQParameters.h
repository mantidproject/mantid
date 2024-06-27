// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL FunctionQParameters {
public:
  FunctionQParameters();
  FunctionQParameters(const Mantid::API::MatrixWorkspace_sptr &workspace);

  std::vector<std::string> names(std::string const &parameterType) const;

  std::vector<std::size_t> spectra(std::string const &parameterType) const;

  std::vector<std::string> types() const;

  operator bool() const { return !m_widthSpectra.empty() || !m_eisfSpectra.empty(); }

private:
  std::vector<std::string> m_widths;
  std::vector<std::size_t> m_widthSpectra;
  std::vector<std::string> m_eisf;
  std::vector<std::size_t> m_eisfSpectra;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
