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
} // namespace

/// number of points in the grid
size_t GridDomain::size() const {
  if (m_grids.empty())
    return 0;
  else
    return std::accumulate(
        m_grids.begin(), m_grids.end(), size_t{1},
        [](size_t n, const boost::shared_ptr<GridDomain> &grid) {
          return n * grid->size();
        });
}

/// number of dimensions of the grid
size_t GridDomain::nDimensions() {
  return std::accumulate(m_grids.begin(), m_grids.end(), size_t{0},
                         [](size_t n, boost::shared_ptr<GridDomain> &grid) {
                           return n + grid->nDimensions();
                         });
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
  for (auto &grid : m_grids)
    grid->reScale(scaling);
}

} // namespace API
} // namespace Mantid
