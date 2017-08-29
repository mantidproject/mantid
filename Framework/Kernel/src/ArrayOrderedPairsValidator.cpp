#include "MantidKernel/ArrayOrderedPairsValidator.h"

#include <boost/make_shared.hpp>
#include <stdint.h>
#include <sstream>

namespace Mantid {
namespace Kernel {

/**
 * Create a clone of the current ArrayBoundedValidator.
 * @return The cloned object.
 */
template <typename TYPE>
IValidator_sptr ArrayOrderedPairsValidator<TYPE>::clone() const {
  return boost::make_shared<ArrayOrderedPairsValidator<TYPE>>(*this);
}

/**
 * Function that actually does the work of checking the validity of the
 * array elements.
 * @param value :: The array to be checked.
 * @return An error message giving the values of wrong entries.
 */
template <typename TYPE>
std::string ArrayOrderedPairsValidator<TYPE>::checkValidity(
    const std::vector<TYPE> &value) const {
  std::stringstream error;
  error << "";
  // Check the number of entries is even
  if (value.size() % 2 != 0) {
    error << "Array has an odd number of entries ("
          << std::to_string(value.size()) << ").";
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

} // Kernel
} // Mantid
