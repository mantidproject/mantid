#ifndef MANTID_PYTHONINTERFACE_TRACKINGINSTANCEMETHOD_H_
#define MANTID_PYTHONINTERFACE_TRACKINGINSTANCEMETHOD_H_

/*
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>

namespace Mantid {
namespace PythonInterface {
/**
 * Add an Instance() & __del__ method to the already exported python type.
 * The methods track how many times instance() & __del__ are called and clear
 * the
 * SingletonType object when the count reaches zero.
 * @tparam SingletonType The main SingletonHolder type. It is expected to have a
 * nested type
 *                       HeldType that defines the implementation
 * @tparam PythonType The boost.python C++ exported type
 */
template <typename SingletonType, typename PythonType>
class TrackingInstanceMethod {
  /// Type returned by instance()
  typedef typename SingletonType::HeldType InstanceType;

public:
  /**
   * Add the instance tracking methods
   * @param classType The python type already exported using class_
   */
  static void define(PythonType &classType) {
    using namespace boost::python;

    classType.def("__del__", &TrackingInstanceMethod::decref);
    classType.def("Instance", &TrackingInstanceMethod::instance,
                  return_value_policy<reference_existing_object>(),
                  "Return a reference to the singleton instance");
    classType.staticmethod("Instance");
  }

  /**
   * Increment reference count & return the singleton instance
   * @return A reference to the InstanceType
   */
  static InstanceType &instance() {
    ++g_py_instance_count;
    return SingletonType::Instance();
  }

  /**
   * @param self The calling object
   */
  static void decref(InstanceType &self) {
    --g_py_instance_count;
    if (g_py_instance_count == 0) {
      self.clear();
    }
  }

private:
  /// Track the number of calls to instance/decref
  static size_t g_py_instance_count;
};

/// Initialize static counter
template <typename T, typename S>
size_t TrackingInstanceMethod<T, S>::g_py_instance_count = 0;
}
}

#endif /* MANTID_PYTHONINTERFACE_TRACKINGINSTANCEMETHOD */
