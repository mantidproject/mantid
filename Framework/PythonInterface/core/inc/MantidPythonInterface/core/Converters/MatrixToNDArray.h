// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Matrix.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include <boost/python/detail/prefix.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
//-----------------------------------------------------------------------
// Converter implementation
//-----------------------------------------------------------------------
/**
 * Converter that takes a Mantid Matrix and converts it into a
 * numpy array.
 *
 * The type of conversion is specified by another struct/class that
 * contains a static member create.
 */
template <typename ElementType, typename ConversionPolicy>
struct DLLExport MatrixToNDArray{/**
                                  * Operator to convert a matrix to a numpy array
                                  * @param cmatrix :: A reference to matrix
                                  * @returns A new PyObject* that points to a numpy array
                                  */
                                 inline PyObject * operator()(const Kernel::Matrix<ElementType> &cmatrix) const {
                                                       const std::pair<size_t, size_t> matrixDims = cmatrix.size();
Py_intptr_t dims[2] = {static_cast<Py_intptr_t>(matrixDims.first), static_cast<Py_intptr_t>(matrixDims.second)};
using policy = typename ConversionPolicy::template apply<ElementType>;
return policy::createFromArray(&(cmatrix[0][0]), 2, dims);
} // namespace Converters
}; // namespace PythonInterface
} // namespace Mantid
} // namespace PythonInterface
} // namespace Mantid
