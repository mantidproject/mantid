// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"

#include <QDropEvent>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

namespace DropEventHelper {
/// Get all filenames from a QDropEvent
EXPORT_OPT_MANTIDQT_COMMON QStringList getFileNames(const QDropEvent *event);
/// Get all python files from q QDropEvent
EXPORT_OPT_MANTIDQT_COMMON QStringList extractPythonFiles(const QDropEvent *event);
} // namespace DropEventHelper

} // namespace MantidWidgets
} // namespace MantidQt
