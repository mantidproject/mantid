//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>
#include <numeric>
#include <stdexcept>

#include "MantidAPI/GridDomain.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {
namespace {
/// static logger
Kernel::Logger g_log("GridDomain");
}

/// number of points in the grid
size_t GridDomain::size() const {
  if (!m_grids.size())
    return 0;
  size_t n = 1;
  for (auto it = m_grids.begin(); it != m_grids.end(); it++)
    n *= (*it)->size();
  return n;
}

/// number of dimensions of the grid
size_t GridDomain::nDimensions() {
  size_t n = 0;
  for (auto it = m_grids.begin(); it != m_grids.end(); it++)
    n += (*it)->nDimensions();
  return n;
}

/* return item of member m_grids
 * @param index the particular item of m_grids to return
 */
GridDomain_sptr GridDomain::getGrid(size_t index) {
  GridDomain_sptr g;
  try {
    g = m_grids.at(index);
  } catch (std::out_of_range &ex) {
    g_log.error(ex.what());
  }
  return g;
}

void GridDomain::reScale(const std::string &scaling) {
  for (auto it = m_grids.begin(); it != m_grids.end(); it++)
    (*it)->reScale(scaling);
}

} // namespace API
} // namespace Mantid
