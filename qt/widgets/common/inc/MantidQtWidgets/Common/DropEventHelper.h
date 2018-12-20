#ifndef MANTIDQT_MANTIDWIDGETS_DROPEVENTHELPER_H
#define MANTIDQT_MANTIDWIDGETS_DROPEVENTHELPER_H

#include "MantidQtWidgets/Common/DllOption.h"

#include <QDropEvent>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

namespace DropEventHelper {
/// Get all filenames from a QDropEvent
EXPORT_OPT_MANTIDQT_COMMON QStringList getFileNames(const QDropEvent *event);
/// Get all python files from q QDropEvent
EXPORT_OPT_MANTIDQT_COMMON QStringList
extractPythonFiles(const QDropEvent *event);
} // namespace DropEventHelper

} // namespace MantidWidgets
} // namespace MantidQt

#endif
