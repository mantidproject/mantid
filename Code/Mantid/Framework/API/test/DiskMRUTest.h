#ifndef MANTID_API_DISKMRUTEST_H_
#define MANTID_API_DISKMRUTEST_H_


#include "MantidAPI/DiskMRU.h"
#include "MantidAPI/ISaveable.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/sequenced_index.hpp>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Kernel::CPUTimer;


class ISaveableTester : public ISaveable
{
public:
  ISaveableTester(size_t id) : ISaveable(id), m_safeToWrite(true){};

  virtual void save() const
  {
    // Fake writing to a file
    std::ostringstream out;
    out << getId() <<",";
    fakeFile += out.str();
  }

  virtual void load()
  {}

  virtual size_t getMRUMemory() const {return 1;};

  bool m_safeToWrite;
  virtual bool safeToWrite() const {return m_safeToWrite; }

  // File position = same as its ID
  virtual uint64_t getFilePosition() const { return uint64_t(getId()); }

  static std::string fakeFile;
};
// Declare the static member here.
std::string ISaveableTester::fakeFile = "";

class DiskMRUTest : public CxxTest::TestSuite
{
public:

  std::vector<ISaveableTester*> data;
  size_t num;

  void setUp()
  {
    // Create the ISaveables
    ISaveableTester::fakeFile = "";
    num = 10;
    data.clear();
    for (size_t i=0; i<num; i++)
      data.push_back( new ISaveableTester(i) );
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

    DiskMRU::item_list list;
    CPUTimer tim;
    for (size_t i=0; i<num; i++)
    {
      std::pair<DiskMRU::item_list::iterator,bool> p;
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


//    typedef boost::multi_index::multi_index_container<
//      ISaveable *,
//      boost::multi_index::indexed_by<
//        boost::multi_index::ordered_non_unique<size_t>
//      >
//    > item_list2;
//
//    item_list2 list2;
//    for (size_t i=0; i<num; i++)
//    {
//      //list2.insert(data[i]);
//    }
//    std::cout << tim << " to fill the list2." << std::endl;

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
  void test_simple()
  {
    ISaveableTester::fakeFile = "";

    // Room for 4 in the MRU, and 3 in the to-write cache
    DiskMRU mru(4, 3);

    // Nothing in cache
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 0);
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 0);

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
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,2,");
  }

  //--------------------------------------------------------------------------------
  /** MRU properly keeps recently used items at the top */
  void test_mru()
  {
    DiskMRU mru(4, 1);
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
    DiskMRU mru(4, 3);
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
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "1,5,9,");
  }

  //--------------------------------------------------------------------------------
  /** Any ISaveable that says it can't be written remains in the cache */
  void test_skipsUnsafeToWriteBlocks()
  {
    DiskMRU mru(4, 3);
    mru.loading(data[0]);
    mru.loading(data[1]); data[1]->m_safeToWrite = false; // Won't get written out
    mru.loading(data[2]);
    // These 4 at the end will be in the cache
    for (size_t i=3; i<7; i++)
      mru.loading(data[i]);
    TS_ASSERT_EQUALS( mru.getMemoryUsed(), 4);

    // Item #1 was skipped and is still in the buffer!
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,2,");
    TS_ASSERT_EQUALS( mru.getMemoryToWrite(), 1);
  }

};


#endif /* MANTID_API_DISKMRUTEST_H_ */

