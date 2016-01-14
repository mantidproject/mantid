#ifndef MANTID_MPI_HELPERS_H_
#define MANTID_MPI_HELPERS_H_

#ifdef MPI_BUILD
#include <boost/mpi/collectives_fwd.hpp>
#endif

#include "MantidAPI/FrameworkManager.h"

namespace Mantid {
namespace MPI {

#ifdef MPI_BUILD
/// Get the standard MPI communicator.
inline boost::mpi::communicator &communicator() {
  return API::FrameworkManager::Instance().getMPICommunicator();
}
#endif

/** Get the MPI rank index of this rank.
 *
 * Always 0 for non-MPI builds */
inline int rank() {
#ifdef MPI_BUILD
  return communicator().rank();
#else
  return 0;
#endif
}

/** Get the MPI rank index of the root/master rank.
 *
 * Usually this is rank 0. */
inline int rootRank() {
  return 0;
}

/// Returns true if this rank is the root/master rank.
inline bool isRoot() {
  return rootRank() == rank();
}

/** Returns the umber of MPI ranks.
 *
 * Always 1 for non-MPI builds. */
inline int numberOfRanks() {
#ifdef MPI_BUILD
  return communicator().size();
#else
  return 1;
#endif
}

} // namespace MPI
} // namespace Mantid

#endif /* MANTID_MPI_HELPERS_H_*/
