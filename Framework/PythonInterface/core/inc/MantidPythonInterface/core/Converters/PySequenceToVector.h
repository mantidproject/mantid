// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_PYSEQUENCETOVECTORCONVERTER_H_
#define MANTID_PYTHONINTERFACE_PYSEQUENCETOVECTORCONVERTER_H_

#include "MantidKernel/System.h"
#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace // <anonymous>
{
/**
 * Extract a C type from a Python object.
 */
template <typename CType> struct ExtractCType {
  /**
   * Calls extract on the Python object using the template type
   * @param value A pointer to the Python object
   * @return The value as a C type
   */
  inline CType operator()(PyObject *value) {
    return boost::python::extract<CType>(value);
  }
};

/**
 * Template specialization to convert a Python object to a C++ std::string
 */
template <> struct ExtractCType<std::string> {
  /**
   * Forces a Python string conversion before calling extract
   * @param value A pointer to the Python object
   * @return The value as a C type
   */
  inline std::string operator()(PyObject *value) {
    return boost::python::extract<std::string>(PyObject_Str(value));
  }
};

} // namespace

namespace Converters {
/**
 * Converts a Python sequence type to a C++ std::vector, where the element
 * type is defined by the template type
 */
template <typename DestElementType> struct DLLExport PySequenceToVector {
  // Alias definitions
  using TypedVector = std::vector<DestElementType>;

  PySequenceToVector(const boost::python::object &value) : m_obj(value) {}

  /**
   * Converts the Python object to a C++ vector
   * @return A std::vector<ElementType> containing the values
   * from the Python sequence
   */
  inline TypedVector operator()() {
    TypedVector cvector(srcSize());
    copyTo(cvector);
    return cvector;
  }

  /**
   * Fill the container with data from the array
   * @param dest A vector<DestElementType> that receives the data
   */
  inline void copyTo(TypedVector &dest) {
    throwIfSizeMismatched(dest);
    ExtractCType<DestElementType> elementConverter;
    auto length = srcSize();
    for (size_t i = 0; i < length; ++i) {
      dest[i] = elementConverter(PySequence_GetItem(ptr(), i));
    }
  }

private:
  inline PyObject *ptr() const { return m_obj.ptr(); }

  inline std::size_t srcSize() const {
    return static_cast<size_t>(PySequence_Size(ptr()));
  }

  inline void throwIfSizeMismatched(const TypedVector &dest) const {
    if (srcSize() != dest.size()) {
      throw std::invalid_argument(
          "Length mismatch between python list & C array. python=" +
          std::to_string(srcSize()) + ", C=" + std::to_string(dest.size()));
    }
  }

  /// Python object to convert
  boost::python::object m_obj;
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_PYSEQUENCETOVECTORCONVERTER_H_ */
