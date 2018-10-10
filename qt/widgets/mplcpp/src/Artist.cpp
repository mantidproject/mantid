// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
 * Set properties on the Artist given by the dict of kwargs
 * @param kwargs A dict of known matplotlib.artist.Artist properties
 */
void Artist::set(Python::Dict kwargs) {
  auto args = Python::NewRef(Py_BuildValue("()"));
  pyobj().attr("set")(*args, **kwargs);
}

/**
 * Call .remove on the underlying artist
 */
void Artist::remove() { pyobj().attr("remove")(); }

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
