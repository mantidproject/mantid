#ifndef MANTID_KERNEL_DISKBUFFER_H_
#define MANTID_KERNEL_DISKBUFFER_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/FreeBlock.h"
#include "MantidKernel/ISaveable.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <map>
#include <stdint.h>
#include <vector>

namespace Mantid
{
namespace Kernel
{

  /** Buffer objects that need to be written out to disk
    so as to optimize writing operations.

    This will be used by file-backed MDEventWorkspaces to
    store boxes (lists of events) before writing them out.

    It also stores a list of "free" blocks in the output file,
    to allow new blocks to fill them later.

    @date 2011-12-30

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport DiskBuffer 
  {
  public:


    /** A map for the buffer of "toWrite" objects.
     * Index 1: Order in the file to save to
     * Index 2: ID of the object
     */
    typedef boost::multi_index::multi_index_container<
      const ISaveable *,
      boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ISaveable, uint64_t, getFilePosition)>,
        boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ISaveable, size_t, getId)>
      >
    > writeBuffer_t;

    /// A way to index the toWrite buffer by ID (instead of the file position)
    typedef writeBuffer_t::nth_index<1>::type writeBuffer_byId_t;

    /** A map for the list of free space blocks in the file.
     * Index 1: Position in the file.
     * Index 2: Size of the free block
     */
    typedef boost::multi_index::multi_index_container<
      FreeBlock,
      boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(FreeBlock, uint64_t, getFilePosition)>,
        boost::multi_index::ordered_non_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(FreeBlock, uint64_t, getSize)>
      >
    > freeSpace_t;

    /// A way to index the free space by their size
    typedef freeSpace_t::nth_index<1>::type freeSpace_bySize_t;


    DiskBuffer();
    DiskBuffer(uint64_t m_writeBufferSize);
    virtual ~DiskBuffer();

    void toWrite(const ISaveable * item);
    void flushCache();
    void objectDeleted(const ISaveable * item, const uint64_t sizeOnFile);

    // Free space map methods
    void freeBlock(uint64_t const pos, uint64_t const size);
    void defragFreeBlocks();

    // Allocating
    uint64_t allocate(uint64_t const newSize);
    uint64_t relocate(uint64_t const oldPos, uint64_t const oldSize, const uint64_t newSize);

    // For reporting and saving
    void getFreeSpaceVector(std::vector<uint64_t> & free) const;
    std::string getMemoryStr() const;


    //-------------------------------------------------------------------------------------------
    /** Set the size of the to-write buffer, in number of events
     * @param buffer :: number of events to accumulate before writing. 0 to NOT use the write buffer  */
    void setWriteBufferSize(uint64_t buffer)
    {
      m_writeBufferSize = buffer;
      m_useWriteBuffer = (buffer > 0);
    }

    /// @return the size of the to-write buffer, in number of events
    uint64_t getWriteBufferSize() const
    { return m_writeBufferSize; }

    ///@return the memory used in the "toWrite" buffer, in number of events
    uint64_t getWriteBufferUsed() const
    { return m_writeBufferUsed;  }

    //-------------------------------------------------------------------------------------------
    ///@return reference to the free space map (for testing only!)
    freeSpace_t & getFreeSpaceMap()
    { return m_free;  }

    //-------------------------------------------------------------------------------------------
    ///@return the position of the last allocated point in the file (for testing only!)
    uint64_t getFileLength() const
    { return m_fileLength; }

    /** Set the length of the file that this MRU writes to.
     * @param length :: length in the same units as the cache, etc. (not necessarily bytes)  */
    void setFileLength(const uint64_t length)
    { m_fileLength = length; }

    //-------------------------------------------------------------------------------------------
    /** @return the file-access mutex */
    Kernel::Mutex & getFileMutex()
    { return m_fileMutex; }


  protected:
    inline void writeOldObjects();

    /// Mutex for accessing the file being buffered
    Kernel::Mutex m_fileMutex;

    // ----------------------- To-write buffer --------------------------------------
    /// Do we use the write buffer?
    bool m_useWriteBuffer;

    /// Amount of memory to accumulate in the write buffer before writing.
    uint64_t m_writeBufferSize;

    /// List of the data objects that should be written out. Ordered by file position.
    writeBuffer_t m_writeBuffer;

    /// Reference to the same m_writeBuffer map, but indexed by item ID instead of by file position.
    writeBuffer_byId_t & m_writeBuffer_byId;

    /// Total amount of memory in the "toWrite" buffer.
    uint64_t m_writeBufferUsed;

    /// Mutex for modifying the the toWrite buffer.
    Kernel::Mutex m_mutex;

    // ----------------------- Free space map --------------------------------------
    /// Map of the free blocks in the file
    freeSpace_t m_free;

    /// Index into m_free, but indexed by block size.
    freeSpace_bySize_t & m_free_bySize;

    /// Mutex for modifying the free space list
    Kernel::Mutex m_freeMutex;

    // ----------------------- File object --------------------------------------
    /// Length of the file. This is where new blocks that don't fit get placed.
    uint64_t m_fileLength;

  private:
    /// Private Copy constructor: NO COPY ALLOWED
    DiskBuffer(const DiskBuffer&);
    /// Private assignment operator: NO ASSIGNMENT ALLOWED
    DiskBuffer& operator=(const DiskBuffer&);


  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_DISKBUFFER_H_ */
