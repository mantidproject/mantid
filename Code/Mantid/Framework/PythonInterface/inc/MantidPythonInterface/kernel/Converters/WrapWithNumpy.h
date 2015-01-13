#ifndef MANTID_PYTHONINTERFACE_NUMPYWRAPMODE_H_
#define MANTID_PYTHONINTERFACE_NUMPYWRAPMODE_H_
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
/// Enum defining wrapping type for conversion to numpy
enum NumpyWrapMode { ReadOnly, ReadWrite };

namespace Impl {
// Forward declare a conversion function. This should be specialized for each
// container type that is to be wrapped
template <typename ElementType>
PyObject *wrapWithNDArray(const ElementType *, const int ndims,
                          Py_intptr_t *dims, const NumpyWrapMode);
}

/**
 * WrapReadOnly is a policy for VectorToNDArray
 * to wrap the vector in a read-only numpy array
 * that looks at the original data. No copy is performed
 */
struct WrapReadOnly {

  template <typename ElementType> struct apply {
    /**
     * Returns a read-only 1D Numpy array wrapped around an existing container
     * that knows its size
     * @param cdata :: A const reference to an object that can be wrapped
     * @return
     */
    static PyObject *create1D(const std::vector<ElementType> &cdata) {
      Py_intptr_t dims[1] = {static_cast<int>(cdata.size())};
      return createFromArray(cdata.data(), 1, dims);
    }
    /**
     * Returns a read-only Numpy array wrapped around an existing array. The
     * template
     * type here refers to the C-array's element type
     * @param cdata :: A pointer to the HEAD of a data array
     * @param ndims :: The number of dimensions
     * @param dims :: An array of size ndims specifying the sizes of each of the
     * dimensions
     * @return
     */
    static PyObject *createFromArray(const ElementType *cdata, const int ndims,
                                     Py_intptr_t *dims) {
      return Impl::wrapWithNDArray(cdata, ndims, dims, ReadOnly);
    }
  };
};

/**
 * WrapReadWrite is a policy for VectorToNDArray
 * to wrap the vector in a read-write numpy array
 * that looks at the original data. No copy is performed
 */
struct WrapReadWrite {

  template <typename ElementType> struct apply {

    /**
     * Returns a read-write Numpy array wrapped around an existing vector
     * @param cdata :: A reference to vector
     * @return
     */
    static PyObject *create1D(const std::vector<ElementType> &cdata) {
      Py_intptr_t dims[1] = {static_cast<int>(cdata.size())};
      return createFromArray(cdata.data(), 1, dims);
    }
    /**
     * Returns a read-only Numpy array wrapped around an existing array. The
     * template
     * type here refers to the C-array's element type
     * @param cdata :: A pointer to the HEAD of a data array
     * @param ndims :: The number of dimensions
     * @param dims :: An array of size ndims specifying the sizes of each of the
     * dimensions
     * @return
     */
    static PyObject *createFromArray(const ElementType *cdata, const int ndims,
                                     Py_intptr_t *dims) {
      return Impl::wrapWithNDArray(cdata, ndims, dims, ReadWrite);
    }
  };
};
}
}
}

#endif
