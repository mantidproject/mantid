#include "MantidKernel/DiskBuffer.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DiskBuffer::DiskBuffer()
    : m_writeBufferSize(50), m_writeBufferUsed(0), m_nObjectsToWrite(0),
      m_free(), m_free_bySize(m_free.get<1>()), m_fileLength(0) {
  m_free.clear();
}

//----------------------------------------------------------------------------------------------
/** Constructor
 *
 * @param m_writeBufferSize :: Amount of memory to accumulate in the write
 *buffer before writing.
 * @return
 */
DiskBuffer::DiskBuffer(uint64_t m_writeBufferSize)
    : m_writeBufferSize(m_writeBufferSize), m_writeBufferUsed(0),
      m_nObjectsToWrite(0), m_free(), m_free_bySize(m_free.get<1>()),
      m_fileLength(0) {
  m_free.clear();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
DiskBuffer::~DiskBuffer() {}

//---------------------------------------------------------------------------------------------
/** Call this method when an object is ready to be written
 * out to disk.
 *
 * When the to-write buffer is full, all of it gets written
 * out to disk using writeOldObjects()
 *
 * @param item :: item that can be written to disk.
 */
void DiskBuffer::toWrite(ISaveable *item) {
  if (item == NULL)
    return;
  //    if (!m_useWriteBuffer) return;

  if (item->getBufPostion()) // already in the buffer and probably have changed
                             // its size in memory
  {
    // forget old memory size
    m_mutex.lock();
    m_writeBufferUsed -= item->getBufferSize();
    // add new size
    size_t newMemorySize = item->getDataMemorySize();
    m_writeBufferUsed += newMemorySize;
    m_mutex.unlock();
    item->setBufferSize(newMemorySize);
  } else {
    m_mutex.lock();
    m_toWriteBuffer.push_front(item);
    m_writeBufferUsed += item->setBufferPosition(m_toWriteBuffer.begin());
    m_nObjectsToWrite++;
    m_mutex.unlock();
  }

  // Should we now write out the old data?
  if (m_writeBufferUsed > m_writeBufferSize)
    writeOldObjects();
}

//---------------------------------------------------------------------------------------------
/** Call this method when an object that might be in the cache
 * is getting deleted.
 * The object is removed from the to-write buffer (if present).
 * The space it uses on disk is marked as free.
 *
 * @param item :: ISaveable object that is getting deleted.
 */
void DiskBuffer::objectDeleted(ISaveable *item) {
  if (item == NULL)
    return;
  // have it ever been in the buffer?
  m_mutex.lock();
  auto opt2it = item->getBufPostion();
  if (opt2it) {
    m_writeBufferUsed -= item->getBufferSize();
    m_toWriteBuffer.erase(*opt2it);
  } else {
    m_mutex.unlock();
    return;
  }

  // indicate to the object that it is not stored in memory any more
  item->clearBufferState();
  m_mutex.unlock();

  // Mark the amount of space used on disk as free
  if (item->wasSaved())
    this->freeBlock(item->getFilePosition(), item->getFileSize());
}

//---------------------------------------------------------------------------------------------
/** Method to write out the old objects that have been
 * stored in the "toWrite" buffer.
 */
void DiskBuffer::writeOldObjects() {

  Poco::ScopedLock<Kernel::Mutex> _lock(m_mutex);
  // Holder for any objects that you were NOT able to write.
  std::list<ISaveable *> couldNotWrite;
  size_t objectsNotWritten(0);
  size_t memoryNotWritten(0);

  // Iterate through the list
  auto it = m_toWriteBuffer.begin();
  auto it_end = m_toWriteBuffer.end();

  ISaveable *obj = NULL;

  for (; it != it_end; ++it) {
    obj = *it;
    if (!obj->isBusy()) {
      uint64_t NumObjEvents = obj->getTotalDataSize();
      uint64_t fileIndexStart;
      if (!obj->wasSaved()) {
        fileIndexStart = this->allocate(NumObjEvents);
        // Write to the disk; this will call the object specific save function;
        // Prevent simultaneous file access (e.g. write while loading)
        obj->saveAt(fileIndexStart, NumObjEvents);
      } else {
        uint64_t NumFileEvents = obj->getFileSize();
        if (NumObjEvents != NumFileEvents) {
          // Event list changed size. The MRU can tell us where it best fits
          // now.
          fileIndexStart = this->relocate(obj->getFilePosition(), NumFileEvents,
                                          NumObjEvents);
          // Write to the disk; this will call the object specific save
          // function;
          obj->saveAt(fileIndexStart, NumObjEvents);
        } else // despite object size have not been changed, it can be modified
               // other way. In this case, the method which changed the data
               // should set dataChanged ID
        {
          if (obj->isDataChanged()) {
            fileIndexStart = obj->getFilePosition();
            // Write to the disk; this will call the object specific save
            // function;
            obj->saveAt(fileIndexStart, NumObjEvents);
            // this is questionable operation, which adjust file size in case
            // when the file postions were allocated externaly
            if (fileIndexStart + NumObjEvents > m_fileLength)
              m_fileLength = fileIndexStart + NumObjEvents;
          } else // just clean the object up -- it just occupies memory
            obj->clearDataFromMemory();
        }
      }
      // tell the object that it has been removed from the buffer
      obj->clearBufferState();
    } else // object busy
    {
      // The object is busy, can't write. Save it for later
      couldNotWrite.push_back(obj);
      // When a prefix or postfix operator is applied to a function argument,
      // the value of the argument is
      // NOT GUARANTEED to be incremented or decremented before it is passed to
      // the function.
      std::list<ISaveable *>::iterator it = --couldNotWrite.end();
      memoryNotWritten += obj->setBufferPosition(it);
      objectsNotWritten++;
    }
  }

  // use last object to clear NeXus buffer and actually write data to HDD
  if (obj) {
    // NXS needs to flush the writes to file by closing and re-opening the data
    // block.
    // For speed, it is best to do this only once per write dump, using last
    // object saved
    obj->flushData();
  }

  // Exchange with the new map you built out of the not-written blocks.
  m_toWriteBuffer.swap(couldNotWrite);
  m_writeBufferUsed = memoryNotWritten;
  m_nObjectsToWrite = objectsNotWritten;
}

//---------------------------------------------------------------------------------------------
/** Flush out all the data in the memory; and writes out everything in the
 * to-write cache. */
void DiskBuffer::flushCache() {
  // Now write everything out.
  writeOldObjects();
}

//---------------------------------------------------------------------------------------------
/** This method is called by this->relocate when object that has shrunk
 * and so has left a bit of free space after itself on the file;
 * or when an object gets moved to a new spot.
 *
 * @param pos :: position in the file of the START of the new free block
 * @param size :: size of the free block
 */
void DiskBuffer::freeBlock(uint64_t const pos, uint64_t const size) {
  if (size == 0 || size == std::numeric_limits<uint64_t>::max())
    return;
  m_freeMutex.lock();

  // Make the block
  FreeBlock newBlock(pos, size);
  // Insert it
  std::pair<freeSpace_t::iterator, bool> p = m_free.insert(newBlock);

  // Failed insert? Should not happen since the map is NOT unique
  // Or, if the map has only 1 item then it cannot do any merging. This solves a
  // hanging bug in MacOS. Refs #3652
  if (!p.second || m_free.size() <= 1) {
    m_freeMutex.unlock();
    return;
  }

  // This is where we inserted
  freeSpace_t::iterator it = p.first;
  if (it != m_free.begin()) {
    freeSpace_t::iterator it_before = it;
    --it_before;
    // There is a block before
    FreeBlock block_before = *it_before;
    if (FreeBlock::merge(block_before, newBlock)) {
      // Change the map by replacing the old "before" block with the new merged
      // one
      m_free.replace(it_before, block_before);
      // Remove the block we just inserted
      m_free.erase(it);
      // For cases where the new block was between two blocks.
      newBlock = block_before;
      it = it_before;
    }
  }
  // Get an iterator to the block AFTER this one
  freeSpace_t::iterator it_after = it;
  ++it_after;
  // There is a block after
  if (it_after != m_free.end()) {
    FreeBlock block_after = *it_after;
    if (FreeBlock::merge(newBlock, block_after)) {
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
void DiskBuffer::defragFreeBlocks() {
  m_freeMutex.lock();

  freeSpace_t::iterator it = m_free.begin();
  FreeBlock thisBlock;
  thisBlock = *it;

  while (it != m_free.end()) {
    // Get iterator to the block after "it".
    freeSpace_t::iterator it_after = it;
    ++it_after;
    FreeBlock block_after = *it_after;

    if (FreeBlock::merge(thisBlock, *it_after)) {
      // Change the map by replacing the old "before" block with the new merged
      // one
      m_free.replace(it, thisBlock);
      // Remove the block that was merged out
      m_free.erase(it_after);
      // And stay at this iterator to
    } else {
      // Move on to the next block
      ++it;
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
uint64_t DiskBuffer::allocate(uint64_t const newSize) {
  m_freeMutex.lock();

  // Now, find the first available block of sufficient size.
  freeSpace_bySize_t::iterator it;
  bool putAtFileEnd = true;
  if (m_free.size() > 0) {
    // Unless there is nothing in the free space map
    it = m_free_bySize.lower_bound(newSize);
    putAtFileEnd = (it == m_free_bySize.end());
  }

  if (putAtFileEnd) {
    // No block found
    // Go to the end of the file.
    uint64_t retVal = m_fileLength;
    // And we assume the file will grow by this much.
    m_fileLength += newSize;
    // Will place the new block at the end of the file
    m_freeMutex.unlock();
    return retVal;
  } else {
    //      std::cout << "Block found for allocate " << newSize << std::endl;
    uint64_t foundPos = it->getFilePosition();
    uint64_t foundSize = it->getSize();
    // Remove the free block you found - it is no longer free
    m_free_bySize.erase(it);
    m_freeMutex.unlock();
    // Block was too large - free the bit of space after it.
    if (foundSize > newSize) {
      this->freeBlock(foundPos + newSize, foundSize - newSize);
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
uint64_t DiskBuffer::relocate(uint64_t const oldPos, uint64_t const oldSize,
                              const uint64_t newSize) {
  // std::cout << "Relocating " << oldPos << ", " << oldSize << ", " << newSize
  // << std::endl;
  // First, release the space in the old block.
  this->freeBlock(oldPos, oldSize);
  return this->allocate(newSize);
}

//---------------------------------------------------------------------------------------------
/** Returns a vector with two entries per free block: position and size.
 * @param[out] free :: vector to fill */
void DiskBuffer::getFreeSpaceVector(std::vector<uint64_t> &free) const {
  free.reserve(m_free.size() * 2);
  freeSpace_bySize_t::const_iterator it = m_free_bySize.begin();
  freeSpace_bySize_t::const_iterator it_end = m_free_bySize.end();
  for (; it != it_end; ++it) {
    free.push_back(it->getFilePosition());
    free.push_back(it->getSize());
  }
}

/** Sets the free space map. Should only be used when loading a file.
 * @param[in] free :: vector containing free space index to set */
void DiskBuffer::setFreeSpaceVector(std::vector<uint64_t> &free) {
  m_free.clear();

  if (free.size() % 2 != 0)
    throw std::length_error("Free vector size is not a factor of 2.");

  for (std::vector<uint64_t>::iterator it = free.begin(); it != free.end();
       it += 2) {
    std::vector<uint64_t>::iterator it_next = boost::next(it);

    if (*it == 0 && *it_next == 0) {
      continue; // Not really a free space block!
    }

    FreeBlock newBlock(*it, *it_next);
    m_free.insert(newBlock);
  }
}

/// @return a string describing the memory buffers, for debugging.
std::string DiskBuffer::getMemoryStr() const {
  std::ostringstream mess;
  mess << "Buffer: " << m_writeBufferUsed << " in " << m_nObjectsToWrite
       << " objects. ";
  return mess.str();
}

} // namespace Mantid
} // namespace Kernel
