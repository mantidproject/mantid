#ifndef MANTID_API_FREEBLOCK_H_
#define MANTID_API_FREEBLOCK_H_

#include "MantidKernel/System.h"

namespace Mantid {
namespace Kernel {

/** FreeBlock: a simple class that holds the position
  and size of block of free space in a file.

  This is used by the DiskBuffer class to track and defrag free space.

  @author Janik Zikovsky, SNS
  @date 2011-08-04

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport FreeBlock {
public:
  /// Default constructor
  FreeBlock() : m_filePos(0), m_size(0) {}

  /** Constructor
   * @param pos :: position in the file
   * @param size :: size of the block  */
  FreeBlock(uint64_t pos, uint64_t size) : m_filePos(pos), m_size(size) {}

  /** Copy constructor
   * @param other :: copy this */
  FreeBlock(const FreeBlock &other)
      : m_filePos(other.m_filePos), m_size(other.m_size) {}

  /** Assignment operator
   * @param other :: copy this */
  FreeBlock &operator=(const FreeBlock &other) {
    m_filePos = other.m_filePos;
    m_size = other.m_size;
    return *this;
  }

  /// Destructor
  ~FreeBlock() {}

  /// @return the position of the free block in the file
  inline uint64_t getFilePosition() const { return m_filePos; }

  /// @return the size of the free block in the file
  inline uint64_t getSize() const { return m_size; }

  //----------------------------------------------------------------
  /** Attempt to merge an adjacent block into this one.
   * If the blocks are contiguous, they get merged into one larger block.
   * NOTE: "second" must be AFTER "first" in the file.
   *
   * @param first :: block to be merged and which will remain
   * @param second :: other block to merge with this one
   * @return true if the merge was successful and the "other" block should be
   *dropped
   *          because "this" has taken its space.
   */
  static bool merge(FreeBlock &first, const FreeBlock &second) {
    if ((first.m_filePos + first.m_size) == second.m_filePos) {
      // Blocks are contiguous and get merged
      first.m_size += second.m_size;
      // Caller will then remove the other block
      return true;
    } else
      return false;
  }

public:
  uint64_t m_filePos;
  uint64_t m_size;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_API_FREEBLOCK_H_ */
