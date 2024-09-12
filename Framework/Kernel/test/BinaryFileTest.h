// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"
#include <cxxtest/TestSuite.h>
#include <sys/stat.h>

using namespace Mantid::Kernel;

using std::runtime_error;
using std::size_t;
using std::vector;

//==========================================================================================
/// Make the code clearer by having this an explicit type
using PixelType = uint32_t;
/// Type for the DAS time of flight (data file)
using DasTofType = uint32_t;
/// Structure that matches the form in the binary event list.
struct DasEvent {
  /// Time of flight.
  DasTofType tof;
  /// Pixel identifier as published by the DAS/DAE/DAQ.
  PixelType pid;
};

/** Creates a dummy file with so many bytes */
static void MakeDummyFile(const std::string &filename, size_t num_bytes) {
  std::vector<char> buffer(num_bytes);
  for (size_t i = 0; i < num_bytes; i++) {
    // Put 1,2,3 in 32-bit ints
    if (i % 4 == 0)
      buffer[i] = static_cast<char>(i / 4);
    else
      buffer[i] = 0;
  }

  std::ofstream myFile(filename.c_str(), std::ios::out | std::ios::binary);
  myFile.write(buffer.data(), num_bytes);
  myFile.close();
}

//==========================================================================================
class BinaryFileTest : public CxxTest::TestSuite {
private:
  BinaryFile<DasEvent> file;
  std::string dummy_file;

public:
  static BinaryFileTest *createSuite() { return new BinaryFileTest(); }
  static void destroySuite(BinaryFileTest *suite) { delete suite; }

  BinaryFileTest() { dummy_file = "dummy.bin"; }

  void testFileNotFound() { TS_ASSERT_THROWS(file.open("nonexistentfile.dat"), const std::invalid_argument &); }

  void testFileWrongSize() {

    MakeDummyFile(dummy_file, 3);
    TS_ASSERT_THROWS(file.open(dummy_file), const std::runtime_error &);
    file.close();
    std::filesystem::remove(dummy_file);
  }

  void testOpen() {
    MakeDummyFile(dummy_file, 20 * 8);

    // If this throws, then the file does not exist.
    file.open(dummy_file);
    // Right size?
    size_t num = 20;
    TS_ASSERT_EQUALS(file.getNumElements(), num);
    // Get it
    std::vector<DasEvent> data;
    TS_ASSERT_THROWS_NOTHING(data = file.loadAll());
    TS_ASSERT_EQUALS(data.size(), num);
    // Check the first event
    TS_ASSERT_EQUALS(data.at(0).tof, 0);
    TS_ASSERT_EQUALS(data.at(0).pid, 1);
    // Check the last event
    TS_ASSERT_EQUALS(data.at(num - 1).tof, 38);
    TS_ASSERT_EQUALS(data.at(num - 1).pid, 39);

    file.close();
    std::filesystem::remove(dummy_file);
  }

  void testLoadAllIntoVector() {
    MakeDummyFile(dummy_file, 20 * 8);
    file.open(dummy_file);

    // Right size?
    size_t num = 20;
    TS_ASSERT_EQUALS(file.getNumElements(), num);
    // Get it
    std::vector<DasEvent> data;
    TS_ASSERT_THROWS_NOTHING(data = file.loadAllIntoVector());
    TS_ASSERT_EQUALS(data.size(), num);
    // Check the first event
    TS_ASSERT_EQUALS(data.at(0).tof, 0);
    TS_ASSERT_EQUALS(data.at(0).pid, 1);
    // Check the last event
    TS_ASSERT_EQUALS(data.at(num - 1).tof, 38);
    TS_ASSERT_EQUALS(data.at(num - 1).pid, 39);
    file.close();
    std::filesystem::remove(dummy_file);
  }

  void testLoadInBlocks() {
    MakeDummyFile(dummy_file, 20 * 8);
    file.open(dummy_file);

    // Right size?
    size_t num = 20;
    TS_ASSERT_EQUALS(file.getNumElements(), num);
    // Get it
    size_t block_size = 10;
    auto data = std::make_unique<DasEvent[]>(block_size);
    size_t loaded_size = file.loadBlock(data.get(), block_size);
    // Yes, we loaded that amount
    TS_ASSERT_EQUALS(loaded_size, block_size);

    // Check the first event
    TS_ASSERT_EQUALS(data[0].tof, 0);
    TS_ASSERT_EQUALS(data[0].pid, 1);

    // Now try to load a lot more - going past the end
    block_size = 10;
    data = std::make_unique<DasEvent[]>(block_size);
    loaded_size = file.loadBlock(data.get(), block_size);
    TS_ASSERT_EQUALS(loaded_size, 10);

    // Check the last event
    TS_ASSERT_EQUALS(data[9].tof, 38);
    TS_ASSERT_EQUALS(data[9].pid, 39);
    file.close();
    std::filesystem::remove(dummy_file);
  }

  void testLoadBlockAt() {
    MakeDummyFile(dummy_file, 20 * 8);
    file.open(dummy_file);

    // Right size?
    size_t num = 20;
    TS_ASSERT_EQUALS(file.getNumElements(), num);
    // Get it
    size_t block_size = 10;
    auto data = std::make_unique<DasEvent[]>(block_size);
    size_t loaded_size = file.loadBlockAt(data.get(), 5, block_size);
    // Yes, we loaded that amount
    TS_ASSERT_EQUALS(loaded_size, block_size);

    // The first event is at index 5
    TS_ASSERT_EQUALS(data[0].tof, 10);
    TS_ASSERT_EQUALS(data[0].pid, 11);

    // Now try to load a lot more - going past the end
    block_size = 10;
    data = std::make_unique<DasEvent[]>(block_size);
    loaded_size = file.loadBlock(data.get(), block_size);
    TS_ASSERT_EQUALS(loaded_size, 5);
    file.close();
    std::filesystem::remove(dummy_file);
  }

  void testCallingDestructorOnUnitializedObject() { BinaryFile<DasEvent> file2; }

  void testReadingNotOpenFile() {
    BinaryFile<DasEvent> file2;
    std::vector<DasEvent> data;
    DasEvent *buffer = nullptr;
    TS_ASSERT_EQUALS(file2.getNumElements(), 0);
    TS_ASSERT_THROWS(file2.loadAll(), const std::runtime_error &);
    TS_ASSERT_THROWS(data = file2.loadAllIntoVector(), const std::runtime_error &);
    TS_ASSERT_THROWS(file2.loadBlock(buffer, 10), const std::runtime_error &);
  }
};
