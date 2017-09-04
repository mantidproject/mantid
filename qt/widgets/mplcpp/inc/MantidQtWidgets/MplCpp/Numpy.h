#ifndef NUMPY_H
#define NUMPY_H
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
#include "MantidQtWidgets/MplCpp/NDArray1D.h"

//
// A collection of utilities for transfer of data to/from numpy arrays.
//

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Create a 1D numpy.ndarray object from the given iterable. It will work with
 * anything support std::begin()/std::end().
 * @param data A const reference to the data container
 * @return A 1D numpy.ndarray object containing the data
 */
template <typename Iterable> NDArray1D copyToNDArray(const Iterable &data);
}
}
}

#endif // NUMPY_H
