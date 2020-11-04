// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/Spectra.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

struct EXPORT_OPT_MANTIDQT_COMMON Dataset {
public:
  Dataset(const QString &workspaceName, const Spectra &spectra);

  QString datasetName() const;
  QStringList domainNames() const;

  std::size_t numberOfSpectra() const;

private:
  QString m_workspaceName;
  Spectra m_spectra;
};

} // namespace MantidWidgets
} // namespace MantidQt
