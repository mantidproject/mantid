#ifndef MANTID_API_DISKMRU_H_
#define MANTID_API_DISKMRU_H_
    
#include "MantidAPI/ISaveable.h"
#include "MantidKernel/System.h"
#include <vector>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/sequenced_index.hpp>


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
  class DLLExport DiskMRU 
  {
  public:
    /** Typedef defines that we will keep the objects with these 2 orderings:
     * 1. sequenced = the order they were added.
     * 2. a hashed, unique index = the "id" number".
     */
    typedef boost::multi_index::multi_index_container<
      ISaveable *,
      boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ISaveable, size_t, getId)>
      >
    > item_list;


    DiskMRU();
    
    ~DiskMRU();

    //---------------------------------------------------------------------------------------------
    /** Tell the MRU that we are loading the given item.
     *
     * @param item :: item that is
     * @param memory :: memory that the object will use.
     */
    void loading(ISaveable * item)
    {
      // Place the item in the MRU list
      std::pair<item_list::iterator,bool> p;
      p = list.push_front(item);

      if (!p.second)
      {
        /* duplicate item */
        list.relocate(list.begin(), p.first); /* put in front */
        return;
      }

      // We are now using more memory.
      m_memoryUsed += item->getMRUMemory();

      if (m_memoryUsed > m_memoryAvail)
      {
        // Pop the least-used object out the back
        ISaveable *toWrite = list.back();
        list.pop_back();

        // And put it in the queue of stuff to write.
        m_toWrite.push_back(toWrite);
        m_memoryToWrite += toWrite->getMRUMemory();

        // Should we now write out the old data?
        if (m_memoryToWrite > 0)
          writeOldObjects();
      }
    }



  protected:
    //---------------------------------------------------------------------------------------------
    /** Method to write out the old objects that have been
     * stored in the "toWrite" buffer.
     */
    void writeOldObjects()
    {
      size_t num = m_toWrite.size();
      for (size_t i=0; i<num; i++)
      {
        // Write...
      }
    }


    /// The MRU list container
    item_list list;

    /// Amount of memory that the MRU is allowed to use.
    /// Note that the units are up to the ISaveable to define; they don't have to be bytes.
    size_t m_memoryAvail;

    /// Amount of memory actually used up.
    size_t m_memoryUsed;

    /// Vector of the data objects that should be written out (no particular order).
    std::vector<ISaveable *> m_toWrite;

    /// Total amount of memory in the "toWrite" buffer.
    size_t m_memoryToWrite;

  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_DISKMRU_H_ */
