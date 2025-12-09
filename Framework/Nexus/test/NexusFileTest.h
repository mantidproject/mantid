// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/NexusException.h"
#include "MantidNexus/NexusFile.h"
#include "MantidTypes/Core/DateAndTime.h"
#include "test_helper.h"
#include <H5Cpp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace NexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

class NexusFileTest : public CxxTest::TestSuite {

public:
  // // This pair of boilerplate methods prevent the suite being created statically
  // // This means the constructor isn't called when running other tests
  static NexusFileTest *createSuite() { return new NexusFileTest(); }
  static void destroySuite(NexusFileTest *suite) { delete suite; }

  // #################################################################################################################
  // TEST CONSTRUCTORS
  // #################################################################################################################

  void test_remove() {
    // create a simple file, and make sure removeFile works as intended
    cout << "\nremoving\n" << std::flush;
    FileResource resource("not_a_real_file.txt");
    std::string filename = resource.fullPath();

    // ensure file doesn't already exist
    if (std::filesystem::exists(filename)) {
      std::filesystem::remove(filename);
    }
    TS_ASSERT(!std::filesystem::exists(filename));

    // removeFile works fine if file doesn't exist
    TS_ASSERT_THROWS_NOTHING(removeFile(filename));

    // create the file
    std::ofstream outfile{filename};
    outfile.close();
    TS_ASSERT(std::filesystem::exists(filename));

    // remove it, make sure removed
    removeFile(filename);
    TS_ASSERT(!std::filesystem::exists(filename));
  }

  void test_can_create() {
    cout << "\ntest creation\n" << std::flush;

    FileResource resource("test_nexus_file_init.h5");
    std::string filename = resource.fullPath();

    // create the file and ensure it exists
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.close();
    TS_ASSERT(std::filesystem::exists(filename));
  }

  void test_can_open_existing() {
    cout << "\ntest open exisitng\n" << std::flush;

    FileResource resource("test_nexus_file_init.h5");
    std::string filename = resource.fullPath();

    // create the file and ensure it exists
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.putAttr("test_attr", "test_value");
    file.close();
    TS_ASSERT(std::filesystem::exists(filename));

    // now open it in read/write
    Mantid::Nexus::File file2(filename, NXaccess::RDWR);
    file2.putAttr("test_attr2", "test_value2");
    file2.close();

    Mantid::Nexus::File file3(filename, NXaccess::READ);
    auto result = file3.getStrAttr("test_attr");
    TS_ASSERT_EQUALS(result, "test_value");
    result = file3.getStrAttr("test_attr2");
    TS_ASSERT_EQUALS(result, "test_value2");
  }

  void test_open_real_file() {
    cout << "\ntest open existing file in unit test data\n";
    // open the file in read-only mode
    std::string filename = getFullPath("CG2_monotonically_increasing_pulse_times.nxs.h5");
    TS_ASSERT_THROWS_NOTHING(Mantid::Nexus::File file(filename, NXaccess::READ));
  }

  void test_fail_open() {
    // test opening a file that exists, but is unreadable
    std::string filename = getFullPath("Test_characterizations_char.txt");
    TS_ASSERT_THROWS_ANYTHING(Mantid::Nexus::File file(filename, NXaccess::READ));

    // test opening an empty file
    FileResource resource("fake_empty_file.nxs.h5");
    std::ofstream file(resource.fullPath());
    file << "mock";
    file.close();
    TS_ASSERT_THROWS_ANYTHING(Mantid::Nexus::File file(resource.fullPath(), NXaccess::READ));
  }

  void test_clear_on_create() {
    // create an empty file
    FileResource resource("fake_empty_file.nxs.h5");
    std::ofstream file(resource.fullPath());
    file << "mock";
    file.close();

    // this file cannot be opened for read
    TS_ASSERT_THROWS_ANYTHING(Mantid::Nexus::File file(resource.fullPath(), NXaccess::READ));

    // but no issue if opened for create
    TS_ASSERT_THROWS_NOTHING(Mantid::Nexus::File file(resource.fullPath(), NXaccess::CREATE5));
  }

  void test_flush() {
    cout << "\ntest flush\n";
    // make sure flush works
    // TODO actually test the buffers
    FileResource resource("test_nexus_file_flush.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.flush();
  }

  // #################################################################################################################
  // TEST MAKE / OPEN / CLOSE GROUP
  // #################################################################################################################

  void test_make_group() {
    cout << "\ntest makeGroup\n";
    FileResource resource("test_nexus_file_grp.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    string grp("test_group"), cls("NXsample");

    // check error conditions
    TS_ASSERT_THROWS(file.makeGroup(grp, ""), Mantid::Nexus::Exception const &);
    TS_ASSERT_THROWS(file.makeGroup("", cls), Mantid::Nexus::Exception const &);
    // check works when correct
    TS_ASSERT_THROWS_NOTHING(file.makeGroup(grp, cls));
  }

  void test_same_make_group() {
    cout << "\ntest same makeGroup\n";
    FileResource resource("test_nexus_file_grp.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    string grp("test_group");

    // check that we can make '/test_group/test_group'
    TS_ASSERT_THROWS_NOTHING(file.makeGroup(grp, "NXsample", true));
    TS_ASSERT_THROWS_NOTHING(file.makeGroup(grp, "NXdata", true));
    TS_ASSERT_EQUALS(file.getAddress(), "/test_group/test_group");
  }

  void test_open_group() {
    cout << "\ntest openGroup\n";
    FileResource resource("test_nexus_file_grp.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // create a group, to be opened
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, false);
    TS_ASSERT_EQUALS(file.getAddress(), "/");

    // check error conditions
    TS_ASSERT_THROWS(file.openGroup(string(), cls), Mantid::Nexus::Exception const &);
    TS_ASSERT_THROWS(file.openGroup("tacos1", cls), Mantid::Nexus::Exception const &);
    TS_ASSERT_THROWS(file.openGroup(grp, string()), Mantid::Nexus::Exception const &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openGroup(grp, cls));
    TS_ASSERT_EQUALS(file.getAddress(), "/test_group");
  }

  void test_open_group_bad() {
    cout << "\ntest openGroup bad\n";
    FileResource resource("test_nexus_file_grp.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // create a group, to be opened
    string grp("test_group"), cls("NXpants");
    file.makeGroup(grp, cls, false);

    // try to open it with wrong class name
    string notcls("NXshorts");
    TS_ASSERT_THROWS(file.openGroup(grp, notcls), Mantid::Nexus::Exception const &);
  }

  void test_open_group_layers() {
    cout << "\ntest openGroup layers\n";
    FileResource resource("test_nexus_file_grp_layers.h5");
    std::string filename = resource.fullPath();
    string grp1("layer1"), grp2("layer2"), cls1("NXpants1"), cls2("NXshorts");

    // create a file with group -- open it
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup(grp1, cls1, false);
    file.openGroup(grp1, cls1);
    TS_ASSERT_EQUALS(file.getAddress(), "/layer1");

    // create a group inside the group -- open it
    TS_ASSERT_THROWS_NOTHING(file.makeGroup(grp2, cls2, false));
    TS_ASSERT_THROWS_NOTHING(file.openGroup(grp2, cls2));
    TS_ASSERT_EQUALS(file.getAddress(), "/layer1/layer2");
  }

  void test_closeGroup() {
    cout << "\ntest closeGroup\n";
    FileResource resource("test_nexus_file_grp.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // check error at root
    TS_ASSERT_THROWS_NOTHING(file.closeGroup());

    // now make group, close it, and check we are back at root
    string grp("test_group"), cls("NXsample");
    file.makeGroup(grp, cls, true);
    TS_ASSERT_EQUALS(file.getAddress(), "/test_group");

    TS_ASSERT_THROWS_NOTHING(file.closeGroup());
    TS_ASSERT_EQUALS(file.getAddress(), "/");
  }

  // #################################################################################################################
  // TEST MAKE / OPEN / PUT / CLOSE DATASET
  // #################################################################################################################

  void test_makeData() {
    cout << "\ntest make data\n";
    FileResource resource("test_nexus_file_data.h5");
    std::string filename = resource.fullPath();

    string name("some_data");
    Mantid::Nexus::DimVector dims({1});
    NXnumtype type(NXnumtype::CHAR);

    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // if there is not a top-level NXentry, should throw error
    TS_ASSERT_THROWS(file.makeData(name, type, dims), Mantid::Nexus::Exception const &);

    // now make a NXentry group and try
    file.makeGroup("entry", "NXentry", true);
    TS_ASSERT_EQUALS(file.getAddress(), "/entry");

    // check some failing cases
    TS_ASSERT_THROWS(file.makeData("", type, dims), Mantid::Nexus::Exception const &);
    TS_ASSERT_THROWS(file.makeData(name, type, Mantid::Nexus::DimVector()), Mantid::Nexus::Exception const &);

    // check it works when it works
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, dims, true));
    TS_ASSERT_EQUALS(file.getAddress(), "/entry/some_data");
  }

  void test_makeData_length() {
    cout << "\ntest make data -- using length\n";
    FileResource resource("test_nexus_file_data.h5");
    std::string filename = resource.fullPath();

    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);
    TS_ASSERT_EQUALS(file.getAddress(), "/entry");

    NXnumtype type(NXnumtype::CHAR);

    // check it works when it works -- int
    string name("some_data_int");
    Mantid::Nexus::dimsize_t len(3);
    TS_ASSERT_THROWS_NOTHING(file.makeData(name, type, len));
  }

  void test_open_dataset() {
    cout << "\ntest openData\n";
    FileResource resource("test_nexus_file_data.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // get location of top-level
    TS_ASSERT_EQUALS(file.getAddress(), "/");

    file.makeGroup("entry", "NXentry", true);
    TS_ASSERT_EQUALS(file.getAddress(), "/entry");

    // create a dataset, to be opened
    string data("test_group");
    NXnumtype type(NXnumtype::CHAR);
    file.makeData(data, type, 3, false);

    // check error conditions
    TS_ASSERT_THROWS(file.openData(string()), Mantid::Nexus::Exception const &);
    TS_ASSERT_THROWS(file.openData("tacos1"), Mantid::Nexus::Exception const &);

    // now open it, check we are at a different location
    TS_ASSERT_THROWS_NOTHING(file.openData(data));
    TS_ASSERT_EQUALS(file.getAddress(), "/entry/test_group");
  }

  void test_make_data_lateral() {
    // this ensures behavior making a dataset while a dataset is opened will instead
    // anchor that dataset into the containing GROUP and not the DATASET
    // this is not good hygienic behavior, but is required by NexusClasses and can
    // lead to test regressions that are otherwise very hard to track down
    cout << "\ntest make data lateral\n" << std::flush;
    FileResource resource("test_napi_file_rdwr.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // make and open data
    NXnumtype type(NXnumtype::CHAR);
    file.makeData("data1", NXnumtype::CHAR, 3, true);
    std::string address1 = file.getAddress();

    // make and open lateral data
    // with the other dataset open, then creates the dataset in the parent group
    TS_ASSERT_THROWS_NOTHING(file.makeData("data2", NXnumtype::CHAR, 2, false));
    TS_ASSERT_THROWS_NOTHING(file.openData("data2"));
    TS_ASSERT(file.hasData("/entry/data2"));
    std::string address2 = file.getAddress();

    // make sure new data is created off of parent group
    TS_ASSERT_DIFFERS(address1, address2);
    TS_ASSERT_EQUALS(address1, "/entry/data1");
    TS_ASSERT_EQUALS(address2, "/entry/data2");
  }

  void test_closeData() {
    cout << "\ntest closeData\n" << std::flush;
    FileResource resource("test_nexus_file_dataclose.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // check error at top-level
    TS_ASSERT_THROWS(file.closeData(), Mantid::Nexus::Exception const &);

    // now make data, close it, and check we are back at beginning
    file.makeData("test_data:", NXnumtype::CHAR, 1, true);
    TS_ASSERT_EQUALS(file.getAddress(), "/entry/test_data:");

    TS_ASSERT_THROWS_NOTHING(file.closeData());
    TS_ASSERT_EQUALS(file.getAddress(), "/entry")

    TS_ASSERT_THROWS(file.closeData(), Mantid::Nexus::Exception const &);
  }

  void test_close_data_lateral() {
    cout << "\ntest close data lateral\n" << std::flush;
    FileResource resource("test_nexus_file_dataclose.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // make and open data
    NXnumtype type(NXnumtype::CHAR);
    file.makeData("data1", NXnumtype::CHAR, 3, true);
    std::string address1 = file.getAddress();

    // make and open lateral data
    TS_ASSERT_THROWS_NOTHING(file.makeData("data2", NXnumtype::CHAR, 2, false));
    TS_ASSERT_THROWS_NOTHING(file.openData("data2"));
    std::string address2 = file.getAddress();

    TS_ASSERT_DIFFERS(address1, address2);

    // now close lateral data... where are we??
    TS_ASSERT_THROWS_NOTHING(file.closeData());
    TS_ASSERT(!file.isDataSetOpen());
    TS_ASSERT_EQUALS(file.getAddress(), "/entry");
  }

  template <typename T> void do_test_data_putget(Mantid::Nexus::File &file, string name, T in) {
    T out;
    file.makeData(name, Mantid::Nexus::getType<T>(), 1, true);
    file.putData(&in);
    file.getData(&out);
    file.closeData();
    TS_ASSERT_EQUALS(in, out);
  }

  void test_data_putget_basic() {
    cout << "\ntest dataset read/write\n" << std::flush;

    // open a file
    FileResource resource("test_nexus_file_dataRW.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // put/get an int
    cout << "\tread/write int..." << std::flush;
    do_test_data_putget<int32_t>(file, "data_int", 12);
    cout << "done\n";

    // put/get an int64_t
    cout << "\tread/write int64_t..." << std::flush;
    do_test_data_putget<int64_t>(file, "data_int64", 12);
    cout << "done\n";

    // put/get a size_t
    cout << "\tread/write size_T..." << std::flush;
    do_test_data_putget<uint64_t>(file, "data_sizet", 12);
    cout << "done\n";

    // put/get a float
    cout << "\tread/write float..." << std::flush;
    do_test_data_putget<float>(file, "data_float", 1.2f);
    cout << "done\n";

    // put/get double
    cout << "\tread/write double..." << std::flush;
    do_test_data_putget<double>(file, "data_double", 1.4);
    cout << "done\n";

    // put/get a single char
    cout << "\tread/write char..." << std::flush;
    do_test_data_putget<char>(file, "data_char", 'x');
    cout << "done\n" << std::flush;
  }

  void test_putData_bad() {
    cout << "\ntest putData -- bad\n" << std::flush;
    // open a file
    FileResource resource("test_nexus_file_dataRW.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // try to put data when not in a dataset -- should fail
    int data = 1;
    file.makeGroup("a_group", "NXshirt", true);
    TS_ASSERT_THROWS(file.putData(&data), Mantid::Nexus::Exception const &);
  }

  void test_data_putget_string() {
    cout << "\ntest dataset read/write -- string" << std::endl;

    // open a file
    FileResource resource("test_nexus_file_stringrw.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // put/get a string
    cout << "\nread/write string..." << std::endl;
    string in("this is a string"), out;
    file.makeData("string_data", NXnumtype::CHAR, in.size(), true);
    file.putData(&in);
    out = file.getStrData();
    file.closeData();
    TS_ASSERT_EQUALS(in, out);

    // do it another way
    // NOTE: to properly set the DataSpace, should be `dims {in.size(), 1}` and use rank = 2
    // However, that seems to contradict notes inside File::compMakeData about rank for string data
    // Using rank = 1 works, but the DataSpace will register size = 1
    in = "this is some different data";
    Mantid::Nexus::DimVector dims{(Mantid::Nexus::dimsize_t)in.size()};
    file.makeData("more_string_data", NXnumtype::CHAR, dims, true);
    file.putData(&in);
    out = file.getStrData();
    file.closeData();
    TS_ASSERT_EQUALS(in, out);

    // yet another way
    in = "even more data";
    file.makeData("string_data_2", NXnumtype::CHAR, in.size(), true);
    file.putData(&in);
    out = file.getStrData();
    TS_ASSERT_EQUALS(in, out);
  }

  void test_check_str_length() {
    cout << "\ntest dataset read/write -- string length" << std::endl;
    FileResource resource("test_nexus_str_len.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    std::string testStr("some_str_data");
    std::string padded(128, '\0');
    std::copy(testStr.begin(), testStr.end(), padded.begin());
    file.makeData("string_data", NXnumtype::CHAR, padded.size(), true);
    file.putData(&padded);
    file.closeData();

    file.openAddress("/entry/string_data");
    Mantid::Nexus::Info info = file.getInfo();
    auto data = file.getStrData();

    TS_ASSERT_EQUALS(info.type, NXnumtype::CHAR);
    TS_ASSERT_EQUALS(info.dims[0], testStr.length());
    TS_ASSERT_EQUALS(data.length(), testStr.length());
    TS_ASSERT_EQUALS(data, testStr);
  }

  void test_data_putget_array() {
    cout << "\ntest dataset read/write -- arrays\n" << std::flush;

    // open a file
    FileResource resource("test_nexus_file_dataRW.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // put/get an int
    file.makeData("data_int", NXnumtype::INT32, 4, true);
    int in[] = {12, 7, 2, 3}, out[4];
    file.putData(&(in[0]));
    Mantid::Nexus::Info info = file.getInfo();
    file.getData(&(out[0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(in[i], out[i]);
    }

    // put/get double array
    file.makeData("data_double", NXnumtype::FLOAT64, 4, true);
    double ind[] = {12.0, 7.22, 2.3, 3.141592}, outd[4];
    file.putData(&(ind[0]));
    info = file.getInfo();
    file.getData(&(outd[0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(ind[i], outd[i]);
    }

    // put/get double 2D array
    Mantid::Nexus::DimVector dims{3, 2};
    double indd[3][2] = {{12.4, 17.89}, {1256.22, 3.141592}, {0.001, 1.0e4}};
    double outdd[3][2];
    file.makeData("data_double_2d", NXnumtype::FLOAT64, dims, true);
    file.putData(&(indd[0][0]));
    info = file.getInfo();
    file.getData(&(outdd[0][0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 2);
    TS_ASSERT_EQUALS(info.dims.front(), 3);
    TS_ASSERT_EQUALS(info.dims.back(), 2);
    for (Mantid::Nexus::dimsize_t i = 0; i < dims[0]; i++) {
      for (Mantid::Nexus::dimsize_t j = 0; j < dims[1]; j++) {
        TS_ASSERT_EQUALS(indd[i][j], outdd[i][j]);
      }
    }

    // put/get a char array
    char word[] = "silicovolcaniosis";
    char read[20] = {'A'};
    file.makeData("data_char", NXnumtype::CHAR, 17, true);
    file.putData(word);
    info = file.getInfo();
    file.getData(read);
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.type, NXnumtype::CHAR);
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 17);
    TS_ASSERT_EQUALS(std::string(read), "silicovolcaniosis");
    TS_ASSERT_EQUALS(std::string(read), std::string(word));

    // put/get a 2D char array
    char words[3][10] = {"First row", "2", ""};
    char reads[3][10];
    dims = {3, 9};
    file.makeData("data_char_2d", NXnumtype::CHAR, dims, true);
    file.putData(&(words[0][0]));
    info = file.getInfo();
    file.getData(&(reads[0][0]));
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 2);
    TS_ASSERT_EQUALS(info.dims.front(), 3);
    TS_ASSERT_EQUALS(info.dims.back(), 9);
    for (Mantid::Nexus::dimsize_t i = 0; i < dims[0]; i++) {
      TS_ASSERT_EQUALS(string(words[i]), string(reads[i]));
    }
  }

  void test_data_putget_vector() {
    cout << "\ntest dataset read/write -- vector\n" << std::flush;

    // open a file
    FileResource resource("test_nexus_file_dataRW_vec.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // put/get an int vector
    vector<int32_t> in{11, 8, 9, 12}, out;
    file.makeData("data_int", NXnumtype::INT32, in.size(), true);
    file.putData(in);
    file.getData(out);
    Mantid::Nexus::Info info = file.getInfo();
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), in.size());
    TS_ASSERT_EQUALS(in, out);

    // put/get a double vector
    vector<double> ind{101.1, 0.008, 9.1123e12, 12.4}, outd;
    file.makeData("data_dbl", NXnumtype::FLOAT64, ind.size(), true);
    file.putData(ind);
    file.getData(outd);
    info = file.getInfo();
    file.closeData();
    // confirm
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), ind.size());
    TS_ASSERT_EQUALS(ind, outd);
  }

  void test_data_string_array_as_char_array() {
    // this test checks that string arrays saved a block char arrays can be read back
    // this guards against a regression that would otherwise occur within PropertyNexusTest
    cout << "\ntest dataset read existing -- char array properties\n" << std::flush;

    // must first create the file by loading an empty instrument and then saving the nexus file
    FileResource fileInfo("PropertyNexusTest.nxs");
    std::string filename = fileInfo.fullPath();

    // create a file and write string data through a char
    // this mimics behavior from TimeSeriesProperty::saveProperty()
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);
    // setup the string data
    std::vector<std::string> values{"help me i", "am stuck in a NXS file", "forever"};
    std::size_t numStr = values.size();
    std::size_t maxlen = values[1].size() + 1;
    // copy the strings into a char array
    std::vector<char> strs(numStr * maxlen);
    std::size_t index = 0;
    for (auto const &prop : values) {
      std::copy(prop.begin(), prop.end(), &strs[index]);
      index += maxlen;
    }
    // write the strings as a flat array, but with dims for a block
    Mantid::Nexus::DimVector dims{numStr, maxlen};
    file.makeData("value", NXnumtype::CHAR, dims, true);
    file.putData(strs.data());

    // read the string data -- mimics ProperyNexus::makeStringProperty()
    Mantid::Nexus::Info info = file.getInfo();
    int64_t numStrings = info.dims[0];
    int64_t span = info.dims[1];
    auto data = std::make_unique<char[]>(numStrings * span);
    file.getData(data.get());
    std::vector<std::string> actual;
    actual.reserve(static_cast<size_t>(numStrings));
    for (int64_t i = 0; i < numStrings; i++)
      actual.emplace_back(data.get() + i * span);
    TS_ASSERT_EQUALS(actual, values);

    // cleanup
    file.closeData();
    file.closeGroup();
    file.close();
  }

  void test_data_zero_dims() {
    // this test checks that string data lengths are still correctly determined
    // even if the dimensions of a char block have been (stupidly) set to 0 by saving as a scalar
    // this guards against a regression that would otherwise occur within NexusGeometrySave
    cout << "\ntest dataset read existing -- zero dims\n" << std::flush;

    // create a file and write string data with zero dimensions
    // this has to be done using the hdf5 C library

    std::string data("this is a string of data");

    // open the file
    FileResource resource("test_ess_instrument.nxs");
    std::string filename = resource.fullPath();
    // file permissions
    Mantid::Nexus::ParameterID fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);
    hid_t fid = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);

    // put an initial entry
    Mantid::Nexus::GroupID groupid = H5Gcreate(fid, "entry", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    // add a NX_class attribute
    Mantid::Nexus::DataTypeID attrtype = H5Tcopy(H5T_C_S1);
    H5Tset_size(attrtype, 7);
    Mantid::Nexus::DataSpaceID attrspce = H5Screate(H5S_SCALAR);
    Mantid::Nexus::AttributeID attrid = H5Acreate(groupid, "NX_class", attrtype, attrspce, H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attrid, attrtype, "NXpants");

    // make and put the data
    Mantid::Nexus::DataTypeID datatype = H5Tcopy(H5T_C_S1);
    H5Tset_size(datatype, data.size());
    // hsize_t dims[] = {0, data.size()}; // dims are zero
    Mantid::Nexus::DataSpaceID dataspace = H5Screate(H5S_SCALAR); // H5Screate_simple(2, dims, NULL);
    Mantid::Nexus::DataSetID dataid =
        H5Dcreate(groupid, "data", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(dataid, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.c_str());

    // verify the file was setup correctly
    hsize_t mydim[4] = {4, 5}; // "junk" values
    int iRank = H5Sget_simple_extent_dims(dataspace, mydim, NULL);
    TS_ASSERT_EQUALS(iRank, 0);
    TS_ASSERT_EQUALS(mydim[0], 4); // junk value unchanged
    TS_ASSERT_EQUALS(mydim[1], 5); // ""
    hsize_t len = H5Tget_size(datatype);
    TS_ASSERT_EQUALS(len, data.size());

    // cleanup and close file
    H5Fclose(fid);

    // now open the file and read
    Mantid::Nexus::File file(filename, NXaccess::READ);
    if (file.hasAddress("entry/data")) {
      file.openAddress("entry/data");
    } else {
      TS_FAIL("Failed to find the written address");
    }
    Mantid::Nexus::Info info = file.getInfo();
    char *value = new char[data.size() + 1];
    file.getData(value);
    std::string actual(value);
    delete[] value;
    TS_ASSERT_EQUALS(info.dims[0], data.size());
    TS_ASSERT_EQUALS(actual, data);

    // cleanup
    file.closeData();
    file.closeGroup();
    file.close();
  }

  void test_data_existing_str_len() {
    // this test protects against a regression that can occur inside LoadNexusLogs
    // the correct length to use for rank-2 char blocks is H5Tget_size() x dims[0],
    // and not a single char more.  Null-termination will be correctly handled this way.
    // Trying to be even-more-null-terminated will lead to buffer overflow errors.
    cout << "\ntest dataset read existing -- string block logs\n" << std::flush;

    // open a file
    std::string filename = getFullPath("SANS2D00022048.nxs");
    Mantid::Nexus::File file(filename, NXaccess::READ);

    // this is the dataset that can cause the buffer errors
    std::string addressOfBad("/raw_data_1/selog/S6/value_log/value");

    // this is meant to mimic the behavior inside LoadNexusLogs::createTimeSeries
    // at around L202 - L243, the section handling NXnumtype::CHAR
    TS_ASSERT_EQUALS(file.hasAddress(addressOfBad), true);
    file.openAddress(addressOfBad);
    Mantid::Nexus::Info info = file.getInfo();
    std::size_t total_length = info.dims[0] * info.dims[1];
    char *val_array = new char[total_length];
    TS_ASSERT_THROWS_NOTHING(file.getData(val_array));
    std::string values(val_array, total_length);
    TS_ASSERT_EQUALS(values, "MediumMediumMediumMedium");
    delete[] val_array;
  }

  // #################################################################################################################
  // TEST ADDRESS METHODS
  // #################################################################################################################

  /* NOTE for historical reasons, additional tests exist in NexusFileReadWriteTest.h*/

  void test_getAddress_groups() {
    cout << "\ntest get_address -- groups only\n" << std::flush;
    FileResource resource("test_nexus_file_grp.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // at root, address should be "/"
    TS_ASSERT_EQUALS("/", file.getAddress());

    // make and open a group -- now at "/abc"
    file.makeGroup("abc", "NXclass", true);
    TS_ASSERT_EQUALS("/abc", file.getAddress());

    // make another layer -- at "/acb/def"
    file.makeGroup("def", "NXentry", true);
    TS_ASSERT_EQUALS("/abc/def", file.getAddress());

    // go down a step -- back to "/abc"
    file.closeGroup();
    TS_ASSERT_EQUALS("/abc", file.getAddress());

    // go up a different step -- at "/abc/ghi"
    file.makeGroup("ghi", "NXfunsicle", true);
    TS_ASSERT_EQUALS("/abc/ghi", file.getAddress());

    // make a group with the same name at this level -- "/abs/ghi/ghi"
    file.makeGroup("ghi", "NXsnowcone", true);
    TS_ASSERT_EQUALS("/abc/ghi/ghi", file.getAddress());
  }

  void test_getAddress_data() {
    cout << "\ntest get_address -- groups and data!\n" << std::flush;
    FileResource resource("test_nexus_file_grpdata.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // at root, address should be "/"
    TS_ASSERT_EQUALS("/", file.getAddress());

    // make and open a group -- now at "/abc"
    file.makeGroup("abc", "NXentry", true);
    TS_ASSERT_EQUALS("/abc", file.getAddress());

    // make another layer -- at "/acb/def"
    file.makeData("def", NXnumtype::INT32, 1, true);
    int in = 17;
    file.putData(&in);
    TS_ASSERT_EQUALS("/abc/def", file.getAddress());
    file.closeData();
  }

  void test_openAddress() {
    using Mantid::Nexus::Entry;
    cout << "\ntest openAddress\n" << std::flush;
    // open a file
    FileResource resource("test_nexus_entries.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    const std::string NXENTRY("NXentry");

    // setup a recursive group tree
    std::vector<Entry> tree{Entry{"/entry1", NXENTRY},
                            Entry{"/entry1/layer2a", NXENTRY},
                            Entry{"/entry1/layer2a/layer3a", NXENTRY},
                            Entry{"/entry1/layer2a/layer3b", NXENTRY},
                            Entry{"/entry1/layer2a/data1", "SDS"},
                            Entry{"/entry1/layer2b", NXENTRY},
                            Entry{"/entry1/layer2b/layer3a", NXENTRY},
                            Entry{"/entry1/layer2b/layer3b", NXENTRY},
                            Entry{"/entry2", NXENTRY},
                            Entry{"/entry2/layer2c", NXENTRY},
                            Entry{"/entry2/layer2c/layer3c", NXENTRY}};

    string current;
    for (auto it = tree.begin(); it != tree.end(); it++) {
      current = file.getAddress();
      string address = it->first;
      while (address.find(current) == address.npos) {
        file.closeGroup();
        current = file.getAddress();
      }
      string name = address.substr(address.find_last_of("/") + 1, address.npos);
      if (it->second == NXENTRY) {
        file.makeGroup(name, it->second, true);
      } else if (it->second == "SDS") {
        string data = "Data";
        file.makeData(name, NXnumtype::CHAR, data.size(), true);
        file.putData(data.data());
        file.closeData();
      }
    }
    file.closeGroup();
    file.closeGroup();
    file.closeGroup();

    // make sure we are at root
    file.openAddress("/");

    // tests invalid cases
    TS_ASSERT_THROWS(file.openAddress(""), Mantid::Nexus::Exception &);
    TS_ASSERT_EQUALS(file.getAddress(), "/");
    TS_ASSERT_THROWS_NOTHING(file.openAddress("entry1"));
    TS_ASSERT_EQUALS(file.getAddress(), "/entry1");
    file.closeGroup();
    TS_ASSERT_EQUALS(file.getAddress(), "/");
    TS_ASSERT_THROWS(file.openAddress("/pants"), Mantid::Nexus::Exception &);
    TS_ASSERT_EQUALS(file.getAddress(), "/");
    // NOTE pre-existent behavior will partially open invalid paths
    TS_ASSERT_THROWS(file.openAddress("/entry1/pants"), Mantid::Nexus::Exception &);
    TS_ASSERT_EQUALS(file.getAddress(), "/");

    // open the root
    std::string expected = "/";
    file.openAddress(expected);
    TS_ASSERT_EQUALS(file.getAddress(), expected);

    // move to inside the entry
    file.openGroup("entry1", "NXentry");

    expected = "/entry1/layer2b/layer3a";
    file.openAddress(expected);
    TS_ASSERT_EQUALS(file.getAddress(), expected);

    expected = "/entry1/layer2a/data1";
    file.openAddress(expected);
    TS_ASSERT_EQUALS(file.getAddress(), expected);

    // open an address without an initial "/"
    file.openAddress("/");
    expected = "entry1/layer2b";
    TS_ASSERT_THROWS_NOTHING(file.openAddress(expected));
    TS_ASSERT_EQUALS(file.getAddress(), "/" + expected);

    // failling should leave path alone
    TS_ASSERT_THROWS(file.openAddress("/pants"), Mantid::Nexus::Exception &);
    TS_ASSERT_EQUALS(file.getAddress(), "/" + expected);

    // intermingle working and failing opens
    file.openAddress("/entry1/layer2a/");
    TS_ASSERT_THROWS(file.openGroup("pants", NXENTRY), Mantid::Nexus::Exception &);
    file.openGroup("layer3a", NXENTRY);
    TS_ASSERT_EQUALS(file.getAddress(), "/entry1/layer2a/layer3a");
  }

  void test_getInfo() {
    cout << "\ntest getInfo -- good\n" << std::flush;

    // open a file
    FileResource resource("test_nexus_file_dataRW.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // put an integer
    int in = 17;
    file.makeData("int_data", NXnumtype::INT32, 1, true);
    file.putData(&in);

    // get the info and check
    Mantid::Nexus::Info info = file.getInfo();
    TS_ASSERT_EQUALS(info.type, NXnumtype::INT32);
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 1);

    file.closeData();

    // put a double
    double ind = 107.2345;
    file.makeData("double_data", NXnumtype::FLOAT64, 1, true);
    file.putData(&ind);

    // get the info and check
    info = file.getInfo();
    TS_ASSERT_EQUALS(info.type, NXnumtype::FLOAT64);
    TS_ASSERT_EQUALS(info.dims.size(), 1);
    TS_ASSERT_EQUALS(info.dims.front(), 1);
  }

  void test_getInfo_bad() {
    cout << "\ntest getInfo -- bad\n" << std::flush;
    // open a file
    FileResource resource("test_nexus_file_dataRW.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // put an integer
    int in = 17;
    file.makeData("int_data", NXnumtype::INT32, 1, true);
    file.putData(&in);
    file.closeData();

    // open a group and try to get info
    file.makeGroup("a_group", "NXshorts", true);
    TS_ASSERT_THROWS(file.getInfo(), Mantid::Nexus::Exception const &);
  }

  void test_isDataSetOpen() {
    cout << "\ntest is data set open\n" << std::flush;
    // open a file
    FileResource resource("test_nexus_file_isdataopen.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // the root is not a dataset
    TS_ASSERT(!file.isDataSetOpen());

    // a group is not a dataset
    file.makeGroup("entry", "NXentry", true);
    TS_ASSERT(!file.isDataSetOpen());

    // a dataset IS a dataset
    file.makeData("data", NXnumtype::CHAR, 1, true);
    TS_ASSERT(file.isDataSetOpen());

    file.close();
  }

  void test_isDataInt() {
    cout << "\ntest is data set open\n";
    // open a file
    FileResource resource("test_nexus_file_isdataopen.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    // char data is not int
    file.makeData("chardata", NXnumtype::CHAR, 1, true);
    TS_ASSERT(!file.isDataInt());

    // float data is not int
    file.makeData("floatdata", NXnumtype::FLOAT32, 1, true);
    TS_ASSERT(!file.isDataInt());

    // all the integer types are ints
    std::vector<NXnumtype> inttypes{NXnumtype::INT8,  NXnumtype::UINT8,  NXnumtype::INT16, NXnumtype::UINT16,
                                    NXnumtype::INT32, NXnumtype::UINT32, NXnumtype::INT64, NXnumtype::UINT64};
    for (NXnumtype const &type : inttypes) {
      file.makeData("data_" + std::string(type), type, 1, true);
      TS_ASSERT(file.isDataInt());
    }

    file.close();
  }

  // ##################################################################################################################
  // TEST ATTRIBUTE METHODS
  // ################################################################################################################

  template <typename T> void do_test_putget_attr(Mantid::Nexus::File &file, string name, T const &data) {
    // test put/get by pointer to data
    T out;
    file.putAttr(name, data);
    file.getAttr(name, out);
    TS_ASSERT_EQUALS(data, out);
  }

  void test_putget_attr_basic() {
    cout << "\ntest attribute read/write\n";

    // open a file
    FileResource resource("test_nexus_attr.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    // move to an entry to avoid conflict with some root-level attributes
    file.makeGroup("entry", "NXentry", true);

    std::vector<std::string> expected_names{"int_attr_", "dbl_attr_"};

    // put/get an int attribute
    do_test_putget_attr(file, expected_names[0], 12);

    // put/get a double attribute
    do_test_putget_attr(file, expected_names[1], 120.2e6);

    // put/get a single char attribute
    // do_test_putget_attr(file, expected_names[2], 'x');

    // check attr infos
    auto attrInfos = file.getAttrInfos();
    TS_ASSERT_EQUALS(attrInfos.size(), expected_names.size());
    for (size_t i = 0; i < attrInfos.size(); i++) {
      TS_ASSERT_EQUALS(attrInfos[i].name, expected_names[i]);
      TS_ASSERT_EQUALS(attrInfos[i].length, 1);
    }
  }

  void test_putget_attr_different_types() {
    cout << "\ntest attribute -- different types\n";

    // open a file
    FileResource resource("test_nexus_attr_different.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    file.makeGroup("entry", "NXentry", true);

    std::vector<std::string> expected_names{"int_attr", "dbl_attr"};

    // put the attributes
    int64_t in(7);
    double rin(124.e7);
    file.putAttr(expected_names[0], in);
    file.putAttr(expected_names[1], rin);
    file.flush();

    int8_t out;
    float rout;
    file.getAttr(expected_names[0], out);
    file.getAttr(expected_names[1], rout);
    TS_ASSERT_EQUALS(in, out);
    TS_ASSERT_EQUALS(rin, rout);
    file.close();
  }

  void test_putget_attr_str() {
    cout << "\ntest string attribute read/write\n";

    // open a file
    FileResource resource("test_nexus_attr.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    // move to an entry to avoid conflict with some root-level attributes
    file.makeGroup("entry", "NXentry", true);

    // put/get a string attribute
    string data = "different string of text";
    do_test_putget_attr(file, "str_attr_", data);

    std::string actual;
    // put/get a string from a string literal
    file.putAttr("units", "kg * mol / parsec");
    file.getAttr("units", actual);
    TS_ASSERT_EQUALS(actual, "kg * mol / parsec");

    std::string again = file.getStrAttr("units");
    TS_ASSERT_EQUALS(again, "kg * mol / parsec");

    // check attr infos
    auto attrInfos = file.getAttrInfos();
    TS_ASSERT_EQUALS(attrInfos.size(), 2);
    TS_ASSERT_EQUALS(attrInfos[0].name, "str_attr_");
    TS_ASSERT_EQUALS(attrInfos[0].type, NXnumtype::CHAR);
    TS_ASSERT_EQUALS(attrInfos[0].length, data.size());
    TS_ASSERT_EQUALS(attrInfos[1].name, "units");
    TS_ASSERT_EQUALS(attrInfos[1].type, NXnumtype::CHAR);
    TS_ASSERT_EQUALS(attrInfos[1].length, actual.size());
  }

  void test_get_bad_attr_fails() {
    cout << "\ntest atttribute -- bad\n";

    // open a file
    FileResource resource("test_nexus_attr_bad.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    // move to an entry to avoid conflict with some root-level attributes
    file.makeGroup("entry", "NXentry", true);

    std::vector<std::string> attr_names{"attr_1", "attr_2"};
    int64_t in = 7, out;
    TS_ASSERT_THROWS_NOTHING(file.putAttr(attr_names[0], in));
    TS_ASSERT_THROWS_NOTHING(file.getAttr(attr_names[0], out));
    TS_ASSERT_EQUALS(in, out);

    // try to get an attribute that doesn't exist
    TS_ASSERT(!file.hasAttr(attr_names[1]));
    TS_ASSERT_THROWS(file.getAttr(attr_names[1], out), Mantid::Nexus::Exception const &);

    // cleanup
    file.close();
  }

  void test_attr_contrary_type() {
    cout << "\ntest read existing -- sample/material\n";

    // open a file
    std::string filename = getFullPath("md_missing_paramater_map.nxs");
    Mantid::Nexus::File file(filename, NXaccess::READ);

    std::string addressOfBad("/MDHistoWorkspace/experiment0/sample/material");
    TS_ASSERT_EQUALS(file.hasAddress(addressOfBad), true);

    // go to this address and read things in
    file.openAddress(addressOfBad);

    // read the name
    std::string name;
    TS_ASSERT_THROWS_NOTHING(file.getAttr("name", name));
    TS_ASSERT(name.empty());

    // confirm version is 2
    // note that it is stored internally as INT64
    float version;
    TS_ASSERT_THROWS_NOTHING(file.getAttr("version", version));
    TS_ASSERT_EQUALS(version, 2);

    // read the formula style
    std::string formulaStyle;
    TS_ASSERT_THROWS_NOTHING(file.getAttr("formulaStyle", formulaStyle));
    TS_ASSERT_EQUALS(formulaStyle, "empty");
    file.close();
  }

  void test_putget_attr_group_and_dataset_and_root() {
    cout << "\ntest attribute read/write on group, on dataset, and on root\n";

    // open a file
    FileResource resource("test_nexus_attr_different_obj.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    std::vector<std::string> expected_names{"root_attr_", "group_attr_", "sds_attr_"};
    std::vector<int> expected_values{13, 12, 17};
    std::vector<std::string> expected_str_names{"root_str_attr_", "group_str_attr_", "sds_str_attr_"};
    std::vector<std::string> expected_string{"root_data", "group_data", "data_data"};

    // NOTE H5cpp catches and does not rethrow the error when closing a Group that was
    // created from a DataSet or H5File ID.  It is only logged to std::cerr.  We therefore need to
    // redirect std::cerr and make sure it stays blank to detect this sort of error.
    // If uncaught this can cause memory leaks
    std::stringstream errors;
    std::streambuf *old_cerr = std::cerr.rdbuf();
    std::cerr.rdbuf(errors.rdbuf());

    // put/get an attribute at root
    do_test_putget_attr(file, expected_names[0], expected_values[0]);
    do_test_putget_attr(file, expected_str_names[0], expected_string[0]);

    // put/get an attribute in a group
    file.makeGroup("entry", "NXentry", true);
    do_test_putget_attr(file, expected_names[1], expected_values[1]);
    do_test_putget_attr(file, expected_str_names[1], expected_string[1]);

    // put/get an attribute in a dataset
    file.makeData("data", NXnumtype::INT64, 1, true);
    do_test_putget_attr(file, expected_names[2], expected_values[2]);
    do_test_putget_attr(file, expected_str_names[2], expected_string[2]);

    // make sure none of the above logged any errors
    TSM_ASSERT(errors.str(), errors.str().empty());

    // cleanup
    file.close();
    std::cerr.rdbuf(old_cerr);
  }

  void test_attr_existing_missing() {
    // this test protects against a regression that can occur in OSX inside of LoadMD
    // for reference, see Material::loadNexus() in Material.cpp
    // see also the test LoadMDTest::test_loading_MD_with_missing_parameter_map
    cout << "\ntest dataset read existing -- sample/material\n";

    // open a file
    std::string filename = getFullPath("md_missing_paramater_map.nxs");
    Mantid::Nexus::File file(filename, NXaccess::READ);

    std::string addressOfBad("/MDHistoWorkspace/experiment0/sample");
    TS_ASSERT_EQUALS(file.hasAddress(addressOfBad), true);

    // go to this address and read things in
    file.openAddress(addressOfBad);

    // confirm version is 1 -- this has no formulaStyle
    int32_t version32;
    int64_t version64;
    file.getAttr("version", version32);
    file.getAttr("version", version64);
    TS_ASSERT_EQUALS(version32, 1);
    TS_ASSERT_EQUALS(version64, 1);

    // read the formula style -- should throw exception
    std::string formulaStyle;
    TS_ASSERT_THROWS(file.getAttr("formulaStyle", formulaStyle), Mantid::Nexus::Exception const &);
  }

  void test_existing_attr_resolved() {
    // test that attributes in existing files are corectly resolved
    // this prevents a regression that will otherwise show up in tests of LoadMD and various python algorithms
    cout << "\ntest open existing file with system-dependent type\n";

    // open the file in read-only mode
    std::string filename = getFullPath("md_missing_paramater_map.nxs");
    Mantid::Nexus::File file(filename, NXaccess::READ);

    // go to main entry (for this file, MDHistoWorkspace)
    file.openGroup("MDHistoWorkspace", "NXentry");

    // get the attribute with different types -- should be no errors
    int32_t version32 = 0;
    TS_ASSERT_THROWS_NOTHING(file.getAttr("SaveMDVersion", version32));
    int64_t version64 = 0;
    TS_ASSERT_THROWS_NOTHING(file.getAttr("SaveMDVersion", version64));
    TS_ASSERT_EQUALS(version32, 2);
    TS_ASSERT_EQUALS(version64, 2);
  }

  void test_existing_attr_bad_length() {
    // some fields have the unit attribute set with value "microsecond" but with a length of 8 instead of 11
    // this prevents a regression that will otherwise show up in tests of LoadEventNexus and various python algorithms
    cout << "\ntest open existing file with a badly set attr length\n";

    // open the file in read-only mode
    std::string filename = getFullPath("CG2_monotonically_increasing_pulse_times.nxs.h5");
    Mantid::Nexus::File file(filename, NXaccess::READ);

    // go to the trouble spot -- /entry/bank39_events/event_time_offset
    std::string entryName("/entry/bank39_events/event_time_offset");
    file.openAddress(entryName);
    // this attribute should be the string "microsecond"
    std::string expected("microsecond");
    // make sure the attrinfo corresponding to units has a length of 11
    auto infos = file.getAttrInfos();
    TS_ASSERT_EQUALS(infos[1].name, "units");
    TS_ASSERT_EQUALS(infos[1].length, expected.size());
    // make sure the entire string attribute is read
    std::string units = file.getAttr<std::string>("units");
    TS_ASSERT_EQUALS(units, expected);
  }

  void test_getEntries() {
    using Mantid::Nexus::Entries;
    using Mantid::Nexus::Entry;

    cout << "\ntest getEntries\n" << std::flush;

    // open a file
    FileResource resource("test_nexus_entries.h5");
    std::string filename = resource.fullPath();
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);

    // setup a recursive group tree
    std::vector<Entry> tree{Entry{"/entry1", "NXentry"},
                            Entry{"/entry1/layer2a", "NXentry"},
                            Entry{"/entry1/layer2a/layer3a", "NXentry"},
                            Entry{"/entry1/layer2a/layer3b", "NXentry"},
                            Entry{"/entry1/layer2a/data1", "SDS"},
                            Entry{"/entry1/layer2b", "NXentry"},
                            Entry{"/entry1/layer2b/layer3a", "NXentry"},
                            Entry{"/entry1/layer2b/layer3b", "NXentry"},
                            Entry{"/entry2", "NXentry"},
                            Entry{"/entry2/layer2c", "NXentry"},
                            Entry{"/entry2/layer2c/layer3c", "NXentry"}};

    string current;
    for (auto it = tree.begin(); it != tree.end(); it++) {
      current = file.getAddress();
      string address = it->first;
      while (address.find(current) == address.npos) {
        file.closeGroup();
        current = file.getAddress();
      }
      string name = address.substr(address.find_last_of("/") + 1, address.npos);
      if (it->second == "NXentry") {
        file.makeGroup(name, it->second, true);
      } else if (it->second == "SDS") {
        string data = "Data";
        file.makeData(name, NXnumtype::CHAR, data.size(), true);
        file.putData(data.data());
        file.closeData();
      }
    }

    // at root level, should be entry1, entry2
    file.openAddress("/");
    Entries actual = file.getEntries();
    Entries expected = {Entry{"entry1", "NXentry"}, Entry{"entry2", "NXentry"}};
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry1, should be layer2a, layer2b
    file.openAddress("/entry1");
    actual = file.getEntries();
    expected = Entries({Entry{"layer2a", "NXentry"}, Entry{"layer2b", "NXentry"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry1/layer2a, should be layer3a, layer3b, data1
    file.openAddress("/entry1/layer2a");
    actual = file.getEntries();
    expected = Entries({Entry{"layer3a", "NXentry"}, Entry{"layer3b", "NXentry"}, Entry{"data1", "SDS"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // within entry2/layer2a, should be layer3a, layer3b, data1
    file.openAddress("/entry2/layer2c");
    actual = file.getEntries();
    expected = Entries({Entry{"layer3c", "NXentry"}});
    for (auto it = expected.begin(); it != expected.end(); it++) {
      TS_ASSERT_EQUALS(actual.count(it->first), 1);
      TS_ASSERT_EQUALS(it->second, actual[it->first]);
    }

    // also test root level name
    TS_ASSERT_EQUALS("/entry1", file.getTopLevelEntryName());

    // Empty Group
    file.openAddress("/entry1");
    file.makeGroup("emptyGroup", "NXentry", true);
    file.openAddress("/entry1/emptyGroup");
    actual = file.getEntries();
    TS_ASSERT_EQUALS(actual.size(), 0);

    // Dataset with zero size
    file.makeData("zeroData", NXnumtype::CHAR, 0, true); // 0-length CHAR
    file.closeData();
    actual = file.getEntries();
    TS_ASSERT_EQUALS(actual.count("zeroData"), 1);
    TS_ASSERT_EQUALS(actual["zeroData"], "SDS");
  }

  void test_getEntries_edgeCases() {
    std::cout << "\ntest getEntries with missing NX_class and soft link\n";

    std::string filename = "test_missing_nxclass.h5";
    hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    TS_ASSERT(file_id >= 0);

    hid_t group_id = H5Gcreate(file_id, "/nogroupclass", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    TS_ASSERT(group_id >= 0);
    H5Gclose(group_id);

    herr_t status = H5Lcreate_soft("/nogroupclass", file_id, "/soft_link", H5P_DEFAULT, H5P_DEFAULT);
    TS_ASSERT(status >= 0);

    H5Fclose(file_id);

    Mantid::Nexus::File file(filename, NXaccess::READ);

    file.openAddress("/");
    Mantid::Nexus::Entries entries = file.getEntries();

    TS_ASSERT_EQUALS(entries.count("nogroupclass"), 1);
    TS_ASSERT_EQUALS(entries["nogroupclass"], "NX_UNKNOWN_GROUP");

    TS_ASSERT_EQUALS(entries.count("soft_link"), 1);
    TS_ASSERT_EQUALS(entries["soft_link"], "NX_UNKNOWN_GROUP");
  }
  // ##################################################################################################################

#ifdef _WIN32
#define TZSET _tzset
#define SETENV(value) _putenv_s("TZ", value)
#define UNSETENV() _putenv_s("TZ", "")
#define TARGET_TIMEZONE "EST5EDT"
#else
#define TZSET tzset
#define SETENV(value) setenv("TZ", value, 1)
#define UNSETENV() unsetenv("TZ")
#define TARGET_TIMEZONE "America/New_York"
#endif

  void test_data_existing_time_string() {
    cout << "\ntest dataset read existing -- time string attr\n";

    // open an existing file
    std::string filename = getFullPath("HB2C_7000.nxs.h5");
    Mantid::Nexus::File file(filename, NXaccess::READ);

    std::string time_str;
    file.getAttr("file_time", time_str);
    file.close();

    char *current_tz = getenv("TZ");
    std::string real_tz = current_tz ? current_tz : "";
    SETENV(TARGET_TIMEZONE);
    TZSET();

    Mantid::Types::Core::DateAndTime dandt(time_str);
    std::time_t ntime = dandt.to_time_t();
    std::string new_str = Mantid::Types::Core::DateAndTime::getLocalTimeISO8601String(ntime);
    TS_ASSERT_EQUALS(time_str, new_str);

    if (real_tz != "") {
      SETENV(real_tz.c_str());
    } else {
      UNSETENV();
    }
    TZSET();
  }

  // ##################################################################################################################
  // TEST LINK METHODS
  // ################################################################################################################

  /* NOTE for historical reasons these exist in NexusFileReadWriteTest.h*/

  // ##################################################################################################################
  // TEST READ / WRITE SLAB METHODS
  // ################################################################################################################

  /* NOTE for historical reasons these exist in NexusFileReadWriteTest.h*/

  // ##################################################################################################################
  // TEST RULE OF THREE
  // ################################################################################################################

  bool file_is_closed(std::string const &filename) {
    // this will check if a file is already opened, by trying to open it with incompatible (WEAK) access
    // if this operation FAILS (fid <= 0), then the file is STILL OPENED
    // if this operation SUCCEEDS (fid > 0), then the file was CLOSED
    // NOTE this is ONLY meaningful AFTER a file with the name has been definitely opened
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fclose_degree(fapl, H5F_CLOSE_WEAK);
    hid_t fid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, fapl);
    bool ret = (fid > 0);
    H5Fclose(fid);
    return ret;
  }

  void test_file_is_closed() {
    cout << "\ntest closing files\n" << std::flush;

    FileResource resource("test_nexus_close.nxs");
    std::string filename(resource.fullPath());
    Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    TS_ASSERT(!file_is_closed(filename));
    file.close();
    TS_ASSERT(file_is_closed(filename));

    { // scope the file to check deconstructor
      Mantid::Nexus::File file2(filename, NXaccess::READ);
      TS_ASSERT(!file_is_closed(filename));
    }
    TS_ASSERT(file_is_closed(filename));
  }

  void test_file_id() {
    cout << "\ntest the file id\n" << std::flush;

    Mantid::Nexus::FileID fid;
    TS_ASSERT(!fid.isValid());

    // create a file
    FileResource resource("test_nexus_fid.nxs");
    std::string filename(resource.fullPath());
    { // scoped file creation
      Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    }
    TS_ASSERT(file_is_closed(filename));

    { // scoped fid
      Mantid::Nexus::ParameterID fapl = H5Pcreate(H5P_FILE_ACCESS);
      H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);
      Mantid::Nexus::FileID fid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, fapl);
      TS_ASSERT(!file_is_closed(filename));
      TS_ASSERT(fid.isValid());
    } // fid goes out of scope and deconstructor is called
    // the file is now closed
    TS_ASSERT(file_is_closed(filename));
  }

  void test_file_id_shared_ptr() {
    cout << "\ntest the file id in shared ptr\n" << std::flush;

    // create a file
    FileResource resource("test_nexus_fid.nxs");
    std::string filename(resource.fullPath());
    { // scoped file creation
      Mantid::Nexus::File file(filename, NXaccess::CREATE5);
    }
    TS_ASSERT(file_is_closed(filename));

    { // scoped fid
      Mantid::Nexus::ParameterID fapl = H5Pcreate(H5P_FILE_ACCESS);
      H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);
      auto pfid1 = std::make_shared<Mantid::Nexus::FileID>(H5Fopen(filename.c_str(), H5F_ACC_RDONLY, fapl));
      auto pfid2(pfid1);
      auto pfid3(pfid2);
      TS_ASSERT(!file_is_closed(filename));
      TS_ASSERT_DIFFERS(pfid1->get(), Mantid::Nexus::FileID::INVALID_ID);
      TS_ASSERT_DIFFERS(pfid2->get(), Mantid::Nexus::FileID::INVALID_ID);
      TS_ASSERT_DIFFERS(pfid3->get(), Mantid::Nexus::FileID::INVALID_ID);
      // close pfid1
      pfid1.reset();
      TS_ASSERT(!file_is_closed(filename));
      TS_ASSERT_DIFFERS(pfid2->get(), Mantid::Nexus::FileID::INVALID_ID);
      TS_ASSERT_DIFFERS(pfid3->get(), Mantid::Nexus::FileID::INVALID_ID);
      // close pfid3
      pfid3.reset();
      TS_ASSERT(!file_is_closed(filename));
      TS_ASSERT_DIFFERS(pfid2->get(), Mantid::Nexus::FileID::INVALID_ID);
    } // last pfid goes out of scope and deconstructor is called
    // the file is now closed
    TS_ASSERT(file_is_closed(filename));
  }

  void test_open_concurrent() {
    cout << "\ntest open two concurrent files\n" << std::flush;

    // create a file with two entries
    FileResource resource("test_nexus_concurrent.nxs");
    std::string filename(resource.fullPath());
    { // scoped file creation
      Mantid::Nexus::File file(filename, NXaccess::CREATE5);
      file.makeGroup("entry1", "NXshorts", false);
      file.makeGroup("entry2", "NXpants", false);
      file.close();
    }
    // check the file is closed
    TS_ASSERT(file_is_closed(filename));

    { // scope the files to verify close
      // open the file, twice
      Mantid::Nexus::File file1(filename, NXaccess::READ);
      Mantid::Nexus::File file2(filename, NXaccess::READ);
      // go to different entries
      file1.openGroup("entry1", "NXshorts");
      file2.openGroup("entry2", "NXpants");
      // confirm we are in different locations
      TS_ASSERT_EQUALS(file2.getAddress(), "/entry2");
      TS_ASSERT_EQUALS(file1.getAddress(), "/entry1");
      TS_ASSERT(!file_is_closed(filename));
    }
    // check the file is closed
    TS_ASSERT(file_is_closed(filename));
  }

  void test_copy_creation() {
    cout << "\ntest copy creation\n" << std::flush;

    // create a file with two entries
    FileResource resource("test_nexus_copy_create.nxs");
    std::string filename(resource.fullPath());
    { // scoped file creation
      Mantid::Nexus::File file(filename, NXaccess::CREATE5);
      file.makeGroup("entry1", "NXshorts", true);
      file.putAttr("info", "some info");
      file.closeGroup();
      file.makeGroup("entry2", "NXpants", false);
      file.close();
    }

    { // scope file1
      // open the file and go to an entry
      Mantid::Nexus::File file1(filename, NXaccess::READ);
      file1.openGroup("entry1", "NXshorts");
      TS_ASSERT_EQUALS(file1.getAddress(), "/entry1");
      TS_ASSERT_EQUALS(file1.getStrAttr("info"), "some info");
      { // sccope file2
        // copy the file and go to a different entry
        Mantid::Nexus::File file2(file1);
        TS_ASSERT_EQUALS(file2.getAddress(), "/");
        file2.openGroup("entry2", "NXpants");
        TS_ASSERT_EQUALS(file2.getAddress(), "/entry2");
      } // file2 goes out of scope

      // confirm the first did not move
      TS_ASSERT_EQUALS(file1.getAddress(), "/entry1");
      // confirm it can still be interacted with
      TS_ASSERT_THROWS_NOTHING(file1.getStrAttr("info"));
      TS_ASSERT_EQUALS(file1.getStrAttr("info"), "some info");
      TS_ASSERT_THROWS_NOTHING(file1.openAddress("/entry2"));
      TS_ASSERT_EQUALS(file1.getAddress(), "/entry2");
    }
    // check the file is closed
    TS_ASSERT(file_is_closed(filename));
  }

  void test_copy_from_pointers() {
    cout << "\ntest copy creation\n" << std::flush;

    // create a file with an entry
    FileResource resource("test_nexus_copy_create.nxs");
    std::string filename(resource.fullPath());

    { // scope the created file
      Mantid::Nexus::File file(filename, NXaccess::CREATE5);
      file.makeGroup("entry", "NXshorts", true);
      file.putAttr("info", "some info");
      file.closeGroup();
    }
    TS_ASSERT(file_is_closed(filename));

    // check with pointers
    { // scope file1
      // open the file and go to an entry
      Mantid::Nexus::File file1(filename, NXaccess::READ);
      Mantid::Nexus::File *pfile = &file1;
      { // sccope file2
        // copy the file and go to a different entry
        Mantid::Nexus::File file2(pfile);
        file2.openGroup("entry", "NXshorts");
        TS_ASSERT_EQUALS(file2.getStrAttr("info"), "some info");
      } // file2 goes out of scope
      TS_ASSERT(!file_is_closed(filename));
      pfile->openGroup("entry", "NXshorts");
      TS_ASSERT_EQUALS(pfile->getStrAttr("info"), "some info");
    }
    // check the file is closed
    TS_ASSERT(file_is_closed(filename));

    // check with shared pointers
    { // scope file1
      // open the file and go to an entry
      auto pfile = std::make_shared<Mantid::Nexus::File>(filename, NXaccess::READ);
      { // scope file2
        // copy the file and go to a different entry
        Mantid::Nexus::File file2(pfile);
        file2.openGroup("entry", "NXshorts");
        TS_ASSERT_EQUALS(file2.getStrAttr("info"), "some info");
      } // file2 goes out of scope
      TS_ASSERT(!file_is_closed(filename));
      pfile->openGroup("entry", "NXshorts");
      TS_ASSERT_EQUALS(pfile->getStrAttr("info"), "some info");
    }
    // check the file is closed
    TS_ASSERT(file_is_closed(filename));
  }
};
