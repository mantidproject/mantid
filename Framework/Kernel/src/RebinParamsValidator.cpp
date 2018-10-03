// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/RebinParamsValidator.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Kernel {
RebinParamsValidator::RebinParamsValidator(bool allowEmpty, bool allowRange)
    : m_allowEmpty(allowEmpty), m_allowRange(allowRange) {}

IValidator_sptr RebinParamsValidator::clone() const {
  return boost::make_shared<RebinParamsValidator>(*this);
}

/** Check on the inputed bin boundaries and widths.
 *  @param value :: The parameter array to check
 *  @return A user level description of any problem or "" if there is no problem
 */
std::string
RebinParamsValidator::checkValidity(const std::vector<double> &value) const {
  // array must not be empty
  if (value.empty()) {
    if (m_allowEmpty) // unless allowed in the constructor
      return "";
    else
      return "Enter values for this property";
  }

  if (m_allowRange && value.size() == 2) {
    if (value[0] < value[1])
      return "";
    else
      return "When giving a range the second value must be larger than the "
             "first";
  }

  // it must have an odd number of values (and be at least 3 elements long)
  if (value.size() % 2 == 0) {
    return "The number of bin boundary parameters provided must be odd";
  }

  // bin widths must not be zero
  for (size_t i = 1; i < value.size(); i += 2) {
    if (value[i] == 0.0) {
      return "Cannot have a zero bin width";
    }
  }

  // bin boundary values must be in increasing order
  double previous = value[0];
  for (size_t i = 2; i < value.size(); i += 2) {
    if ((value[i - 1] < 0) && (previous <= 0)) {
      return "Bin boundaries must be positive for logarithmic binning";
    }
    if (value[i] <= previous) {
      return "Bin boundary values must be given in order of increasing value";
    } else
      previous = value[i];
  }

  // All's OK if we get to here
  return "";
}

} // namespace Kernel
} // namespace Mantid
