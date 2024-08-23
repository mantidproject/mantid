// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionModelDataset.h"

#include <stdexcept>
#include <utility>

namespace MantidQt::MantidWidgets {

FunctionModelDataset::FunctionModelDataset(std::string workspaceName, FunctionModelSpectra spectra)
    : m_workspaceName(std::move(workspaceName)), m_spectra(std::move(spectra)) {}

/**
 * Returns the names given to each domain (i.e. spectrum) in this dataset. This
 * is required for display in the EditLocalParameter Dialog.
 *
 * @returns the names given to each domain (i.e. spectrum) in this dataset.
 */
std::vector<std::string> FunctionModelDataset::domainNames() const {
  const auto numOfSpectra = m_spectra.size().value;
  if (numOfSpectra == 0u)
    throw std::runtime_error("There are no spectra in this Dataset.");
  if (numOfSpectra == 1u)
    return std::vector<std::string>{m_workspaceName};
  else {
    std::vector<std::string> domains;
    std::transform(m_spectra.begin(), m_spectra.end(), std::back_inserter(domains),
                   [&](auto const &spectrum) { return m_workspaceName + " (" + std::to_string(spectrum.value) + ")"; });
    return domains;
  }
}

} // namespace MantidQt::MantidWidgets
