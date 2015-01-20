//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>

#include "MantidAPI/LinearScale.h"
#include "MantidAPI/TransformScaleFactory.h"

namespace Mantid {
namespace API {

DECLARE_TRANSFORMSCALE(LinearScale);

/* Transform the grid to adopt a linear scale
 * @param gd a grid object
 */
void LinearScale::transform(std::vector<double> &gd) {
  size_t n = gd.size();
  if (n < 3)
    return; // no need to process
  double startX = gd.front();
  double endX = gd.back();
  double spacing = (endX - startX) / double(n);
  double x = startX + spacing;
  for (auto it = gd.begin() + 1; it != gd.end() - 1; it++) {
    *it = x;
    x += spacing;
  }
}

} // namespace API
} // namespace Mantid
