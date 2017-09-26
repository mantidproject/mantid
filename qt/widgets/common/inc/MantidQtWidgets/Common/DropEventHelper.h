#ifndef MANTIDQT_MANTIDWIDGETS_DRAGEVENTHELPER_H
#define MANTIDQT_MANTIDWIDGETS_DRAGEVENTHELPER_H

#include "MantidQtWidgets/Common/DllOption.h"

#include <QDropEvent>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON DropEventHelper {
public:
  /// Get all filenames from a QDropEvent
  static QStringList getFileNames(const QDropEvent *event);
  /// Get all python files from q QDropEvent
  static QStringList extractPythonFiles(const QDropEvent *event);
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif
