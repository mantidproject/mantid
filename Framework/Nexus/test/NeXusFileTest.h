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
      TS_ASSERT_THROWS_NOTHING(NeXus::File file(filename, NXACC_RDWR));
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

  /**
   * These correspond to former napi tests
   * - leak_test1
   * - leak_test2
   * - leak_test3
   */

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

  /**
   * These tests correspond to tests inside napi_test.cpp
   * Refactored to work as unit tests with asserts and comparisons
   * as opposed to a single long print-out test
   */

private:
  File do_prep_files(std::string const nxFile) {
    removeFile(nxFile); // in case previous run didn't clean up

    std::cout << "Creating \"" << nxFile << "\"" << std::endl;
    // create file
    File fileid(nxFile, NXACC_CREATE5);

    fileid.makeGroup("entry", "NXentry");
    fileid.openGroup("entry", "NXentry");
    fileid.putAttr("hugo", "namenlos");
    fileid.putAttr("cucumber", "passion");
    return fileid;
  }

  template <typename T> void do_rw_test(File &fileid, std::string const &dataname, T const &data) {
    cout << "Testing attribute " << dataname << "\n";
    // write
    fileid.writeData(dataname, data);

    // read
    T output;
    fileid.readData(dataname, output);

    // compare
    TS_ASSERT_EQUALS(data, output);
  }

  template <size_t N, size_t M, typename T>
  void do_rw2darray_test(File &fileid, std::string const &dataname, T const (&data)[N][M]) {
    cout << "Testing attribute " << dataname << "\n";
    // write
    fileid.makeData(dataname, getType<T>(), DimVector({N, M}));
    fileid.openData(dataname);
    fileid.putData(data);
    fileid.closeData();

    // read
    T output[N][M];
    fileid.openData(dataname);
    fileid.getData(&(output[0][0]));
    fileid.closeData();

    // compare
    for (size_t i = 0; i < N; i++) {
      for (size_t j = 0; j < M; j++) {
        TS_ASSERT_EQUALS(data[i][j], output[i][j]);
      }
    }
  }

  // template<typename T>
  // void do_rwslabvec_test(File &fileid, std::string const dataname, vector<T> const &data) {
  //   cout << "Testing attribute " << dataname << "\n";

  //   // write
  //   dimsize_t start(0), size(data.size());
  //   fileid.makeData(dataname, getType<T>(), size);
  //   fileid.openData(dataname);
  //   fileid.putSlab(data, start, size);
  //   fileid.closeData();

  //   // read
  //   vector<T> output;
  //   fileid.openData(dataname);
  //   fileid.getSlab(&output, start, size);

  //   // compare
  //   TS_ASSERT_EQUALS(data, output);

  // fileid.putAttr("ch_attribute", ch_test_data, strlen(ch_test_data), NXnumtype::CHAR);

  // int i = 42;
  // fileid.putAttr("i4_attribute", &i, 1, Nxnumtype::INT32);
  // float r = 3.14159265f;
  // fileid.putattr("r4_attribute", &r, 1, NXnumtype::FLOAT32);
  // dlink = fileid.getDataID();
  // fileid.closeData();
  // }

  // // read test
  // void do_read_test(std::string const nxFile) {

  //   char name[NX_MAXNAMELEN], char_class[NX_MAXNAMELEN], char_buffer[128];
  //   char group_name[NX_MAXNAMELEN], class_name[NX_MAXNAMELEN];
  //   char path[512];

  //   std::cout << "Read/Write to read \"" << nxFile << "\"" << std::endl;
  //   File fileid(nxFile, NXACC_RDWR);

  //   NXnumtype NXtype;
  //   NXstatus entry_status, attr_status;
  //   int NXrank, NXdims[32];
  //   for (auto ainfo : fileid.getAttrInfos()) {
  //     if (ainfo.type == NXnumtype::CHAR) {
  //       NXlen = sizeof(char_buffer);
  //       fileid.getAttr(ainfo.name, char_buffer, &NXlen, &NXtype);
  //     }
  //   }
  //   fileid.openGroup("entry", "NXentry");
  //   for (auto ainfo : fileid.getAttrInfos()) {
  //     if (ainfo.type == NXnumtype::CHAR) {
  //       NXlen = sizeof(char_buffer);
  //       fileid.getAttr(ainfo,name, char_buffer, &NXlen, ainfo.type);
  //     }
  //   }
  //   for (auto entry : fileid.getEntries()) {
  //     if (entry.class.substr("SDS") != 0) {
  //       if (entry_status != NXstatus::NX_EOD) {
  //         printf("   Subgroup: %s(%s)\n", name, char_class);
  //         entry_status = NXstatus::NX_OK;
  //       }
  //     } else {
  //       void *data_buffer;
  //       if (entry_status == NXstatus::NX_OK) {
  //         fileid.openData(name);
  //         fileid.getPath(path, 512);
  //         fileid.getInfo(&NXrank, NXdims, &NXtype);

  //         int n = 1;
  //         for (int k = 0; k < NXrank; k++) {
  //           n *= NXdims[k];
  //         }
  //         if (entry.type == NXnumtype::CHAR) {
  //           fileid.getData(data_buffer);
  //         } else if (entry.type != NXnumtype::FLOAT32 && entry.type != NXnumtype::FLOAT64) {
  //           fileid.getData(data_buffer);
  //         } else {
  //           slab_start = {0, 0};
  //           slab_size = {1, 4};
  //           if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
  //             return TEST_FAILED;
  //           print_data("\n      ", std::cout, data_buffer, NXtype, 4);
  //           slab_start[0] = TEST_FAILED;
  //           if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
  //             return TEST_FAILED;
  //           print_data("      ", std::cout, data_buffer, NXtype, 4);
  //           slab_start[0] = 2;
  //           if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
  //             return TEST_FAILED;
  //           print_data("      ", std::cout, data_buffer, NXtype, 4);
  //           slab_start[0] = 3;
  //           if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
  //             return TEST_FAILED;
  //           print_data("      ", std::cout, data_buffer, NXtype, 4);
  //           slab_start[0] = 4;
  //           if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
  //             return TEST_FAILED;
  //           print_data("      ", std::cout, data_buffer, NXtype, 4);
  //           if (NXgetattrinfo(fileid, &i) != NXstatus::NX_OK)
  //             return TEST_FAILED;
  //           if (i > 0) {
  //             printf("      Number of attributes : %d\n", i);
  //           }
  //           for (auto ainfo : fileid.getAttrInfos()) {
  //             switch (ainfo.type) {
  //             case NXnumtype::INT32:
  //               fileid.getAttr(ainfo.name, &i, &NXlen, ainfo.type);
  //               break;
  //             case NXnumtype::FLOAT32:
  //               fileid.getAttr(ainfo.name, &r, &NXlen, ainfo.type);
  //               break;
  //             case NXnumtype::CHAR:
  //               NXlen = sizeof(char_buffer);
  //               fileid.getAttr(name, char_bufer, &NXlen, ainfo.type);
  //               break;
  //             default:
  //               continue;
  //             }
  //         }
  //       }
  //       fileid.closeData();
  //     }
  //   }
  //   fileid.closeGroup();
  // }

public:
  void test_napi_old() {
    std::string const nxFile("NXtest.h5");
    File fileid = do_prep_files(nxFile);

    // tests of string/char read/write
    string const ch_test_data = "NeXus ><}&{'\\&\" Data";
    char const c1_array[5][4] = {
        {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
    char const c2_array[3][2] = {{'z', 'y'}, {'x', 'w'}, {'v', 'u'}};
    char const c3_array[6][1] = {{'z'}, {'y'}, {'x'}, {'w'}, {'v'}, {'u'}};
    char const c4_array[1][7] = {{'a', 'b', 'c', 'd', 'e', 'f', 'g'}};
    do_rw_test(fileid, "ch_data", ch_test_data);
    do_rw2darray_test(fileid, "c1_data", c1_array);
    do_rw2darray_test(fileid, "c2_data", c2_array);
    do_rw2darray_test(fileid, "c3_data", c3_array);
    do_rw2darray_test(fileid, "c4_data", c4_array);

    // tests of integer read/write
    vector<unsigned char> const i1_array{1, 2, 3, 4};
    vector<short int> const i2_array{1000, 2000, 3000, 4000};
    vector<int> const i4_array{1000000, 2000000, 3000000, 4000000};
    do_rw_test(fileid, "i1_data", i1_array);
    do_rw_test(fileid, "i2_data", i2_array);
    do_rw_test(fileid, "i4_data", i4_array);

    vector<float> const r4_vec{12.f, 13.f, 14.f, 15.f, 16.f};
    vector<double> const r8_vec{12.l, 13.l, 14.l, 15.l, 16.l};
    float const r4_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    double const r8_array[5][4] = {
        {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
    do_rw_test(fileid, "r4_vec_data", r4_vec);
    do_rw_test(fileid, "r8_vec_data", r8_vec);
    do_rw2darray_test(fileid, "r4_data", r4_array);
    do_rw2darray_test(fileid, "r8_data", r8_array);

    // do_rwslabvec_test(fileid, "r4_slab", r4_vec);
    // do_rwslabvec_test(fileid, "r8_slab", r8_vec);

    // int array_dims[2] = {5, 4};
    // int unlimited_dims[1] = {NX_UNLIMITED};
    // int chunk_size[2] = {5, 4};
    // int slab_start[2], slab_size[2];
    // NXlink glink, dlink;
    // int comp_array[100][20];

    // cleanup and return
    fileid.close();
    std::cout << "all ok - done\n";
    removeFile(nxFile);
  }

  //   void test_link(File fileid) {
  //     cout << "Link Test\n";
  //     // BEGIN LINK TEST
  //     fileid.makeGroup("data", "NXdata");
  //     fileid.openGroup("data", "NXdata");
  //     NXlink dlink;
  //     fileid.makeLink(dlink);
  //     int dims[2] = {100, 20};
  //     int comp_array[100, 20];
  //     for (i = 0; i < 100; i++) {
  //       for (j = 0; j < 20; j++) {
  //         comp_array[i][j] = i;
  //       }
  //     }
  //     int cdims[2] = {20, 20};
  //     if (NXcompmakedata(fileid, "comp_data", NXnumtype::INT32, 2, dims, NX_COMP_LZW, cdims) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopendata(fileid, "comp_data") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXputdata(fileid, comp_array) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosedata(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXflush(&fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXmakedata(fileid, "flush_data", NXnumtype::INT32, 1, unlimited_dims) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     slab_size[0] = 1;
  //     for (i = 0; i < 7; i++) {
  //       slab_start[0] = i;
  //       if (NXopendata(fileid, "flush_data") != NXstatus::NX_OK)
  //         return TEST_FAILED;
  //       if (NXputslab(fileid, &i, slab_start, slab_size) != NXstatus::NX_OK)
  //         return TEST_FAILED;
  //       if (NXflush(&fileid) != NXstatus::NX_OK)
  //         return TEST_FAILED;
  //     }
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXmakegroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopengroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     NXlen = 12;
  //     if (NXmakedata(fileid, "ch_data", NXnumtype::CHAR, 1, &NXlen) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopendata(fileid, "ch_data") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXputdata(fileid, "NeXus sample") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosedata(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXgetgroupID(fileid, &glink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXmakegroup(fileid, "link", "NXentry") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopengroup(fileid, "link", "NXentry") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXmakelink(fileid, &glink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXmakenamedlink(fileid, "renLinkGroup", &glink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXmakenamedlink(fileid, "renLinkData", &dlink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclose(&fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     // END LINK TEST
  //     // check links
  //     std::cout << "check links\n";
  //     NXlink blink;
  //     if (NXopengroup(fileid, "entry", "NXentry") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopengroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXgetgroupID(fileid, &glink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopengroup(fileid, "data", "NXdata") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopendata(fileid, "r8_data") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXgetdataID(fileid, &dlink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosedata(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopendata(fileid, "r8_data") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXgetdataID(fileid, &blink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosedata(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXsameID(fileid, &dlink, &blink) != NXstatus::NX_OK) {
  //       std::cout << "Link check FAILED (r8_data)\n"
  //                 << "original data\n";
  //       NXIprintlink(fileid, &dlink);
  //       std::cout << "linked data\n";
  //       NXIprintlink(fileid, &blink);
  //       return TEST_FAILED;
  //     }
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;

  //     if (NXopengroup(fileid, "link", "NXentry") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXopengroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXgetpath(fileid, path, 512) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     std::cout << "Group path " << path << "\n";
  //     if (NXgetgroupID(fileid, &blink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXsameID(fileid, &glink, &blink) != NXstatus::NX_OK) {
  //       std::cout << "Link check FAILED (sample)\n"
  //                 << "original group\n";
  //       NXIprintlink(fileid, &glink);
  //       std::cout << "linked group\n";
  //       NXIprintlink(fileid, &blink);
  //       return TEST_FAILED;
  //     }
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;

  //     std::cout << "renLinkGroup NXsample test\n";
  //     if (NXopengroup(fileid, "renLinkGroup", "NXsample") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXgetgroupID(fileid, &blink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXsameID(fileid, &glink, &blink) != NXstatus::NX_OK) {
  //       std::cout << "Link check FAILED (renLinkGroup)\n"
  //                 << "original group\n";
  //       NXIprintlink(fileid, &glink);
  //       std::cout << "linked group\n";
  //       NXIprintlink(fileid, &blink);
  //       return TEST_FAILED;
  //     }
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;

  //     std::cout << "renLinkData test\n";
  //     if (NXopendata(fileid, "renLinkData") != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXgetdataID(fileid, &blink) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXsameID(fileid, &dlink, &blink) != NXstatus::NX_OK) {
  //       std::cout << "Link check FAILED (renLinkData)\n"
  //                 << "original group\n";
  //       NXIprintlink(fileid, &glink);
  //       std::cout << "linked group\n";
  //       NXIprintlink(fileid, &blink);
  //       return TEST_FAILED;
  //     }
  //     if (NXclosedata(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     if (NXclosegroup(fileid) != NXstatus::NX_OK)
  //       return TEST_FAILED;
  //     std::cout << "Link check OK\n";
  // }

  void test_openPath() {
    std::cout << "tests for openPath\n";

    string const filename("openpathtest.nxs");
    File fileid(filename, NXACC_CREATE5);

    // make path /entry
    fileid.makeGroup("entry", "NXentry");
    fileid.openGroup("entry", "NXentry");

    // make path /entry/data1
    fileid.writeData("data1", '1');

    // make path /entry/data2
    fileid.writeData("data2", '2');

    // make path /entry/data1/more_data
    fileid.makeGroup("data", "NXdata");
    fileid.openGroup("data", "NXdata");
    fileid.writeData("more_data", '3');

    // make path /link
    fileid.closeGroup(); // close /entry/data
    fileid.closeGroup(); // close /entry
    fileid.makeGroup("link", "NXentry");
    fileid.openGroup("link", "NXentry"); // open /link
    fileid.writeData("data4", '4');

    // compare
    char output;
    fileid.closeGroup();

    fileid.openPath("/entry/data1");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('1', output);

    fileid.openPath("/link/data4");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('4', output);

    fileid.openPath("/entry/data/more_data");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('3', output);

    fileid.openData("/entry/data2");
    fileid.getData(&output);
    TS_ASSERT_EQUALS('2', output);

    // cleanup
    fileid.close();
    removeFile(filename);
    std::cout << "NXopenpath checks OK\n";
  }

  // void do_link_test(File &fileid) {
  //   fileid.makeGroup("data", "NXdata");
  //   fileid.openGroup("data", "NXdata");
  //   fileid.makeLink(dlink);
  //   int dims[2] = {100, 20};
  //   for (i = 0; i < 100; i++) {
  //     for (j = 0; j < 20; j++) {
  //       comp_array[i][j] = i;
  //     }
  //   }
  //   int cdims[2] = {20, 20};
  //   fileid.makeCompData("comp_data", NXnumtype::INT32, 2, dims, NX_COMP_LZW, dims);
  //   fileid.openData("comp_data");
  //   fileid.putData(comp_array);
  //   fileid.closeData();
  //   fileid.flush();
  // }

  // void do_flush_test(File &fileid) {
  //   fileid.makeData("flush_data", NXnumtype::INT32, 1, unlimited_dims);
  //   slab_size[0] = 1;
  //   for (int i = 0; i < 7; i++) {
  //     slab_start[0] = i;
  //     fileid.openData("flush_data");
  //     fileid.putSlab(&i, slab_start, slab_size);
  //     fileid.flush();
  //   }
  //   fileid.closeGroup();

  //   fileid.makeGroup("sample", "NXsample");
  //   fileid.openGroup("sample", "NXsample");
  //   NXlen = 12;
  //   fileid.makeData("ch_data", NXnumtype::CHAR, 1, &NXlen);
  //   fileid.openData("ch_data");
  //   fileid.putData("NeXus sample");
  //   fileid.closeData();
  //   glink = fileid.getGroupID();
  //   fileid.closeGroup();
  //   fileid.closeGroup();

  //   fileid.makeGroup("link", "NXentry");
  //   fileid.openGroup("link", "NXentry");
  //   fileid.makeLink(glink);
  //   fileid.closeGroup();
  // }
};
