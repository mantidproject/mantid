#include "MantidQtWidgets/MplCpp/Axes.h"
#include <cassert>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Construct an Axes wrapper around an existing Axes instance
 * @param obj A matplotlib.axes.Axes instance
 */
Axes::Axes(Python::Object obj) : InstanceHolder(std::move(obj), "plot") {}

/**
 * @brief Set the X-axis label
 * @param label String for the axis label
 */
void Axes::setXLabel(const char *label) { pyobj().attr("set_xlabel")(label); }

/**
 * @brief Set the Y-axis label
 * @param label String for the axis label
 */
void Axes::setYLabel(const char *label) { pyobj().attr("set_ylabel")(label); }

/**
 * @brief Set the title
 * @param label String for the title label
 */
void Axes::setTitle(const char *label) { pyobj().attr("set_title")(label); }

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
