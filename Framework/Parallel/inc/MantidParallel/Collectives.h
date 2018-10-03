#ifndef MANTID_PARALLEL_COLLECTIVES_H_
#define MANTID_PARALLEL_COLLECTIVES_H_

#include "MantidParallel/Communicator.h"
#include "MantidParallel/DllConfig.h"
#include "MantidParallel/Nonblocking.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/collectives.hpp>
#endif

namespace Mantid {
namespace Parallel {

/** Wrapper for boost::mpi::gather and other collective communication. For
  non-MPI builds an equivalent implementation with reduced functionality is
  provided.

  @author Simon Heybrock
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace detail {
template <typename T>
void gather(const Communicator &comm, const T &in_value,
            std::vector<T> &out_values, int root) {
  int tag{0};
  if (comm.rank() != root) {
    comm.send(root, tag, in_value);
  } else {
    out_values.resize(comm.size());
    out_values[root] = in_value;
    for (int rank = 0; rank < comm.size(); ++rank) {
      if (rank == root)
        continue;
      comm.recv(rank, tag, out_values[rank]);
    }
  }
}

template <typename T>
void gather(const Communicator &comm, const T &in_value, int root) {
  int tag{0};
  if (comm.rank() != root) {
    comm.send(root, tag, in_value);
  } else {
    throw std::logic_error(
        "Parallel::gather on root rank without output argument.");
  }
}

template <typename... T>
void all_gather(const Communicator &comm, T &&... args) {
  for (int root = 0; root < comm.size(); ++root)
    gather(comm, std::forward<T>(args)..., root);
}

template <typename T>
void all_to_all(const Communicator &comm, const std::vector<T> &in_values,
                std::vector<T> &out_values) {
  int tag{0};
  out_values.resize(comm.size());
  std::vector<Request> requests;
  for (int rank = 0; rank < comm.size(); ++rank)
    requests.emplace_back(comm.irecv(rank, tag, out_values[rank]));
  for (int rank = 0; rank < comm.size(); ++rank)
    comm.send(rank, tag, in_values[rank]);
  wait_all(requests.begin(), requests.end());
}
} // namespace detail

template <typename... T> void gather(const Communicator &comm, T &&... args) {
#ifdef MPI_EXPERIMENTAL
  if (!comm.hasBackend())
    return boost::mpi::gather(comm, std::forward<T>(args)...);
#endif
  detail::gather(comm, std::forward<T>(args)...);
}

template <typename... T>
void all_gather(const Communicator &comm, T &&... args) {
#ifdef MPI_EXPERIMENTAL
  if (!comm.hasBackend())
    return boost::mpi::all_gather(comm, std::forward<T>(args)...);
#endif
  detail::all_gather(comm, std::forward<T>(args)...);
}

template <typename... T>
void all_to_all(const Communicator &comm, T &&... args) {
#ifdef MPI_EXPERIMENTAL
  if (!comm.hasBackend())
    return boost::mpi::all_to_all(comm, std::forward<T>(args)...);
#endif
  detail::all_to_all(comm, std::forward<T>(args)...);
}

} // namespace Parallel
} // namespace Mantid

#endif /* MANTID_PARALLEL_COLLECTIVES_H_ */
