#include "MantidKernel/EqualBinsChecker.h"
#include "MantidKernel/Logger.h"
#include <ostream>
#include <cmath>

namespace {
// Initialize the logger
Mantid::Kernel::Logger g_log("EqualBinsChecker");
}

namespace Mantid {
namespace Kernel {

/**
 * Constructor
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
      m_warningLevel(warningLevel) {}

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

  // average bin width
  const double dx =
      (xData[xSize - 1] - xData[0]) / static_cast<double>(xSize - 1);

  // Use cumulative errors
  auto difference = [&xData, &dx](size_t i) {
    return std::fabs((xData[i] - xData[0] - (double)i * dx) / dx);
  };

  // Check each width against dx
  bool printWarning = false;
  for (size_t i = 1; i < xSize - 2; i++) {
    const double diff = difference(i);
    if (diff > m_errorLevel) {
      // return an actual error
      g_log.error() << "dx=" << xData[i + 1] - xData[i] << ' ' << dx << ' ' << i
                    << std::endl;
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

} // namespace Kernel
} // namespace Mantid
