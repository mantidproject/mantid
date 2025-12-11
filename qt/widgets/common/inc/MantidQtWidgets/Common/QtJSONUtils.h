// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include <QMap>
#include <QString>
#include <QVariant>
#include <string>

namespace MantidQt {
namespace API {

void EXPORT_OPT_MANTIDQT_COMMON saveJSONToFile(QString &filename, const QMap<QString, QVariant> &map);

QMap<QString, QVariant> EXPORT_OPT_MANTIDQT_COMMON loadJSONFromFile(const QString &filename);

QMap<QString, QVariant> EXPORT_OPT_MANTIDQT_COMMON loadJSONFromString(const QString &jsonString);

std::string EXPORT_OPT_MANTIDQT_COMMON outputJsonToString(const QMap<QString, QVariant> &map);

} // namespace API
} // namespace MantidQt
