#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"

#include "MantidPythonInterface/core/ErrorHandling.h"

using Mantid::PythonInterface::PythonRuntimeError;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Construct a Colormap object given a name
 */
Colormap::Colormap(Python::Object obj)
    : Python::InstanceHolder(obj, "is_gray") {}

/**
 * @return A reference to the matplotlib.cm module
 */
Python::Object cmModule() {
  Python::Object cmModule{
      Python::NewRef(PyImport_ImportModule("matplotlib.cm"))};
  return cmModule;
}

/**
 * @param name The name of an existing colormap.
 * @return A new Colormap instance for the named map
 * @throws std::invalid_argument if the name is unknown
 */
Colormap getCMap(const QString &name) {
  try {
    return cmModule().attr("get_cmap")(name.toLatin1().constData());
  } catch (Python::ErrorAlreadySet &) {
    throw PythonRuntimeError();
  }
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
