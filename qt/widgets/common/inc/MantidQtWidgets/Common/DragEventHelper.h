#ifndef MANTIDQT_MANTIDWIDGETS_DRAGEVENTHELPER_H
#define MANTIDQT_MANTIDWIDGETS_DRAGEVENTHELPER_H

#include "MantidQtWidgets/Common/DllOption.h"

#include <QDropEvent>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON DragEventHelper {
public:
    static QStringList getFileNames(const QDropEvent* event);
};

} // MantidWidgets
} // MantidQt

#endif
