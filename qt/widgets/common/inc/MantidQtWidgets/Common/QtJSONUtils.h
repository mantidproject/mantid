// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDQT_API_QJSONUTILS_H
#define MANTIDQT_API_QJSONUTILS_H

#include "DllOption.h"

#include <QMap>
#include <QString>
#include <QVariant>
#include <string>

namespace MantidQt {
namespace API {

void EXPORT_OPT_MANTIDQT_COMMON
saveJSONToFile(const QString &filename, const QMap<QString, QVariant> &map);

QMap<QString, QVariant>
    EXPORT_OPT_MANTIDQT_COMMON loadJSONFromFile(const QString &filename);

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_QJSONUTILS_H */