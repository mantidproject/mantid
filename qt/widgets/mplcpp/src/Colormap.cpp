// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"

#include "MantidPythonInterface/core/ErrorHandling.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;
using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {

/**
 * @brief Construct a Colormap object given a name
 */
Colormap::Colormap(Python::Object obj) : Python::InstanceHolder(std::move(obj), "is_gray") {}

/**
 * @return A reference to the matplotlib.cm module
 */
Python::Object cmModule() {
  Python::Object cmModule{Python::NewRef(PyImport_ImportModule("matplotlib.cm"))};
  return cmModule;
}

/**
 * @return A reference to the matplotlib.colormaps dict
 */
Python::Dict colormaps() {
  Python::Object mpl{Python::NewRef(PyImport_ImportModule("matplotlib"))};
  return Python::Dict(mpl.attr("colormaps"));
}

/**
 * @param name The name of a possible colormap
 * @return True if the map is known, false otherwise
 */
bool cmapExists(const QString &name) {
  try {
    getCMap(name);
    return true;
  } catch (std::invalid_argument &) {
    return false;
  } catch (PythonException &) {
    return false;
  }
}

/**
 * @param name The name of an existing colormap.
 * @return A new Colormap instance for the named map
 * @throws std::invalid_argument if the name is unknown
 */
Colormap getCMap(const QString &name) {
  try {
    GlobalInterpreterLock lock;
    const auto cmap = colormaps().get(name.toLatin1().constData());
    if (!cmap.is_none()) {
      return cmap;
    } else {
      throw std::invalid_argument(name.toStdString() + " is not a valid color map name.");
    }
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

/**
 * Return the name of the default color map. We prefer viridis if it is
 * available otherwise we fallback to jet.
 * @return The string name of the default colormap we want to
 * use in the library
 */
QString defaultCMapName() {
  if (cmapExists("viridis")) {
    return "viridis";
  } else {
    return "jet";
  }
}

} // namespace MantidQt::Widgets::MplCpp
