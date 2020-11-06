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

/*
 * Represents a workspace containing a number of spectra to be fitted.
 *
 * Holds a workspace name, and a Spectra object which can represent
 * a continuous or discontinuous spectra range.
 */
struct EXPORT_OPT_MANTIDQT_COMMON FunctionModelDataset {
public:
  FunctionModelDataset(const QString &workspaceName, const Spectra &spectra);
  FunctionModelDataset(QString workspaceName, Spectra &&spectra);

  inline QString datasetName() const noexcept;
  QStringList domainNames() const;

  inline std::size_t numberOfSpectra() const noexcept;

private:
  QString m_workspaceName;
  Spectra m_spectra;
};

} // namespace MantidWidgets
} // namespace MantidQt
