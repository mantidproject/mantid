#ifndef MANTID_API_MASTER_ONLY_ALGORITHM_H_
#define MANTID_API_MASTER_ONLY_ALGORITHM_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {
/**
 TODO
 Copyright &copy; 2007-15 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_API_DLL MasterOnlyAlgorithm : public Algorithm {
public:
  virtual ~MasterOnlyAlgorithm() = default;

protected:
  /// TODO
  virtual MPI::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, MPI::StorageMode> &storageModes) const {
    for (const auto &mode : storageModes)
      if (mode.second != MPI::StorageMode::MasterOnly)
        return MPI::ExecutionMode::Invalid;
    return MPI::ExecutionMode::MasterOnly;
  }
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MASTER_ONLY_ALGORITHM_H_ */
