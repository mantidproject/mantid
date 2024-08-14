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

typedef std::pair<std::string, std::size_t> PairNameSpectra;

class MANTIDQT_INELASTIC_DLL FunctionQParameters {
public:
  FunctionQParameters();
  FunctionQParameters(const Mantid::API::MatrixWorkspace_sptr &workspace);

  std::vector<std::string> names(std::string const &parameterType) const;

  std::vector<std::size_t> spectra(std::string const &parameterType) const;

  std::vector<std::string> types() const;

  operator bool() const { return !m_widths.empty() || !m_eisfs.empty() || !m_a0s.empty(); }

private:
  std::vector<PairNameSpectra> m_widths;
  std::vector<PairNameSpectra> m_eisfs;
  std::vector<PairNameSpectra> m_a0s;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
