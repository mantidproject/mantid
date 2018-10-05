// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_TOPYLIST_H_
#define MANTID_PYTHONINTERFACE_TOPYLIST_H_

#include <boost/python/list.hpp>
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
//-----------------------------------------------------------------------
// Converter implementation
//-----------------------------------------------------------------------
/**
 * Converter that takes a std::vector and converts it into a python list.
 * It is able to convert anything for which a converter is already registered
 */
template <typename ElementType> struct ToPyList {
  /**
   * Converts a cvector to a numpy array
   * @param cdata :: A const reference to a vector
   * @returns A new python list object
   */
  inline boost::python::list
  operator()(const std::vector<ElementType> &cdata) const {
    boost::python::list result;
    for (const auto &item : cdata) {
      result.append(item);
    }
    return result;
  }
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_TOPYLIST_H_ */
