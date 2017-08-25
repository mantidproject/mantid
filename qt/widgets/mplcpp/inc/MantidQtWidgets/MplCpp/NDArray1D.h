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
#include "MantidQtWidgets/MplCpp/Numpy.h"

#include <array>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Encapsulates a 1D numpy.ndarray Python object.
 *
 * Input is an iterable container. The data is copied from the input
 * to the newly created array.
 */
class EXPORT_OPT_MANTIDQT_MPLCPP NDArray1D : public PythonObject {
public:
  // inherit constructors
  using PythonObject::PythonObject;

  /**
   * Create an array from an Iterable. Iterable must support std::begin/end
   */
  template <typename Iterable>
  NDArray1D(const Iterable &data)
      : PythonObject(copyToNDArray(data)) {}

  // Return the shape of the array in numpy parlance
  std::array<size_t, 1> shape() const;
};
}
}
}

#endif // NDARRAY1D_H
