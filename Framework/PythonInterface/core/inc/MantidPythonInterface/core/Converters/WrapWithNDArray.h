// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <boost/python/detail/prefix.hpp>
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/// Enum defining wrapping type for conversion to numpy
enum NumpyWrapMode { ReadOnly, ReadWrite };
/// Enum defining transfer of ownership when converting to numpy array
enum OwnershipMode { Cpp, Python };

namespace Impl {
// Forward declare a conversion function. This should be specialized for each
// container type that is to be wrapped
template <typename ElementType>
PyObject *wrapWithNDArray(const ElementType *, const int ndims, Py_intptr_t *dims, const NumpyWrapMode mode,
                          const OwnershipMode oMode = OwnershipMode::Cpp);
} // namespace Impl

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
    static PyObject *createFromArray(const ElementType *cdata, const int ndims, Py_intptr_t *dims) {
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
    static PyObject *createFromArray(const ElementType *cdata, const int ndims, Py_intptr_t *dims) {
      return Impl::wrapWithNDArray(cdata, ndims, dims, ReadWrite);
    }
  };
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
