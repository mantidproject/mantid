// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ArrayOrderedPairsValidator.h"

#include <cstdint>
#include <memory>
#include <sstream>

namespace Mantid::Kernel {

/**
 * Create a clone of the current ArrayOrderedPairsValidator.
 * @return The cloned object.
 */
template <typename TYPE> IValidator_sptr ArrayOrderedPairsValidator<TYPE>::clone() const {
  return std::make_shared<ArrayOrderedPairsValidator<TYPE>>(*this);
}

/**
 * Function that actually does the work of checking the validity of the
 * array elements.
 * @param value :: The array to be checked.
 * @return An error message giving the values of wrong entries.
 */
template <typename TYPE>
std::string ArrayOrderedPairsValidator<TYPE>::checkValidity(const std::vector<TYPE> &value) const {
  std::stringstream error;
  error << "";
  // Check the number of entries is even
  if (value.size() % 2 != 0) {
    error << "Array has an odd number of entries (" << std::to_string(value.size()) << ").";
  } else {
    // Check that each pair is ordered.
    for (auto it = value.begin(); it != value.end(); it += 2) {
      if (*it > *(it + 1)) {
        error << "Pair (" << *it << ", " << *(it + 1) << ") is not ordered.\n";
      }
    }
  }
  return error.str();
}

// Required explicit instantiations
template class ArrayOrderedPairsValidator<double>;
template class ArrayOrderedPairsValidator<int32_t>;
template class ArrayOrderedPairsValidator<int64_t>;
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
template class ArrayOrderedPairsValidator<long>;
#endif

} // namespace Mantid::Kernel
