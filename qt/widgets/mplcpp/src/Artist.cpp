#include "MantidQtWidgets/MplCpp/Artist.h"
#include <cassert>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Create an Artist instance around an existing matplotlib Artist
 * @param obj A Python object pointing to a matplotlib artist
 */
Artist::Artist(Python::Object obj) : InstanceHolder(std::move(obj), "draw") {}

/**
 * Call .remove on the underlying artist
 */
void Artist::remove() { pyobj().attr("remove")(); }

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
