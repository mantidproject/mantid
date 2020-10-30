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

Spectra::Spectra(const QList<std::size_t> &spectra) : m_spectraList(spectra) {}

int Spectra::numberOfSpectra() const { return m_spectraList.size(); }

Dataset::Dataset(const QString &workspaceName,
                 const QList<std::size_t> &spectra)
    : m_workspaceName(workspaceName), m_spectra(Spectra(spectra)) {}

QString Dataset::datasetName() const { return m_workspaceName; }

QStringList Dataset::domainNames() const {
  const auto numberOfSpectra = m_spectra.numberOfSpectra();
  if (numberOfSpectra == 0)
    throw std::runtime_error("There are no spectra in this Dataset.");
  if (numberOfSpectra == 1)
    return QStringList(m_workspaceName);
  else {
    QStringList domainNames;
    for (const auto &specNum : spectraList())
      domainNames << m_workspaceName + " (" + QString::number(specNum) + ")";
    return domainNames;
  }
}

QList<std::size_t> Dataset::spectraList() const {
  return m_spectra.m_spectraList;
}

} // namespace MantidWidgets
} // namespace MantidQt
