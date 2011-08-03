#ifndef MANTID_API_DISKMRU_H_
#define MANTID_API_DISKMRU_H_
    
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ISaveable.h"
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
namespace API
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
  class MANTID_API_DLL DiskMRU
  {
  public:
    /** Typedef defines that we will keep the objects with these 2 orderings:
     * 1. sequenced = the order they were added.
     * 2. a hashed, unique index = the "id" number".
     */
    typedef boost::multi_index::multi_index_container<
      const ISaveable *,
      boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ISaveable, size_t, getId)>
      >
    > mru_list;

    /// Typedef for a par for the map. Key = position in the file; value = the ISaveable object
    typedef std::pair<uint64_t, const ISaveable*> pairObj_t;


    /** A map for the buffer of "toWrite" objects.
     * Index 1: Order in the file to save to
     * Index 2: ID of the object
     */
    //typedef std::multimap<uint64_t, ISaveable*> toWriteMap_t;
    typedef boost::multi_index::multi_index_container<
      const ISaveable *,
      boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ISaveable, uint64_t, getFilePosition)>,
        boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ISaveable, size_t, getId)>
      >
    > toWriteMap_t;

    /// A way to index the toWrite buffer by ID (instead of the file position)
    typedef toWriteMap_t::nth_index<1>::type toWriteMap_by_Id_t;


    DiskMRU();

    DiskMRU(size_t m_memoryAvail, size_t m_writeBufferSize, bool useWriteBuffer);
    
    virtual ~DiskMRU();

    void loading(const ISaveable * item);

    void loadingWithWriteBuffer(const ISaveable * item);

    void flushCache();

    //-------------------------------------------------------------------------------------------
    ///@return the memory used in the MRU, in number of events
    size_t getMemoryUsed() const
    { return m_memoryUsed;  }

    ///@return the memory in the "toWrite" buffer, in number of events
    size_t getMemoryToWrite() const
    { return m_memoryToWrite;  }

    //-------------------------------------------------------------------------------------------
    /** Set the size of the to-write buffer, in number of events
     * @param buffer :: number of events to accumulate before writing  */
    void setWriteBufferSize(size_t buffer)
    { m_writeBufferSize = buffer; }

    /// @return the size of the to-write buffer, in number of events
    size_t getWriteBufferSize() const
    { return m_writeBufferSize; }

    //-------------------------------------------------------------------------------------------
    /** Set the size of the in-memory cache, in number of events
     * @param buffer :: max number of events to keep in memory */
    void setMemoryAvail(size_t buffer)
    { m_memoryAvail = buffer; }

    /// @return the size of the in-memory cache, in number of events
    size_t getMemoryAvail() const
    { return m_memoryAvail; }


  protected:
    void writeOldObjects();

    /// The MRU list container
    mru_list list;

    /// Amount of memory that the MRU is allowed to use.
    /// Note that the units are up to the ISaveable to define; they don't have to be bytes.
    size_t m_memoryAvail;

    /// Amount of memory to accumulate in the write buffer before writing.
    size_t m_writeBufferSize;

    /// Do we use the write buffer?
    bool m_useWriteBuffer;

    /// Amount of memory actually used up (in the MRU, not the toWriteBuffer)
    size_t m_memoryUsed;

    /// List of the data objects that should be written out. Ordered by file position.
    toWriteMap_t m_toWrite;

    /// Reference to the same m_toWrite map, but indexed by item ID instead of by file position.
    toWriteMap_by_Id_t & m_toWrite_byId;

    /// Total amount of memory in the "toWrite" buffer.
    size_t m_memoryToWrite;

    /// Mutex for modifying the MRU list
    Kernel::Mutex m_mruMutex;

  private:
    /// Private Copy constructor: NO COPY ALLOWED
    DiskMRU(const DiskMRU&);
    /// Private assignment operator: NO ASSIGNMENT ALLOWED
    DiskMRU& operator=(const DiskMRU&);

  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_DISKMRU_H_ */
