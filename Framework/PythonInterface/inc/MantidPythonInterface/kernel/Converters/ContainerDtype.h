// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_
#define MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_

#include <string>
#include <type_traits>

/**
    ContainerDtype Header File

    A helper free function to allow identification of data type being used by
    providing a numpy friendly string (using the numpy array interface).

    @author Lamar Moore STFC, Bhuvan Bezawada STFC
    @date 21/06/2018
*/

namespace Mantid {
namespace PythonInterface {
namespace Converters {

// Free function to determine data type being stored in container
template <template <class> class Container, typename HeldType>
std::string dtype(const Container<HeldType> &) {
  if (std::is_same<HeldType, bool>::value) {
    return "b";
  } else if (std::is_integral<HeldType>::value) {
    return "i";
  } else if (std::is_floating_point<HeldType>::value) {
    return "f";
  } else {
    return "O";
  }
}

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /*MANTID_PYTHONINTERFACE_CONVERTERS_CONTAINERDTYPE_H_*/
