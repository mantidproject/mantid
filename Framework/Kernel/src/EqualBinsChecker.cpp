#include "MantidKernel/EqualBinsChecker.h"
#include "MantidKernel/Logger.h"
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace {
// Initialize the logger
Mantid::Kernel::Logger g_log("EqualBinsChecker");
} // namespace

namespace Mantid {
namespace Kernel {

/**
 * Constructor, setting data and thresholds for errors and warnings.
 * By default, checks against average bin width and uses cumulative errors.
 * @param xData :: [input] Reference to bins to check
 * @param errorLevel :: [input] Threshold for error. If bin differences are
 * larger than this, check fails.
 * @param warningLevel :: [input] Threshold for warning. If bin difference are
 * larger than this, the user is warned but the check doesn't necessarily fail.
 * If set negative, warnings are off and only error level is used (default).
 */
EqualBinsChecker::EqualBinsChecker(const MantidVec &xData,
                                   const double errorLevel,
                                   const double warningLevel)
    : m_xData(xData), m_errorLevel(errorLevel), m_warn(warningLevel > 0),
      m_warningLevel(warningLevel), m_refBinType(ReferenceBin::Average),
      m_errorType(ErrorType::Cumulative) {}

/**
 * Set whether to compare each bin to the first bin width or the average.
 * Used for FFT without "AcceptXRoundingErrors" - compatibility with previous
 * behaviour.
 * @param refBinType :: [input] Either average bin or first bin
 */
void EqualBinsChecker::setReferenceBin(const ReferenceBin &refBinType) {
  m_refBinType = refBinType;
}

/**
 * Set whether to use cumulative errors or compare each in turn.
 * Used for FFT without "AcceptXRoundingErrors" - compatibility with previous
 * behaviour.
 * @param errorType :: [input] Either cumulative or individual errors
 */
void EqualBinsChecker::setErrorType(const ErrorType &errorType) {
  m_errorType = errorType;
}

/**
 * Perform validation of the given X array
 * @returns :: Error string (empty if no error)
 */
std::string EqualBinsChecker::validate() const {
  const auto &xData = m_xData;
  const auto xSize = xData.size();
  // First check for empty input
  if (xSize == 0) {
    return "Input workspace must not be empty";
  }

  // reference bin width to compare to
  const double dx = getReferenceDx();

  // Check each width against dx
  bool printWarning = false;
  for (size_t bin = 0; bin < xSize - 2; bin++) {
    const double diff = getDifference(bin, dx);
    if (diff > m_errorLevel) {
      // return an actual error
      g_log.error() << "dx=" << xData[bin + 1] - xData[bin] << ' ' << dx << ' '
                    << bin << std::endl;
      return "X axis must be linear (all bins must have the same width)";
    } else if (m_warn && diff > m_warningLevel) {
      // just warn the user
      printWarning = true;
    }
  }

  if (printWarning) {
    g_log.warning() << "Bin widths differ by more than " << m_warningLevel * 100
                    << "% of average." << std::endl;
  }

  return std::string();
}

/**
 * Returns the bin width to compare against:
 * either the average or the first depending on options
 * @returns :: bin width to compare against
 * @throws std::runtime_error if input X data was empty
 */
double EqualBinsChecker::getReferenceDx() const {
  if (m_xData.size() < 2) {
    throw std::runtime_error("No bins in input X data");
  }

  if (m_refBinType == ReferenceBin::Average) {
    // average bin width
    const auto xSize = m_xData.size();
    return (m_xData[xSize - 1] - m_xData[0]) / static_cast<double>(xSize - 1);
  } else {
    // first bin width
    return m_xData[1] - m_xData[0];
  }
}

/**
 * Returns the error (simple or cumulative) at the given point
 * @param bin :: [input] which bin to get the error for
 * @param dx :: [input] bin width to compare to
 * @returns :: error, simple or cumulative depending on options
 * @throws std::invalid_argument if there is no such bin in the data
 */
double EqualBinsChecker::getDifference(const size_t bin,
                                       const double dx) const {
  if (bin > m_xData.size() - 2) {
    throw std::invalid_argument("Not enough bins in input X data");
  }

  if (m_errorType == ErrorType::Individual) {
    return std::fabs(dx - m_xData[bin + 1] + m_xData[bin]) / dx;
  } else {
    // cumulative errors
    return std::fabs(
        (m_xData[bin + 1] - m_xData[0] - static_cast<double>(bin + 1) * dx) /
        dx);
  }
}

} // namespace Kernel
} // namespace Mantid
