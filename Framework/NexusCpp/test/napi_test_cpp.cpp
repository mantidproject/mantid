#include "MantidNexusCpp/NeXusFile.hpp"
#include "napi_test_util.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using NexusCppTest::removeFile;
using NexusCppTest::write_dmc01;
using NexusCppTest::write_dmc02;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

namespace { // anonymous namespace
const std::string DMC01("dmc01cpp");
const std::string DMC02("dmc02cpp");

} // anonymous namespace

static void writeTest(const string &filename, NXaccess create_code) {
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
  file.makeData("c1_data", NeXus::CHAR, array_dims, true);
  file.putData(c1_array);
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
  file.makeData("r8_data", NeXus::FLOAT64, array_dims, true);
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
}

template <typename NumT> string toString(const vector<NumT> &data) {
  std::stringstream result;
  result << "[";
  size_t size = data.size();
  for (size_t i = 0; i < size; i++) {
    result << data[i];
    if (i + 1 < size) {
      result << ",";
    }
  }
  result << "]";
  return result.str();
}

int readTest(const string &filename) {
  std::cout << "readTest(" << filename << ") started\n";
  const string SDS("SDS");
  // top level file information
  NeXus::File file(filename);
  vector<NeXus::AttrInfo> attr_infos = file.getAttrInfos();
  cout << "Number of global attributes: " << attr_infos.size() << endl;
  for (vector<NeXus::AttrInfo>::iterator it = attr_infos.begin(); it != attr_infos.end(); ++it) {
    if (it->name != "file_time" && it->name != "HDF_version" && it->name != "HDF5_Version" &&
        it->name != "XML_version") {
      cout << "   " << it->name << " = ";
      if (it->type == NeXus::CHAR) {
        cout << file.getStrAttr(*it);
      }
      cout << endl;
    }
  }

  // check group attributes
  file.openGroup("entry", "NXentry");
  attr_infos = file.getAttrInfos();
  cout << "Number of group attributes: " << attr_infos.size() << endl;
  for (vector<NeXus::AttrInfo>::iterator it = attr_infos.begin(); it != attr_infos.end(); ++it) {
    cout << "   " << it->name << " = ";
    if (it->type == NeXus::CHAR) {
      cout << file.getStrAttr(*it);
    }
    cout << endl;
  }

  // print out the entry level fields
  map<string, string> entries = file.getEntries();
  cout << "Group contains " << entries.size() << " items" << endl;
  NeXus::Info info;
  for (map<string, string>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
    cout << "   " << it->first;
    if (it->second == SDS) {
      file.openData(it->first);
      info = file.getInfo();
      cout << toString(info.dims) << " = ";
      if (info.type == NeXus::CHAR) {
        if (info.dims.size() == 1) {
          cout << file.getStrData();
        } else {
          cout << "2d character array";
        }
      } else if (info.type == NeXus::FLOAT32) {
        vector<float> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::FLOAT64) {
        vector<double> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::INT8) {
        vector<int8_t> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::UINT8) {
        vector<uint8_t> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::INT16) {
        vector<int16_t> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::UINT16) {
        vector<uint16_t> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::INT32) {
        vector<int32_t> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::UINT32) {
        vector<uint32_t> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::INT64) {
        vector<int64_t> result;
        file.getData(result);
        cout << toString(result);
      } else if (info.type == NeXus::UINT64) {
        vector<uint64_t> result;
        file.getData(result);
        cout << toString(result);
      }
      cout << endl;
      cout << "   Path = " << file.getPath() << endl;
      file.closeData();
    } else {
      cout << ":" << it->second << endl;
    }
  }

  // Test getDataCoerce() -------------------
  std::vector<int> ints;
  std::vector<double> doubles;

  ints.clear();
  file.openData("i1_data");
  file.getDataCoerce(ints);
  if (ints.size() != 4)
    return TEST_FAILED;
  if (ints[0] != 1)
    return TEST_FAILED;
  file.closeData();

  ints.clear();
  file.openData("i2_data");
  file.getDataCoerce(ints);
  if (ints.size() != 4)
    return TEST_FAILED;
  if (ints[0] != 1000)
    return TEST_FAILED;
  file.closeData();

  ints.clear();
  file.openData("i4_data");
  file.getDataCoerce(ints);
  if (ints.size() != 4)
    return TEST_FAILED;
  if (ints[0] != 1000000)
    return TEST_FAILED;
  file.closeData();

  doubles.clear();
  file.openData("r4_data");
  file.getDataCoerce(doubles);
  if (doubles.size() != 20)
    return TEST_FAILED;
  if (doubles[1] != 1.0)
    return TEST_FAILED;
  file.closeData();

  doubles.clear();
  file.openData("r8_data");
  file.getDataCoerce(doubles);
  if (doubles.size() != 20)
    return TEST_FAILED;
  if (doubles[1] != 21.0)
    return TEST_FAILED;
  file.closeData();

  // Throws when you coerce to int from a real/double source
  bool didThrow = false;
  try {
    ints.clear();
    file.openData("r8_data");
    file.getDataCoerce(ints);
    file.closeData();
    cout << "getDataCoerce(int) of doubles did not throw (it is supposed to throw)." << endl;
  } catch (...) {
    // Good! It is supposed to throw
    didThrow = true;
    file.closeData();
  }
  if (!didThrow)
    return TEST_FAILED;

  // Close the "entry" group
  file.closeGroup();

  // openpath checks
  file.openPath("/entry/data/comp_data");
  file.openPath("/entry/data/comp_data");
  file.openPath("../r8_data");
  printf("NXopenpath checks OK\n");

  // everything went fine
  std::cout << "readTest(" << filename << ") successful\n";
  return TEST_SUCCEED;
}

int testLoadPath(const string &filename) {
  if (getenv("NX_LOAD_PATH") != NULL) {
    NeXus::File file(filename);
    cout << "Success loading NeXus file from path" << endl;
    // cout << file.inquireFile() << endl; // DEBUG print
    return TEST_SUCCEED;
  } else {
    cout << "NX_LOAD_PATH variable not defined. Skipping testLoadPath\n";
    return TEST_SUCCEED;
  }
}

int main(int argc, char **argv) {
  NXaccess nx_creation_code;
  string fileext;
  if (strstr(argv[0], "napi_test_cpp-hdf5") != NULL) {
    nx_creation_code = NXACC_CREATE5;
    fileext = ".h5";
  } else if (strstr(argv[0], "napi_test_cpp-xml-table") != NULL) {
    cout << "napi_test_cpp-xml-table is not supported" << endl;
    return TEST_FAILED;
  } else if (strstr(argv[0], "napi_test_cpp-xml") != NULL) {
    cout << "napi_test_cpp-xml is not supported" << endl;
    return TEST_FAILED;
  } else {
    nx_creation_code = NXACC_CREATE4;
    fileext = ".hdf";
  }
  const string filename("napi_test_cpp" + fileext);

  removeFile(filename); // in case last round failed
  try {
    writeTest(filename, nx_creation_code);
  } catch (const std::runtime_error &e) {
    cout << "writeTest failed:\n" << e.what() << endl;
    return TEST_FAILED;
  }
  if ((argc >= 2) && !strcmp(argv[1], "-q")) {
    cout << "Ending test early\n";
    return TEST_SUCCEED;
  }

  if (!std::filesystem::exists(filename)) {
    std::cerr << "NeXus file \"" << filename << "\" does not exist after write test\n";
    return TEST_FAILED;
  }

  // try reading a file
  try {
    if (readTest(filename) != TEST_SUCCEED) {
      cout << "readTest failed" << endl;
      return TEST_FAILED;
    }
  } catch (const std::runtime_error &e) {
    cout << "readTest failed:\n" << e.what() << endl;
    return TEST_FAILED;
  }

  removeFile(filename); // cleanup

  // try using the load path
  if (testLoadPath(DMC01 + fileext) != TEST_SUCCEED) {
    cout << "testLoadPath failed" << endl;
    return TEST_FAILED;
  }

  removeFile(DMC01 + fileext);
  removeFile(DMC02 + fileext);

  // everything went ok
  return TEST_SUCCEED;
}
