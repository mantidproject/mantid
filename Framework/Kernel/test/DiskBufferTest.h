// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/FreeBlock.h"
#include "MantidKernel/ISaveable.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Timer.h"
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using Mantid::Kernel::CPUTimer;

//====================================================================================
/** An ISaveable that fakes writing to a fixed-size file */
class SaveableTesterWithFile : public ISaveable {
public:
  SaveableTesterWithFile(uint64_t pos, uint64_t size, char ch, bool wasSaved = true)
      : ISaveable(), m_memory(size), m_ch(ch) {
    // the object knows its place on file
    this->setFilePosition(pos, size, wasSaved);
    this->setLoaded(true);
  }
  // this is testing/special routine
  void setSaved(bool On = true) { this->m_wasSaved = On; }

  ~SaveableTesterWithFile() override = default;

  void clearDataFromMemory() override {
    this->setLoaded(false);
    m_memory = 0;
  }

  uint64_t m_memory;
  uint64_t getTotalDataSize() const override {
    if (this->wasSaved()) {
      if (this->isLoaded())
        return m_memory;
      else
        return m_memory + this->getFileSize();
    } else
      return m_memory;
  };
  size_t getDataMemorySize() const override { return size_t(m_memory); }

  void AddNewObjects(uint64_t nNewObj) {
    if (this->wasSaved()) {
      if (this->isLoaded()) {
        m_memory += nNewObj;
      } else {
        m_memory = nNewObj;
      }
    } else
      m_memory += nNewObj;
  }

  char m_ch;
  void save() const override {
    // Fake writing to a file
    streamMutex.lock();
    uint64_t mPos = this->getFilePosition();
    uint64_t mMem = this->getTotalDataSize();
    if (fakeFile.size() < mPos + mMem)
      fakeFile.resize(mPos + mMem, ' ');

    for (size_t i = mPos; i < mPos + mMem; i++)
      fakeFile[i] = m_ch;

    streamMutex.unlock();
    // this is important function call which has to be implemented by any save
    // function
    this->m_wasSaved = true;
  }

  void load() override {
    if (this->wasSaved() && !this->isLoaded()) {
      m_memory += this->getFileSize();
    }
    // this is important function call which has to be implemented by any load
    // function
    this->setLoaded(true);
  }
  void flushData() const override {}

  static std::string fakeFile;
  static std::mutex streamMutex;
};

// Declare the static members here.
std::string SaveableTesterWithFile::fakeFile;
std::mutex SaveableTesterWithFile::streamMutex;

//====================================================================================
class DiskBufferTest : public CxxTest::TestSuite {
public:
  std::vector<SaveableTesterWithFile *> data;
  size_t num;

  void setUp() override {
    // Create the ISaveables
    num = 10;
    SaveableTesterWithFile::fakeFile = "";
    data.clear();
    for (size_t i = 0; i < num; i++)
      data.emplace_back(new SaveableTesterWithFile(uint64_t(2 * i), 2, char(i + 0x41)));
  }

  void tearDown() override {
    for (auto &i : data) {
      delete i;
    }
  }
  void xest_nothing() { TS_WARN("Tests here were disabled for the time being"); }

  /** Extreme case with nothing writable but exceeding the writable buffer */
  void test_noWriteBuffer_nothingWritable() {
    // Room for 4 in the write buffer
    DiskBuffer dbuf(4);
    for (size_t i = 0; i < 9; i++) {
      data[i]->setBusy(true);
      dbuf.toWrite(data[i]);
    }
    // We ended up with too much in the buffer since nothing could be written.
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 2 * 9);
    // Let's make it all writable
    for (size_t i = 0; i < 9; i++) {
      data[i]->setBusy(false);
      data[i]->setDataChanged();
    }
    // Trigger a write
    data[9]->setDataChanged();
    dbuf.toWrite(data[9]);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);
    // And all of these get written out at once
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AABBCCDDEEFFGGHHIIJJ");
  }

  /** Extreme case with nothing writable but exceeding the writable buffer */
  void test_noWriteBuffer_nothingWritableWasSaved() {
    // Room for 4 in the write buffer
    DiskBuffer dbuf(4);
    for (size_t i = 0; i < 10; i++) {
      data[i]->setBusy(true);
      data[i]->setSaved(false);
      dbuf.toWrite(data[i]);
    }
    // We ended up with too much in the buffer since nothing could be written.
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 20);
    // Let's make it all writable
    for (size_t i = 0; i < 9; i++)
      data[i]->setBusy(false);
    // Trigger a write
    dbuf.toWrite(data[9]);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 2);
    // And all of these get written out at once
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "IIHHGGFFEEDDCCBBAA");
  }

  ////--------------------------------------------------------------------------------
  ///** Sorts by file position when writing to a file */
  void test_writesOutInFileOrder() {
    for (auto &i : data) {
      i->setDataChanged();
    }
    // Room for 2 objects of size 2 in the to-write cache
    DiskBuffer dbuf(2 * 2);
    // These 3 will get written out
    dbuf.toWrite(data[5]);
    dbuf.toWrite(data[1]);
    dbuf.toWrite(data[9]);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);

    // 0 1 2 3 4 5 6 7 8 9
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "  BB      FF      JJ");
    // These 4 at the end will be in the cache
    dbuf.toWrite(data[2]);
    dbuf.toWrite(data[3]);
    dbuf.toWrite(data[4]);
    dbuf.toWrite(data[6]);

    // 1 left in the buffer
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 2);

    // The "file" was written out this way (sorted by file position):
    // 0 1 2 3 4 5 6 7 8 9
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "  BBCCDDEEFF      JJ");
  }

  //--------------------------------------------------------------------------------
  /** If a block will get deleted it needs to be taken
   * out of the caches */
  void test_objectDeleted() {
    // Room for 6 objects of 2 in the to-write cache
    DiskBuffer dbuf(12);
    // Fill the buffer
    for (size_t i = 0; i < 5; i++) {
      dbuf.toWrite(data[i]);
      data[i]->setDataChanged();
    }
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 10);

    // First let's get rid of something in to to-write buffer

    dbuf.objectDeleted(data[1]);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 8);
    TSM_ASSERT_EQUALS("The data marked as been saved, so delete should free this", dbuf.getFreeSpaceMap().size(), 1);

    dbuf.flushCache();
    // This triggers a write. 1 is no longer in the to-write buffer
    //                                               "0,  2,3,4,"
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AA  CCDDEE");
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);

    // assume now we have loaded the data back; (THIS MAY BE WRONG THOURH NOT
    // AFFECT FURTHER TESTS)
    size_t ic(0);
    for (size_t i = 0; i < 5; i++) {
      if (i == 1)
        continue;
      data[i]->setFilePosition(2 * ic, 2, true);
      data[i]->m_memory = 2;
      data[i]->setDataChanged();
      dbuf.toWrite(data[i]);
      ic++;
    }

    dbuf.objectDeleted(data[2]);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 6);
    TSM_ASSERT_EQUALS("It is still free space mapping the data on hdd", dbuf.getFreeSpaceMap().size(), 2);
    TSM_ASSERT_EQUALS(" and file is still the same size: ", dbuf.getFileLength(), 10);
  }

  //--------------------------------------------------------------------------------
  /** Accessing the map from multiple threads simultaneously does not segfault
   */
  void test_thread_safety() {
    // Room for 3 in the to-write cache
    DiskBuffer dbuf(3);
    size_t bigNum = 1000;
    std::vector<ISaveable *> bigData;
    bigData.reserve(bigNum);
    for (size_t i = 0; i < bigNum; i++)
      bigData.emplace_back(new SaveableTesterWithFile(2 * i, 2, char(i + 0x41)));

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < int(bigNum); i++) {
      dbuf.toWrite(bigData[i]);
    }
    // std::cout << ISaveableTester::fakeFile << '\n';
    for (size_t i = 0; i < size_t(bigNum); i++)
      delete bigData[i];
  }
  ////--------------------------------------------------------------------------------
  ////--------------------------------------------------------------------------------
  ////----------TESTS FOR FREE SPACE MAPS
  ///--------------------------------------------
  ////--------------------------------------------------------------------------------
  ////--------------------------------------------------------------------------------
  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithPrevious() {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    TS_ASSERT_EQUALS(map.size(), 0);
    dbuf.freeBlock(0, 50);
    TS_ASSERT_EQUALS(map.size(), 1);
    // zero-sized free block does nothing
    dbuf.freeBlock(1234, 0);
    TS_ASSERT_EQUALS(map.size(), 1);
    dbuf.freeBlock(100, 50);
    TS_ASSERT_EQUALS(map.size(), 2);
    // Free a block next to another one, AFTER
    dbuf.freeBlock(150, 50);
    TSM_ASSERT_EQUALS("Map remained the same size because adjacent blocks were merged", map.size(), 2);

    // Get a vector of the free blocks and sizes
    std::vector<uint64_t> free;
    dbuf.getFreeSpaceVector(free);
    TS_ASSERT_EQUALS(free[0], 0);
    TS_ASSERT_EQUALS(free[1], 50);
    TS_ASSERT_EQUALS(free[2], 100);
    TS_ASSERT_EQUALS(free[3], 100);
  }

  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithNext() {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    dbuf.freeBlock(0, 50);
    dbuf.freeBlock(200, 50);
    TS_ASSERT_EQUALS(map.size(), 2);
    // Free a block next to another one, BEFORE
    dbuf.freeBlock(150, 50);
    TSM_ASSERT_EQUALS("Map remained the same size because adjacent blocks were merged", map.size(), 2);

    // Get the 2nd free block.
    DiskBuffer::freeSpace_t::iterator it = map.begin();
    it++;
    b = *it;
    TS_ASSERT_EQUALS(b.getFilePosition(), 150);
    TS_ASSERT_EQUALS(b.getSize(), 100);

    dbuf.freeBlock(50, 50);
    TSM_ASSERT_EQUALS("Map remained the same size because adjacent blocks were merged", map.size(), 2);
    TS_ASSERT_EQUALS(map.begin()->getSize(), 100);
  }

  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithBothNeighbours() {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    dbuf.freeBlock(0, 50);
    dbuf.freeBlock(200, 50);
    dbuf.freeBlock(300, 50);
    dbuf.freeBlock(400, 50); // Disconnected 4th one
    TS_ASSERT_EQUALS(map.size(), 4);
    // Free a block between two block
    dbuf.freeBlock(250, 50);
    TSM_ASSERT_EQUALS("Map shrank because three blocks were merged", map.size(), 3);

    // Get the 2nd free block.
    DiskBuffer::freeSpace_t::iterator it = map.begin();
    it++;
    b = *it;
    TS_ASSERT_EQUALS(b.getFilePosition(), 200);
    TS_ASSERT_EQUALS(b.getSize(), 150);
  }

  /** Add blocks to the free block list in parallel threads,
   * should not segfault or anything */
  void test_freeBlock_threadSafety() {
    DiskBuffer dbuf(0);
    PRAGMA_OMP( parallel for)
    for (int i = 0; i < 10000; i++) {
      dbuf.freeBlock(uint64_t(i) * 100, (i % 3 == 0) ? 100 : 50);
    }
    // 1/3 of the blocks got merged
    TS_ASSERT_EQUALS(dbuf.getFreeSpaceMap().size(), 6667);
  }

  ///** Disabled because it is not necessary to defrag since that happens on the
  /// fly */
  // void xtest_defragFreeBlocks()
  //{
  //  DiskBuffer dbuf(3);
  //  DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
  //  FreeBlock b;

  //  dbuf.freeBlock(0, 50);
  //  dbuf.freeBlock(100, 50);
  //  dbuf.freeBlock(150, 50);
  //  dbuf.freeBlock(500, 50);
  //  dbuf.freeBlock(550, 50);
  //  dbuf.freeBlock(600, 50);
  //  dbuf.freeBlock(650, 50);
  //  dbuf.freeBlock(1000, 50);
  //  TS_ASSERT_EQUALS( map.size(), 8);

  //  dbuf.defragFreeBlocks();
  //  TS_ASSERT_EQUALS( map.size(), 4);
  //}

  /// You can call relocate() if an block is shrinking.
  void test_relocate_when_shrinking() {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    // You stay in the same place because that's the only free spot.
    TS_ASSERT_EQUALS(dbuf.relocate(100, 10, 5), 100);
    // You left a free block at 105.
    TS_ASSERT_EQUALS(map.size(), 1);
    // This one, instead of staying in place, will fill in that previously freed
    // 5-sized block
    //  since that's the smallest one that fits the whole block.
    TS_ASSERT_EQUALS(dbuf.relocate(200, 10, 5), 105);
    // Still one free block, but its at 200-209 now.
    TS_ASSERT_EQUALS(map.size(), 1);
  }

  /// You can call relocate() if an block is shrinking.
  void test_relocate_when_growing() {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    dbuf.freeBlock(200, 20);
    dbuf.freeBlock(300, 30);
    TS_ASSERT_EQUALS(map.size(), 2);

    // Grab the smallest block that's big enough
    TS_ASSERT_EQUALS(dbuf.relocate(100, 10, 20), 200);
    // You left a free block at 100 of size 10 to replace that one.
    TS_ASSERT_EQUALS(map.size(), 2);
    // A zero-sized block is "relocated" by basically allocating it to the free
    // spot
    TS_ASSERT_EQUALS(dbuf.relocate(100, 0, 5), 100);
    TS_ASSERT_EQUALS(map.size(), 2);
  }

  /// Various tests of allocating and relocating
  void test_allocate_from_empty_freeMap() {
    DiskBuffer dbuf(3);
    dbuf.setFileLength(1000); // Lets say the file goes up to 1000
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    FreeBlock b;
    TS_ASSERT_EQUALS(map.size(), 0);
    // No free blocks? End up at the end
    TS_ASSERT_EQUALS(dbuf.allocate(20), 1000);
    TS_ASSERT_EQUALS(dbuf.getFileLength(), 1020);

    for (size_t i = 0; i < 100000; i++)
      dbuf.allocate(20);

    DiskBuffer mru2;
    mru2.setFileLength(1000);
    for (size_t i = 0; i < 100000; i++)
      mru2.allocate(20);
  }

  /// Various tests of allocating and relocating
  void test_allocate_and_relocate() {
    DiskBuffer dbuf(3);
    dbuf.setFileLength(1000); // Lets say the file goes up to 1000
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    dbuf.freeBlock(100, 10);
    dbuf.freeBlock(200, 20);
    dbuf.freeBlock(300, 30);
    dbuf.freeBlock(400, 40);
    TS_ASSERT_EQUALS(map.size(), 4);
    // Where does the block end up?
    TS_ASSERT_EQUALS(dbuf.allocate(20), 200);
    // The map has shrunk by one since the new one was removed.
    TS_ASSERT_EQUALS(map.size(), 3);
    // OK, now look for a smaller block, size of 4
    TS_ASSERT_EQUALS(dbuf.allocate(4), 100);
    // This left a little chunk of space free, sized 6 at position 104. So the #
    // of entries in the free space map did not change
    TS_ASSERT_EQUALS(map.size(), 3);
    TS_ASSERT_EQUALS(map.begin()->getFilePosition(), 104);
    TS_ASSERT_EQUALS(map.begin()->getSize(), 6);

    // Now try to relocate. Had a block after a 30-sized free block at 300.
    // It gets freed, opening up a slot for the new chunk of memory
    TS_ASSERT_EQUALS(dbuf.relocate(330, 5, 35), 300);
    // One fewer free block.
    TS_ASSERT_EQUALS(map.size(), 2);

    // Ok, now lets ask for a block that is too big. It puts us at the end of
    // the file
    TS_ASSERT_EQUALS(dbuf.allocate(55), 1000);
    TS_ASSERT_EQUALS(dbuf.getFileLength(), 1055);
  }

  //// Test for setting the DiskBuffer
  void test_set_free_space_blocks() {
    DiskBuffer dbuf(0);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();

    uint64_t freeSpaceBlocksArray[] = {1, 3, 6, 5};
    std::vector<uint64_t> freeSpaceBlocksVector(freeSpaceBlocksArray,
                                                freeSpaceBlocksArray + sizeof(freeSpaceBlocksArray) / sizeof(uint64_t));
    dbuf.setFreeSpaceVector(freeSpaceBlocksVector);

    TS_ASSERT_EQUALS(map.size(), 2);

    std::vector<uint64_t> assignedVector;
    dbuf.getFreeSpaceVector(assignedVector);
    TS_ASSERT_EQUALS(assignedVector, freeSpaceBlocksVector);
  }

  //// Test for setting the DiskBuffer with an invalid vector
  void test_set_free_space_blocks_with_odd_sized_vector_throws_exception() {
    DiskBuffer dbuf(0);

    uint64_t freeSpaceBlocksArray[] = {1, 3, 6};
    std::vector<uint64_t> freeSpaceBlocksVector(freeSpaceBlocksArray,
                                                freeSpaceBlocksArray + sizeof(freeSpaceBlocksArray) / sizeof(uint64_t));

    TS_ASSERT_THROWS(dbuf.setFreeSpaceVector(freeSpaceBlocksVector), const std::length_error &);
  }

  //// Test for setting the DiskBuffer with a zero sized vector
  void test_set_free_space_blocks_with_zero_sized_vector() {
    DiskBuffer dbuf(0);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();

    std::vector<uint64_t> freeSpaceBlocksVector;

    dbuf.setFreeSpaceVector(freeSpaceBlocksVector);

    TS_ASSERT_EQUALS(map.size(), 0);
  }

  ////--------------------------------------------------------------------------------
  ////--------------------------------------------------------------------------------
  ////--------------------------------------------------------------------------------
  ////--------------------------------------------------------------------------------

  void test_allocate_with_file_manually() {
    // Start by faking a file
    SaveableTesterWithFile *blockA = new SaveableTesterWithFile(0, 2, 'A');
    SaveableTesterWithFile *blockB = new SaveableTesterWithFile(2, 3, 'B');
    SaveableTesterWithFile *blockC = new SaveableTesterWithFile(5, 5, 'C');
    blockA->save();
    blockB->save();
    blockC->save();
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AABBBCCCCC");

    DiskBuffer dbuf(3);
    dbuf.setFileLength(10);
    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();
    uint64_t newPos;

    // File lengths are known correctly
    TS_ASSERT_EQUALS(dbuf.getFileLength(), 10);

    // Asking for a new chunk of space that needs to be at the end
    // This all now happens inside the writeBuffer
    uint64_t oldMem = blockB->getTotalDataSize();
    blockB->AddNewObjects(4);
    uint64_t mPos = blockB->getFilePosition();
    uint64_t newMem = blockB->getTotalDataSize();
    newPos = dbuf.relocate(mPos, oldMem, newMem);
    TSM_ASSERT_EQUALS("One freed block", map.size(), 1);
    TS_ASSERT_EQUALS(dbuf.getFileLength(), 17);

    // Simulate saving
    blockB->setFilePosition(newPos, 7, true);
    blockB->save();
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AABBBCCCCCBBBBBBB");

    // Now let's allocate a new block
    newPos = dbuf.allocate(2);
    TS_ASSERT_EQUALS(newPos, 2);
    SaveableTesterWithFile *blockD = new SaveableTesterWithFile(newPos, 2, 'D');
    blockD->save();
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AADDBCCCCCBBBBBBB");
    TSM_ASSERT_EQUALS("Still one freed block", map.size(), 1);

    // Grow blockD by 1
    blockD->AddNewObjects(1);
    newPos = dbuf.relocate(2, 2, 3);
    TSM_ASSERT_EQUALS("Block D stayed in the same place since there was room after it", newPos, 2);
    blockD->setFilePosition(newPos, 3, true);
    blockD->save();
    dbuf.flushCache();
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AADDDCCCCCBBBBBBB");

    // Allocate a little block at the end
    newPos = dbuf.allocate(1);
    TSM_ASSERT_EQUALS("The new block went to the end of the file", newPos, 17);
    // Which is now longer by 1
    TS_ASSERT_EQUALS(dbuf.getFileLength(), 18);

    delete blockA;
    delete blockB;
    delete blockC;
    delete blockD;
    // std::cout <<  SaveableTesterWithFile::fakeFile << "!\n";
  }

  void test_allocate_with_file() {
    SaveableTesterWithFile::fakeFile = "";
    // filePosition has to be identified by the fileBuffer
    uint64_t filePos = std::numeric_limits<uint64_t>::max();
    // Start by faking a file
    SaveableTesterWithFile *blockA = new SaveableTesterWithFile(filePos, 2, 'A', false);
    SaveableTesterWithFile *blockB = new SaveableTesterWithFile(filePos, 3, 'B', false);
    SaveableTesterWithFile *blockC = new SaveableTesterWithFile(filePos, 5, 'C', false);

    DiskBuffer dbuf(3);
    dbuf.toWrite(blockB);
    dbuf.toWrite(blockA);
    dbuf.toWrite(blockC);
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AABBBCCCCC");

    DiskBuffer::freeSpace_t &map = dbuf.getFreeSpaceMap();

    // Asking for a new chunk of space that needs to be at the end
    blockB->AddNewObjects(4);
    dbuf.toWrite(blockB);
    TSM_ASSERT_EQUALS("One freed block", map.size(), 1);
    TS_ASSERT_EQUALS(dbuf.getFileLength(), 17);
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AABBBCCCCCBBBBBBB");
    // Simulate saving

    dbuf.toWrite(blockB);
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AABBBCCCCCBBBBBBB");
    TS_ASSERT(!blockB->isDataChanged())

    //// Now let's allocate a new block
    SaveableTesterWithFile *blockD = new SaveableTesterWithFile(filePos, 2, 'D', false);
    dbuf.toWrite(blockD);
    // small block, nothing still sitting in the buffer
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AABBBCCCCCBBBBBBB");
    // this will remove block from the cash and place the file to sutable
    // position
    dbuf.flushCache();
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AADDBCCCCCBBBBBBB");
    TSM_ASSERT_EQUALS("Still one freed block", map.size(), 1);

    //// Grow blockD by 1
    blockD->AddNewObjects(1);
    dbuf.toWrite(blockD);
    // nothing happens with file
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AADDBCCCCCBBBBBBB");
    // trigger save as object will stay in buffer otherwise (only 1 block is im
    // memory)
    dbuf.flushCache();
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AADDDCCCCCBBBBBBB");
    TSM_ASSERT_EQUALS("Nothing left one freed block", map.size(), 0);

    //// Allocate a little block at the end
    blockD->AddNewObjects(1);
    dbuf.toWrite(blockD);
    // nothing have changed as only 1 part of the object is in the memory and 3
    // are already on HDD
    TS_ASSERT_EQUALS(SaveableTesterWithFile::fakeFile, "AADDDCCCCCBBBBBBB");
    TSM_ASSERT_EQUALS("Nothing left one freed block", map.size(), 0);
    // trigger save as object will stay in buffer otherwise (only 1 block is im
    // memory)
    dbuf.flushCache();
    TSM_ASSERT_EQUALS("The new block went to the end of the file", SaveableTesterWithFile::fakeFile,
                      "AADDDCCCCCBBBBBBBDDDD");
    TS_ASSERT_EQUALS(dbuf.getFileLength(), 21);
    TSM_ASSERT_EQUALS("Nothing left one freed block", map.size(), 1);

    delete blockA;
    delete blockB;
    delete blockC;
    delete blockD;
    // std::cout <<  ISaveableTesterWithFile::fakeFile << "!\n";
  }
};
//====================================================================================
// THIS TEST DOES NOT PROBABLY EXIST IN A WHILD ANY MORE; LEFT JUST IN CASE
//====================================================================================
//====================================================================================
/** An Saveable that will fake seeking to disk */
class SaveableTesterWithSeek : public ISaveable {
  size_t m_memory;

public:
  SaveableTesterWithSeek(size_t id) : ISaveable() {
    m_memory = 1;
    this->setFilePosition(10 + id, this->m_memory, true);
  }

  /// Method to flush the data to disk and ensure it is written.
  void flushData() const override {};
  /** @return the amount of memory that the object takes as a whole.
      For filebased objects it should be the amount the object occupies in
     memory plus the size it occupies in file if the object has not been fully
     loaded
      or modified.
     * If the object has never been loaded, this should be equal to number of
     data points in the file
     */
  uint64_t getTotalDataSize() const override { return m_memory; }
  /// the data size kept in memory
  size_t getDataMemorySize() const override { return m_memory; };

  virtual void load(DiskBuffer & /*dbuf*/) {
    uint64_t myFilePos = this->getFilePosition();
    // std::cout << "Block " << getFileId() << " loading at " << myFilePos <<
    // '\n';
    SaveableTesterWithSeek::fakeSeekAndWrite(myFilePos);
    this->setLoaded(true);
  }

  void save() const override {
    // Pretend to seek to the point and write
    uint64_t myFilePos = this->getFilePosition();
    // std::cout << "Block " << getFileId() << " saving at " << myFilePos <<
    // '\n';
    fakeSeekAndWrite(myFilePos);
  }
  void clearDataFromMemory() override {
    m_memory = 0;
    this->setLoaded(false);
  }

  void grow(DiskBuffer &dbuf, bool /*tellMRU*/) {
    // OK first you seek to where the OLD data was and load it.
    uint64_t myFilePos = this->getFilePosition();
    // std::cout << "Block " << getFileId() << " loading at " << myFilePos <<
    // '\n';
    SaveableTesterWithSeek::fakeSeekAndWrite(myFilePos);
    // Simulate that the data is growing and so needs to be written out
    size_t newfilePos = dbuf.relocate(myFilePos, m_memory, m_memory + 1);
    // std::cout << "Block " << getFileId() << " has moved from " << myFilePos
    // << " to " << newfilePos << '\n';
    myFilePos = newfilePos;
    // Grow the size by 1
    m_memory++;

    this->setFilePosition(myFilePos, m_memory, true);
  }

  /// Fake a seek followed by a write
  static void fakeSeekAndWrite(uint64_t newPos) {
    std::lock_guard<std::mutex> lock(streamMutex);
    int64_t seek = int64_t(filePos) - int64_t(newPos);
    if (seek < 0)
      seek = -seek;
    double seekTime = 5e-3 * double(seek) / 2000.0; // 5 msec for a 2000-unit seek.
    // A short write time (500 microsec) for a small block of data
    seekTime += 0.5e-3;
    Timer tim;
    while (tim.elapsed_no_reset() < seekTime) { /*Wait*/
    }
    filePos = newPos;
  }
  void load() override {
    if (this->wasSaved() && !this->isLoaded()) {
      m_memory += this->getFileSize();
    }
    this->setLoaded(true);
  }

  static uint64_t filePos;
  static std::string fakeFile;
  static std::mutex streamMutex;
};
uint64_t SaveableTesterWithSeek::filePos;
// Declare the static members here.
std::string SaveableTesterWithSeek::fakeFile;
std::mutex SaveableTesterWithSeek::streamMutex;

//====================================================================================
class DiskBufferTestPerformance : public CxxTest::TestSuite {
public:
  std::vector<SaveableTesterWithSeek *> dataSeek;
  size_t num;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other xests
  static DiskBufferTestPerformance *createSuite() { return new DiskBufferTestPerformance(); }
  static void destroySuite(DiskBufferTestPerformance *suite) { delete suite; }

  DiskBufferTestPerformance() {

    dataSeek.clear();
    dataSeek.reserve(200);
    for (size_t i = 0; i < 200; i++)
      dataSeek.emplace_back(new SaveableTesterWithSeek(i));
  }
  void setUp() override { SaveableTesterWithSeek::fakeFile = ""; }

  void xest_nothing() { TS_WARN("Tests here were disabled for the time being"); }

  /** Demonstrate that using a write buffer reduces time spent seeking on disk
   */
  void test_withFakeSeeking_withWriteBuffer() {
    CPUTimer tim;
    DiskBuffer dbuf(10);
    for (auto &i : dataSeek) {
      // Pretend you just loaded the data
      i->load(dbuf);
    }
    std::cout << tim << " to load " << dataSeek.size() << " into MRU with fake seeking. \n";
  }

  /** Use a 0-sized write buffer so that it constantly needs to seek and write
   * out. This should be slower due to seeking. */
  void test_withFakeSeeking_noWriteBuffer() {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (auto &i : dataSeek) {
      // Pretend you just loaded the data
      i->load(dbuf);
    }
    std::cout << tim << " to load " << dataSeek.size() << " into MRU with fake seeking. \n";
  }

  /** Example of a situation where vectors grew, meaning that they need to be
   * relocated causing lots of seeking if no write buffer exists.*/
  void test_withFakeSeeking_growingData() {
    CPUTimer tim;
    DiskBuffer dbuf(20);
    dbuf.setFileLength(dataSeek.size());
    for (auto &i : dataSeek) {
      // Pretend you just loaded the data
      i->grow(dbuf, true);
      dbuf.toWrite(i);
    }
    std::cout << "About to flush the cache to finish writes.\n";
    dbuf.flushCache();
    std::cout << tim << " to grow " << dataSeek.size() << " into MRU with fake seeking. \n";
  }

  /** Demonstrate that calling "save" manually without using the MRU write
   * buffer will slow things down
   * due to seeking. Was an issue in LoadMD */
  void test_withFakeSeeking_growingData_savingWithoutUsingMRU() {
    CPUTimer tim;
    DiskBuffer dbuf(dataSeek.size());
    for (auto &i : dataSeek) {
      // Pretend you just loaded the data
      i->grow(dbuf, false);
      i->save();
    }
    std::cout << tim << " to grow " << dataSeek.size() << " into MRU with fake seeking. \n";
  }

  /** Speed of freeing a lot of blocks and putting them in the free space map */
  void test_freeBlock() {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (size_t i = 0; i < 100000; i++) {
      dbuf.freeBlock(i * 100, (i % 3 == 0) ? 100 : 50);
    }
    // dbuf.defragFreeBlocks();
    TS_ASSERT_EQUALS(dbuf.getFreeSpaceMap().size(), 66667);
    std::cout << tim << " to add " << 100000 << " blocks in the free space list.\n";
  }
};
