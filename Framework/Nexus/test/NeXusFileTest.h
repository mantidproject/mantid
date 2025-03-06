// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"

#include "MantidNexus/NeXusFile.hpp"
#include "napi_test_util.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace NeXus;
using NexusNapiTest::write_dmc01;
using NexusNapiTest::write_dmc02;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

namespace {
const std::string DMC01("dmc01cpp");
const std::string DMC02("dmc02cpp");

/**
 * Let's face it, std::string is poorly designed,
 * and this is the constructor that it needed to have.
 * Initialize a string from a c-style formatting string.
 */
std::string strmakef(const char *fmt, ...) {
  char buf[256];

  va_list args;
  va_start(args, fmt);
  const auto r = std::vsnprintf(buf, sizeof buf, fmt, args);
  va_end(args);

  if (r < 0)
    // conversion failed
    return {};

  const size_t len = r;
  if (len < sizeof buf)
    // we fit in the buffer
    return {buf, len};

  std::string s(len, '\0');
  va_start(args, fmt);
  std::vsnprintf(&(*s.begin()), len + 1, fmt, args);
  va_end(args);
  return s;
}

void removeFile(const std::string &filename) {
  if (std::filesystem::exists(filename)) {
    std::filesystem::remove(filename);
  }
}

} // namespace

class NeXusFileTest : public CxxTest::TestSuite {
public:
  void do_test_write(const string &filename, NXaccess create_code) {
    std::cout << "writeTest(" << filename << ") started\n";
    NeXus::File file(filename, create_code);
    // create group
    file.makeGroup("entry", "NXentry", true);
    // group attributes
    file.putAttr("hugo", "namenlos");
    file.putAttr("cucumber", "passion");
    // put string
    file.writeData("ch_data", "NeXus_data");

    // 2d array
    vector<int> array_dims;
    array_dims.push_back(5);
    array_dims.push_back(4);
    char c1_array[5][4] = {
        {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
    file.makeData("c1_data", NXnumtype::CHAR, array_dims, true);
    file.putData(&c1_array);
    file.closeData();

    // 1d uint8 array
    vector<uint8_t> i1_array;
    for (uint8_t i = 0; i < 4; i++) {
      i1_array.push_back(static_cast<uint8_t>(i + 1));
    }
    file.writeData("i1_data", i1_array);

    // 1d int16 array
    vector<int16_t> i2_array;
    for (int16_t i = 0; i < 4; i++) {
      i2_array.push_back(static_cast<int16_t>(1000 * (i + 1)));
    }
    file.writeData("i2_data", i2_array);

    // 1d int32 data
    vector<int32_t> i4_array;
    for (int32_t i = 0; i < 4; i++) {
      i4_array.push_back(1000000 * (i + 1));
    }
    file.writeData("i4_data", i4_array);

    // 2d float data
    vector<float> r4_array;
    for (size_t i = 0; i < 5 * 4; i++) {
      r4_array.push_back(static_cast<float>(i));
    }
    file.writeData("r4_data", r4_array, array_dims);

    // 2d double data - slab test
    vector<double> r8_array;
    for (size_t i = 0; i < 5 * 4; i++) {
      r8_array.push_back(static_cast<double>(i + 20));
    }
    file.makeData("r8_data", NXnumtype::FLOAT64, array_dims, true);
    vector<int> slab_start;
    slab_start.push_back(4);
    slab_start.push_back(0);
    vector<int> slab_size;
    slab_size.push_back(1);
    slab_size.push_back(4);
    file.putSlab(&(r8_array[16]), slab_start, slab_size);
    slab_start[0] = 0;
    slab_start[1] = 0;
    slab_size[0] = 4;
    slab_size[1] = 4;
    file.putSlab(&(r8_array[0]), slab_start, slab_size);

    // add some attributes
    std::cout << "writing attributes to r8_data" << std::endl;
    file.putAttr("ch_attribute", "NeXus");
    file.putAttr("i4_attribute", 42);
    file.putAttr("r4_attribute", 3.14159265);
    std::cout << "... done" << std::endl;

    // set up for creating a link
    NXlink link = file.getDataID();
    file.closeData();

    // int64 tests
#if HAVE_LONG_LONG_INT
    vector<int64_t> grossezahl{12, 555555555555LL, 23, 777777777777LL};
#else
    vector<int64_t> grossezahl{12, 555555, 23, 77777};
#endif
    if (create_code != NXACC_CREATE4) {
      file.writeData("grosszahl", grossezahl);
    }

    // create a new group inside this one
    file.makeGroup("data", "NXdata", true);

    // create a link
    file.makeLink(link);

    // compressed data
    array_dims[0] = 100;
    array_dims[1] = 20;
    vector<int> comp_array;
    for (int i = 0; i < array_dims[0]; i++) {
      for (int j = 0; j < array_dims[1]; j++) {
        comp_array.push_back(i);
      }
    }
    vector<int> cdims;
    cdims.push_back(20);
    cdims.push_back(20);
    file.writeCompData("comp_data", comp_array, array_dims, NeXus::LZW, cdims);

    // ---------- Test write Extendible Data --------------------------
    std::vector<int> data(10, 123);
    file.makeGroup("extendible_data", "NXdata", 1);
    file.writeExtendibleData("mydata1", data);
    file.writeExtendibleData("mydata2", data, 1000);
    std::vector<int64_t> dims(2);
    dims[0] = 5;
    dims[1] = 2;
    std::vector<int64_t> chunk(2, 2);
    file.writeExtendibleData("my2Ddata", data, dims, chunk);
    file.putAttr("string_attrib", "some short string");

    // Data vector can grow
    for (size_t i = 0; i < 6; i++)
      data.push_back(456);
    data[0] = 789;
    file.writeUpdatedData("mydata1", data);

    dims[0] = 8;
    dims[1] = 2;
    file.writeUpdatedData("my2Ddata", data, dims);

    // Data vector can also shrink!
    data.clear();
    data.resize(5, 234);
    file.writeUpdatedData("mydata2", data);

    // Exit the group
    file.closeGroup();
    // ---------- End Test write Extendible Data --------------------------

    // simple flush test
    file.flush();

    // real flush test
    file.makeData("flush_data", NeXus::getType<int>(), NX_UNLIMITED, true);
    vector<int> slab_array;
    slab_array.push_back(0);
    for (int i = 0; i < 7; i++) {
      slab_array[0] = i;
      file.putSlab(slab_array, i, 1);
      file.flush();
      file.openData("flush_data");
    }
    file.closeData();
    file.closeGroup();

    // create a sample
    file.makeGroup("sample", "NXsample", true);
    file.writeData("ch_data", "NeXus sample");

    // make more links
    NXlink glink = file.getGroupID();
    file.openPath("/");
    file.makeGroup("link", "NXentry", true);
    file.makeLink(glink);
    std::cout << "writeTest(" << filename << ") successful\n";

    TS_ASSERT_EQUALS(std::filesystem::exists(filename), true);
  }

  void do_test_read(const string &filename) {
    std::cout << "readTest(" << filename << ") started\n";
    const string SDS("SDS");
    // top level file information
    NeXus::File file(filename);
    file.openGroup("entry", "NXentry");

    // Test getDataCoerce() -------------------
    std::vector<int> ints;
    std::vector<double> doubles;

    ints.clear();
    file.openData("i1_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1);
    file.closeData();

    ints.clear();
    file.openData("i2_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1000);
    file.closeData();

    ints.clear();
    file.openData("i4_data");
    file.getDataCoerce(ints);
    TS_ASSERT_EQUALS(ints.size(), 4);
    TS_ASSERT_EQUALS(ints[0], 1000000)
    file.closeData();

    doubles.clear();
    file.openData("r4_data");
    file.getDataCoerce(doubles);
    TS_ASSERT_EQUALS(doubles.size(), 20);
    TS_ASSERT_EQUALS(doubles[1], 1.0)
    file.closeData();

    doubles.clear();
    file.openData("r8_data");
    file.getDataCoerce(doubles);
    TS_ASSERT_EQUALS(doubles.size(), 20);
    TS_ASSERT_EQUALS(doubles[1], 21.0)
    file.closeData();

    // Throws when you coerce to int from a real/double source
    ints.clear();
    file.openData("r8_data");
    TS_ASSERT_THROWS_ANYTHING(file.getDataCoerce(ints));
    file.closeData();

    // Close the "entry" group
    file.closeGroup();

    // openpath checks
    file.openPath("/entry/data/comp_data");
    file.openPath("/entry/data/comp_data");
    file.openPath("../r8_data");
    cout << "NXopenpath checks OK\n";

    // everything went fine
    std::cout << "readTest(" << filename << ") successful\n";
  }

  void do_test_loadPath(const string &filename) {
    if (getenv("NX_LOAD_PATH") != NULL) {
      TS_ASSERT_THROWS_NOTHING(NeXus::File file(filename));
      cout << "Success loading NeXus file from path" << endl;
    } else {
      cout << "NX_LOAD_PATH variable not defined. Skipping testLoadPath\n";
    }
  }

  void test_readwrite_hdf5() {
    NXaccess const nx_creation_code = NXACC_CREATE5;
    string const fileext = ".h5";
    string const filename("napi_test_cpp" + fileext);

    removeFile(filename); // in case last round failed

    // try writing a file
    do_test_write(filename, nx_creation_code);

    // try reading a file
    do_test_read(filename);

    removeFile(filename); // cleanup

    // try using the load path
    do_test_loadPath(DMC01 + fileext);
    do_test_loadPath(DMC02 + fileext);

    removeFile(DMC01 + fileext);
    removeFile(DMC02 + fileext);
  }
};

/**
 * These correspond to former napi tests
 * - leak_test1
 * - leak_test2
 * - leak_test3
 */
class NeXusFileLeakTest : public CxxTest::TestSuite {
public:
  static NeXusFileLeakTest *createSuite() { return new NeXusFileLeakTest(); }
  static void destroySuite(NeXusFileLeakTest *suite) { delete suite; }

  NeXusFileLeakTest() : CxxTest::TestSuite() { Mantid::API::FrameworkManager::Instance(); }

  void test_leak1() {
    int const nReOpen = 1000;
    cout << "Running for " << nReOpen << " iterations\n";
    std::string const szFile("leak_test1.nxs");

    removeFile(szFile); // in case it was left over from previous run

    File file_obj(szFile, NXACC_CREATE5);
    file_obj.close();

    for (int iReOpen = 0; iReOpen < nReOpen; iReOpen++) {
      if (0 == iReOpen % 100) {
        cout << "loop count " << iReOpen << "\n";
      }

      file_obj = File(szFile, NXACC_RDWR);
      file_obj.close();
    }

    removeFile(szFile); // cleanup
  }

  void test_leak2() {
    int const nFiles = 10;
    int const nEntry = 10;
    int const nData = 10;
    vector<short int> const i2_array{1000, 2000, 3000, 4000};

    cout << strmakef("Running for %d iterations", nFiles);
    NXaccess access_mode = NXACC_CREATE5;
    std::string strFile;

    for (int iFile = 0; iFile < nFiles; iFile++) {
      strFile = strmakef("leak_test2_%03d.nxs", iFile);
      removeFile(strFile);
      cout << "file " << strFile << "\n";

      File fileid(strFile, access_mode);

      for (int iEntry = 0; iEntry < nEntry; iEntry++) {
        std::string oss(strmakef("entry_%d", iEntry));
        fileid.makeGroup(oss, "NXentry");
        fileid.openGroup(oss, "NXentry");
        for (int iNXdata = 0; iNXdata < nData; iNXdata++) {
          std::string oss2(strmakef("data_%d", iNXdata));
          fileid.makeGroup(oss2, "NXdata");
          fileid.openGroup(oss2, "NXdata");
          for (int iData = 0; iData < nData; iData++) {
            std::string oss3(strmakef("i2_data_%d", iData));
            DimVector dims({(int64_t)i2_array.size()});
            fileid.makeData(oss3, NXnumtype::INT16, dims);
            fileid.openData(oss3);
            fileid.putData(&i2_array);
            fileid.closeData();
          }
          fileid.closeGroup();
        }
        fileid.closeGroup();
      }
      fileid.close();
      removeFile(strFile);
    }
  }

  void test_leak3() {
    const int nFiles = 10;
    const int nEntry = 2;
    const int nData = 2;
    DimVector array_dims({512, 512});
    std::string const szFile("leak_test.nxs");
    const int iBinarySize = 512 * 512;
    int aiBinaryData[iBinarySize];

    for (int i = 0; i < iBinarySize; i++) {
      aiBinaryData[i] = rand();
    }

    for (int iFile = 0; iFile < nFiles; iFile++) {
      cout << "file " << iFile << "\n";

      File fileid(szFile, NXACC_CREATE5);

      for (int iEntry = 0; iEntry < nEntry; iEntry++) {
        std::string oss(strmakef("entry_%d", iEntry));

        fileid.makeGroup(oss, "NXentry");
        fileid.openGroup(oss, "NXentry");
        for (int iNXdata = 0; iNXdata < nData; iNXdata++) {
          std::string oss2(strmakef("data_%d", iNXdata));
          fileid.makeGroup(oss2, "NXdata");
          fileid.openGroup(oss2, "NXdata");
          fileid.getGroupID();
          for (int iData = 0; iData < nData; iData++) {
            std::string oss3(strmakef("i2_data_%d", iData));
            fileid.makeCompData(oss3, NXnumtype::INT16, array_dims, NXcompression::LZW, array_dims);
            fileid.openData(oss3);
            fileid.putData(&aiBinaryData);
            fileid.closeData();
          }
          fileid.closeGroup();
        }
        fileid.closeGroup();
      }

      fileid.close();

      // Delete file
      removeFile(szFile);
    }
  }
};
