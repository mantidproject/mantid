// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/napi.h"
#include "MantidNexus/napi5.h"
#include "test_helper.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace NeXus;
using namespace NexusTest;
using std::cout;
using std::endl;
using std::map;
using std::multimap;
using std::string;
using std::vector;

#define ASSERT_OKAY(status, msg)                                                                                       \
  if ((status) != NXstatus::NX_OK) {                                                                                   \
    NXclose(fid);                                                                                                      \
    std::cerr << msg;                                                                                                  \
    fflush(stderr);                                                                                                    \
    TS_FAIL(msg);                                                                                                      \
  }

#define ASSERT_ERROR(status, msg)                                                                                      \
  if ((status) != NXstatus::NX_ERROR) {                                                                                \
    NXclose(fid);                                                                                                      \
    std::cerr << msg;                                                                                                  \
    fflush(stderr);                                                                                                    \
    TS_FAIL(msg);                                                                                                      \
  }

class NapiUnitTest : public CxxTest::TestSuite {

public:
  // // This pair of boilerplate methods prevent the suite being created statically
  // // This means the constructor isn't called when running other tests
  static NapiUnitTest *createSuite() { return new NapiUnitTest(); }
  static void destroySuite(NapiUnitTest *suite) { delete suite; }

  // #################################################################################################################
  // TEST CONSTRUCTORS
  // #################################################################################################################

  void test_can_create() {
    cout << "\ntest creation\n";

    FileResource resource("test_napi_file_init.h5");
    std::string filename = resource.fullPath();

    // create the file and ensure it exists
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "NXopen file");

    // make sure the fid is set
    TS_ASSERT_DIFFERS(fid->iFID, NULL);

    ASSERT_OKAY(NXclose(fid), "Nxclose file");
    TS_ASSERT(std::filesystem::exists(filename));
  }

  void test_can_open_existing() {
    cout << "\ntest open existing\n";

    FileResource resource("test_napi_file_init.h5");
    std::string filename = resource.fullPath();

    // create the file and ensure it exists
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "NXopen file");
    TS_ASSERT_DIFFERS(fid->iFID, NULL);
    ASSERT_OKAY(NXclose(fid), "Nxclose file");
    TS_ASSERT(std::filesystem::exists(filename));

    // now open it in read mode
    fid = NULL;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_READ, fid), "NXopen existing file");
    char path[10];
    H5Iget_name(fid->iFID, path, 10);
    TS_ASSERT_EQUALS(std::string(path), "/");
    ASSERT_OKAY(NXclose(fid), "Nxclose file");
  }

  void test_clear_on_create() {
    cout << "\ncreation clear old\n";
    fflush(stdout);

    // create an empty file
    FileResource resource("fake_empty_file.nxs.h5");
    std::string filename = resource.fullPath();
    std::ofstream file(filename);
    file << "mock";
    file.close();

    NexusFile5 *fid;

    // open the existing file
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "NXopen bad existing file");
    char path[10];
    H5Iget_name(fid->iFID, path, 10);
    TS_ASSERT_EQUALS(std::string(path), "/");
    ASSERT_OKAY(NXclose(fid), "NXclose file");
    TS_ASSERT(std::filesystem::exists(filename));
  }

  void test_flush() {
    cout << "\ntest flush\n";
    // make sure flush works
    // TODO actually test the buffers
    FileResource resource("test_napi_file_flush.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "Can't open");
    ASSERT_OKAY(NXflush(fid), "Can't flush");
    ASSERT_OKAY(NXclose(fid), "Can't close");
  }

  // #################################################################################################################
  // TEST MAKE / OPEN / CLOSE GROUP
  // #################################################################################################################

  void test_make_group() {
    cout << "\ntest make group\n";
    FileResource resource("test_napi_file_grp.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "opening file");
    ASSERT_OKAY(NXmakegroup(fid, "test_group", "NXsample"), "making group");
    ASSERT_OKAY(NXclose(fid), "closing file");
  }

  void test_open_group() {
    cout << "\ntest openGroup\n";
    FileResource resource("test_napi_file_grp.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;

    // open the fie
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open file");

    // create a group, to be opened
    string grp("test_group"), cls("NXsample");
    ASSERT_OKAY(NXmakegroup(fid, "test_group", "NXsample"), "failed to make group");

    // now open it, check we are at a different location
    ASSERT_OKAY(NXopengroup(fid, grp.c_str(), cls.c_str()), "failed to open group");
    char path[12];
    H5Iget_name(fid->iCurrentG, path, 12);
    TS_ASSERT_EQUALS(std::string(path), "/test_group");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_open_group_bad() {
    cout << "\ntest open bad group\n";
    FileResource resource("test_napi_file_grp.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open file");

    // create a group, to be opened
    string grp("test_group"), cls("NXpants");
    ASSERT_OKAY(NXmakegroup(fid, grp.c_str(), cls.c_str()), "failed to make group");
    // try to open it with wrong class name
    string notcls("NXshorts");
    ASSERT_ERROR(NXopengroup(fid, grp.c_str(), notcls.c_str()), "expected error not raised")
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_open_group_layers() {
    cout << "\ntest open group layers\n";
    FileResource resource("test_napi_file_grp_layers.h5");
    std::string filename = resource.fullPath();
    string grp1("layer1"), grp2("layer2"), cls1("NXpants1"), cls2("NXshorts");

    // create a file with group -- open it
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open file");
    ASSERT_OKAY(NXmakegroup(fid, grp1.c_str(), cls1.c_str()), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, grp1.c_str(), cls1.c_str()), "failed to open group");

    // get the path here for later comparison
    char path1[128];
    H5Iget_name(fid->iCurrentG, path1, 128);

    // create a group inside the group -- open it
    ASSERT_OKAY(NXmakegroup(fid, grp2.c_str(), cls2.c_str()), "failed to make inner group");
    ASSERT_OKAY(NXopengroup(fid, grp2.c_str(), cls2.c_str()), "failed to open inner group");

    char path2[128];
    H5Iget_name(fid->iCurrentG, path2, 128);
    TS_ASSERT_EQUALS(std::string(path2), "/layer1/layer2");
    TS_ASSERT_DIFFERS(std::string(path1), std::string(path2));

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_close_group() {
    cout << "\ntest close group\n";
    FileResource resource("test_napi_file_grp.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open file");

    char root[128];
    H5Iget_name(fid->iFID, root, 128);

    // check error at root
    ASSERT_OKAY(NXclosegroup(fid), "closing root threw an error");

    // now make group, open it, and save address
    string grp("test_group"), cls("NXsample");
    ASSERT_OKAY(NXmakegroup(fid, grp.c_str(), cls.c_str()), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, grp.c_str(), cls.c_str()), "failed to open group");
    char path1[128];
    H5Iget_name(fid->iCurrentG, path1, 128);

    // close the group and check the address
    ASSERT_OKAY(NXclosegroup(fid), "failed to close group");
    char path2[128];
    // NOTE this has to use NXgetpath and not HFIget_name
    // so that it can distinguish the root address from the group ID
    ASSERT_OKAY(NXgetpath(fid, path2, 128), "did not get path");
    TS_ASSERT_EQUALS(std::string(path2), std::string(root));
    TS_ASSERT_DIFFERS(std::string(path1), std::string(path2));

    ASSERT_OKAY(NXclose(fid), "failed to close");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  // #################################################################################################################
  // TEST MAKE / OPEN / PUT / CLOSE DATASET
  // #################################################################################################################

  void test_make_data() {
    cout << "\ntest make data\n";
    FileResource resource("test_napi_file_data.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;

    char name[] = "some_data";
    DimVector dims({1});
    NXnumtype type(NXnumtype::CHAR);

    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");

    // if there is not a top-level NXentry, should throw error
    ASSERT_ERROR(NXmakedata64(fid, name, type, 1, dims.data()), "data made without error");

    // now make a NXentry group and try
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");
    ASSERT_OKAY(NXmakedata64(fid, name, type, 1, dims.data()), "faled to make data");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_open_dataset() {
    cout << "\ntest open data\n";
    FileResource resource("test_napi_file_data.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // create a dataset, to be opened
    char data[] = "test_data";
    DimVector dims({3});
    NXnumtype type(NXnumtype::CHAR);
    ASSERT_OKAY(NXmakedata64(fid, data, type, 1, dims.data()), "failed to make data");

    // check error conditions
    ASSERT_ERROR(NXopendata(fid, "tacos1"), "opened bad data");

    // now open it, check we are at a different location
    ASSERT_OKAY(NXopendata(fid, data), "failed to open data");
    char path[128];
    H5Iget_name(fid->iCurrentD, path, 128);
    TS_ASSERT_EQUALS(std::string(path), "/entry/test_data");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_make_data_lateral() {
    cout << "\ntest make data lateral\n";
    FileResource resource("test_napi_file_rdwr.h5");
    std::string filename = resource.fullPath();

    // setup file for data
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // make and open data
    NXnumtype type(NXnumtype::CHAR);
    DimVector dims({3});
    char data1[] = "data1";
    ASSERT_OKAY(NXmakedata64(fid, data1, type, 1, dims.data()), "failed to make data1");
    ASSERT_OKAY(NXopendata(fid, data1), "failed to open data");
    char path1[128];
    H5Iget_name(fid->iCurrentD, path1, 128);

    // make and open lateral data
    // NOTE this behavior is not what is actually desired and causes confusion
    // Making a dataset while a dataset is already open should be disallowed
    char data2[] = "data2";
    ASSERT_OKAY(NXmakedata64(fid, data2, type, 1, dims.data()), "made a nested data2");
    ASSERT_OKAY(NXopendata(fid, data2), "failed to open data");
    char path2[128];
    H5Iget_name(fid->iCurrentD, path2, 128);

    // make sure new data is created off of entry
    TS_ASSERT_DIFFERS(std::string(path1), std::string(path2));
    TS_ASSERT_EQUALS(std::string(path1), "/entry/data1");
    TS_ASSERT_EQUALS(std::string(path2), "/entry/data2");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_close_data() {
    cout << "\ntest close data\n";
    FileResource resource("test_napi_file_dataclose.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // check error at root
    ASSERT_ERROR(NXclosedata(fid), "");

    // now make data, close it, and check we are back at root
    NXnumtype type(NXnumtype::CHAR);
    DimVector dims({3});
    ASSERT_OKAY(NXmakedata64(fid, "data1", type, 1, dims.data()), "failed to make data1");
    ASSERT_OKAY(NXopendata(fid, "data1"), "failed to open data");
    TS_ASSERT_DIFFERS(fid->iCurrentD, 0);
    ASSERT_OKAY(NXclosedata(fid), "");
    TS_ASSERT_EQUALS(fid->iCurrentD, 0);
    TS_ASSERT_EQUALS(fid->pathPointer, 0);

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_close_data_lateral() {
    cout << "\ntest close data lateral\n";
    FileResource resource("test_napi_file_dataclose.h5");
    std::string filename = resource.fullPath();

    // setup file for data
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    char entry[128];
    H5Iget_name(fid->iCurrentG, entry, 128);

    // make and open data
    NXnumtype type(NXnumtype::CHAR);
    DimVector dims({3});
    char data1[] = "data1";
    ASSERT_OKAY(NXmakedata64(fid, data1, type, 1, dims.data()), "failed to make data1");
    ASSERT_OKAY(NXopendata(fid, data1), "failed to open data");
    char path1[128];
    H5Iget_name(fid->iCurrentD, path1, 128);

    // make and open lateral data
    char data2[] = "data2";
    ASSERT_OKAY(NXmakedata64(fid, data2, type, 1, dims.data()), "made a nested data2");
    ASSERT_OKAY(NXopendata(fid, data2), "failed to open data");
    char path2[128];
    H5Iget_name(fid->iCurrentD, path2, 128);

    // now close lateral data... where are we??
    // NOTE this behavior is not what is actually desired and causes confusion
    // Opening a dataset while a dataset is already open should be disallowed
    // The path is also not pointing in the correct location
    // On close, it would make more sense to go to the previous dataset
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    TS_ASSERT_EQUALS(fid->iCurrentD, 0);
    TS_ASSERT_DIFFERS(fid->iCurrentG, 0);
    char lastpath[128], lastname[128];
    NXgetpath(fid, lastpath, 128);
    H5Iget_name(fid->iCurrentG, lastname, 128);
    TS_ASSERT_EQUALS(std::string(lastname), "/entry");
    TS_ASSERT_EQUALS(std::string(lastpath), "/entry/data1");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  // template <typename T> void do_test_data_putget(NeXus::File &file, string name, T in) {
  //   T out;
  //   file.makeData(name, NeXus::getType<T>(), 1, true);
  //   file.putData(&in);
  //   file.getData(&out);
  //   file.closeData();
  //   TS_ASSERT_EQUALS(in, out);
  // }

  // void test_data_putget_basic() {
  //   cout << "\ntest dataset read/write\n";

  //   // open a file
  //   FileResource resource("test_napi_file_dataRW.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   file.makeGroup("entry", "NXentry", true);

  //   // put/get an int
  //   cout << "\tread/write int...";
  //   do_test_data_putget<int32_t>(file, "data_int", 12);
  //   cout << "done\n";

  //   // put/get an int64_t
  //   cout << "\tread/write int64_t...";
  //   do_test_data_putget<int64_t>(file, "data_int64", 12);
  //   cout << "done\n";

  //   // put/get a size_t
  //   cout << "\tread/write size_T...";
  //   do_test_data_putget<uint64_t>(file, "data_sizet", 12);
  //   cout << "done\n";

  //   // put/get a float
  //   cout << "\tread/write float...";
  //   do_test_data_putget<float>(file, "data_float", 1.2f);
  //   cout << "done\n";

  //   // put/get double
  //   cout << "\tread/write double...";
  //   do_test_data_putget<double>(file, "data_double", 1.4);
  //   cout << "done\n";

  //   // put/get a single char
  //   cout << "\tread/write char...";
  //   do_test_data_putget<char>(file, "data_char", 'x');
  //   cout << "done\n";
  // }

  // void test_putData_bad() {
  //   cout << "\ntest putData -- bad\n";
  //   // open a file
  //   FileResource resource("test_napi_file_dataRW.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   file.makeGroup("entry", "NXentry", true);

  //   // try to put data when not in a dataset -- should fail
  //   int data = 1;
  //   file.makeGroup("a_group", "NXshirt", true);
  //   TS_ASSERT_THROWS(file.putData(&data), NeXus::Exception &);
  // }

  // void test_data_putget_string() {
  //   cout << "\ntest dataset read/write -- string\n";

  //   // open a file
  //   FileResource resource("test_napi_file_stringrw.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   file.makeGroup("entry", "NXentry", true);

  //   // put/get a string
  //   cout << "\nread/write string...\n";
  //   string in("this is a string"), out;
  //   file.makeData("string_data_2", NXnumtype::CHAR, in.size(), true);
  //   file.putData(&in);
  //   out = file.getStrData();
  //   TS_ASSERT_EQUALS(in, out);
  // }

  // void test_data_putget_array() {
  //   cout << "\ntest dataset read/write -- arrays\n";

  //   // open a file
  //   FileResource resource("test_napi_file_dataRW.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   file.makeGroup("entry", "NXentry", true);

  //   // put/get an int
  //   file.makeData("data_int", NeXus::getType<int32_t>(), 4, true);
  //   int in[] = {12, 7, 2, 3}, out[4];
  //   file.putData(&(in[0]));
  //   Info info = file.getInfo();
  //   file.getData(&(out[0]));
  //   file.closeData();
  //   // confirm
  //   TS_ASSERT_EQUALS(info.dims.size(), 1);
  //   TS_ASSERT_EQUALS(info.dims.front(), 4);
  //   for (int i = 0; i < 4; i++) {
  //     TS_ASSERT_EQUALS(in[i], out[i]);
  //   }

  //   // put/get double array
  //   file.makeData("data_double", NeXus::getType<double>(), 4, true);
  //   double ind[] = {12.0, 7.22, 2.3, 3.141592}, outd[4];
  //   file.putData(&(ind[0]));
  //   info = file.getInfo();
  //   file.getData(&(outd[0]));
  //   file.closeData();
  //   // confirm
  //   TS_ASSERT_EQUALS(info.dims.size(), 1);
  //   TS_ASSERT_EQUALS(info.dims.front(), 4);
  //   for (int i = 0; i < 4; i++) {
  //     TS_ASSERT_EQUALS(ind[i], outd[i]);
  //   }

  //   // put/get double 2D array
  //   DimVector dims{3, 2};
  //   double indd[3][2] = {{12.4, 17.89}, {1256.22, 3.141592}, {0.001, 1.0e4}};
  //   double outdd[3][2];
  //   file.makeData("data_double_2d", NeXus::getType<double>(), dims, true);
  //   file.putData(&(indd[0][0]));
  //   info = file.getInfo();
  //   file.getData(&(outdd[0][0]));
  //   file.closeData();
  //   // confirm
  //   TS_ASSERT_EQUALS(info.dims.size(), 2);
  //   TS_ASSERT_EQUALS(info.dims.front(), 3);
  //   TS_ASSERT_EQUALS(info.dims.back(), 2);
  //   for (dimsize_t i = 0; i < dims[0]; i++) {
  //     for (dimsize_t j = 0; j < dims[1]; j++) {
  //       TS_ASSERT_EQUALS(indd[i][j], outdd[i][j]);
  //     }
  //   }

  //   // put/get a char array
  //   char word[] = "silicovolcaniosis";
  //   char read[18];
  //   file.makeData("data_char", NeXus::getType<char>(), 17, true);
  //   file.putData(word);
  //   info = file.getInfo();
  //   file.getData(read);
  //   file.closeData();
  //   // confirm
  //   TS_ASSERT_EQUALS(info.dims.size(), 1);
  //   TS_ASSERT_EQUALS(info.dims.front(), 17);
  // }

  // void test_data_putget_vector() {
  //   cout << "\ntest dataset read/write -- vector\n";

  //   // open a file
  //   FileResource resource("test_napi_file_dataRW_vec.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   file.makeGroup("entry", "NXentry", true);

  //   // put/get an int vector
  //   vector<int32_t> in{11, 8, 9, 12}, out;
  //   file.makeData("data_int", NeXus::getType<int32_t>(), in.size(), true);
  //   file.putData(in);
  //   file.getData(out);
  //   Info info = file.getInfo();
  //   file.closeData();
  //   // confirm
  //   TS_ASSERT_EQUALS(info.dims.size(), 1);
  //   TS_ASSERT_EQUALS(info.dims.front(), in.size());
  //   TS_ASSERT_EQUALS(in, out);

  //   // put/get a double vector
  //   vector<double> ind{101.1, 0.008, 9.1123e12, 12.4}, outd;
  //   file.makeData("data_dbl", NeXus::getType<double>(), ind.size(), true);
  //   file.putData(ind);
  //   file.getData(outd);
  //   info = file.getInfo();
  //   file.closeData();
  //   // confirm
  //   TS_ASSERT_EQUALS(info.dims.size(), 1);
  //   TS_ASSERT_EQUALS(info.dims.front(), ind.size());
  //   TS_ASSERT_EQUALS(ind, outd);
  // }

  // #################################################################################################################
  // TEST PATH METHODS
  // #################################################################################################################

  // void test_get_path_groups() {
  //   cout << "\ntest get path -- groups only\n";
  //   FileResource resource("test_napi_file_grp.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);

  //   // at root, path should be "/"
  //   TS_ASSERT_EQUALS("/", file.getPath());

  //   // make and open a group -- now at "/abc"
  //   file.makeGroup("abc", "NXclass", true);
  //   TS_ASSERT_EQUALS("/abc", file.getPath());

  //   // make another layer -- at "/acb/def"
  //   file.makeGroup("def", "NXentry", true);
  //   TS_ASSERT_EQUALS("/abc/def", file.getPath());

  //   // go down a step -- back to "/abc"
  //   file.closeGroup();
  //   TS_ASSERT_EQUALS("/abc", file.getPath());

  //   // go up a different step -- at "/abc/ghi"
  //   file.makeGroup("ghi", "NXfunsicle", true);
  //   TS_ASSERT_EQUALS("/abc/ghi", file.getPath());
  // }

  // void test_get_path_data() {
  //   cout << "\ntest get path -- groups and data!\n";
  //   FileResource resource("test_napi_file_grpdata.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);

  //   // at root, path should be "/"
  //   TS_ASSERT_EQUALS("/", file.getPath());

  //   // make and open a group -- now at "/abc"
  //   file.makeGroup("abc", "NXentry", true);
  //   TS_ASSERT_EQUALS("/abc", file.getPath());

  //   // make another layer -- at "/acb/def"
  //   file.makeData("def", NeXus::getType<int32_t>(), 1, true);
  //   int in = 17;
  //   file.putData(&in);
  //   TS_ASSERT_EQUALS("/abc/def", file.getPath());
  //   file.closeData();
  // }

  // void test_open_paths() {
  //   cout << "tests for open path\n";

  //   // make file with path /entry
  //   string const filename("napi_openpathtest.nxs");
  //   File fileid = do_prep_files(filename);

  //   // make path /entry/data1
  //   fileid.writeData("data1", '1');

  //   // make path /entry/data2
  //   fileid.writeData("data2", '2');

  //   // make path /entry/data/more_data
  //   fileid.makeGroup("data", "NXdata");
  //   fileid.openGroup("data", "NXdata");
  //   fileid.writeData("more_data", '3');

  //   // make path /link
  //   fileid.closeGroup(); // close /entry/data
  //   fileid.closeGroup(); // close /entry
  //   fileid.makeGroup("link", "NXentry");
  //   fileid.openGroup("link", "NXentry"); // open /link
  //   fileid.writeData("data4", '4');

  //   // compare
  //   char output;
  //   fileid.closeGroup();

  //   fileid.openPath("/entry/data1");
  //   fileid.getData(&output);
  //   TS_ASSERT_EQUALS('1', output);

  //   fileid.openPath("/link/data4");
  //   fileid.getData(&output);
  //   TS_ASSERT_EQUALS('4', output);

  //   fileid.openPath("/entry/data/more_data");
  //   fileid.getData(&output);
  //   TS_ASSERT_EQUALS('3', output);

  //   fileid.openPath("/entry/data2");
  //   fileid.getData(&output);
  //   TS_ASSERT_EQUALS('2', output);

  //   // cleanup
  //   fileid.close();
  //   removeFile(filename);
  //   cout << "NXopenpath checks OK\n";
  // }

  // void test_open_path_tree() {
  //   cout << "\ntest open path\n";
  //   // open a file
  //   FileResource resource("test_napi_entries.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);

  //   // setup a recursive group tree
  //   std::vector<Entry> tree{Entry{"/entry1", "NXentry"},
  //                           Entry{"/entry1/layer2a", "NXentry"},
  //                           Entry{"/entry1/layer2a/layer3a", "NXentry"},
  //                           Entry{"/entry1/layer2a/layer3b", "NXentry"},
  //                           Entry{"/entry1/layer2a/data1", "SDS"},
  //                           Entry{"/entry1/layer2b", "NXentry"},
  //                           Entry{"/entry1/layer2b/layer3a", "NXentry"},
  //                           Entry{"/entry1/layer2b/layer3b", "NXentry"},
  //                           Entry{"/entry2", "NXentry"},
  //                           Entry{"/entry2/layer2c", "NXentry"},
  //                           Entry{"/entry2/layer2c/layer3c", "NXentry"}};

  //   string current;
  //   for (auto it = tree.begin(); it != tree.end(); it++) {
  //     current = file.getPath();
  //     string path = it->first;
  //     while (path.find(current) == path.npos) {
  //       file.closeGroup();
  //       current = file.getPath();
  //     }
  //     string name = path.substr(path.find_last_of("/") + 1, path.npos);
  //     if (it->second == "NXentry") {
  //       file.makeGroup(name, it->second, true);
  //     } else if (it->second == "SDS") {
  //       string data = "Data";
  //       file.makeData(name, NXnumtype::CHAR, data.size(), true);
  //       file.putData(data.data());
  //       file.closeData();
  //     }
  //   }
  //   file.closeGroup();
  //   file.closeGroup();
  //   file.closeGroup();

  //   // tests invalid cases
  //   TS_ASSERT_THROWS(file.openPath(""), NeXus::Exception &);
  //   TS_ASSERT_THROWS(file.openPath("/pants"), NeXus::Exception &);
  //   TS_ASSERT_THROWS(file.openPath("/entry1/pants"), NeXus::Exception &);

  //   // make sure we are at root
  //   file.openPath("/");

  //   // open the root
  //   file.openGroup("entry1", "NXentry");
  //   std::string actual, expected = "/";
  //   file.openPath(expected);
  //   actual = file.getPath();
  //   TS_ASSERT_EQUALS(actual, expected);

  //   expected = "/entry1/layer2b/layer3a";
  //   file.openPath(expected);
  //   actual = file.getPath();
  //   TS_ASSERT_EQUALS(actual, expected);

  //   expected = "/entry1/layer2a/data1";
  //   file.openPath(expected);
  //   actual = file.getPath();
  //   TS_ASSERT_EQUALS(actual, expected);
  // }

  // void test_get_info() {
  //   cout << "\ntest get info -- good\n";

  //   // open a file
  //   FileResource resource("test_napi_file_dataRW.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   file.makeGroup("entry", "NXentry", true);

  //   // put an integer
  //   int in = 17;
  //   file.makeData("int_data", NeXus::getType<int32_t>(), 1, true);
  //   file.putData(&in);

  //   // get the info and check
  //   Info info = file.getInfo();
  //   TS_ASSERT_EQUALS(info.type, NeXus::getType<int32_t>());
  //   TS_ASSERT_EQUALS(info.dims.size(), 1);
  //   TS_ASSERT_EQUALS(info.dims.front(), 1);

  //   file.closeData();

  //   // put a double
  //   double ind = 107.2345;
  //   file.makeData("double_data", NeXus::getType<double>(), 1, true);
  //   file.putData(&ind);

  //   // get the info and check
  //   info = file.getInfo();
  //   TS_ASSERT_EQUALS(info.type, NeXus::getType<double>());
  //   TS_ASSERT_EQUALS(info.dims.size(), 1);
  //   TS_ASSERT_EQUALS(info.dims.front(), 1);
  // }

  // void test_get_info_bad() {
  //   cout << "\ntest get info -- bad\n";
  //   // open a file
  //   FileResource resource("test_napi_file_dataRW.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   file.makeGroup("entry", "NXentry", true);

  //   // put an integer
  //   int in = 17;
  //   file.makeData("int_data", NeXus::getType<int32_t>(), 1, true);
  //   file.putData(&in);
  //   file.closeData();

  //   // open a group and try to get info
  //   file.makeGroup("a_group", "NXshorts", true);
  //   TS_ASSERT_THROWS(file.getInfo(), NeXus::Exception &);
  // }

  // //
  // ##################################################################################################################
  // // TEST ATTRIBUTE METHODS
  // // ################################################################################################################

  // template <typename T> void do_test_putget_attr(NeXus::File &file, string name, T const &data) {
  //   // test put/get by pointer to data
  //   T out;
  //   file.putAttr(name, data);
  //   file.getAttr(name, out);
  //   TS_ASSERT_EQUALS(data, out);
  // }

  // void test_putget_attr_basic() {
  //   cout << "\ntest attribute read/write\n";

  //   // open a file
  //   FileResource resource("test_napi_attr.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   // move to an entry to avoid conflict with some root-level attributes
  //   file.makeGroup("entry", "NXentry", true);

  //   std::vector<std::string> expected_names{"int_attr_", "dbl_attr_"};

  //   // put/get an int attribute
  //   do_test_putget_attr(file, expected_names[0], 12);

  //   // put/get a double attribute
  //   do_test_putget_attr(file, expected_names[1], 120.2e6);

  //   // check attr infos
  //   auto attrInfos = file.getAttrInfos();
  //   TS_ASSERT_EQUALS(attrInfos.size(), expected_names.size());
  //   for (size_t i = 0; i < attrInfos.size(); i++) {
  //     TS_ASSERT_EQUALS(attrInfos[i].name, expected_names[i]);
  //     TS_ASSERT_EQUALS(attrInfos[i].length, 1);
  //   }
  // }

  // void test_putget_attr_str() {
  //   cout << "\ntest string attribute read/write\n";

  //   // open a file
  //   FileResource resource("test_napi_attr.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);
  //   // move to an entry to avoid conflict with some root-level attributes
  //   file.makeGroup("entry", "NXentry", true);

  //   // put/get a string attribute
  //   string data = "different string of text";
  //   do_test_putget_attr(file, "str_attr_", data);

  //   std::string actual;
  //   // put/get a string from a string literal
  //   file.putAttr("units", "kg * mol / parsec");
  //   file.getAttr("units", actual);
  //   TS_ASSERT_EQUALS(actual, "kg * mol / parsec");

  //   // check attr infos
  //   auto attrInfos = file.getAttrInfos();
  //   TS_ASSERT_EQUALS(attrInfos.size(), 2);
  //   TS_ASSERT_EQUALS(attrInfos[0].name, "str_attr_");
  //   TS_ASSERT_EQUALS(attrInfos[0].type, NXnumtype::CHAR);
  //   TS_ASSERT_EQUALS(attrInfos[0].length, data.size());
  //   TS_ASSERT_EQUALS(attrInfos[1].name, "units");
  //   TS_ASSERT_EQUALS(attrInfos[1].type, NXnumtype::CHAR);
  //   TS_ASSERT_EQUALS(attrInfos[1].length, actual.size());
  // }

  // void test_getEntries() {
  //   cout << "\ntest getEntries\n";

  //   // open a file
  //   FileResource resource("test_napi_entries.h5");
  //   std::string filename = resource.fullPath();
  //   NeXus::File file(filename, NXACC_CREATE5);

  //   // setup a recursive group tree
  //   std::vector<Entry> tree{Entry{"/entry1", "NXentry"},
  //                           Entry{"/entry1/layer2a", "NXentry"},
  //                           Entry{"/entry1/layer2a/layer3a", "NXentry"},
  //                           Entry{"/entry1/layer2a/layer3b", "NXentry"},
  //                           Entry{"/entry1/layer2a/data1", "SDS"},
  //                           Entry{"/entry1/layer2b", "NXentry"},
  //                           Entry{"/entry1/layer2b/layer3a", "NXentry"},
  //                           Entry{"/entry1/layer2b/layer3b", "NXentry"},
  //                           Entry{"/entry2", "NXentry"},
  //                           Entry{"/entry2/layer2c", "NXentry"},
  //                           Entry{"/entry2/layer2c/layer3c", "NXentry"}};

  //   string current;
  //   for (auto it = tree.begin(); it != tree.end(); it++) {
  //     current = file.getPath();
  //     string path = it->first;
  //     while (path.find(current) == path.npos) {
  //       file.closeGroup();
  //       current = file.getPath();
  //     }
  //     string name = path.substr(path.find_last_of("/") + 1, path.npos);
  //     if (it->second == "NXentry") {
  //       file.makeGroup(name, it->second, true);
  //     } else if (it->second == "SDS") {
  //       string data = "Data";
  //       file.makeData(name, NXnumtype::CHAR, data.size(), true);
  //       file.putData(data.data());
  //       file.closeData();
  //     }
  //   }

  //   // at root level, should be entry1, entry2
  //   file.openPath("/");
  //   Entries actual = file.getEntries();
  //   Entries expected = {Entry{"entry1", "NXentry"}, Entry{"entry2", "NXentry"}};
  //   for (auto it = expected.begin(); it != expected.end(); it++) {
  //     TS_ASSERT_EQUALS(actual.count(it->first), 1);
  //     TS_ASSERT_EQUALS(it->second, actual[it->first]);
  //   }

  //   // within entry1, should be layer2a, layer2b
  //   file.openPath("/entry1");
  //   actual = file.getEntries();
  //   expected = Entries({Entry{"layer2a", "NXentry"}, Entry{"layer2b", "NXentry"}});
  //   for (auto it = expected.begin(); it != expected.end(); it++) {
  //     TS_ASSERT_EQUALS(actual.count(it->first), 1);
  //     TS_ASSERT_EQUALS(it->second, actual[it->first]);
  //   }

  //   // within entry1/layer2a, should be layer3a, layer3b, data1
  //   file.openPath("/entry1/layer2a");
  //   actual = file.getEntries();
  //   expected = Entries({Entry{"layer3a", "NXentry"}, Entry{"layer3b", "NXentry"}, Entry{"data1", "SDS"}});
  //   for (auto it = expected.begin(); it != expected.end(); it++) {
  //     TS_ASSERT_EQUALS(actual.count(it->first), 1);
  //     TS_ASSERT_EQUALS(it->second, actual[it->first]);
  //   }

  //   // within entry2/layer2a, should be layer3a, layer3b, data1
  //   file.openPath("/entry2/layer2c");
  //   actual = file.getEntries();
  //   expected = Entries({Entry{"layer3c", "NXentry"}});
  //   for (auto it = expected.begin(); it != expected.end(); it++) {
  //     TS_ASSERT_EQUALS(actual.count(it->first), 1);
  //     TS_ASSERT_EQUALS(it->second, actual[it->first]);
  //   }
  // }

  // ##################################################################################################################
  // TEST LINK METHODS
  // ################################################################################################################

  // void test_links() {
  //   cout << "tests of linkature\n";

  //   string const filename("NexusFile_linktest.nxs");
  //   removeFile(filename);
  //   File fileid = do_prep_files(filename);

  //   // Create some data with a link
  //   cout << "create entry at /entry/some_data\n";
  //   string const somedata("this is some data");
  //   fileid.makeData("some_data", NXnumtype::CHAR, DimVector({(dimsize_t)somedata.size()}));
  //   fileid.openData("some_data");
  //   fileid.putData(somedata.c_str());
  //   NXlink datalink = fileid.getDataID();
  //   fileid.closeData();
  //   fileid.flush();

  //   // Create a group, and link it to that data
  //   cout << "create group at /entry/data to link to the data\n";
  //   fileid.makeGroup("data", "NXdata");
  //   fileid.openGroup("data", "NXdata");
  //   fileid.makeLink(datalink);
  //   fileid.closeGroup();
  //   fileid.flush();

  //   // check data link
  //   fileid.openPath("/entry/data/some_data");
  //   // TODO why can't we get the data through the link?
  //   // string output1;
  //   // fileid.getData(&output1);
  //   // TS_ASSERT_EQUALS(somedata, output1);
  //   NXlink res1 = fileid.getDataID();
  //   TS_ASSERT_EQUALS(datalink.linkType, res1.linkType);
  //   TS_ASSERT_EQUALS(datalink.targetPath, res1.targetPath);
  //   cout << "data link works\n";
  //   fileid.closeData();

  //   fileid.openPath("/entry");

  //   // Create two groups, group1 and group2
  //   // Make a link inside group2 to group1
  //   // make group1
  //   cout << "create group /entry/group1\n";
  //   std::string const strdata("NeXus sample data");
  //   fileid.makeGroup("group1", "NXentry");
  //   fileid.openGroup("group1", "NXentry");
  //   NXlink grouplink = fileid.getGroupID();
  //   fileid.closeGroup();

  //   // make group 2
  //   cout << "create group /entry/group2/group1\n";
  //   fileid.makeGroup("group2", "NXentry");
  //   fileid.openGroup("group2", "NXentry");
  //   fileid.makeLink(grouplink);
  //   fileid.closeGroup();

  //   // check group link
  //   fileid.openPath("/entry/group2/group1");
  //   NXlink res2 = fileid.getGroupID();
  //   TS_ASSERT_EQUALS(grouplink.linkType, res2.linkType);
  //   TS_ASSERT_EQUALS(string(grouplink.targetPath), string(res2.targetPath));
  //   cout << "group link works\n";
  // }
};
