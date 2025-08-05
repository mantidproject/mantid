// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/NexusFile.h"
#include "test_helper.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <hdf5.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace NexusTest;
using namespace Mantid::Nexus;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

namespace {
const std::string DMC01("dmc01cpp");
const std::string DMC02("dmc02cpp");
} // namespace

/** NOTE
 * This test is a faithful duplicate of the former test, napi_test_cpp.cpp,
 * which was in turn based on the napi test, napi_test.cpp
 * Some of the print-outs were converted to assertions, to make this a true test
 * see https://github.com/nexusformat/code/blob/master/test/napi_test_cpp.cxx
 */

class NexusFileNapiTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusFileNapiTest *createSuite() { return new NexusFileNapiTest(); }
  static void destroySuite(NexusFileNapiTest *suite) { delete suite; }

private:
  void do_test_write(const string &filename, NXaccess create_code) {
    cout << "writeTest(" << filename << ") started\n";
    Mantid::Nexus::File file(filename, create_code);
    // create group
    file.makeGroup("entry", "NXentry", true);
    // group attributes
    file.putAttr("hugo", "namenlos");
    file.putAttr("cucumber", "passion");
    // put string
    file.writeData("ch_data", "NeXus_data");

    // 2d array
    Mantid::Nexus::DimVector array_dims{5, 4};
    char c1_array[5][4] = {
        {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
    file.makeData("c1_data", NXnumtype::CHAR, array_dims, true);
    file.putData(&(c1_array[0][0]));
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
    DimVector slab_start{4, 0};
    DimSizeVector slab_size{1, 4};
    file.putSlab(&(r8_array[16]), slab_start, slab_size);
    slab_start[0] = 0;
    slab_start[1] = 0;
    slab_size[0] = 4;
    slab_size[1] = 4;
    file.putSlab(&(r8_array[0]), slab_start, slab_size);

    // add some attributes
    cout << "writing attributes to r8_data...";
    file.putAttr("ch_attribute", "NeXus");
    file.putAttr("i4_attribute", 42);
    file.putAttr("r4_attribute", 3.14159265);
    cout << "done" << std::endl;

    // set up for creating a link
    NXlink link = file.getDataID();
    file.closeData();

    // int64 tests
#if HAVE_LONG_LONG_INT
    vector<int64_t> grossezahl{12, 555555555555LL, 23, 777777777777LL};
#else
    vector<int64_t> grossezahl{12, 555555, 23, 77777};
#endif
    file.writeData("grosszahl", grossezahl);

    // create a new group inside this one
    file.makeGroup("data", "NXdata", true);

    // create a link
    file.makeLink(link);

    // compressed data
    array_dims[0] = 100;
    array_dims[1] = 20;
    vector<dimsize_t> comp_array;
    for (dimsize_t i = 0; i < array_dims[0]; i++) {
      for (dimsize_t j = 0; j < array_dims[1]; j++) {
        comp_array.push_back(i);
      }
    }
    const DimVector cdims{20, 20};
    file.writeCompData("comp_data", comp_array, array_dims, NXcompression::LZW, cdims);

    // ---------- Test write Extendible Data --------------------------
    std::vector<int> data(10, 123);
    file.makeGroup("extendible_data", "NXdata", true);
    file.writeExtendibleData("mydata1", data);
    file.writeExtendibleData("mydata2", data, 1000);
    DimVector dims{5, 2}, chunk{2, 2};
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
    file.makeData("flush_data", Mantid::Nexus::getType<int>(), NX_UNLIMITED, true);
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
    file.openAddress("/");
    file.makeGroup("link", "NXentry", true);
    file.makeLink(glink);
    cout << "writeTest(" << filename << ") successful\n";

    TS_ASSERT_EQUALS(std::filesystem::exists(filename), true);
  }

  void do_test_read(const string &filename) {
    cout << "readTest(" << filename << ") started\n";
    const string SDS("SDS");
    // top level file information
    Mantid::Nexus::File file(filename);
    // NOTE napi_test_cpp.cpp had logic here to print out global attributes
    // should have NeXus_version, file_name, HDF5_Version, and file_time
    std::vector<Mantid::Nexus::AttrInfo> attr_infos = file.getAttrInfos();
    // set up the correct HDF5 version
    unsigned major, minor, release;
    H5get_libversion(&major, &minor, &release);
    std::string hdf_version = strmakef("%u.%u.%u", major, minor, release);
    Entries global_attrs{{"NeXus_version", NEXUS_VERSION},
                         {"file_name", filename},
                         {"HDF5_Version", hdf_version},
                         {"file_time", "today's date"}};
    TS_ASSERT_EQUALS(attr_infos.size(), 4);
    for (Mantid::Nexus::AttrInfo const &attr : attr_infos) {
      TS_ASSERT_EQUALS(global_attrs.count(attr.name), 1);
      if (attr.name != "file_time") {
        TS_ASSERT_EQUALS(global_attrs[attr.name], file.getStrAttr(attr.name));
      }
    }

    // check group attributes
    file.openGroup("entry", "NXentry");
    // NOTE napi_test_cpp.cpp had logic here to print out all entry-level attributes
    attr_infos = file.getAttrInfos();
    std::map<std::string, std::string> exp_names{
        // Hugo Namenlos and his passion for cucumbers will live in Mantid infamy forever
        {"hugo", "namenlos"},
        {"cucumber", "passion"}};
    TS_ASSERT_EQUALS(attr_infos.size(), 2);
    for (Mantid::Nexus::AttrInfo const &attr : attr_infos) {
      TS_ASSERT_EQUALS(exp_names.count(attr.name), 1);
      TS_ASSERT_EQUALS(exp_names[attr.name], file.getStrAttr(attr.name));
    }

    // print out the entry level fields
    // NOTE napi_test_cpp.cpp had logic here to print out all entries off of entry-level
    // and also the value contained in any dataset
    Mantid::Nexus::Entries entries = file.getEntries();
    TS_ASSERT_EQUALS(entries.size(), 10);
    std::vector<float> r4_array;
    for (size_t i = 0; i < 20; i++) {
      r4_array.push_back(static_cast<float>(i));
    }
    std::vector<double> r8_array;
    for (size_t i = 20; i < 40; i++) {
      r8_array.push_back(static_cast<double>(i));
    }
    std::set<std::string> exp_entries{"c1_data", "ch_data", "data",    "grosszahl", "i1_data",
                                      "i2_data", "i4_data", "r4_data", "r8_data",   "sample"};
    for (Mantid::Nexus::Entry entry : entries) {
      TS_ASSERT_EQUALS(exp_entries.count(entry.first), 1);
      if (entry.second == "SDS") {
        // NOTE c1_data is a 2d char array and is skipped in napi_test_cpp.cpp
        // NOTE grosszahl will be platform dependent
        if (entry.first == "ch_data") {
          file.openData(entry.first);
          TS_ASSERT_EQUALS(file.getStrData(), "NeXus_data");
          file.closeData();
        } else if (entry.first == "i1_data") {
          std::vector<uint8_t> res;
          file.readData<uint8_t>(entry.first, res);
          TS_ASSERT_EQUALS(res, std::vector<uint8_t>({1, 2, 3, 4}));
        } else if (entry.first == "i2_data") {
          std::vector<int16_t> res;
          file.readData<int16_t>(entry.first, res);
          TS_ASSERT_EQUALS(res, std::vector<int16_t>({1000, 2000, 3000, 4000}));
        } else if (entry.first == "i4_data") {
          std::vector<int32_t> res;
          file.readData<int32_t>(entry.first, res);
          TS_ASSERT_EQUALS(res, std::vector<int32_t>({1000000, 2000000, 3000000, 4000000}));
        } else if (entry.first == "r4_data") {
          std::vector<float> res;
          file.readData<float>(entry.first, res);
          TS_ASSERT_EQUALS(res, r4_array);
        } else if (entry.first == "r8_data") {
          std::vector<double> res;
          file.readData<double>(entry.first, res);
          TS_ASSERT_EQUALS(res, r8_array);
        }
      }
    }

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

    // openAddress checks
    file.openAddress("/entry/data/comp_data");
    file.openAddress("/entry/data/comp_data");
    file.openAddress("../r8_data");
    cout << "NXopenaddress checks OK\n";

    // everything went fine
    cout << "readTest(" << filename << ") successful\n";
  }

  void do_test_loadPath(const string &filename) {
    if (getenv("NX_LOAD_PATH") != NULL) {
      TS_ASSERT_THROWS_NOTHING(Mantid::Nexus::File file(filename, NXaccess::RDWR));
      cout << "Success loading Nexus file from path" << endl;
    } else {
      cout << "NX_LOAD_PATH variable not defined. Skipping testLoadPath\n";
    }
  }

public:
  void test_readwrite_hdf5() {
    cout << " Nexus File Tests\n";
    NXaccess const nx_creation_code = NXaccess::CREATE5;
    NexusTest::FileResource resource("nexus_file_napi_test_cpp.h5");
    std::string filename(resource.fullPath());

    // try writing a file
    do_test_write(filename, nx_creation_code);

    // try reading a file
    do_test_read(filename);

    // try using the load path
    std::string fileext(".h5");
    do_test_loadPath(DMC01 + fileext);
    do_test_loadPath(DMC02 + fileext);

    removeFile(DMC01 + fileext);
    removeFile(DMC02 + fileext);
  }
};
