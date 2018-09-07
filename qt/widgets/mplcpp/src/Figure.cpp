#include "MantidQtWidgets/MplCpp/Figure.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
/**
 * @return The module containing the Figure object
 */
Python::Object figureModule() {
  static Python::Object figureModule(
      Python::Handle<>(PyImport_ImportModule("matplotlib.figure")));
  return figureModule;
}
} // namespace

/**
 * Create a new matplotlib figure instance. The figure is empty.
 */
Figure::Figure() : InstanceHolder(figureModule().attr("Figure")()) {}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
