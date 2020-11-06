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

FunctionModelDataset::FunctionModelDataset(const QString &workspaceName,
                                           const Spectra &spectra)
    : m_workspaceName(workspaceName), m_spectra(spectra) {}

QString FunctionModelDataset::datasetName() const { return m_workspaceName; }

QStringList FunctionModelDataset::domainNames() const {
  const auto numberOfSpectra = m_spectra.size().value;
  if (numberOfSpectra == 0u)
    throw std::runtime_error("There are no spectra in this Dataset.");
  if (numberOfSpectra == 1u)
    return QStringList(m_workspaceName);
  else {
    QStringList domainNames;
    for (const auto &spectrum : m_spectra)
      domainNames << m_workspaceName + " (" + QString::number(spectrum.value) +
                         ")";
    return domainNames;
  }
}

std::size_t FunctionModelDataset::numberOfSpectra() const {
  return m_spectra.size().value;
}

} // namespace MantidWidgets
} // namespace MantidQt
