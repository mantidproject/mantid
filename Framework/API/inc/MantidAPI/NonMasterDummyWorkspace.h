#ifndef MANTID_API_NONMASTERDUMMYWORKSPACE_H_
#define MANTID_API_NONMASTERDUMMYWORKSPACE_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Workspace.h"
#include "MantidParallel/Communicator.h"

namespace Mantid {
namespace API {

/** NonMasterDummyWorkspace is an empty dummy workspace that can be created in
  MPI builds on non-master ranks to represent a workspace with storage mode
  `MasterOnly`.

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
class MANTID_API_DLL NonMasterDummyWorkspace : public Workspace {
public:
  NonMasterDummyWorkspace(const Parallel::Communicator &communicator);
  const std::string id() const override;
  const std::string toString() const override;
  size_t getMemorySize() const override;

private:
  NonMasterDummyWorkspace *doClone() const override;
  Parallel::Communicator m_communicator;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_NONMASTERDUMMYWORKSPACE_H_ */
