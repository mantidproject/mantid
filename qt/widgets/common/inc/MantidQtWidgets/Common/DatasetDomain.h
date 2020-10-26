// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <cstring>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {

struct DatasetDomain {
  DatasetDomain();
  DatasetDomain(QString const &workspaceName, std::size_t const &spectrumNumber,
                bool isOnlySpectrum = true);

  QString domainName() const;

  QString m_workspaceName;
  std::size_t m_spectrumNumber;
  bool m_isOnlySpectrum;
};

} // namespace MantidWidgets
} // namespace MantidQt
