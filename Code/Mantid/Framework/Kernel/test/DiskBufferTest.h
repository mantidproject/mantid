#ifndef MANTID_KERNEL_DISKBUFFERTEST_H_
#define MANTID_KERNEL_DISKBUFFERTEST_H_


#include "MantidKernel/DiskBuffer.h"
#include "MantidKernel/FreeBlock.h"
#include "MantidKernel/ISaveable.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using Mantid::Kernel::CPUTimer;


//====================================================================================
class ISaveableTester : public ISaveable
{
public:
  ISaveableTester(size_t id) : ISaveable(id),
  m_doSave(true), m_memory(1), m_dataBusy(false)
  {}

  bool m_doSave;
  virtual void save() const
  {
    if (!m_doSave) return;
    // Fake writing to a file
    std::ostringstream out;
    out << getId() <<",";
    streamMutex.lock();
    fakeFile += out.str();
    streamMutex.unlock();
  }

  virtual void load() {}
  virtual void flushData() const {}

  uint64_t m_memory;
  virtual uint64_t getMRUMemorySize() const { return m_memory; };

  bool m_dataBusy;
  virtual bool dataBusy() const {return m_dataBusy; }

  // File position = same as its ID
  virtual uint64_t getFilePosition() const { return uint64_t(10-getId()); }

  static std::string fakeFile;

  static Kernel::Mutex streamMutex;
};

// Declare the static members here.
std::string ISaveableTester::fakeFile = "";
Kernel::Mutex ISaveableTester::streamMutex;




//====================================================================================
/** An ISaveable that will fake seeking to disk */
class ISaveableTesterWithSeek : public ISaveableTester
{
public:
  ISaveableTesterWithSeek(size_t id) : ISaveableTester(id)
  {
    myFilePos = id;
  }

  using ISaveableTester::load; // Unhide base class method to avoid Intel compiler warning
  virtual void load(DiskBuffer & /*dbuf*/) const
  {
    std::cout << "Block " << getId() << " loading at " << myFilePos << std::endl;
    ISaveableTesterWithSeek::fakeSeekAndWrite( this->getFilePosition() );
  }

  virtual void save() const
  {
    if (!m_doSave) return;
    // Pretend to seek to the point and write
    std::cout << "Block " << getId() << " saving at " << myFilePos << std::endl;
    fakeSeekAndWrite(getFilePosition());
  }

  void grow(DiskBuffer & dbuf, bool /*tellMRU*/)
  {
    // OK first you seek to where the OLD data was and load it.
    std::cout << "Block " << getId() << " loading at " << myFilePos << std::endl;
    ISaveableTesterWithSeek::fakeSeekAndWrite( this->getFilePosition() );
    // Simulate that the data is growing and so needs to be written out
    size_t newfilePos = dbuf.relocate(myFilePos, m_memory, m_memory+1);
    std::cout << "Block " << getId() << " has moved from " << myFilePos << " to " << newfilePos << std::endl;
    myFilePos = newfilePos;
    // Grow the size by 1
    m_memory = m_memory + 1;
  }

  /// Fake a seek followed by a write
  static void fakeSeekAndWrite(uint64_t newPos)
  {
    streamMutex.lock();
    int64_t seek = int64_t(filePos) - int64_t(newPos);
    if (seek < 0) seek = -seek;
    double seekTime = 5e-3 * double(seek) / 2000.0; // 5 msec for a 2000-unit seek.
    // A short write time (500 microsec) for a small block of data
    seekTime += 0.5e-3;
    Timer tim;
    while (tim.elapsed_no_reset() < seekTime)
    { /*Wait*/ }
    filePos = newPos;
    streamMutex.unlock();
  }

  // File position = same as its ID
  uint64_t myFilePos;
  virtual uint64_t getFilePosition() const { return myFilePos; }

  static uint64_t filePos;
};
uint64_t ISaveableTesterWithSeek::filePos;




//====================================================================================
/** An ISaveable that fakes writing to a fixed-size file */
class ISaveableTesterWithFile : public ISaveable
{
public:
  ISaveableTesterWithFile(size_t id, uint64_t pos, uint64_t size, char ch) : ISaveable(id),
  m_ch(ch), m_memory(size), m_pos(pos), m_dataBusy(false)
  {}

  char m_ch;
  virtual void save() const
  {
    // Fake writing to a file
    streamMutex.lock();
    if (fakeFile.size() < m_pos+m_memory)
      fakeFile.resize(m_pos+m_memory, ' ');
    for (size_t i=m_pos; i< m_pos+m_memory; i++)
      fakeFile[i] = m_ch;
    streamMutex.unlock();
  }

  virtual void load() {}
  virtual void flushData() const {}

  uint64_t m_memory;
  virtual uint64_t getMRUMemorySize() const {return m_memory;};

  uint64_t m_pos;
  virtual uint64_t getFilePosition() const { return m_pos; }

  bool m_dataBusy;
  virtual bool dataBusy() const {return m_dataBusy; }

  static std::string fakeFile;

  static Kernel::Mutex streamMutex;
};

// Declare the static members here.
std::string ISaveableTesterWithFile::fakeFile;
Kernel::Mutex ISaveableTesterWithFile::streamMutex;






//====================================================================================
class DiskBufferTest : public CxxTest::TestSuite
{
public:

  std::vector<ISaveableTester*> data;
  size_t num;
  std::vector<ISaveableTester*> bigData;
  size_t bigNum;

  void setUp()
  {
    // Create the ISaveables
    ISaveableTester::fakeFile = "";
    num = 10;
    data.clear();
    for (size_t i=0; i<num; i++)
      data.push_back( new ISaveableTester(i) );
    bigNum = 1000;
    bigData.clear();
    for (size_t i=0; i<bigNum; i++)
      bigData.push_back( new ISaveableTester(i) );
  }


  //--------------------------------------------------------------------------------
  /** Getting and setting the cache sizes */
  void test_set_and_get_methods()
  {
    DiskBuffer dbuf(3);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferSize(), 3);
    dbuf.setWriteBufferSize(11);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferSize(), 11);
  }

  //--------------------------------------------------------------------------------
  /** Test calling toWrite() */
  void test_basic()
  {
    // No MRU, 3 in the to-write cache
    DiskBuffer dbuf(3);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferSize(), 3);

    // Nothing in cache
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);

    dbuf.toWrite(data[0]);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 1);
    dbuf.toWrite(data[1]);
    dbuf.toWrite(data[2]);
    // Write buffer now got flushed out
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);

    // The "file" was written out this way (the right order):
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "2,1,0,");
    ISaveableTester::fakeFile ="";

    // If you add the same one multiple times, it only is tracked once in the to-write buffer.
    dbuf.toWrite(data[4]);
    dbuf.toWrite(data[4]);
    dbuf.toWrite(data[4]);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 1);
  }


  //--------------------------------------------------------------------------------
  /** Set a buffer size of 0 */
  void test_basic_noWriteBuffer()
  {
    // No write buffer
    DiskBuffer dbuf(0);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferSize(), 0);
    // Nothing in cache
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);

    dbuf.toWrite(data[0]);
    dbuf.toWrite(data[1]);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
    dbuf.toWrite(data[2]);
    dbuf.toWrite(data[3]);
    dbuf.toWrite(data[4]);
    // Nothing gets written out
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "");
  }



  //--------------------------------------------------------------------------------
  /// Empty out the cache with the flushCache() method
  void test_flushCache()
  {
    DiskBuffer dbuf(10);
    for (size_t i=0; i<6; i++)
      dbuf.toWrite(data[i]);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 6);
    // Nothing written out yet
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "");
    dbuf.flushCache();
    // Everything was written out at once (sorted by file index)
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "5,4,3,2,1,0,");
    // Nothing left in cache
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
  }


  //--------------------------------------------------------------------------------
  /** Extreme case with nothing writable but exceeding the writable buffer */
  void test_noWriteBuffer_nothingWritable()
  {
    // Room for 4 in the write buffer
    DiskBuffer dbuf(4);
    for (size_t i=0; i<9; i++)
    {
      data[i]->m_dataBusy = true;
      dbuf.toWrite(data[i]);
    }
    // We ended up with too much in the buffer since nothing could be written.
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 9);
    // Let's make it all writable
    for (size_t i=0; i<9; i++)
      data[i]->m_dataBusy = false;
    // Trigger a write
    dbuf.toWrite(data[9]);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
    // And all of these get written out at once
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "9,8,7,6,5,4,3,2,1,0,");
  }


  //--------------------------------------------------------------------------------
  /** Sorts by file position when writing to a file */
  void test_writesOutInFileOrder()
  {
    // Room for 3 in the to-write cache
    DiskBuffer dbuf(3);
    // These 3 will get written out
    dbuf.toWrite(data[5]);
    dbuf.toWrite(data[1]);
    dbuf.toWrite(data[9]);
    // These 4 at the end will be in the cache
    dbuf.toWrite(data[2]);
    dbuf.toWrite(data[3]);
    dbuf.toWrite(data[4]);
    dbuf.toWrite(data[6]);

    // 1 left in the buffer
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 1);

    // The "file" was written out this way (sorted by file position):
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "9,5,1,4,3,2,");
  }

  //--------------------------------------------------------------------------------
  /** Any ISaveable that says it can't be written remains in the cache */
  void test_skips_dataBusy_Blocks()
  {
    DiskBuffer dbuf(3);
    dbuf.toWrite(data[0]);
    dbuf.toWrite(data[1]); data[1]->m_dataBusy = true; // Won't get written out
    dbuf.toWrite(data[2]);
    dbuf.flushCache();

    // Item #1 was skipped and is still in the buffer!
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "2,0,");
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 1);

    // But it'll get written out next time
    ISaveableTester::fakeFile = "";
    data[1]->m_dataBusy = false;
    dbuf.flushCache();
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "1,");
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
  }


  //--------------------------------------------------------------------------------
  /** If a block will get deleted it needs to be taken
   * out of the caches */
  void test_objectDeleted()
  {
    // Room for 6 in the to-write cache
    DiskBuffer dbuf(6);
    // Fill the buffer
    for (size_t i=0; i<5; i++)
      dbuf.toWrite(data[i]);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 5);

    // First let's get rid of something in to to-write buffer
    dbuf.objectDeleted(data[1], 1);
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 4);
    TSM_ASSERT_EQUALS( "Space on disk was marked as free", dbuf.getFreeSpaceMap().size(), 1);

    dbuf.flushCache();
    // This triggers a write. 1 is no longer in the to-write buffer
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "4,3,2,0,");
    TS_ASSERT_EQUALS( dbuf.getWriteBufferUsed(), 0);
  }


  //--------------------------------------------------------------------------------
  /** Accessing the map from multiple threads simultaneously does not segfault */
  void test_thread_safety()
  {
    // Room for 3 in the to-write cache
    DiskBuffer dbuf(3);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i<int(bigNum); i++)
    {
      dbuf.toWrite(bigData[i]);
    }
    //std::cout << ISaveableTester::fakeFile << std::endl;
  }


  //--------------------------------------------------------------------------------
  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithPrevious()
  {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    TS_ASSERT_EQUALS( map.size(), 0);
    dbuf.freeBlock(0, 50);
    TS_ASSERT_EQUALS( map.size(), 1);
    // zero-sized free block does nothing
    dbuf.freeBlock(1234, 0);
    TS_ASSERT_EQUALS( map.size(), 1);
    dbuf.freeBlock(100, 50);
    TS_ASSERT_EQUALS( map.size(), 2);
    // Free a block next to another one, AFTER
    dbuf.freeBlock(150, 50);
    TSM_ASSERT_EQUALS( "Map remained the same size because adjacent blocks were merged", map.size(), 2);

    // Get a vector of the free blocks and sizes
    std::vector<uint64_t> free;
    dbuf.getFreeSpaceVector(free);
    TS_ASSERT_EQUALS( free[0], 0);
    TS_ASSERT_EQUALS( free[1], 50);
    TS_ASSERT_EQUALS( free[2], 100);
    TS_ASSERT_EQUALS( free[3], 100);
  }

  //--------------------------------------------------------------------------------
  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithNext()
  {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    dbuf.freeBlock(0, 50);
    dbuf.freeBlock(200, 50);
    TS_ASSERT_EQUALS( map.size(), 2);
    // Free a block next to another one, BEFORE
    dbuf.freeBlock(150, 50);
    TSM_ASSERT_EQUALS( "Map remained the same size because adjacent blocks were merged", map.size(), 2);

    // Get the 2nd free block.
    DiskBuffer::freeSpace_t::iterator it =map.begin();
    it++;
    b = *it;
    TS_ASSERT_EQUALS( b.getFilePosition(), 150);
    TS_ASSERT_EQUALS( b.getSize(), 100);

    dbuf.freeBlock(50, 50);
    TSM_ASSERT_EQUALS( "Map remained the same size because adjacent blocks were merged", map.size(), 2);
    TS_ASSERT_EQUALS( map.begin()->getSize(), 100);
  }

  //--------------------------------------------------------------------------------
  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithBothNeighbours()
  {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    dbuf.freeBlock(0, 50);
    dbuf.freeBlock(200, 50);
    dbuf.freeBlock(300, 50);
    dbuf.freeBlock(400, 50); // Disconnected 4th one
    TS_ASSERT_EQUALS( map.size(), 4);
    // Free a block between two block
    dbuf.freeBlock(250, 50);
    TSM_ASSERT_EQUALS( "Map shrank because three blocks were merged", map.size(), 3);

    // Get the 2nd free block.
    DiskBuffer::freeSpace_t::iterator it =map.begin();
    it++;
    b = *it;
    TS_ASSERT_EQUALS( b.getFilePosition(), 200);
    TS_ASSERT_EQUALS( b.getSize(), 150);
  }

  //--------------------------------------------------------------------------------
  /** Add blocks to the free block list in parallel threads,
   * should not segfault or anything */
  void test_freeBlock_threadSafety()
  {
    DiskBuffer dbuf(0);
    PRAGMA_OMP( parallel for)
    for (int i=0; i<10000; i++)
    {
      dbuf.freeBlock(uint64_t(i)*100, (i%3==0) ? 100 : 50);
    }
    // 1/3 of the blocks got merged
    TS_ASSERT_EQUALS( dbuf.getFreeSpaceMap().size(), 6667);
  }


  /** Disabled because it is not necessary to defrag since that happens on the fly */
  void xtest_defragFreeBlocks()
  {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    dbuf.freeBlock(0, 50);
    dbuf.freeBlock(100, 50);
    dbuf.freeBlock(150, 50);
    dbuf.freeBlock(500, 50);
    dbuf.freeBlock(550, 50);
    dbuf.freeBlock(600, 50);
    dbuf.freeBlock(650, 50);
    dbuf.freeBlock(1000, 50);
    TS_ASSERT_EQUALS( map.size(), 8);

    dbuf.defragFreeBlocks();
    TS_ASSERT_EQUALS( map.size(), 4);
  }

  /// You can call relocate() if an block is shrinking.
  void test_relocate_when_shrinking()
  {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    // You stay in the same place because that's the only free spot.
    TS_ASSERT_EQUALS( dbuf.relocate(100, 10, 5), 100 );
    // You left a free block at 105.
    TS_ASSERT_EQUALS( map.size(), 1);
    // This one, instead of staying in place, will fill in that previously freed 5-sized block
    //  since that's the smallest one that fits the whole block.
    TS_ASSERT_EQUALS( dbuf.relocate(200, 10, 5), 105 );
    // Still one free block, but its at 200-209 now.
    TS_ASSERT_EQUALS( map.size(), 1);
  }

  /// You can call relocate() if an block is shrinking.
  void test_relocate_when_growing()
  {
    DiskBuffer dbuf(3);
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    dbuf.freeBlock(200, 20);
    dbuf.freeBlock(300, 30);
    TS_ASSERT_EQUALS( map.size(), 2);

    // Grab the smallest block that's big enough
    TS_ASSERT_EQUALS( dbuf.relocate(100, 10, 20), 200 );
    // You left a free block at 100 of size 10 to replace that one.
    TS_ASSERT_EQUALS( map.size(), 2);
    // A zero-sized block is "relocated" by basically allocating it to the free spot
    TS_ASSERT_EQUALS( dbuf.relocate(100, 0, 5), 100 );
    TS_ASSERT_EQUALS( map.size(), 2);

  }


  /// Various tests of allocating and relocating
  void test_allocate_from_empty_freeMap()
  {
    DiskBuffer dbuf(3);
    dbuf.setFileLength(1000); // Lets say the file goes up to 1000
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    FreeBlock b;
    TS_ASSERT_EQUALS( map.size(), 0);
    // No free blocks? End up at the end
    TS_ASSERT_EQUALS( dbuf.allocate(20), 1000 );
    TS_ASSERT_EQUALS( dbuf.getFileLength(), 1020 );

    for (size_t i=0; i<100000; i++)
      dbuf.allocate(20);

    DiskBuffer mru2;
    mru2.setFileLength(1000);
    for (size_t i=0; i<100000; i++)
      mru2.allocate(20);

  }


  /// Various tests of allocating and relocating
  void test_allocate_and_relocate()
  {
    DiskBuffer dbuf(3);
    dbuf.setFileLength(1000); // Lets say the file goes up to 1000
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    FreeBlock b;

    dbuf.freeBlock(100, 10);
    dbuf.freeBlock(200, 20);
    dbuf.freeBlock(300, 30);
    dbuf.freeBlock(400, 40);
    TS_ASSERT_EQUALS( map.size(), 4);
    // Where does the block end up?
    TS_ASSERT_EQUALS( dbuf.allocate(20), 200 );
    // The map has shrunk by one since the new one was removed.
    TS_ASSERT_EQUALS( map.size(), 3);
    // OK, now look for a smaller block, size of 4
    TS_ASSERT_EQUALS( dbuf.allocate(4), 100 );
    // This left a little chunk of space free, sized 6 at position 104. So the # of entries in the free space map did not change
    TS_ASSERT_EQUALS( map.size(), 3);
    TS_ASSERT_EQUALS( map.begin()->getFilePosition(), 104);
    TS_ASSERT_EQUALS( map.begin()->getSize(), 6);

    // Now try to relocate. Had a block after a 30-sized free block at 300.
    // It gets freed, opening up a slot for the new chunk of memory
    TS_ASSERT_EQUALS( dbuf.relocate(330, 5, 35), 300 );
    // One fewer free block.
    TS_ASSERT_EQUALS( map.size(), 2);

    // Ok, now lets ask for a block that is too big. It puts us at the end of the file
    TS_ASSERT_EQUALS( dbuf.allocate(55), 1000 );
    TS_ASSERT_EQUALS( dbuf.getFileLength(), 1055 );
  }

  void test_allocate_with_file()
  {
    // Start by faking a file
    ISaveableTesterWithFile * blockA = new ISaveableTesterWithFile(0, 0, 2, 'A');
    ISaveableTesterWithFile * blockB = new ISaveableTesterWithFile(1, 2, 3, 'B');
    ISaveableTesterWithFile * blockC = new ISaveableTesterWithFile(2, 5, 5, 'C');
    blockA->save();
    blockB->save();
    blockC->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AABBBCCCCC");

    DiskBuffer dbuf(3);
    dbuf.setFileLength(10);
    DiskBuffer::freeSpace_t & map = dbuf.getFreeSpaceMap();
    uint64_t newPos;

    // File lengths are known correctly
    TS_ASSERT_EQUALS( dbuf.getFileLength(), 10);

    // Asking for a new chunk of space that needs to be at the end
    newPos = dbuf.relocate(blockB->m_pos, blockB->m_memory, 7);
    TSM_ASSERT_EQUALS( "One freed block", map.size(), 1);
    TS_ASSERT_EQUALS( dbuf.getFileLength(), 17);

    // Simulate saving
    blockB->m_pos = newPos;
    blockB->m_memory = 7;
    blockB->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AABBBCCCCCBBBBBBB");

    // Now let's allocate a new block
    newPos = dbuf.allocate(2);
    TS_ASSERT_EQUALS( newPos, 2 );
    ISaveableTesterWithFile * blockD = new ISaveableTesterWithFile(3, newPos, 2, 'D');
    blockD->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AADDBCCCCCBBBBBBB");
    TSM_ASSERT_EQUALS( "Still one freed block", map.size(), 1);

    // Grow blockD by 1
    newPos = dbuf.relocate(2, 2, 3);
    TSM_ASSERT_EQUALS( "Block D stayed in the same place since there was room after it", newPos, 2 );
    blockD->m_memory = 3;
    blockD->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AADDDCCCCCBBBBBBB");

    // Allocate a little block at the end
    newPos = dbuf.allocate(1);
    TSM_ASSERT_EQUALS( "The new block went to the end of the file", newPos, 17 );
    // Which is now longer by 1
    TS_ASSERT_EQUALS( dbuf.getFileLength(), 18);


    //std::cout <<  ISaveableTesterWithFile::fakeFile << "!" << std::endl;
  }

};




//====================================================================================
class DiskBufferTestPerformance : public CxxTest::TestSuite
{
public:

  std::vector<ISaveableTester*> data;
  std::vector<ISaveableTesterWithSeek*> dataSeek;
  size_t num;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiskBufferTestPerformance *createSuite() { return new DiskBufferTestPerformance(); }
  static void destroySuite( DiskBufferTestPerformance *suite ) { delete suite; }

  DiskBufferTestPerformance()
  {
    num = 100000;
    data.clear();
    for (size_t i=0; i<num; i++)
    {
      data.push_back( new ISaveableTester(i) );
      data[i]->m_doSave = false; // Items won't do any real saving
    }
    dataSeek.clear();
    for (size_t i=0; i<200; i++)
      dataSeek.push_back( new ISaveableTesterWithSeek(i) );
  }

  void setUp()
  {
    ISaveableTester::fakeFile = "";
  }


  void test_smallCache_writeBuffer()
  {
    CPUTimer tim;
    DiskBuffer dbuf(3);
    for (int i=0; i<int(num); i++)
      dbuf.toWrite(data[i]);
    std::cout << tim << " to load " << num << " into MRU." << std::endl;
  }

  void test_smallCache_no_writeBuffer()
  {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (int i=0; i<int(num); i++)
      dbuf.toWrite(data[i]);
    std::cout << tim << " to load " << num << " into MRU (no write cache)." << std::endl;
  }

  void test_largeCache_writeBuffer()
  {
    CPUTimer tim;
    DiskBuffer dbuf(1000);
    for (int i=0; i<int(num); i++)
      dbuf.toWrite(data[i]);
    std::cout << tim << " to load " << num << " into MRU." << std::endl;
  }

  void test_largeCache_noWriteBuffer()
  {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (int i=0; i<int(num); i++)
      dbuf.toWrite(data[i]);
    std::cout << tim << " to load " << num << " into MRU (no write buffer)." << std::endl;
  }

  /** Demonstrate that using a write buffer reduces time spent seeking on disk */
  void test_withFakeSeeking_withWriteBuffer()
  {
    CPUTimer tim;
    DiskBuffer dbuf(10);
    for (int i=0; i<int(dataSeek.size()); i++)
    {
      // Pretend you just loaded the data
      dataSeek[i]->load(dbuf);
    }
    std::cout << tim << " to load " << dataSeek.size() << " into MRU with fake seeking. " << std::endl;
  }

  /** Use a 0-sized write buffer so that it constantly needs to seek and write out. This should be slower due to seeking. */
  void test_withFakeSeeking_noWriteBuffer()
  {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (int i=0; i<int(dataSeek.size()); i++)
    {
      // Pretend you just loaded the data
      dataSeek[i]->load(dbuf);
    }
    std::cout << tim << " to load " << dataSeek.size() << " into MRU with fake seeking. " << std::endl;
  }

  /** Example of a situation where vectors grew, meaning that they need to be
   * relocated causing lots of seeking if no write buffer exists.*/
  void test_withFakeSeeking_growingData()
  {
    CPUTimer tim;
    DiskBuffer dbuf(20);
    dbuf.setFileLength(dataSeek.size());
    for (int i=0; i<int(dataSeek.size()); i++)
    {
      // Pretend you just loaded the data
      dataSeek[i]->grow(dbuf, true);
    }
    std::cout << "About to flush the cache to finish writes." << std::endl;
    dbuf.flushCache();
    std::cout << tim << " to grow " << dataSeek.size() << " into MRU with fake seeking. " << std::endl;
  }

  /** Demonstrate that calling "save" manually without using the MRU write buffer will slow things down
   * due to seeking. Was an issue in LoadMD */
  void test_withFakeSeeking_growingData_savingWithoutUsingMRU()
  {
    CPUTimer tim;
    DiskBuffer dbuf(dataSeek.size());
    for (int i=0; i<int(dataSeek.size()); i++)
    {
      // Pretend you just loaded the data
      dataSeek[i]->grow(dbuf, false);
      dataSeek[i]->save();
    }
    std::cout << tim << " to grow " << dataSeek.size() << " into MRU with fake seeking. " << std::endl;
  }

  /** Speed of freeing a lot of blocks and putting them in the free space map */
  void test_freeBlock()
  {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (size_t i=0; i<100000; i++)
    {
      dbuf.freeBlock(i*100, (i%3==0) ? 100 : 50);
    }
    //dbuf.defragFreeBlocks();
    TS_ASSERT_EQUALS( dbuf.getFreeSpaceMap().size(), 66667);
    std::cout << tim << " to add " << 100000 << " blocks in the free space list." << std::endl;
  }

};


#endif /* MANTID_KERNEL_DISKBUFFERTEST_H_ */
