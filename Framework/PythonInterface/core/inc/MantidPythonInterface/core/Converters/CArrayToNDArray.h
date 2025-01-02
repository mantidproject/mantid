// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/Converters/WrapWithNumpy.h"

namespace Mantid {
namespace PythonInterface {
namespace Converters {
//-----------------------------------------------------------------------
// Converter implementation
//-----------------------------------------------------------------------
/**
 * Converter that takes a c array and its size then converts/wraps it into a
 *numpy array.
 *
 * The type of conversion is specified by another struct/class that
 * contains a static member create.
 */
template <typename ElementType, typename ConversionPolicy> struct CArrayToNDArray {
  inline PyObject *operator()(const ElementType *carray, const int ndims, Py_intptr_t *dims) const {
    // Round about way of calling the wrapNDArray template function that is
    // defined
    // in the cpp file
    using policy = typename ConversionPolicy::template apply<ElementType>;
    return policy::createFromArray(carray, ndims, dims);
  }
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
