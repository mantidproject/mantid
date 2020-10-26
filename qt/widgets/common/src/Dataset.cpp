// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Dataset.h"

namespace MantidQt {
namespace MantidWidgets {

Dataset::Dataset() : m_workspaceName(), m_spectraList() {}

Dataset::Dataset(QString const &workspaceName,
                 QList<std::size_t> const &spectraList)
    : m_workspaceName(workspaceName), m_spectraList(spectraList) {}

std::size_t Dataset::numberOfDomains() const {
  return static_cast<std::size_t>(m_spectraList.size());
}

QStringList Dataset::getDatasetDomainNames() const {
  if (numberOfDomains() == 1) {
    return QStringList(m_workspaceName);
  } else {
    QStringList domainNames;
    for (const auto &specNum : m_spectraList)
      domainNames << m_workspaceName + " (" + QString::number(specNum) + ")";
    return domainNames;
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
