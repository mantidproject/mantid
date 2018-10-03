// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/
class DLLExport FreeBlock {
public:
  /// Default constructor
  FreeBlock() : m_filePos(0), m_size(0) {}

  /** Constructor
   * @param pos :: position in the file
   * @param size :: size of the block  */
  FreeBlock(uint64_t pos, uint64_t size) : m_filePos(pos), m_size(size) {}

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
