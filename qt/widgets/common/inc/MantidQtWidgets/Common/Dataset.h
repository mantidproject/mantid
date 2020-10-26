// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <cstring>

#include <QList>
#include <QString>
#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

struct Dataset {
  Dataset();
  Dataset(QString const &workspaceName, QList<std::size_t> const &spectraList);

  std::size_t numberOfDomains() const;

  QStringList getDatasetDomainNames() const;

  QString m_workspaceName;
  QList<std::size_t> m_spectraList;
};

} // namespace MantidWidgets
} // namespace MantidQt
