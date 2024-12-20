// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/FreeBlock.h"
#ifndef Q_MOC_RUN
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#endif
#include <cstdint>
#include <limits>
#include <list>
#include <mutex>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

// Forward declare
class ISaveable;

/** Buffer objects that need to be written out to disk
  so as to optimize writing operations.

  This will be used by file-backed MDEventWorkspaces to
  store boxes (lists of events) before writing them out.

  It also stores a list of "free" blocks in the output file,
  to allow new blocks to fill them later.

  @date 2011-12-30
*/
class MANTID_KERNEL_DLL DiskBuffer {
public:
  /** A map for the list of free space blocks in the file.
   * Index 1: Position in the file.
   * Index 2: Size of the free block
   */
  using freeSpace_t = boost::multi_index::multi_index_container<
      FreeBlock, boost::multi_index::indexed_by<
                     boost::multi_index::ordered_non_unique<
                         ::boost::multi_index::const_mem_fun<FreeBlock, uint64_t, &FreeBlock::getFilePosition>>,
                     boost::multi_index::ordered_non_unique<
                         ::boost::multi_index::const_mem_fun<FreeBlock, uint64_t, &FreeBlock::getSize>>>>;

  /// A way to index the free space by their size
  using freeSpace_bySize_t = freeSpace_t::nth_index<1>::type;

  DiskBuffer();
  DiskBuffer(uint64_t m_writeBufferSize);
  DiskBuffer(const DiskBuffer &) = delete;
  DiskBuffer &operator=(const DiskBuffer &) = delete;
  virtual ~DiskBuffer() = default;

  void toWrite(ISaveable *item);
  void flushCache();
  void objectDeleted(ISaveable *item);

  // Free space map methods
  void freeBlock(uint64_t const pos, uint64_t const size);
  void defragFreeBlocks();

  // Allocating
  uint64_t allocate(uint64_t const newSize);
  uint64_t relocate(uint64_t const oldPos, uint64_t const oldSize, const uint64_t newSize);

  // For reporting and saving
  void getFreeSpaceVector(std::vector<uint64_t> &free) const;
  void setFreeSpaceVector(std::vector<uint64_t> &free);
  std::string getMemoryStr() const;

  //-------------------------------------------------------------------------------------------
  /** Set the size of the to-write buffer, in number of events
   * @param buffer :: number of events to accumulate before writing. 0 to NOT
   * use the write buffer  */
  void setWriteBufferSize(uint64_t buffer) {
    if (buffer > std::numeric_limits<size_t>::max() / 2)
      throw std::runtime_error(" Can not aloocate memory for that many events "
                               "on given architecture ");

    m_writeBufferSize = static_cast<size_t>(buffer);
  }

  /// @return the size of the to-write buffer, in number of events
  uint64_t getWriteBufferSize() const { return m_writeBufferSize; }

  ///@return the memory used in the "toWrite" buffer, in number of events
  uint64_t getWriteBufferUsed() const { return m_writeBufferUsed; }

  //-------------------------------------------------------------------------------------------
  ///@return reference to the free space map (for testing only!)
  freeSpace_t &getFreeSpaceMap() { return m_free; }

  //-------------------------------------------------------------------------------------------
  ///@return the position of the last allocated point in the file (for testing
  /// only!)
  uint64_t getFileLength() const { return m_fileLength; }

  /** Set the length of the file that this MRU writes to.
   * @param length :: length in the same units as the cache, etc. (not
   * necessarily bytes)  */
  void setFileLength(const uint64_t length) const { m_fileLength = length; }

  //-------------------------------------------------------------------------------------------

protected:
  inline void writeOldObjects();

  // ----------------------- To-write buffer
  // --------------------------------------
  /// Do we use the write buffer? Always now
  // bool m_useWriteBuffer;

  /// Amount of memory to accumulate in the write buffer before writing.
  size_t m_writeBufferSize;

  /// Total amount of memory in the "toWrite" buffer.
  size_t m_writeBufferUsed;
  /// number of objects stored in to write buffer list
  size_t m_nObjectsToWrite;
  /** A forward list for the buffer of "toWrite" objects.   */
  std::list<ISaveable *> m_toWriteBuffer;

  /// Mutex for modifying the toWrite buffer.
  std::mutex m_mutex;

  // ----------------------- Free space map
  // --------------------------------------
  /// Map of the free blocks in the file
  freeSpace_t m_free;

  /// Index into m_free, but indexed by block size.
  freeSpace_bySize_t &m_free_bySize;

  /// Mutex for modifying the free space list
  std::mutex m_freeMutex;

  // ----------------------- File object --------------------------------------
  /// Length of the file. This is where new blocks that don't fit get placed.
  mutable uint64_t m_fileLength;

private:
};

} // namespace Kernel
} // namespace Mantid
