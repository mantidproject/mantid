#include "MantidKernel/DiskMRU.h"
#include "MantidKernel/System.h"
#include <iostream>

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DiskMRU::DiskMRU()
  : m_memoryAvail(100),
    m_memoryUsed(0),
    m_useWriteBuffer(false),
    m_writeBufferSize(50),
    m_toWrite_byId( m_toWrite.get<1>() ),
    m_memoryToWrite(0),
    m_free_bySize( m_free.get<1>() ),
    m_file(NULL),
    m_fileUsed(0),
    m_fileLength(0)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_memoryAvail :: Amount of memory that the MRU is allowed to use.
   * @param m_writeBufferSize :: Amount of memory to accumulate in the write buffer before writing.
   * @param useWriteBuffer :: True if you want to use the "to-Write" buffer.
   * @param file :: ptr to a IFile object that allows use to resize the file.
   * @return
   */
  DiskMRU::DiskMRU(uint64_t m_memoryAvail, uint64_t m_writeBufferSize, bool useWriteBuffer, IFile * file)
  : m_memoryAvail(m_memoryAvail),
    m_memoryUsed(0),
    m_useWriteBuffer(useWriteBuffer),
    m_writeBufferSize(m_writeBufferSize),
    m_toWrite_byId( m_toWrite.get<1>() ),
    m_memoryToWrite(0),
    m_free_bySize( m_free.get<1>() ),
    m_file(file),
    m_fileUsed(0),
    m_fileLength(0)
  {
    if (m_file)
    {
      m_fileLength = m_file->getFileLength();
      m_fileUsed = m_fileLength;
    }
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
    if (m_useWriteBuffer) return loadingWithWriteBuffer(item);

//    std::cout << "Loading " << item->getId() << std::endl;
    m_mruMutex.lock();

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
    m_memoryUsed += item->getSizeOnFile();

    // You might have to pop 1 or more items until the MRU memory is below the limit
    mru_t::iterator it = m_mru.end();

    while (m_memoryUsed > m_memoryAvail)
    {
      // Pop the least-used object out the back
      it--;
      if (it == m_mru.begin()) break;
      const ISaveable *toWrite = *it;
      // Can you save it to disk?
      if (!toWrite->dataBusy())
      {
        toWrite->save();
        m_memoryUsed -= toWrite->getSizeOnFile();
        m_mru.erase(it);
      }
    }
    m_mruMutex.unlock();
  }


  //---------------------------------------------------------------------------------------------
  /** Tell the MRU that we are loading the given item.
   * Use the methods that keep a buffer of items to write and
   * and writes them as a block.
   *
   * @param item :: item that is
   * @param memory :: memory that the object will use.
   */
  void DiskMRU::loadingWithWriteBuffer(const ISaveable * item)
  {
    m_mruMutex.lock();

    // Place the item in the MRU list
    std::pair<mru_t::iterator,bool> p;
    p = m_mru.push_front(item);

    // Find the item in the toWrite buffer (using the item's ID)
    toWriteMap_by_Id_t::iterator found = m_toWrite_byId.find( item->getId() );
    if (found != m_toWrite_byId.end())
    {
      m_toWrite_byId.erase(item->getId());
      m_memoryToWrite -= item->getSizeOnFile();
    }

    if (!p.second)
    {
      // duplicate item: put it back at the front of the list
      m_mru.relocate(m_mru.begin(), p.first);
      m_mruMutex.unlock();
      return;
    }

    // We are now using more memory.
    m_memoryUsed += item->getSizeOnFile();

    if (m_memoryUsed > m_memoryAvail)
    {
      // You might have to pop 1 or more items until the MRU memory is below the limit
      while (m_memoryUsed > m_memoryAvail)
      {
        // Pop the least-used object out the back
        const ISaveable *toWrite = m_mru.back();
        m_mru.pop_back();

        // And put it in the queue of stuff to write.
        m_toWrite.insert(toWrite);
        //m_toWrite.insert( pairObj_t(toWrite->getFilePosition(), toWrite) );

        // Track the memory change in the two buffers
        size_t thisMem = toWrite->getSizeOnFile();
        m_memoryToWrite += thisMem;
        m_memoryUsed -= thisMem;
      }

      // Should we now write out the old data?
      if (m_memoryToWrite >= m_writeBufferSize)
        writeOldObjects();
    }
    m_mruMutex.unlock();
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
      const ISaveable * obj = *it; //->second;
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
        memoryNotWritten += obj->getSizeOnFile();
      }
    }

    // Exchange with the new map you built out of the not-written blocks.
    m_toWrite.swap(couldNotWrite);
    m_memoryToWrite = memoryNotWritten;
  }


  //---------------------------------------------------------------------------------------------
  /** Flush out all the data in the memory; and writes out everything in the to-write cache.
   * Mostly used for debugging and unit tests */
  void DiskMRU::flushCache()
  {
    // Pop everything from the cache
    while (m_memoryUsed > 0)
    {
      // Pop the least-used object out the back
      const ISaveable *toWrite = m_mru.back();
      m_mru.pop_back();

      // And put it in the queue of stuff to write.
      m_toWrite.insert(toWrite);

      // Track the memory change in the two buffers
      size_t thisMem = toWrite->getSizeOnFile();
      m_memoryToWrite += thisMem;
      m_memoryUsed -= thisMem;
    }
    // Now write everything out.
    writeOldObjects();
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

    // Make the block
    FreeBlock newBlock(pos, size);
    // Insert it
    std::pair<freeSpace_t::iterator,bool> p = m_free.insert( newBlock );
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
    if (it != m_free.end())
    {
      // There is a block after
      freeSpace_t::iterator it_after = it; it_after++;
      //std::cout << "There is a block after " << pos << std::endl;
      FreeBlock block_after = *it_after;
      if (FreeBlock::merge(newBlock, block_after))
      {
        //std::cout << "Merged with after block" << std::endl;
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
    freeSpace_by_size_t::iterator it = m_free_bySize.lower_bound( newSize );
    if (it == m_free_bySize.end())
    {
      // No block found
//      std::cout << "No block found for allocate " << newSize << std::endl;
      if (m_file)
      {
        // Go at the end of the file.
        uint64_t retVal = m_fileUsed;
        m_fileUsed += newSize;
        if (m_fileUsed > m_fileLength)
        {
          // Grow the file by 10% more than the requested size,
          // each time it grows. This avoids too many calls to extend,
          // at the cost of up to 10% wasted space.
          m_fileLength = uint64_t( double(m_fileUsed) * 1.1);
          m_file->extendFile(m_fileLength);
        }
        // Will place the new block at the end of the file
        m_freeMutex.unlock();
        return retVal;
      }
      else
      {
        m_freeMutex.unlock();
        throw std::runtime_error("DiskMRU::allocate(): No free block is large enough, and you did not give a IFile object to allow extending the file. Cannot relocate.");
      }
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
    // First, release the space in the old block.
    this->freeBlock(oldPos, oldSize);
    return this->allocate(newSize);
  }

} // namespace Mantid
} // namespace Kernel

