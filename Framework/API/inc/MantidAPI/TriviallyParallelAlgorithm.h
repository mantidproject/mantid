#ifndef MANTID_API_TRIVIALLY_PARALLEL_ALGORITHM_H_
#define MANTID_API_TRIVIALLY_PARALLEL_ALGORITHM_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {
/**
 Base class for algorithms that treat all spectra independently, i.e., we can
 trivially parallelize over the spectra without changes. The assumption is that
 we have one input and one output workspace. The storage mode is just propagated
 from input to output. When a specific algorithm is determined to be trivially
 parallel (this is a manual process), the only required change to add MPI
 support is to inherit from this class instead of Algorithm.

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
class MANTID_API_DLL TriviallyParallelAlgorithm : public Algorithm {
public:
  virtual ~TriviallyParallelAlgorithm() = default;

protected:
  /// We override this method to support parallel execution.
  virtual MPI::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, MPI::StorageMode> &storageModes)
      const override {
  // We have only one input workspace => consider only first map entry.
  return getCorrespondingExecutionMode(std::begin(storageModes)->second);
  }
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_TRIVIALLY_PARALLEL_ALGORITHM_H_ */
