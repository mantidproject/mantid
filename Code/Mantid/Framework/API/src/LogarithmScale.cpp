//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>
#include <cmath>
#include <stdexcept>

#include "MantidAPI/LogarithmScale.h"
#include "MantidAPI/TransformScaleFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {
namespace {
/// static logger
Kernel::Logger g_log("LogarithmScale");
}

DECLARE_TRANSFORMSCALE(LogarithmScale);

void LogarithmScale::setBase(double &base) {
  if (base <= 0) {
    g_log.error("Error: logarithm base must be a positive number");
  }
  m_base = base;
}

/* Transform the grid to adopt a logarithm scale.
 * @param gd a grid object
 */
void LogarithmScale::transform(std::vector<double> &gd) {
  double a = 1.0 / log(m_base);
  size_t n = gd.size();
  if (n == 0)
    return; // no need to process
  if (gd.front() <= 0) {
    g_log.error("LogarithmScale::transform Error: negative values");
    return;
  }
  if (n < 3)
    return; // no need to process
  double startX = a * log(gd.front());
  double endX = a * log(gd.back());
  double spacing = (endX - startX) / double(n);

  double x = startX + spacing;
  for (auto it = gd.begin() + 1; it != gd.end() - 1; it++) {
    *it = pow(m_base, x);
    x += spacing;
  }
}

} // namespace API
} // namespace Mantid
