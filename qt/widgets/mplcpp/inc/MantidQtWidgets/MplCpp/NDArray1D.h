#ifndef NDARRAY1D_H
#define NDARRAY1D_H
/*
 Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
#include "MantidQtWidgets/MplCpp/PythonObject.h"
#include <array>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace detail {
/**
* Create a 1D numpy.ndarray object from the given iterable. It will work with
* anything support std::begin()/std::end().
* @param data A const reference to the data container
* @return A new reference to a 1D numpy.ndarray object containing the data
*/
template <typename Iterable> PyObject *copyToNDArray(const Iterable &data);
}

/**
 * Encapsulates a 1D numpy.ndarray Python object.
 *
 * Input is an iterable container. The data is copied from the input
 * to the newly created array.
 */
template <typename ElementType>
class EXPORT_OPT_MANTIDQT_MPLCPP NDArray1D : public PythonObject {
public:
  /// Create a new wrapper object from a new reference
  static NDArray1D fromNewRef(PyObject *ptr);

  /**
   * Create an array from an Iterable. Iterable must support std::begin/end
   * and contain a value_type typedef indicating the element type
   * @param data A container holding the source of data
   */
  template <typename Iterable>
  NDArray1D(const Iterable &data)
      : PythonObject(detail::copyToNDArray(data)) {
    static_assert(
        std::is_same<typename Iterable::value_type, ElementType>::value,
        "Element type in iterable must match declared ElementType");
  }

  // Return the shape of the array in numpy parlance
  std::array<size_t, 1> shape() const;
  // Return the element at the given index
  ElementType operator[](size_t i) const;

private:
  /// Private constructor used by static creation methods
  NDArray1D(PyObject *ptr) : PythonObject(ptr) {}
};
}
}
}

#endif // NDARRAY1D_H
