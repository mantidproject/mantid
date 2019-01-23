// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_NDARRAYTOVECTORCONVERTER_H_
#define MANTID_PYTHONINTERFACE_NDARRAYTOVECTORCONVERTER_H_

#include "MantidKernel/System.h"
#include "MantidPythonInterface/core/NDArray.h"
#include <vector>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Converter taking an input numpy array and converting it to a std::vector.
 * Multi-dimensional arrays are flattened and copied.
 */
template <typename DestElementType> struct DLLExport NDArrayToVector {
  // Alias definitions
  using TypedVector = std::vector<DestElementType>;
  using TypedVectorIterator = typename std::vector<DestElementType>::iterator;

  /// Constructor
  NDArrayToVector(const NDArray &value);
  /// Create a new vector from the contents of the array
  TypedVector operator()();
  /// Fill the container with data from the array
  void copyTo(TypedVector &dest) const;

private:
  void throwIfSizeMismatched(const TypedVector &dest) const;

  // reference to the held array
  NDArray m_arr;
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_NDARRAYTOVECTORCONVERTER_H_ */
