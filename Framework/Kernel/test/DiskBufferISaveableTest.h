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
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <cxxtest/TestSuite.h>
#include <mutex>

using namespace Mantid;
using namespace Mantid::Kernel;
using Mantid::Kernel::CPUTimer;

//====================================================================================
class ISaveableTester : public ISaveable {
  size_t id;

public:
  ISaveableTester(size_t idIn) : ISaveable(), id(idIn) {}
  ~ISaveableTester() override = default;
  size_t getFileId() const { return id; }
  //-----------------------------------------------------------------------------------------------

  /// Save the data - to be overriden
  void save() const override {
    // Fake writing to a file
    std::ostringstream out;
    out << id << ",";
    streamMutex.lock();
    fakeFile += out.str();
    streamMutex.unlock();
    this->m_wasSaved = true;
  }

  /// Load the data - to be overriden
  void load() override { this->setLoaded(true); };

  /// Method to flush the data to disk and ensure it is written.
  void flushData() const override {};
  /// remove objects data from memory
  void clearDataFromMemory() override { this->setLoaded(false); };

  /** @return the amount of memory that the object takes as a whole.
      For filebased objects it should be the amount the object occupies in
     memory plus the size it occupies in file if the object has not been fully
     loaded
      or modified.
     * If the object has never been loaded, this should be equal to number of
     data points in the file
     */
  uint64_t getTotalDataSize() const override { return 1; }
  /// the data size kept in memory
  size_t getDataMemorySize() const override { return 1; };

  static std::string fakeFile;
  static std::mutex streamMutex;
};
// Declare the static members here.
std::string ISaveableTester::fakeFile = "";
std::mutex ISaveableTester::streamMutex;

//====================================================================================
class DiskBufferISaveableTest : public CxxTest::TestSuite {
public:
  std::vector<std::unique_ptr<ISaveableTester>> data;
  size_t num;
  std::vector<std::unique_ptr<ISaveableTester>> bigData;
  long BIG_NUM;

  void setUp() override {
    // Create the ISaveables

    num = 10;
    data.clear();
    for (size_t i = 0; i < num; i++)
      data.emplace_back(std::make_unique<ISaveableTester>(i));
    BIG_NUM = 1000;
    bigData.clear();
    bigData.reserve(BIG_NUM);
    for (long i = 0; i < BIG_NUM; i++)
      bigData.emplace_back(std::make_unique<ISaveableTester>(i));
  }

  void tearDown() override {
    data.clear();
    bigData.clear();

    ISaveableTester::fakeFile = "";
  }
  void testIsaveable() {
    ISaveableTester Sav(0);

    TSM_ASSERT_EQUALS("Default data ID should be 0 ", 0, Sav.getFileId());
    TSM_ASSERT_EQUALS("Default file position is wrong ", std::numeric_limits<uint64_t>::max(), Sav.getFilePosition());
    TSM_ASSERT_EQUALS("Default size should be 0 ", 0, Sav.getFileSize());

    ISaveableTester CopyTester(Sav);
    TSM_ASSERT_EQUALS("Default data ID should be 0 ", 0, CopyTester.getFileId());
    TSM_ASSERT_EQUALS("Default file position is wrong ", std::numeric_limits<uint64_t>::max(),
                      CopyTester.getFilePosition());
    TSM_ASSERT_EQUALS("Default size should be 0 ", 0, CopyTester.getFileSize());
  }

  //--------------------------------------------------------------------------------
  /** Getting and setting the cache sizes */
  void test_set_and_get_methods() {
    DiskBuffer dbuf(3);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferSize(), 3);
    dbuf.setWriteBufferSize(11);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferSize(), 11);
  }

  //--------------------------------------------------------------------------------
  /** Test calling toWrite() */
  void test_basic() {
    // No MRU, 3 in the to-write cache
    DiskBuffer dbuf(2);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferSize(), 2);

    // Nothing in cache
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);

    dbuf.toWrite(data[0].get());
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 1);
    dbuf.toWrite(data[1].get());
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 2);
    dbuf.toWrite(data[2].get());
    // Write buffer now got flushed out
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);

    // The "file" was written out this way (the right order):
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "2,1,0,");
    ISaveableTester::fakeFile = "";

    // If you add the same one multiple times, it only is tracked once in the
    // to-write buffer.
    dbuf.toWrite(data[4].get());
    dbuf.toWrite(data[4].get());
    dbuf.toWrite(data[4].get());
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 1);
  }
  //--------------------------------------------------------------------------------
  /** Set a buffer size of 0 */
  void test_basic_WriteBuffer() {
    // No write buffer
    DiskBuffer dbuf(0);
    TS_ASSERT_EQUALS(dbuf.getWriteBufferSize(), 0);
    // Nothing in cache
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);

    dbuf.toWrite(data[0].get());
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,");
    dbuf.toWrite(data[1].get());
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,");
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);
    dbuf.toWrite(data[2].get());
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,2,");
    dbuf.toWrite(data[3].get());
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,2,3,");
    dbuf.toWrite(data[4].get());
    // Everything get written immidiately;
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,1,2,3,4,");
    ISaveableTester::fakeFile = "";
  }

  //--------------------------------------------------------------------------------
  /// Empty out the cache with the flushCache() method
  void test_flushCache() {
    DiskBuffer dbuf(10);
    for (size_t i = 0; i < 6; i++)
      dbuf.toWrite(data[i].get());
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 6);
    // Nothing written out yet
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "");
    dbuf.flushCache();
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "5,4,3,2,1,0,");
    // Nothing left in cache
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);
  }

  //--------------------------------------------------------------------------------
  /** Buffer allocates file positions, so sorts according to the alloction order
   * by the DiskBuffer */
  void test_writesOutDBOrder() {
    // Room for 3 in the to-write cache
    DiskBuffer dbuf(3);
    // These 3 will get written out
    dbuf.toWrite(data[5].get());
    TSM_ASSERT_EQUALS("Not yet written to file", std::numeric_limits<uint64_t>::max(), data[5]->getFilePosition());

    dbuf.toWrite(data[1].get());
    TSM_ASSERT_EQUALS("Not yet written to file", std::numeric_limits<uint64_t>::max(), data[1]->getFilePosition());
    dbuf.toWrite(data[9].get());
    TSM_ASSERT_EQUALS("Not yet written to file", std::numeric_limits<uint64_t>::max(), data[9]->getFilePosition());
    dbuf.flushCache();

    TSM_ASSERT_EQUALS("Is written to file at ", 0, data[9]->getFilePosition());
    TSM_ASSERT_EQUALS("Is written to file at ", 1, data[1]->getFilePosition());
    TSM_ASSERT_EQUALS("Is written to file at ", 2, data[5]->getFilePosition());
    // These 4 at the end will be in the cache
    dbuf.toWrite(data[2].get());
    TSM_ASSERT_EQUALS("Not yet written to file", std::numeric_limits<uint64_t>::max(), data[2]->getFilePosition());
    dbuf.toWrite(data[3].get());
    TSM_ASSERT_EQUALS("Not yet written to file", std::numeric_limits<uint64_t>::max(), data[3]->getFilePosition());
    dbuf.toWrite(data[4].get());
    TSM_ASSERT_EQUALS("Not yet written to file", std::numeric_limits<uint64_t>::max(), data[4]->getFilePosition());
    dbuf.flushCache();

    TSM_ASSERT_EQUALS("Is written to file at ", 3, data[4]->getFilePosition());
    TSM_ASSERT_EQUALS("Is written to file at ", 4, data[3]->getFilePosition());
    TSM_ASSERT_EQUALS("Is written to file at ", 5, data[2]->getFilePosition());

    dbuf.toWrite(data[6].get());
    TSM_ASSERT_EQUALS("Not yet written to file", std::numeric_limits<uint64_t>::max(), data[6]->getFilePosition());

    // 1 left in the buffer
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 1);
  }

  //--------------------------------------------------------------------------------
  /** Extreme case with nothing writable but exceeding the writable buffer */
  void test_noWriteBuffer_nothingWritable() {
    // Room for 4 in the write buffer
    DiskBuffer dbuf(4);
    for (size_t i = 0; i < 9; i++) {
      data[i]->setBusy(true);
      dbuf.toWrite(data[i].get());
    }
    // We ended up with too much in the buffer since nothing could be written.
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 9);
    // Let's make it all writable
    for (size_t i = 0; i < 9; i++)
      data[i]->setBusy(false);
    // Trigger a write
    dbuf.toWrite(data[8].get());
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);
    // And all of these get written out at once
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "8,7,6,5,4,3,2,1,0,");
  }

  //--------------------------------------------------------------------------------
  /** If a block gets deleted it needs to be taken out of the caches */
  void test_objectDeleted() {
    // Room for 6 in the to-write cache
    DiskBuffer dbuf(6);
    // Fill the buffer
    for (size_t i = 0; i < 5; i++)
      dbuf.toWrite(data[i].get());
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 5);

    // First let's get rid of something in to to-write buffer

    dbuf.objectDeleted(data[1].get());
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 4);
    TSM_ASSERT_EQUALS("The data have never been written", dbuf.getFreeSpaceMap().size(), 0);

    dbuf.flushCache();
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "4,3,2,0,");
  }

  //--------------------------------------------------------------------------------
  /** Any ISaveable that says it can't be written remains in the cache */
  void test_skips_dataBusy_Blocks() {
    DiskBuffer dbuf(3);
    dbuf.toWrite(data[0].get());
    dbuf.toWrite(data[1].get());
    data[1]->setBusy(true); // Won't get written out
    dbuf.toWrite(data[2].get());
    dbuf.flushCache();

    // Item #1 was skipped and is still in the buffer!
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "2,0,");
    // TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "0,2,");
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 1);

    // But it'll get written out next time
    ISaveableTester::fakeFile = "";
    data[1]->setBusy(false);
    dbuf.flushCache();
    TS_ASSERT_EQUALS(ISaveableTester::fakeFile, "1,");
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), 0);
  }

  //--------------------------------------------------------------------------------
  /** Accessing the map from multiple threads simultaneously does not segfault
   */
  void test_thread_safety() {
    // Room for 3 in the to-write cache
    DiskBuffer dbuf(3);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (long i = 0; i < int(BIG_NUM); i++) {
      dbuf.toWrite(bigData[i].get());
    }
    // std::cout << ISaveableTester::fakeFile << '\n';
  }

  void test_addAndRemove() {
    long DATA_SIZE(500);
    std::vector<size_t> indexToRemove(DATA_SIZE);
    std::vector<ISaveable *> objToAdd(DATA_SIZE);
    long iStep = BIG_NUM / DATA_SIZE;
    if (iStep < 1 || DATA_SIZE > BIG_NUM) {
      TSM_ASSERT("Test has wrong setting", false);
      return;
    }
    for (long i = 0; i < DATA_SIZE; i++) {
      indexToRemove[i] = i * iStep;
      objToAdd[i] = new ISaveableTester(size_t(BIG_NUM + i * iStep));
    }

    DiskBuffer dbuf(size_t(BIG_NUM + DATA_SIZE));
    Kernel::Timer clock;
    for (long i = 0; i < BIG_NUM; i++) {
      dbuf.toWrite(bigData[i].get());
    }
    std::cout << "\nFinished DiskBuffer insertion performance test, inserted " << BIG_NUM << " objects on 1 thread in "
              << clock.elapsed() << " sec\n";

    for (long i = 0; i < DATA_SIZE; i++) {
      dbuf.objectDeleted(bigData[indexToRemove[i]].get());
      dbuf.toWrite(objToAdd[i]);
      dbuf.toWrite(bigData[indexToRemove[i]].get());
    }
    std::cout << "Finished DiskBuffer inserting/deleting performance test, 1 "
                 "thread in "
              << clock.elapsed() << " sec\n";
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), BIG_NUM + DATA_SIZE);

    // cleanup memory
    for (long i = 0; i < DATA_SIZE; i++) {
      delete objToAdd[i];
    }
  }

  void test_addAndRemoveMultithread() {
    long DATA_SIZE(500);
    std::vector<size_t> indexToRemove(DATA_SIZE);
    std::vector<ISaveable *> objToAdd(DATA_SIZE);
    long iStep = BIG_NUM / DATA_SIZE;
    if (iStep < 1 || DATA_SIZE > BIG_NUM) {
      TSM_ASSERT("Test has wrong setting", false);
      return;
    }
    for (long i = 0; i < DATA_SIZE; i++) {
      indexToRemove[i] = i * iStep;
      objToAdd[i] = new ISaveableTester(BIG_NUM + i * iStep);
    }

    DiskBuffer dbuf(BIG_NUM + DATA_SIZE);
    Kernel::Timer clock;
    PARALLEL_FOR_NO_WSP_CHECK()
    for (long i = 0; i < BIG_NUM; i++) {
      dbuf.toWrite(bigData[i].get());
    }
    std::cout << "\nFinished DiskBuffer insertion performance test, inserted " << BIG_NUM
              << " objects on multithread in " << clock.elapsed() << " sec\n";

    PARALLEL_FOR_NO_WSP_CHECK()
    for (long i = 0; i < DATA_SIZE; i++) {
      dbuf.objectDeleted(bigData[indexToRemove[i]].get());
      dbuf.toWrite(objToAdd[i]);
      dbuf.toWrite(bigData[indexToRemove[i]].get());
    }
    std::cout << "Finished DiskBuffer inserting/deleting performance test, "
                 "multithread in "
              << clock.elapsed() << " sec\n";
    TS_ASSERT_EQUALS(dbuf.getWriteBufferUsed(), BIG_NUM + DATA_SIZE);

    // cleanup memory
    for (long i = 0; i < DATA_SIZE; i++) {
      delete objToAdd[i];
    }
  }
};
//====================================================================================
class DiskBufferISaveableTestPerformance : public CxxTest::TestSuite {
public:
  std::vector<ISaveableTester *> data;
  size_t num;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiskBufferISaveableTestPerformance *createSuite() { return new DiskBufferISaveableTestPerformance(); }
  static void destroySuite(DiskBufferISaveableTestPerformance *suite) { delete suite; }

  DiskBufferISaveableTestPerformance() {
    num = 100000;
    data.clear();
    for (size_t i = 0; i < num; i++) {
      data.emplace_back(new ISaveableTester(i));
      data[i]->setBusy(true); // Items won't do any real saving
    }
  }
  void setUp() override { ISaveableTester::fakeFile = ""; }

  void test_smallCache_writeBuffer() {
    CPUTimer tim;
    DiskBuffer dbuf(3);
    for (auto &i : data) {
      dbuf.toWrite(i);
      i->setBusy(false);
    }
    std::cout << " Elapsed : " << tim << " to load " << num << " into MRU.\n";
  }
  //
  void test_smallCache_no_writeBuffer() {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (auto &i : data) {
      i->setBusy(true); // Items won't do any real saving
    }

    for (auto &i : data) {
      dbuf.toWrite(i);
      i->setBusy(false);
    }
    std::cout << " Elapsed : " << tim << " to load " << num << " into MRU (no write cache).\n";
  }

  void test_largeCache_writeBuffer() {
    CPUTimer tim;
    DiskBuffer dbuf(1000);
    for (auto &i : data) {
      dbuf.toWrite(i);
      i->setBusy(false);
    }
    std::cout << tim << " to load " << num << " into MRU.\n";
  }

  void test_largeCache_noWriteBuffer() {
    CPUTimer tim;
    DiskBuffer dbuf(0);
    for (auto &i : data) {
      dbuf.toWrite(i);
      i->setBusy(false);
    }
    std::cout << " Elapsed : " << tim << " to load " << num << " into MRU (no write buffer).\n";
  }
};
