//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
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
  for (const auto &m_grid : m_grids)
    n *= m_grid->size();
  return n;
}

/// number of dimensions of the grid
size_t GridDomain::nDimensions() {
  size_t n = 0;
  for (auto &m_grid : m_grids)
    n += m_grid->nDimensions();
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
  for (auto &m_grid : m_grids)
    m_grid->reScale(scaling);
}

} // namespace API
} // namespace Mantid
