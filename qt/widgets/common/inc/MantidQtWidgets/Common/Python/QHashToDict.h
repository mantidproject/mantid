// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
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
