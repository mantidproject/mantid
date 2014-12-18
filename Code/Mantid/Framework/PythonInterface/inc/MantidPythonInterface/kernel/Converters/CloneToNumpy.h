#ifndef MANTID_PYTHONINTERFACE_CLONETONUMPY_H_
#define MANTID_PYTHONINTERFACE_CLONETONUMPY_H_
/**
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidKernel/System.h"
#include <boost/python/detail/prefix.hpp>
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
namespace Impl {
// Forward declaration of implementations. Keeps numpy header out of this header
template <typename ElementType>
PyObject *clone1D(const std::vector<ElementType> &cvector);
template <typename ElementType>
PyObject *cloneND(const ElementType *carray, const int ndims,
                  Py_intptr_t *dims);
}

/**
 * Clone is a policy (in the C++ sense)for converting to an ND Array. The result
 * is a numpy array with a copy of the input data.
 */
struct Clone {
  template <typename ElementType> struct apply {
    /**
     * Returns a Numpy array that has a copy of the vectors data
     * @param cvector An object that knows its length and is contiguous in
     * memory
     * @return
     */
    static PyObject *create1D(const std::vector<ElementType> &cvector) {
      return Impl::clone1D<ElementType>(cvector);
    }
    /**
     * Returns a Numpy array that has a copy of the array data
     * @param carray :: The input data array
     * @param ndims :: The number of dimensions the data represents
     * @param dims :: The extents in each of the dimensions
     * @return
     */
    static PyObject *createFromArray(const ElementType *carray, const int ndims,
                                     Py_intptr_t *dims) {
      return Impl::cloneND<ElementType>(carray, ndims, dims);
    }
  };
};
}
}
}

#endif //
