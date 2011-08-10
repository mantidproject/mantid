#ifndef MANTID_API_DISKMRUTEST_H_
#define MANTID_API_DISKMRUTEST_H_

#include "MantidKernel/DiskMRU.h"
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
#include "MantidKernel/IFile.h"

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

  virtual void load()
  {}

  uint64_t m_memory;
  virtual uint64_t getSizeOnFile() const {return m_memory;};

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
  {}

  virtual void save() const
  {
    if (!m_doSave) return;
    // Pretend to seek to the point and write
    fakeSeekAndWrite(getFilePosition());
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
  virtual uint64_t getFilePosition() const { return uint64_t(getId()); }

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
    for (size_t i=m_pos; i< m_pos+m_memory; i++)
      fakeFile[i] = m_ch;
    streamMutex.unlock();
  }

  virtual void load()
  {}

  uint64_t m_memory;
  virtual uint64_t getSizeOnFile() const {return m_memory;};

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
/// Tester for IFile
class IFileTester : public IFile
{
  /// Return the length of the file, in units compatible with DiskMRU (not necessarily bytes).
  virtual uint64_t getFileLength() const
  { return ISaveableTesterWithFile::fakeFile.size(); }

  /// Increase the size of the file to the given length.
  virtual void extendFile(uint64_t newLength)
  {
    uint64_t added = newLength - ISaveableTesterWithFile::fakeFile.size();
    std::string add;
    for (size_t i=0; i<added; i++)
      add += " ";
    ISaveableTesterWithFile::fakeFile += add;
  }

};





//====================================================================================
class DiskMRUTest : public CxxTest::TestSuite
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



  /** Speed comparisons. Boost is faster! */
  void xtest_to_compare_to_stl()
  {
    size_t num = 1000000;
    std::vector<ISaveableTester*> data;
    for (size_t i=0; i<num; i++)
    {
      data.push_back( new ISaveableTester(i) );
    }

    DiskMRU::mru_t list;
    CPUTimer tim;
    for (size_t i=0; i<num; i++)
    {
      std::pair<DiskMRU::mru_t::iterator,bool> p;
      p = list.push_front(data[i]);
    }
    std::cout << tim << " to fill the list." << std::endl;

    std::set<ISaveable*> mySet;
    for (size_t i=0; i<num; i++)
    {
      mySet.insert(data[i]);
    }
    std::cout << tim << " to fill a set[*]." << std::endl;

    std::map<size_t, ISaveable*> myMap;
    for (size_t i=0; i<num; i++)
    {
      ISaveable * s = data[i];
      myMap[s->getId()] = s;
    }
    std::cout << tim << " to fill a map[size_t, *]." << std::endl;

    std::multimap<size_t, size_t> mmap;
    for (size_t i=0; i<num; i++)
    {
      mmap.insert(std::pair<size_t,size_t>(i,i));
    }
    std::cout << tim << " to fill a multimap[size_t, size_t]." << std::endl;

    std::map<size_t, size_t> map;
    for (size_t i=0; i<num; i++)
    {
      map[i] = i;
    }
    std::cout << tim << " to fill a map[size_t, size_t]." << std::endl;

  }


  //--------------------------------------------------------------------------------
  /** Getting and setting the cache sizes */
  void test_set_and_get_methods()
  {
    DiskMRU mru(4, 3, true);
    TS_ASSERT_EQUALS( mru.getMemoryAvail(), 4);
    TS_ASSERT_EQUALS( mru.getWriteBufferSize(), 3);
    mru.setMemoryAvail(15);
    mru.setWriteBufferSize(11);
    TS_ASSERT_EQUALS( mru.getMemoryAvail(), 15);
    TS_ASSERT_EQUALS( mru.getWriteBufferSize(), 11);
  }

  //--------------------------------------------------------------------------------
  /** Basic operation of pushing */
  void test_basic_writeBuffer()
  {
    // Room for 4 in the MRU, and 3 in the to-write cache
    DiskMRU mru(4, 3, true);

    // Nothing in cache
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 0);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);

    // NULLs are ignored
    TS_ASSERT_THROWS_NOTHING( mru.loading(NULL) );

    mru.loading(data[0]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 1);
    mru.loading(data[1]);
    mru.loading(data[2]);
    mru.loading(data[3]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);

    // Adding a 5th item drops off the oldest one and moves it to the toWrite buffer.
    mru.loading(data[4]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 1);
    mru.loading(data[5]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 2);

    // Next one will reach 3 in the "toWrite" buffer and so trigger a write out
    mru.loading(data[6]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4); //We should have 3,4,5,6 in there now
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);
    // The "file" was written out this way (the right order):
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "2,1,0,");

  }

  /// Empty out the cache with the flushCache() method
  void test_flushCache()
  {
    DiskMRU mru(4, 3, true);
    for (size_t i=0; i<6; i++)
      mru.loading(data[i]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4); //We should have 2,3,4,5 in there now
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 2); // We should have 0,1 in there
    // Nothing written out yet
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "");
    mru.flushCache();
    // Everything was written out at once (sorted by file index)
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "5,4,3,2,1,0,");
    // Nothing left in cache
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 0);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);
  }



  //--------------------------------------------------------------------------------
  /** Basic operation of pushing, this time no write-out buffer */
  void test_basic_no_writeBuffer()
  {
    // Room for 4 in the MRU, no write buffer
    DiskMRU mru(4, 0, false);
    // Nothing in cache
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 0);
    // NULLs are ignored
    TS_ASSERT_THROWS_NOTHING( mru.loading(NULL) );
    mru.loading(data[0]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 1);
    mru.loading(data[1]);
    mru.loading(data[2]);
    mru.loading(data[3]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);

    // Adding a 5th item drops off the oldest one and saves it to disk
    mru.loading(data[4]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,");
    mru.loading(data[5]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,");

    // Avoid dropping off the next one
    data[2]->m_dataBusy = true;
    mru.loading(data[6]);
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,3,");
  }

  //--------------------------------------------------------------------------------
  /** Extreme case with nothing writable but exceeding the writable buffer */
  void test_noWriteBuffer_nothingWritable()
  {
    // Room for 4 in the MRU, no write buffer
    DiskMRU mru(4, 0, false);
    for (size_t i=0; i<9; i++)
    {
      data[i]->m_dataBusy = true;
      mru.loading(data[i]);
    }
    // We ended up with too much in the buffer since nothing could be written.
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 9);
    // Let's make it all writable
    for (size_t i=0; i<9; i++)
      data[i]->m_dataBusy = false;
    // Trigger a write
    mru.loading(data[9]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    // And all of these get written out at once
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,2,3,4,5,");
  }


  //--------------------------------------------------------------------------------
  /** MRU properly keeps recently used items at the top */
  void test_mru()
  {
    DiskMRU mru(4, 1, true);
    mru.loading(data[0]);
    mru.loading(data[1]);
    mru.loading(data[2]);
    mru.loading(data[0]);
    mru.loading(data[3]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    // 1 is actually the oldest one
    mru.loading(data[4]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4); //We should have 0,2,3,4 in there now
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);
    // # 1 was written out
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "1,");
  }


  //--------------------------------------------------------------------------------
  /** Sorts by file position when writing to a file */
  void test_writesOutInFileOrder()
  {
    // Room for 4 in the MRU, and 3 in the to-write cache
    DiskMRU mru(4, 3, true);
    // These 3 will get written out
    mru.loading(data[5]);
    mru.loading(data[1]);
    mru.loading(data[9]);
    // These 4 at the end will be in the cache
    mru.loading(data[2]);
    mru.loading(data[3]);
    mru.loading(data[4]);
    mru.loading(data[6]);

    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);

    // The "file" was written out this way (sorted by file position):
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "9,5,1,");
  }

  //--------------------------------------------------------------------------------
  /** Any ISaveable that says it can't be written remains in the cache */
  void test_skips_dataBusy_Blocks()
  {
    DiskMRU mru(4, 3, true);
    mru.loading(data[0]);
    mru.loading(data[1]); data[1]->m_dataBusy = true; // Won't get written out
    mru.loading(data[2]);
    // These 4 at the end will be in the cache
    for (size_t i=3; i<7; i++)
      mru.loading(data[i]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);

    // Item #1 was skipped and is still in the buffer!
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "2,0,");
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 1);

    // But it'll get written out next time
    ISaveableTester::fakeFile = "";
    data[1]->m_dataBusy = false;
    mru.loading(data[7]);
    mru.loading(data[8]);
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "4,3,1,");
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);
  }

  //--------------------------------------------------------------------------------
  /** If a new block being loaded is big, it'll push more than one into the to-write buffer */
  void test_canPushTwoIntoTheToWriteBuffer()
  {
    // Room for 4 in the MRU, and 3 in the to-write cache
    DiskMRU mru(4, 3, true);
    // Fill the cache
    for (size_t i=0; i<4; i++)
      mru.loading(data[i]);
    // This one uses 2 blocks worth of memory
    data[4]->m_memory = 2;
    mru.loading(data[4]);
    // So there's now 3 blocks (with 4 mem) in the MRU
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    // And 2 in the toWrite buffer
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 2);

    // This will write out the 3 in the cache
    mru.loading(data[5]);
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "2,1,0,");
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);
  }

  //--------------------------------------------------------------------------------
  /** A block placed in the toWrite buffer should get taken out */
  void test_takingBlockOutOfToWriteBuffer()
  {
    // Room for 4 in the MRU, and 3 in the to-write cache
    DiskMRU mru(4, 3, true);
    // Fill the cache. 0,1 in the toWrite buffer
    for (size_t i=0; i<6; i++)
      mru.loading(data[i]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 2);
    // Should pop #0 out of the toWrite buffer and push another one in (#2 in this case)
    mru.loading(data[0]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 2);

    // 1,2,3 (and not 0) should be in "toWrite"
    mru.loading(data[6]);
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "3,2,1,");
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);
  }


  //--------------------------------------------------------------------------------
  /** Accessing the map from multiple threads simultaneously does not segfault */
  void test_thread_safety()
  {
    // Room for 4 in the MRU, and 3 in the to-write cache
    DiskMRU mru(4, 3, true);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i<int(bigNum); i++)
    {
      mru.loading(bigData[i]);
    }
    //std::cout << ISaveableTester::fakeFile << std::endl;
  }


  //--------------------------------------------------------------------------------
  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithPrevious()
  {
    DiskMRU mru(4, 3, true);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    FreeBlock b;

    TS_ASSERT_EQUALS( map.size(), 0);
    mru.freeBlock(0, 50);
    TS_ASSERT_EQUALS( map.size(), 1);
    // zero-sized free block does nothing
    mru.freeBlock(1234, 0);
    TS_ASSERT_EQUALS( map.size(), 1);
    mru.freeBlock(100, 50);
    TS_ASSERT_EQUALS( map.size(), 2);
    // Free a block next to another one, AFTER
    mru.freeBlock(150, 50);
    TSM_ASSERT_EQUALS( "Map remained the same size because adjacent blocks were merged", map.size(), 2);

    // Get the 2nd free block.
    DiskMRU::freeSpace_t::iterator it =map.begin();
    it++;
    b = *it;
    TS_ASSERT_EQUALS( b.getFilePosition(), 100);
    TS_ASSERT_EQUALS( b.getSize(), 100);
  }

  //--------------------------------------------------------------------------------
  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithNext()
  {
    DiskMRU mru(4, 3, true);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    FreeBlock b;

    mru.freeBlock(0, 50);
    mru.freeBlock(200, 50);
    TS_ASSERT_EQUALS( map.size(), 2);
    // Free a block next to another one, BEFORE
    mru.freeBlock(150, 50);
    TSM_ASSERT_EQUALS( "Map remained the same size because adjacent blocks were merged", map.size(), 2);

    // Get the 2nd free block.
    DiskMRU::freeSpace_t::iterator it =map.begin();
    it++;
    b = *it;
    TS_ASSERT_EQUALS( b.getFilePosition(), 150);
    TS_ASSERT_EQUALS( b.getSize(), 100);

    mru.freeBlock(50, 50);
    TSM_ASSERT_EQUALS( "Map remained the same size because adjacent blocks were merged", map.size(), 2);
    TS_ASSERT_EQUALS( map.begin()->getSize(), 100);
  }

  //--------------------------------------------------------------------------------
  /** Freeing blocks get merged properly */
  void test_freeBlock_mergesWithBothNeighbours()
  {
    DiskMRU mru(4, 3, true);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    FreeBlock b;

    mru.freeBlock(0, 50);
    mru.freeBlock(200, 50);
    mru.freeBlock(300, 50);
    mru.freeBlock(400, 50); // Disconnected 4th one
    TS_ASSERT_EQUALS( map.size(), 4);
    // Free a block between two block
    mru.freeBlock(250, 50);
    TSM_ASSERT_EQUALS( "Map shrank because three blocks were merged", map.size(), 3);

    // Get the 2nd free block.
    DiskMRU::freeSpace_t::iterator it =map.begin();
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
    DiskMRU mru(100, 0, false);
    PRAGMA_OMP( parallel for)
    for (int i=0; i<10000; i++)
    {
      mru.freeBlock(uint64_t(i)*100, (i%3==0) ? 100 : 50);
    }
    // 1/3 of the blocks got merged
    TS_ASSERT_EQUALS( mru.getFreeSpaceMap().size(), 6667);
  }


  /** Disabled because it is not necessary to defrag since that happens on the fly */
  void xtest_defragFreeBlocks()
  {
    DiskMRU mru(4, 3, true);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    FreeBlock b;

    mru.freeBlock(0, 50);
    mru.freeBlock(100, 50);
    mru.freeBlock(150, 50);
    mru.freeBlock(500, 50);
    mru.freeBlock(550, 50);
    mru.freeBlock(600, 50);
    mru.freeBlock(650, 50);
    mru.freeBlock(1000, 50);
    TS_ASSERT_EQUALS( map.size(), 8);

    mru.defragFreeBlocks();
    TS_ASSERT_EQUALS( map.size(), 4);
  }

  /// You can call relocate() if an block is shrinking.
  void test_relocate_when_shrinking()
  {
    DiskMRU mru(4, 3, true);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    // You stay in the same place because that's the only free spot.
    TS_ASSERT_EQUALS( mru.relocate(100, 10, 5), 100 );
    // You left a free block at 105.
    TS_ASSERT_EQUALS( map.size(), 1);
    // This one, instead of staying in place, will fill in that previously freed 5-sized block
    //  since that's the smallest one that fits the whole block.
    TS_ASSERT_EQUALS( mru.relocate(200, 10, 5), 105 );
    // Still one free block, but its at 200-209 now.
    TS_ASSERT_EQUALS( map.size(), 1);
  }

  /// You can call relocate() if an block is shrinking.
  void test_relocate_when_growing()
  {
    DiskMRU mru(4, 3, true);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    mru.freeBlock(200, 20);
    mru.freeBlock(300, 30);
    TS_ASSERT_EQUALS( map.size(), 2);

    // Grab the smallest block that's big enough
    TS_ASSERT_EQUALS( mru.relocate(100, 10, 20), 200 );
    // You left a free block at 100 of size 10 to replace that one.
    TS_ASSERT_EQUALS( map.size(), 2);
    // A zero-sized block is "relocated" by basically allocating it to the free spot
    TS_ASSERT_EQUALS( mru.relocate(100, 0, 5), 100 );
    TS_ASSERT_EQUALS( map.size(), 2);

  }

  /// Various tests of allocating and relocating
  void test_allocate_and_relocate()
  {
    DiskMRU mru(4, 3, true);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    FreeBlock b;

    mru.freeBlock(100, 10);
    mru.freeBlock(200, 20);
    mru.freeBlock(300, 30);
    mru.freeBlock(400, 40);
    TS_ASSERT_EQUALS( map.size(), 4);
    // Where does the block end up?
    TS_ASSERT_EQUALS( mru.allocate(20), 200 );
    // The map has shrunk by one since the new one was removed.
    TS_ASSERT_EQUALS( map.size(), 3);
    // OK, now look for a smaller block, size of 4
    TS_ASSERT_EQUALS( mru.allocate(4), 100 );
    // This left a little chunk of space free, sized 6 at position 104. So the # of entries in the free space map did not change
    TS_ASSERT_EQUALS( map.size(), 3);
    TS_ASSERT_EQUALS( map.begin()->getFilePosition(), 104);
    TS_ASSERT_EQUALS( map.begin()->getSize(), 6);

    // Now try to relocate. Had a block after a 30-sized free block at 300.
    // It gets freed, opening up a slot for the new chunk of memory
    TS_ASSERT_EQUALS( mru.relocate(330, 5, 35), 300 );
    // One fewer free block.
    TS_ASSERT_EQUALS( map.size(), 2);

    // Ok, now lets ask for a block that is too big.
    // This throws because no file is set
    TS_ASSERT_THROWS_ANYTHING( mru.allocate(55) );
  }

  void test_allocate_with_file()
  {
    IFile * file = new IFileTester;
    file->extendFile(10);

    DiskMRU mru(4, 3, true, file);
    DiskMRU::freeSpace_t & map = mru.getFreeSpaceMap();
    uint64_t newPos;

    // File lengths are known correctly
    TS_ASSERT_EQUALS( mru.getFileLength(), 10);
    TS_ASSERT_EQUALS( mru.getFileUsed(), 10);

    ISaveableTesterWithFile * blockA = new ISaveableTesterWithFile(0, 0, 2, 'A');
    ISaveableTesterWithFile * blockB = new ISaveableTesterWithFile(1, 2, 3, 'B');
    ISaveableTesterWithFile * blockC = new ISaveableTesterWithFile(2, 5, 5, 'C');
    blockA->save();
    blockB->save();
    blockC->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AABBBCCCCC");

    // Asking for a new chunk of space that needs to be at the end
    newPos = mru.relocate(blockB->m_pos, blockB->m_memory, 7);
    TSM_ASSERT_EQUALS( "One freed block", map.size(), 1);
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile.size(), 18); // One extra cause of that 10%
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AABBBCCCCC        ");
    TS_ASSERT_EQUALS( mru.getFileLength(), 18);
    TS_ASSERT_EQUALS( mru.getFileUsed(), 17);

    // Simulate saving
    blockB->m_pos = newPos;
    blockB->m_memory = 7;
    blockB->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AABBBCCCCCBBBBBBB ");

    // Now let's allocate a new block
    newPos = mru.allocate(2);
    TS_ASSERT_EQUALS( newPos, 2 );
    ISaveableTesterWithFile * blockD = new ISaveableTesterWithFile(3, newPos, 2, 'D');
    blockD->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AADDBCCCCCBBBBBBB ");
    TSM_ASSERT_EQUALS( "Still one freed block", map.size(), 1);

    // Grow blockD by 1
    newPos = mru.relocate(2, 2, 3);
    TSM_ASSERT_EQUALS( "Block D stayed in the same place since there was room after it", newPos, 2 );
    blockD->m_memory = 3;
    blockD->save();
    TS_ASSERT_EQUALS( ISaveableTesterWithFile::fakeFile, "AADDDCCCCCBBBBBBB ");

    // Allocate a little block at the end
    newPos = mru.allocate(1);
    TSM_ASSERT_EQUALS( "The extra free space at the end of the file was used", newPos, 17 );
    TS_ASSERT_EQUALS( mru.getFileLength(), 18);
    TS_ASSERT_EQUALS( mru.getFileUsed(), 18);


    //std::cout <<  ISaveableTesterWithFile::fakeFile << "!" << std::endl;
  }

};




//====================================================================================
class DiskMRUTestPerformance : public CxxTest::TestSuite
{
public:

  std::vector<ISaveableTester*> data;
  std::vector<ISaveableTesterWithSeek*> dataSeek;
  size_t num;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiskMRUTestPerformance *createSuite() { return new DiskMRUTestPerformance(); }
  static void destroySuite( DiskMRUTestPerformance *suite ) { delete suite; }

  DiskMRUTestPerformance()
  {
    num = 100000;
    data.clear();
    for (size_t i=0; i<num; i++)
    {
      data.push_back( new ISaveableTester(i) );
      data[i]->m_doSave = false; // Items won't do any real saving
    }
    dataSeek.clear();
    for (size_t i=0; i<100; i++)
      dataSeek.push_back( new ISaveableTesterWithSeek(i) );
  }

  void setUp()
  {
    ISaveableTester::fakeFile = "";
  }


  void test_smallCache_writeBuffer()
  {
    CPUTimer tim;
    DiskMRU mru(4, 3, true);
    for (int i=0; i<int(num); i++)
      mru.loading(data[i]);
    std::cout << tim << " to load " << num << " into MRU." << std::endl;
  }

  void test_smallCache_no_writeBuffer()
  {
    CPUTimer tim;
    DiskMRU mru(4, 0, false);
    for (int i=0; i<int(num); i++)
      mru.loading(data[i]);
    std::cout << tim << " to load " << num << " into MRU (no write cache)." << std::endl;
  }

  void test_largeCache_writeBuffer()
  {
    CPUTimer tim;
    DiskMRU mru(50000, 1000, true);
    for (int i=0; i<int(num); i++)
      mru.loading(data[i]);
    std::cout << tim << " to load " << num << " into MRU." << std::endl;
  }

  void test_largeCache_noWriteBuffer()
  {
    CPUTimer tim;
    DiskMRU mru(50000, 0, false);
    for (int i=0; i<int(num); i++)
      mru.loading(data[i]);
    std::cout << tim << " to load " << num << " into MRU (no write buffer)." << std::endl;
  }

  void test_withFakeSeeking()
  {
    CPUTimer tim;
    DiskMRU mru(100, 10, true);
    for (int i=0; i<int(dataSeek.size()); i++)
    {
      // Pretend you just loaded the data
      ISaveableTesterWithSeek::fakeSeekAndWrite( dataSeek[i]->getFilePosition() );
      // Now pretend you're adding it to the MRU and might write out old stuff.
      mru.loading(dataSeek[i]);
    }
    std::cout << tim << " to load " << dataSeek.size() << " into MRU with fake seeking. 0.095 sec = shortest possible time." << std::endl;
  }

  void test_withFakeSeeking_noWriteBuffer()
  {
    CPUTimer tim;
    DiskMRU mru(100, 0, false);
    for (int i=0; i<int(dataSeek.size()); i++)
    {
      // Pretend you just loaded the data
      ISaveableTesterWithSeek::fakeSeekAndWrite( dataSeek[i]->getFilePosition() );
      // Now pretend you're adding it to the MRU and might write out old stuff.
      mru.loading(dataSeek[i]);
    }
    std::cout << tim << " to load " << dataSeek.size() << " into MRU with fake seeking. 0.095 sec = shortest possible time." << std::endl;
  }

  void test_freeBlock()
  {
    CPUTimer tim;
    DiskMRU mru(100, 0, false);
    for (size_t i=0; i<100000; i++)
    {
      mru.freeBlock(i*100, (i%3==0) ? 100 : 50);
    }
    //mru.defragFreeBlocks();
    TS_ASSERT_EQUALS( mru.getFreeSpaceMap().size(), 66667);
    std::cout << tim << " to add " << 100000 << " blocks in the free space list." << std::endl;
  }

};

#endif /* MANTID_API_DISKMRUTEST_H_ */

