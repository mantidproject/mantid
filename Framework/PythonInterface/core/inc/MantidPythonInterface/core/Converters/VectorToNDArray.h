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
//-----------------------------------------------------------------------
// Converter implementation
//-----------------------------------------------------------------------
/**
 * Converter that takes a std::vector and converts it into a flat numpy array.
 *
 * The type of conversion is specified by another struct/class that
 * contains a static member create1D.
 */
template <typename ElementType, typename ConversionPolicy> struct VectorToNDArray {
  /**
   * Converts a cvector to a numpy array
   * @param cdata :: A const reference to a vector
   * @returns A new PyObject that wraps the vector in a numpy array
   */
  inline PyObject *operator()(const std::vector<ElementType> &cdata) const {
    // Hand off the work to the conversion policy
    using policy = typename ConversionPolicy::template apply<ElementType>;
    return policy::create1D(cdata);
  }
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
