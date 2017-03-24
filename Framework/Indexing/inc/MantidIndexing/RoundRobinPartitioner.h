#ifndef MANTID_INDEXING_ROUNDROBINPARTITIONER_H_
#define MANTID_INDEXING_ROUNDROBINPARTITIONER_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/Partitioner.h"

#include <stdexcept>

namespace Mantid {
namespace Indexing {

/** A round-robin partitioning pattern, i.e., indices are assigned to partitions
  one at a time, looping over partitions.

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
class MANTID_INDEXING_DLL RoundRobinPartitioner : public Partitioner {
public:
  using Partitioner::Partitioner;

private:
  PartitionIndex doIndexOf(const GlobalSpectrumIndex index) const override;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_ROUNDROBINPARTITIONER_H_ */
