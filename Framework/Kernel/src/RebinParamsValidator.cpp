// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Memory.h"
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
    } else { // odd
      // for checking memory allocation
      size_t numBins = 0;
      // ensure all bin widths are non-zero
      double previousBoundary = value[0];
      for (size_t i = 1; i < value.size() - 1; i += 2) {
        double binWidth = value[i];
        double nextBoundary = value[i + 1];
        // first validate the bins and boundaries
        if (binWidth == 0.0) {
          error = "Cannot have a zero bin width";
          break;
        } else {
          if (nextBoundary <= previousBoundary) { // check bin boundaries are in increasing order
            error = "Bin boundary values must be given in order of increasing value";
            break;
          } else if (binWidth < 0.0 &&
                     previousBoundary <=
                         0.0) { // if bin width is negative (log binning) check boundaries are positive-definite
            error = "Bin boundaries must be positive for logarithmic binning";
            break;
          }
        }
        // the bins and bounds are okay, accumulate memory
        if (binWidth < 0.0) {
          // logarithmic binning
          numBins += static_cast<size_t>(std::log(nextBoundary / previousBoundary) / std::log(1. - binWidth));
        } else {
          // linear binning
          numBins += static_cast<size_t>((nextBoundary - previousBoundary) / binWidth);
        }
        previousBoundary = nextBoundary;
      } // end for checking bins/boundaries
      double memInBytes = static_cast<double>(MemoryStats().availMem() * 1024);
      double binSpaceInBytes = static_cast<double>(numBins * sizeof(double));
      if (binSpaceInBytes > memInBytes) {
        double bytesInGB = 1e9;
        error = "The number of bins requested is expected to exceed available memory. "
                "This binning requires approximately " +
                std::to_string(binSpaceInBytes / bytesInGB) + " GB of memory, but only " +
                std::to_string(memInBytes / bytesInGB) + " GB is available.";
      }
    } // end else odd
  } // end default case
  }

  // return string, which contains any error messages
  return error;
}

} // namespace Mantid::Kernel
