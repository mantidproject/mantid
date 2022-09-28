// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Wraps a matplotlib.artist object with a C++ interface
 */
class MANTID_MPLCPP_DLL Artist : public Common::Python::InstanceHolder {
public:
  // Holds a reference to the matplotlib artist object
  explicit Artist(Common::Python::Object obj);

  /**
   * Set a property on the Artist given by name and value
   * @param name The name of the property
   * @param value The value of the property
   */
  template <typename ValueType> void set(const std::string &name, ValueType value) {
    Mantid::PythonInterface::callMethodNoCheck<void>(pyobj().ptr(), ("set_" + name).c_str(), std::move(value));
  }
  void set(const Common::Python::Dict &kwargs);

  void remove();
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
