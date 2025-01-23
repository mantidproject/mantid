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
namespace Impl {
// Forward declaration of implementations. Keeps numpy header out of this header
template <typename ElementType> PyObject *clone1D(const std::vector<ElementType> &cvector);
template <typename ElementType> PyObject *cloneND(const ElementType *carray, const int ndims, Py_intptr_t *dims);
} // namespace Impl

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
    static PyObject *create1D(const std::vector<ElementType> &cvector) { return Impl::clone1D<ElementType>(cvector); }
    /**
     * Returns a Numpy array that has a copy of the array data
     * @param carray :: The input data array
     * @param ndims :: The number of dimensions the data represents
     * @param dims :: The extents in each of the dimensions
     * @return
     */
    static PyObject *createFromArray(const ElementType *carray, const int ndims, Py_intptr_t *dims) {
      return Impl::cloneND<ElementType>(carray, ndims, dims);
    }
  };
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
