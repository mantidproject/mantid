// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionModelDataset.h"

#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {

FunctionModelDataset::FunctionModelDataset(const QString &workspaceName, const FunctionModelSpectra &spectra)
    : m_workspaceName(workspaceName), m_spectra(spectra) {}

FunctionModelDataset::FunctionModelDataset(QString workspaceName, FunctionModelSpectra &&spectra)
    : m_workspaceName(workspaceName), m_spectra(std::move(spectra)) {}

/**
 * Returns the names given to each domain (i.e. spectrum) in this dataset. This
 * is required for display in the EditLocalParameter Dialog.
 *
 * @returns the names given to each domain (i.e. spectrum) in this dataset.
 */
QStringList FunctionModelDataset::domainNames() const {
  const auto numberOfSpectra = m_spectra.size().value;
  if (numberOfSpectra == 0u)
    throw std::runtime_error("There are no spectra in this Dataset.");
  if (numberOfSpectra == 1u)
    return QStringList(m_workspaceName);
  else {
    QStringList domainNames;
    for (const auto &spectrum : m_spectra)
      domainNames << m_workspaceName + " (" + QString::number(spectrum.value) + ")";
    return domainNames;
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
