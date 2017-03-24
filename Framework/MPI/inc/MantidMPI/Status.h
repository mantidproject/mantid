#ifndef MANTID_MPI_STATUS_H_
#define MANTID_MPI_STATUS_H_

#include "MantidMPI/DllConfig.h"

#ifdef MPI_EXPERIMENTAL
#include <boost/mpi/status.hpp>
#endif

namespace Mantid {
namespace MPI {

/** Wrapper for boost::mpi::status. For non-MPI builds an equivalent
  implementation is provided.

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
class MANTID_MPI_DLL Status {
public:
#ifdef MPI_EXPERIMENTAL
  Status(const boost::mpi::status &status);
#else
  Status(int source, int tag, int error);
#endif

  int source() const;
  int tag() const;
  int error() const;

private:
#ifdef MPI_EXPERIMENTAL
  boost::mpi::status m_status;
#else
  int m_source;
  int m_tag;
  int m_error;
#endif
};

} // namespace MPI
} // namespace Mantid

#endif /* MANTID_MPI_STATUS_H_ */
