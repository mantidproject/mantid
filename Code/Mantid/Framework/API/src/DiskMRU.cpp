#include "MantidAPI/DiskMRU.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DiskMRU::DiskMRU()
  : m_memoryAvail(100), m_writeBufferSize(50),
    m_memoryUsed(0),
    m_toWrite_byId( m_toWrite.get<1>() ),
    m_memoryToWrite(0)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_memoryAvail :: Amount of memory that the MRU is allowed to use.
   * @param m_writeBufferSize :: Amount of memory to accumulate in the write buffer before writing.
   * @return
   */
  DiskMRU::DiskMRU(size_t m_memoryAvail, size_t m_writeBufferSize)
  : m_memoryAvail(m_memoryAvail), m_writeBufferSize(m_writeBufferSize),
    m_memoryUsed(0),
    m_toWrite_byId( m_toWrite.get<1>() ),
    m_memoryToWrite(0)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DiskMRU::~DiskMRU()
  {
    // The item pointers are NOT owned by the MRU.
    list.clear();
  }
  

  //---------------------------------------------------------------------------------------------
  /** Tell the MRU that we are loading the given item.
   *
   * @param item :: item that is
   * @param memory :: memory that the object will use.
   */
  void DiskMRU::loading(ISaveable * item)
  {
    // Place the item in the MRU list
    std::pair<mru_list::iterator,bool> p;
    p = list.push_front(item);

    // Find the item in the toWrite buffer (using the item's ID)
    toWriteMap_by_Id_t::iterator found = m_toWrite_byId.find( item->getId() );
    if (found != m_toWrite_byId.end())
    {
      m_toWrite_byId.erase(item->getId());
      m_memoryToWrite -= item->getMRUMemory();
    }

    if (!p.second)
    {
      // duplicate item: put it back at the front of the list
      list.relocate(list.begin(), p.first);
      return;
    }

    // We are now using more memory.
    m_memoryUsed += item->getMRUMemory();

    if (m_memoryUsed > m_memoryAvail)
    {
      // You might have to pop 1 or more items until the MRU memory is below the limit
      while (m_memoryUsed > m_memoryAvail)
      {
        // Pop the least-used object out the back
        ISaveable *toWrite = list.back();
        list.pop_back();

        // And put it in the queue of stuff to write.
        m_toWrite.insert(toWrite);
        //m_toWrite.insert( pairObj_t(toWrite->getFilePosition(), toWrite) );

        // Track the memory change in the two buffers
        size_t thisMem = toWrite->getMRUMemory();
        m_memoryToWrite += thisMem;
        m_memoryUsed -= thisMem;
      }

      // Should we now write out the old data?
      if (m_memoryToWrite >= m_writeBufferSize)
        writeOldObjects();
    }
  }



  //---------------------------------------------------------------------------------------------
  /** Method to write out the old objects that have been
   * stored in the "toWrite" buffer.
   */
  void DiskMRU::writeOldObjects()
  {
    // Holder for any objects that you were NOT able to write.
    toWriteMap_t couldNotWrite;
    size_t memoryNotWritten = 0;

    // Iterate through the map
    toWriteMap_t::iterator it = m_toWrite.begin();
    toWriteMap_t::iterator it_end = m_toWrite.end();

    for (; it != it_end; it++)
    {
      ISaveable * obj = *it; //->second;
      if (obj->safeToWrite())
      {
        // Write to the disk
        obj->save();
      }
      else
      {
        // The object is busy, can't write. Save it for later
        //couldNotWrite.insert( pairObj_t(obj->getFilePosition(), obj) );
        couldNotWrite.insert( obj );
        memoryNotWritten += obj->getMRUMemory();
      }
    }

    // Exchange with the new map you built out of the not-written blocks.
    m_toWrite.swap(couldNotWrite);
    m_memoryToWrite = memoryNotWritten;
  }


} // namespace Mantid
} // namespace API

