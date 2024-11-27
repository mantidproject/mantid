#include "MantidNexusCpp/NeXusFile.hpp"
#include "MantidNexusCpp/napiconfig.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <direct.h> /* for getcwd() */
#else
#include <unistd.h>
#endif /* _WIN32 */

using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

static std::string relativePathOf(const std::string &filenamestr) {
  char cwd[1024];

  getcwd(cwd, sizeof(cwd));

  if (filenamestr.compare(0, strlen(cwd), cwd) == 0) {
    return filenamestr.substr(strlen(cwd) + 1); // +1 to skip trailing /
  } else {
    return filenamestr;
  }
}

static int writeTest(const string &filename, NXaccess create_code) {
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
  for (size_t i = 0; i < 4; i++) {
    i1_array.push_back(static_cast<uint8_t>(i + 1));
  }
  file.writeData("i1_data", i1_array);

  // 1d int16 array
  vector<int16_t> i2_array;
  for (size_t i = 0; i < 4; i++) {
    i2_array.push_back(1000 * (i + 1));
  }
  file.writeData("i2_data", i2_array);

  // 1d int32 data
  vector<int32_t> i4_array;
  for (size_t i = 0; i < 4; i++) {
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
  file.putAttr("ch_attribute", "NeXus");
  file.putAttr("i4_attribute", 42);
  file.putAttr("r4_attribute", 3.14159265);

  // set up for creating a link
  NXlink link = file.getDataID();
  file.closeData();

  // int64 tests
  vector<int64_t> grossezahl;
#if HAVE_LONG_LONG_INT
  grossezahl.push_back(12);
  grossezahl.push_back(555555555555LL);
  grossezahl.push_back(23);
  grossezahl.push_back(777777777777LL);
#else
  grossezahl.push_back(12);
  grossezahl.push_back(555555);
  grossezahl.push_back(23);
  grossezahl.push_back(77777);
#endif
  if (create_code != NXACC_CREATE) {
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
  file.makeNamedLink("renLinkGroup", glink);
  file.makeNamedLink("renLinkData", link);

  return 0;
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
  const string SDS("SDS");
  // top level file information
  NeXus::File file(filename);
  cout << "NXinquirefile found: " << relativePathOf(file.inquireFile()) << endl;
  vector<NeXus::AttrInfo> attr_infos = file.getAttrInfos();
  cout << "Number of global attributes: " << attr_infos.size() << endl;
  for (vector<NeXus::AttrInfo>::iterator it = attr_infos.begin(); it != attr_infos.end(); it++) {
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
  for (vector<NeXus::AttrInfo>::iterator it = attr_infos.begin(); it != attr_infos.end(); it++) {
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
  for (map<string, string>::const_iterator it = entries.begin(); it != entries.end(); it++) {
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
        vector<float> *result = file.getData<float>();
        cout << toString(*result);
        delete result;
      } else if (info.type == NeXus::FLOAT64) {
        vector<double> *result = file.getData<double>();
        cout << toString(*result);
        delete result;
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
    return 1;
  if (ints[0] != 1)
    return 1;
  file.closeData();

  ints.clear();
  file.openData("i2_data");
  file.getDataCoerce(ints);
  if (ints.size() != 4)
    return 1;
  if (ints[0] != 1000)
    return 1;
  file.closeData();

  ints.clear();
  file.openData("i4_data");
  file.getDataCoerce(ints);
  if (ints.size() != 4)
    return 1;
  if (ints[0] != 1000000)
    return 1;
  file.closeData();

  doubles.clear();
  file.openData("r4_data");
  file.getDataCoerce(doubles);
  if (doubles.size() != 20)
    return 1;
  if (doubles[1] != 1.0)
    return 1;
  file.closeData();

  doubles.clear();
  file.openData("r8_data");
  file.getDataCoerce(doubles);
  if (doubles.size() != 20)
    return 1;
  if (doubles[1] != 21.0)
    return 1;
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
    return 1;

  // Close the "entry" group
  file.closeGroup();

  // check links
  file.openGroup("entry", "NXentry");
  file.openGroup("sample", "NXsample");
  NXlink glink = file.getGroupID();
  file.closeGroup();
  file.openGroup("data", "NXdata");
  file.openData("r8_data");
  NXlink dlink = file.getDataID();
  file.closeData();
  file.closeGroup();
  file.openData("r8_data");
  NXlink d2link = file.getDataID();
  file.closeData();
  if (!file.sameID(dlink, d2link)) {
    cout << "Link check FAILED (r8_data)" << endl;
    cout << "     original data = ";
    file.printLink(dlink);
    cout << "     linked data = ";
    file.printLink(d2link);
    return 1;
  }
  file.closeGroup();

  file.openGroup("link", "NXentry");
  file.openGroup("sample", "NXsample");
  NXlink g2link = file.getGroupID();
  if (!file.sameID(glink, g2link)) {
    cout << "Link check FAILED (sample)" << endl;
    cout << "     original group = ";
    file.printLink(glink);
    cout << "     linked group = ";
    file.printLink(g2link);
    return 1;
  }
  file.closeGroup();

  file.openGroup("renLinkGroup", "NXsample");
  g2link = file.getGroupID();
  file.closeGroup();
  if (!file.sameID(glink, g2link)) {
    cout << "Link check FAILED (renLinkGroup)" << endl;
    cout << "     original group = ";
    file.printLink(glink);
    cout << "     linked group = ";
    file.printLink(g2link);
    return 1;
  }

  file.openData("renLinkData");
  d2link = file.getDataID();
  file.closeData();
  if (!file.sameID(dlink, d2link)) {
    cout << "Link check FAILED (renLinkData)" << endl;
    cout << "     original data = ";
    file.printLink(dlink);
    cout << "     linked data = ";
    file.printLink(d2link);
    return 1;
  }

  file.closeGroup();
  cout << "Link check OK" << endl;

  // openpath checks
  file.openPath("/entry/data/comp_data");
  file.openPath("/entry/data/comp_data");
  file.openPath("../r8_data");
  printf("NXopenpath checks OK\n");

  // everything went fine
  return 0;
}

int testLoadPath(const string &filename) {
  if (getenv("NX_LOAD_PATH") != NULL) {
    NeXus::File file(filename);
    cout << "Success loading NeXus file from path" << endl;
    // cout << file.inquireFile() << endl; // DEBUG print
  }
  return 0;
}

int testExternal(const string &fileext, NXaccess create_code) {
  string filename("nxext_cpp" + fileext);
  string exturl1("nxfile://data/dmc01" + fileext + "#entry1");
  string exturl2("nxfile://data/dmc02" + fileext + "#entry1");

  // create the external link
  NeXus::File fileout(filename, create_code);
  fileout.linkExternal("entry1", "NXentry", exturl1);
  fileout.linkExternal("entry2", "NXentry", exturl2);
  fileout.close();

  // read the file to make sure things worked
  NeXus::File filein(filename);
  filein.openPath("/entry1/start_time");
  cout << "First file time: " << filein.getStrData() << endl;
  cout << "NXinquirefile found: " << relativePathOf(filein.inquireFile()) << endl;
  filein.openPath("/entry2/sample/sample_name");
  cout << "Second file sample: " << filein.getStrData() << endl;
  cout << "NXinquirefile found: " << relativePathOf(filein.inquireFile()) << endl;
  filein.openPath("/entry2/start_time");
  cout << "Second file time: " << filein.getStrData() << endl;
  filein.openPath("/");
  cout << "entry1 external URL = " << filein.isExternalGroup("entry1", "NXentry") << endl;

  return 0;
}

static int streamTest(const std::string &fname, NXaccess create_mode) {
  using namespace NeXus;
  using namespace NeXus::Stream;
  std::vector<double> w;
  std::vector<double> w1;
  w.push_back(1.0);
  double d, d1;
  int i;
  // create an entry and a data item
  File nf(fname, create_mode);
  nf << Group("entry1", "NXentry") << Data("dat1", w, "int_attr", 3);
  nf.close();
  File nf1(fname, NXACC_RDWR);
  // add a double_attr to an existing setup
  nf1 >> Group("entry1", "NXentry") >> Data("dat1") << Attr("double_attr", 6.0) << Close;
  nf1.close();
  // read back data items
  File nf2(fname, NXACC_READ);
  nf2 >> Group("entry1", "NXentry") >> Data("dat1", w1, "int_attr", i, "double_attr", d);
  // alternative way to read d1
  nf2 >> Data("dat1") >> Attr("double_attr", d1);
  nf2.close();
  if (i != 3 || w != w1 || d != 6.0 || d1 != 6.0) {
    return 1;
  }
  return 0;
}

int testTypeMap(const std::string &fname) {
  NeXus::File file(fname);
  multimap<string, string> *map = file.getTypeMap();
  size_t mapsize = 25;
  // HDF4 does not have int64 capability, so resulting map is one shorter than HDF5 and XML files
  if (fname == string("napi_test_cpp.hdf")) {
    if (map->size() != (mapsize - 1)) {
      cout << "TypeMap is incorrect" << endl;
      return 1;
    }
  } else {
    if (map->size() != mapsize) {
      cout << "TypeMap is incorrect" << endl;
      return 1;
    }
  }

  cout << "TypeMap is correct size" << endl;

  return 0;
}

int main(int argc, char **argv) {
  NXaccess nx_creation_code;
  string filename;
  string extfile_ext;
  if (strstr(argv[0], "napi_test_cpp-hdf5") != NULL) {
    nx_creation_code = NXACC_CREATE5;
    filename = "napi_test_cpp.h5";
    extfile_ext = ".h5";
  } else if (strstr(argv[0], "napi_test_cpp-xml-table") != NULL) {
    nx_creation_code = NXACC_CREATEXML | NXACC_TABLE;
    filename = "napi_test_cpp-table.xml";
    extfile_ext = "-table.xml";
  } else if (strstr(argv[0], "napi_test_cpp-xml") != NULL) {
    nx_creation_code = NXACC_CREATEXML;
    filename = "napi_test_cpp.xml";
    extfile_ext = ".xml";
  } else {
    nx_creation_code = NXACC_CREATE;
    filename = "napi_test_cpp.hdf";
    extfile_ext = ".hdf";
  }

  int result;
  result = writeTest(filename, nx_creation_code);
  if (result) {
    cout << "writeTest failed" << endl;
    return result;
  }
  if ((argc >= 2) && !strcmp(argv[1], "-q")) {
    return 0; //
  }

  // try reading a file
  result = readTest(filename);
  if (result) {
    cout << "readTest failed" << endl;
    return result;
  }

  // try using the load path
  result = testLoadPath("dmc01.hdf");
  if (result) {
    cout << "testLoadPath failed" << endl;
    return result;
  }

  // try external linking
  result = testExternal(extfile_ext, nx_creation_code);
  if (result) {
    cout << "testExternal failed" << endl;
    return result;
  }

  // quick test of stream interface
  std::string fname = string("stream_test") + extfile_ext;
  result = streamTest(fname, nx_creation_code);
  remove(fname.c_str());
  if (result) {
    cout << "streamTest failed" << endl;
    return result;
  }

  // test of typemap generation
  result = testTypeMap(filename);
  if (result) {
    cout << "testTypeMap failed" << endl;
    return result;
  }

  // everything went ok
  return 0;
}
