// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/VectorHelper.h"
#include <cmath>

namespace Mantid::Kernel {
RebinParamsValidator::RebinParamsValidator(bool allowEmpty, bool allowRange)
    : m_allowEmpty(allowEmpty), m_allowRange(allowRange) {}

IValidator_sptr RebinParamsValidator::clone() const { return std::make_shared<RebinParamsValidator>(*this); }

/** Check on the inputed bin boundaries and widths.
 *  @param value :: The parameter array to check
 *  @return A user level description of any problem or "" if there is no problem
 */
std::string RebinParamsValidator::checkValidity(const std::vector<double> &value) const {
  // Rebin Parameters allow three forms of input:
  // 1) a single value, the bin width, in which case the bin boundaries will be calculated from the input workspace
  // 2) a pair of values (a range), the lower and upper limits of the binning
  // 3) a list of values in the form x1,dx1,x2,dx2,...,xn where the x's are bin boundaries and the dx's are bin widths

  std::string error = "";
  size_t numParams = value.size();
  switch (numParams) {
  case 0: {
    if (!m_allowEmpty) { // unless allowed in the constructor
      error = "Enter values for this property";
    }
  } break;
  case 1: {
    if (value[0] == 0.0) {
      error = "Cannot have a zero bin width";
    }
  } break;
  case 2: {
    if (m_allowRange) { // if we allow ranges, must be in order
      if (value[0] >= value[1]) {
        error = "When giving a range the second value must be larger than the first";
      }
    } else { // if we don't allow ranges, then this is wrong
      error = "The number of bin boundary parameters provided must be odd";
    }
  } break;
  // three or more parameters must be in the form x1,dx1,x2,dx2,...,xn
  default: {
    if (value.size() % 2 == 0) { // even
      error = "The number of bin boundary parameters provided must be odd";
    } else {                   // odd
      std::size_t numBins = 0; // defensively init to 0
      try {
        numBins = VectorHelper::estimateNumberOfBins(value);
      } catch (std::runtime_error &e) {
        error = e.what();
        break;
      }
      std::size_t binSpaceInBytes = numBins * sizeof(double);
      error = MemoryStats().checkAvailableMemory(binSpaceInBytes);
    } // end else odd
  } // end default case
  }

  // return string, which contains any error messages
  return error;
}

} // namespace Mantid::Kernel
