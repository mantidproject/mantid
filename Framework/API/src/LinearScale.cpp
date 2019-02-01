// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/LinearScale.h"
#include "MantidAPI/TransformScaleFactory.h"

namespace Mantid {
namespace API {

DECLARE_TRANSFORMSCALE(LinearScale)

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
