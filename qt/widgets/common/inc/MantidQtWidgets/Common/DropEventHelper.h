#ifndef MANTIDQT_MANTIDWIDGETS_DROPEVENTHELPER_H
#define MANTIDQT_MANTIDWIDGETS_DROPEVENTHELPER_H

#include "MantidQtWidgets/Common/DllOption.h"

#include <QDropEvent>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

namespace DropEventHelper {
/// Get all filenames from a QDropEvent
QStringList getFileNames(const QDropEvent *event);
/// Get all python files from q QDropEvent
QStringList extractPythonFiles(const QDropEvent *event);
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif
