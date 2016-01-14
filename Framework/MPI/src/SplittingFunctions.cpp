#include "MantidMPI/SplittingFunctions.h"
#include <boost/mpi.hpp>

namespace Mantid {
namespace MPI {

bool indexIsOnThisRank(int64_t index) {
  boost::mpi::communicator world;
  return index % world.size() == world.rank();
}

int rankOfIndex(int64_t index) {
  boost::mpi::communicator world;
  return index % world.size();
}

} // namespace MPI
} // namespace Mantid
