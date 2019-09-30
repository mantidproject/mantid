// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTWIDGETS_QHASHTODICT_H
#define MANTIDQTWIDGETS_QHASHTODICT_H
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Python/Object.h"

#include <QHash>
#include <QVariant>

namespace MantidQt {
namespace Widgets {
namespace Common {
namespace Python {

using KwArgs = QHash<QString, QVariant>;

EXPORT_OPT_MANTIDQT_COMMON Python::Dict qHashToDict(const KwArgs &hash);

} // namespace Python
} // namespace Common
} // namespace Widgets
} // namespace MantidQt

#endif /* MANTIDQTWIDGETS_QHASHTODICT_H */
