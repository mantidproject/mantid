#ifndef MANTID_API_DISKMRU_H_
#define MANTID_API_DISKMRU_H_
    
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

  /** A Most-Recently-Used list of objects defined specifically for caching to disk.
    
    This class is to be used by the file-back-end of MDEventWorkspace, but build it more generally. This is a class that:

    Limits the amount of objects in the cache to a certain amount of memory (not a fixed number of items) since objects will have varied sizes.
    Keeps the most recently used objects in memory.
    Delegates the loading/saving of the data to the object itself (because the object will stay in memory but its contents won't).
      * Use an ISaveable simple interface to delegate the loading and saving.
      * Each ISaveable tells the the DiskMRU when it needs to load itself so that the MRU :
          * Marks it as recently used.
          * Frees some memory by writing out another one.

    Also, the DiskMRU should:

    Combine write operations in "blocks" so that seeking is minimized.
      * A certain minimum write size will be accumulated before writing to disk.
      * Objects will be sorted by their file index position before writing.


    @author Janik Zikovsky
    @date 2011-07-28

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
  class DLLExport DiskMRU
  {
  public:
    /** Typedef defines that we will keep the objects with these 2 orderings:
     * 1. sequenced = the order they were added.
     * 2. a hashed, unique index = the "id" number.
     */
    typedef boost::multi_index::multi_index_container<
      const ISaveable *,
      boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ISaveable, size_t, getId)>
      >
    > mru_t;

    /// A way to index the MRU buffer by ID (instead of the file position)
    typedef mru_t::nth_index<1>::type mru_byId_t;

    /// Typedef for a par for the map. Key = position in the file; value = the ISaveable object
    typedef std::pair<uint64_t, const ISaveable*> pairObj_t;


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


    DiskMRU();
    DiskMRU(uint64_t m_mruSize, uint64_t m_writeBufferSize, uint64_t smallBufferSize = 0);
    virtual ~DiskMRU();

    // MRU and list management
    void loading(const ISaveable * item);
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

    // For the Small buffer
    void setNumberOfObjects(size_t numObjects);
    bool shouldStayInMemory(size_t id, uint64_t size);

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
    /** Set the size of the memory allowed in the MRU list, in number of events
     * @param buffer :: max number of events to keep in memory. 0 to NOT use the MRU */
    void setMruSize(uint64_t buffer)
    {
      m_mruSize = buffer;
      m_useWriteBuffer = (buffer > 0);
    }

    /// @return the size of the in-memory MRU, in number of events
    uint64_t getMruSize() const
    { return m_mruSize; }

    ///@return the memory used in the MRU, in number of events
    uint64_t getMruUsed() const
    { return m_mruUsed;  }

    //-------------------------------------------------------------------------------------------
    /** Set the size of the "small" buffer, in number of events.
     * This is the buffer for event lists that are too small to bother caching to disk
     * @param buffer :: total number of events to allow in the "small" buffer */
    void setSmallBufferSize(uint64_t buffer)
    {
      m_smallBufferSize = buffer;
      m_useSmallBuffer = (buffer > 0);
      calcSmallThreshold();
    }

    /// @return the size of the to-Small buffer, in number of events
    uint64_t getSmallBufferSize() const
    { return m_smallBufferSize; }

    ///@return the memory used in the "toSmall" buffer, in number of events
    uint64_t getSmallBufferUsed() const
    { return m_smallBufferUsed;  }

    ///@return Threshold number of events for an object to be considered "small"
    uint64_t getSmallThreshold() const
    { return m_smallThreshold;  }

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



  protected:
    inline void writeOldObjects();
    void calcSmallThreshold();

    // ----------------------- In-memory buffer --------------------------------------
    /// Do we use the MRU buffer?
    bool m_useWriteBuffer;

    /// The MRU list container
    mru_t m_mru;

    /// The MRU list, indexed by ID
    mru_byId_t & m_mru_byId;

    /// Amount of memory that the MRU is allowed to use.
    /// Note that the units are up to the ISaveable to define; they don't have to be bytes.
    uint64_t m_mruSize;

    /// Amount of memory actually used up in the MRU
    uint64_t m_mruUsed;

    /// Mutex for modifying the MRU list and/or the toWrite buffer.
    Kernel::Mutex m_mruMutex;

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

    // ----------------------- Small Objects Buffer --------------------------------------
    /// Do we use the buffer of "small" objects?
    bool m_useSmallBuffer;

    /// Approximate amount of memory to allow in "small" objects. This will be an UPPER bound.
    uint64_t m_smallBufferSize;

    /// Vector where the index = object ID; value = size of the object if it is in the small object buffer.
    std::vector<uint32_t> m_smallBuffer;

    /// Total amount of memory in the "small objects" buffer.
    uint64_t m_smallBufferUsed;

    /// Threshold number of events for an object to be considered "small"
    uint64_t m_smallThreshold;

    /// Mutex for modifying entries in the small buffer
    Kernel::Mutex m_smallBufferMutex;


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
    DiskMRU(const DiskMRU&);
    /// Private assignment operator: NO ASSIGNMENT ALLOWED
    DiskMRU& operator=(const DiskMRU&);


  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_API_DISKMRU_H_ */
