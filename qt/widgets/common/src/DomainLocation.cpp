// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DomainLocation.h"

namespace MantidQt {
namespace MantidWidgets {

DomainLocation::DomainLocation()
    : m_workspaceName(), m_spectrumNumber(), m_isOnlySpectrum() {}

DomainLocation::DomainLocation(QString const &workspaceName,
                               std::size_t const &spectrumNumber,
                               bool isOnlySpectrum)
    : m_workspaceName(workspaceName), m_spectrumNumber(spectrumNumber),
      m_isOnlySpectrum(isOnlySpectrum) {}

QString DomainLocation::domainName() const {
  if (m_isOnlySpectrum)
    return m_workspaceName;
  return m_workspaceName + " (" + QString::number(m_spectrumNumber) + ")";
}

} // namespace MantidWidgets
} // namespace MantidQt
