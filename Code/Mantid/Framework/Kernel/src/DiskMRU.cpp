#include "MantidKernel/DiskMRU.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <sstream>

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DiskMRU::DiskMRU()
  : m_useMRU(true),
    m_mru_byId( m_mru.get<1>() ),
    m_mruSize(100),
    m_mruUsed(0),
    m_useWriteBuffer(false),
    m_writeBufferSize(50),
    m_writeBuffer_byId( m_writeBuffer.get<1>() ),
    m_writeBufferUsed(0),
    m_useSmallBuffer(false),
    m_smallBufferSize(0),
    m_smallBufferUsed(0),
    m_smallThreshold(0),
    m_free(),
    m_free_bySize( m_free.get<1>() ),
    m_fileLength(0)
  {
    m_free.clear();
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_mruSize :: Amount of memory that the MRU is allowed to use.
   * @param m_writeBufferSize :: Amount of memory to accumulate in the write buffer before writing.
   * @param smallBufferSize :: Number of events in the "small objects" buffer
   * @return
   */
  DiskMRU::DiskMRU(uint64_t m_mruSize, uint64_t m_writeBufferSize, uint64_t smallBufferSize)
  : m_useMRU(m_mruSize > 0),
    m_mru_byId( m_mru.get<1>() ),
    m_mruSize(m_mruSize),
    m_mruUsed(0),
    m_useWriteBuffer(m_writeBufferSize > 0),
    m_writeBufferSize(m_writeBufferSize),
    m_writeBuffer_byId( m_writeBuffer.get<1>() ),
    m_writeBufferUsed(0),
    m_useSmallBuffer(smallBufferSize > 0),
    m_smallBufferSize(smallBufferSize),
    m_smallBufferUsed(0),
    m_smallThreshold(0),
    m_free(),
    m_free_bySize( m_free.get<1>() ),
    m_fileLength(0)
  {
    calcSmallThreshold();
    m_free.clear();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DiskMRU::~DiskMRU()
  {
    // The item pointers are NOT owned by the MRU.
    m_mru.clear();
  }
  

  //---------------------------------------------------------------------------------------------
  /** Tell the MRU that we are loading the given item.
   *
   * @param item :: item that is
   * @param memory :: memory that the object will use.
   */
  void DiskMRU::loading(const ISaveable * item)
  {
    if (item == NULL) return;
    m_mruMutex.lock();

    if (m_useMRU)
    {
      // Place the item in the MRU list
      std::pair<mru_t::iterator,bool> p;
      p = m_mru.push_front(item);

      if (!p.second)
      {
        // duplicate item: put it back at the front of the list
        m_mru.relocate(m_mru.begin(), p.first);
        m_mruMutex.unlock();
        return;
      }

      // We are now using more memory.
      m_mruUsed += item->getMRUMemorySize();

      if (!m_useWriteBuffer)
      {
        // No write buffer - just write things out directly.
        // You might have to pop 1 or more items until the MRU memory is below the limit
        mru_t::iterator it = m_mru.end();

        while (m_mruUsed > m_mruSize && (m_mru.size() > 0))
        {
          // Pop the least-used object out the back
          it--;
          if (it == m_mru.begin()) break; // Avoid going out of bounds
          const ISaveable *toWrite = *it;
          // Can you save it to disk?
          if (!toWrite->dataBusy())
          {
            toWrite->save();
            m_mruUsed -= toWrite->getMRUMemorySize();
            m_mru.erase(it);
          }
        }
      }
      else
      {
        // Write buffer - accumulate objects to write

        // Find the newly loaded item in the toWrite buffer (using the item's ID)
        writeBuffer_byId_t::iterator found = m_writeBuffer_byId.find( item->getId() );
        if (found != m_writeBuffer_byId.end())
        {
          m_writeBuffer_byId.erase(item->getId());
          m_writeBufferUsed -= item->getMRUMemorySize();
        }

        // You might have to pop 1 or more items until the MRU memory is below the limit
        while ((m_mruUsed > m_mruSize) && (m_mru.size() > 0))
        {
          // Pop the least-used object out the back
          const ISaveable *toWrite = m_mru.back();
          m_mru.pop_back();

          // And put it in the queue of stuff to write.
          m_writeBuffer.insert(toWrite);

          // Track the memory change in the two buffers
          size_t thisMem = toWrite->getMRUMemorySize();
          m_writeBufferUsed += thisMem;
          m_mruUsed -= thisMem;
        }
      }
    } // end (if using the MRU)
    else
    {
      if (m_useWriteBuffer)
      {
        // Take the newly loaded item and immediately put it in the "to-write" queue.
        if (m_writeBuffer.insert(item).second)
        {
          // The insert() method returns a pair with the 'second" member = TRUE if the insert was successful
          m_writeBufferUsed += item->getMRUMemorySize();
        }
      }
    } // end (not using the mru)


    if (m_useWriteBuffer)
    {
      // Should we now write out the old data?
      if (m_writeBufferUsed >= m_writeBufferSize)
        writeOldObjects();
    }

    m_mruMutex.unlock();
  }


  //---------------------------------------------------------------------------------------------
  /** Call this method when an object that might be in the cache
   * is getting deleted.
   * The object is removed from the MRU and the to-write buffer (if present).
   * The space it uses on disk is marked as free
   *
   * @param item :: ISaveable object that is getting deleted.
   * @param sizeOnFile :: size that the object used on file. This amount of space is marked as "free"
   */
  void DiskMRU::objectDeleted(const ISaveable * item, const uint64_t sizeOnFile)
  {
    size_t id = item->getId();
    uint64_t size = item->getMRUMemorySize();

    m_mruMutex.lock();

    // Take it out of the MRU cache
    mru_byId_t::iterator it = m_mru_byId.find(id);
    if (it != m_mru_byId.end())
    {
      m_mru_byId.erase(it);
      m_mruUsed -= size;
    }

    // Take it out of the to-write buffer
    writeBuffer_byId_t::iterator it2 = m_writeBuffer_byId.find(id);
    if (it2 != m_writeBuffer_byId.end())
    {
      m_writeBuffer_byId.erase(it2);
      m_writeBufferUsed -= size;
    }
    m_mruMutex.unlock();

    // Take it out of the small buffer
    if (id < m_smallBuffer.size())
    {
      uint32_t & bufferValue = m_smallBuffer[id];
      m_smallBufferUsed -= bufferValue;
      bufferValue = 0;
    }

    // Mark the amount of space used on disk as free
    this->freeBlock(item->getFilePosition(), sizeOnFile);
  }


  //---------------------------------------------------------------------------------------------
  /** Method to write out the old objects that have been
   * stored in the "toWrite" buffer.
   */
  void DiskMRU::writeOldObjects()
  {
    if (m_writeBufferUsed > 0)
      std::cout << "DiskMRU:: Writing out " << m_writeBufferUsed << " events in " << m_writeBuffer.size() << " blocks." << std::endl;
//    std::cout << getMemoryStr() << std::endl;
//    std::cout << getFreeSpaceMap().size() << " entries in the free size map." << std::endl;
//    for (freeSpace_t::iterator it = m_free.begin(); it != m_free.end(); it++)
//      std::cout << " Free : " << it->getFilePosition() << " size " << it->getSize() << std::endl;
//    std::cout << m_fileLength << " length of file" << std::endl;

    // Holder for any objects that you were NOT able to write.
    writeBuffer_t couldNotWrite;
    size_t memoryNotWritten = 0;

    // Iterate through the map
    writeBuffer_t::iterator it = m_writeBuffer.begin();
    writeBuffer_t::iterator it_end = m_writeBuffer.end();

    const ISaveable * obj = NULL;
    for (; it != it_end; it++)
    {
      obj = *it;
      if (!obj->dataBusy())
      {
        // Write to the disk
        obj->save();
      }
      else
      {
        // The object is busy, can't write. Save it for later
        //couldNotWrite.insert( pairObj_t(obj->getFilePosition(), obj) );
        couldNotWrite.insert( obj );
        memoryNotWritten += obj->getMRUMemorySize();
      }
    }

    if (obj)
    {
      // NXS needs to flush the writes to file by closing and re-opening the data block.
      // For speed, it is best to do this only once per write dump.
      obj->flushData();
    }

    // Exchange with the new map you built out of the not-written blocks.
    m_writeBuffer.swap(couldNotWrite);
    m_writeBufferUsed = memoryNotWritten;
  }


  //---------------------------------------------------------------------------------------------
  /** Flush out all the data in the memory; and writes out everything in the to-write cache.
   * Mostly used for debugging and unit tests */
  void DiskMRU::flushCache()
  {
    m_mruMutex.lock();

    // Pop everything from the cache
    while (m_mru.size() > 0)
    {
      // Pop the least-used object out the back
      const ISaveable *toWrite = m_mru.back();
      m_mru.pop_back();

      // And put it in the queue of stuff to write.
      m_writeBuffer.insert(toWrite);
    }

    // Track the memory change in the two buffers
    m_writeBufferUsed += m_mruUsed;
    m_mruUsed = 0;

    // Now write everything out.
    writeOldObjects();
    m_mruMutex.unlock();
  }

  //---------------------------------------------------------------------------------------------
  /** This method is called by an ISaveable object that has shrunk
   * and so has left a bit of free space after itself on the file;
   * or when an object gets moved to a new spot.
   *
   * @param pos :: position in the file of the START of the new free block
   * @param size :: size of the free block
   */
  void DiskMRU::freeBlock(uint64_t const pos, uint64_t const size)
  {
    if (size == 0) return;
    m_freeMutex.lock();
    //std::cout << "Freeing block " << pos << " size " << size << std::endl;

    // Make the block
    FreeBlock newBlock(pos, size);
    // Insert it
    std::pair<freeSpace_t::iterator,bool> p = m_free.insert( newBlock );

    // Failed insert? Should not happen since the map is NOT unique
    // Or, if the map has only 1 item then it cannot do any merging. This solves a hanging bug in MacOS. Refs #3652
    if (!p.second || m_free.size() <= 1)
    {
      m_freeMutex.unlock();
      return;
    }

    // This is where we inserted
    freeSpace_t::iterator it = p.first;
    if (it != m_free.begin())
    {
      freeSpace_t::iterator it_before = it; it_before--;
      // There is a block before
      //std::cout << "There is a block before " << pos << std::endl;
      FreeBlock block_before = *it_before;
      if (FreeBlock::merge(block_before, newBlock))
      {
        //std::cout << "Merged with before block" << std::endl;
        // Change the map by replacing the old "before" block with the new merged one
        m_free.replace(it_before, block_before);
        // Remove the block we just inserted
        m_free.erase(it);
        // For cases where the new block was between two blocks.
        newBlock = block_before;
        it = it_before;
      }
    }
    // Get an iterator to the block AFTER this one
    freeSpace_t::iterator it_after = it; it_after++;
    // There is a block after
    if (it_after != m_free.end())
    {
      FreeBlock block_after = *it_after;
      if (FreeBlock::merge(newBlock, block_after))
      {
        // Change the map by replacing the old "new" block with the new merged one
        m_free.replace(it, newBlock);
        // Remove the block that was after this one
        m_free.erase(it_after);
      }
    }

    m_freeMutex.unlock();
  }


  //---------------------------------------------------------------------------------------------
  /** Method that defrags free blocks by combining adjacent ones together
   * NOTE: This is not necessary to run since the freeBlock() methods
   * automatically defrags neighboring blocks.
   */
  void  DiskMRU::defragFreeBlocks()
  {
    m_freeMutex.lock();

    freeSpace_t::iterator it = m_free.begin();
    FreeBlock thisBlock;
    thisBlock = *it;

    while (it != m_free.end())
    {
      // Get iterator to the block after "it".
      freeSpace_t::iterator it_after = it;
      it_after++;
      FreeBlock block_after = *it_after;

      if (FreeBlock::merge(thisBlock, *it_after))
      {
        // Change the map by replacing the old "before" block with the new merged one
        m_free.replace(it, thisBlock);
        // Remove the block that was merged out
        m_free.erase(it_after);
        // And stay at this iterator to
      }
      else
      {
        // Move on to the next block
        it++;
        thisBlock = *it;
      }
    }
    m_freeMutex.unlock();
  }

  //---------------------------------------------------------------------------------------------
  /** Allocate a block of the given size in a free spot in the file,
   * or at the end of the file if there is no space.
   *
   * @param newSize :: new size of the data
   * @return a new position at which the data can be saved.
   */
  uint64_t DiskMRU::allocate(uint64_t const newSize)
  {
    m_freeMutex.lock();

    // Now, find the first available block of sufficient size.
    freeSpace_bySize_t::iterator it;
    bool putAtFileEnd = true;
    if (m_free.size() > 0)
    {
      // Unless there is nothing in the free space map
      it = m_free_bySize.lower_bound( newSize );
      putAtFileEnd = (it == m_free_bySize.end());
    }

    if (putAtFileEnd)
    {
      // No block found
      // Go to the end of the file.
      uint64_t retVal = m_fileLength;
      // And we assume the file will grow by this much.
      m_fileLength += newSize;
      // Will place the new block at the end of the file
      m_freeMutex.unlock();
      return retVal;
    }
    else
    {
//      std::cout << "Block found for allocate " << newSize << std::endl;
      uint64_t foundPos = it->getFilePosition();
      uint64_t foundSize = it->getSize();
      // Remove the free block you found - it is no longer free
      m_free_bySize.erase(it);
      m_freeMutex.unlock();
      // Block was too large - free the bit of space after it.
      if (foundSize > newSize)
      {
        this->freeBlock( foundPos + newSize, foundSize - newSize );
      }
      return foundPos;
    }
  }

  //---------------------------------------------------------------------------------------------
  /** This method is called by an ISaveable object that has outgrown
   * its space allocated on file and needs to relocate.
   *
   * This should only be called when the MRU is saving a block to disk,
   * i.e. the ISaveable cannot be in either the MRU buffer or the toWrite buffer.
   *
   * @param oldPos :: original position in the file
   * @param oldSize :: original size in the file. This will be marked as "free"
   * @param newSize :: new size of the data
   * @return a new position at which the data can be saved.
   */
  uint64_t DiskMRU::relocate(uint64_t const oldPos, uint64_t const oldSize, const uint64_t newSize)
  {
    //std::cout << "Relocating " << oldPos << ", " << oldSize << ", " << newSize << std::endl;
    // First, release the space in the old block.
    this->freeBlock(oldPos, oldSize);
    return this->allocate(newSize);
  }

  //---------------------------------------------------------------------------------------------
  /** Returns a vector with two entries per free block: position and size.
   * @param[out] free :: vector to fill */
  void DiskMRU::getFreeSpaceVector(std::vector<uint64_t> & free) const
  {
    free.reserve( m_free.size() * 2);
    freeSpace_bySize_t::const_iterator it = m_free_bySize.begin();
    freeSpace_bySize_t::const_iterator it_end = m_free_bySize.end();
    for (; it != it_end; it++)
    {
      free.push_back(it->getFilePosition());
      free.push_back(it->getSize());
    }
  }

  /// @return a string describing the memory buffers, for debugging.
  std::string DiskMRU::getMemoryStr() const
  {
    std::ostringstream mess;
    mess << "MRU: " << m_mruUsed << " events in " << m_mru.size() << " blocks. "
        << "Write Buffer: " << m_writeBufferUsed << " in " << m_writeBuffer.size() << " blocks. "
        << "Small objects: " << m_smallBufferSize << " events.";
    return mess.str();
  }



  //---------------------------------------------------------------------------------------------
  /** Sets the total number of objects to consider.
   * This is used by the "small objects" buffer to determine the threshold to allow.
   * @param numObjects
   */
  void DiskMRU::setNumberOfObjects(size_t numObjects)
  {
    m_smallBufferMutex.lock();
    if (m_smallBuffer.size() < numObjects)
      m_smallBuffer.resize(numObjects, 0);
    calcSmallThreshold();
    m_smallBufferMutex.unlock();
  }

  //---------------------------------------------------------------------------------------------
  /** Calculate the threshold # of events below which objects are never
   * cached to disk.
   */
  void DiskMRU::calcSmallThreshold()
  {
    if (m_smallBuffer.size() > 0)
      m_smallThreshold = m_smallBufferSize / m_smallBuffer.size();
    else
      // Something is not set
      m_smallThreshold = 0;
  }


  //---------------------------------------------------------------------------------------------
  /** Returns true if the object is small enough that it should stay
   * in memory and never cache to disk.
   *
   * @param id :: id of the object
   * @param size :: current or new size of the object
   * @return true if the object is small enough to stay in memory
   */
  bool DiskMRU::shouldStayInMemory(size_t id, uint64_t size)
  {
    // Out of bounds? Just return NO
    if (id >= m_smallBuffer.size())
      return false;
    if (size < m_smallThreshold)
    {
      m_smallBufferMutex.lock();
      // Update the entry in the buffer, and the total memory size
      uint32_t & sizeInBuffer = m_smallBuffer[id];
      m_smallBufferUsed -= sizeInBuffer;
      sizeInBuffer = uint32_t(size);
      m_smallBufferUsed += sizeInBuffer;
      m_smallBufferMutex.unlock();
      return true;
    }
    else
    {
      m_smallBufferMutex.lock();
      // Too big - take it out of the tracking buffer it it is in there.
      uint32_t & sizeInBuffer = m_smallBuffer[id];
      m_smallBufferUsed -= sizeInBuffer;
      sizeInBuffer = 0;
      m_smallBufferMutex.unlock();
      return false;
    }
  }



} // namespace Mantid
} // namespace Kernel

