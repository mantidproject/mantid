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

using namespace Mantid::Nexus;
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

namespace {
// catch for undefined types
template <typename NumT> NXnumtype getType() { return NXnumtype::BAD; }

// template specialisations for types we know
template <> NXnumtype getType<float>() { return NXnumtype::FLOAT32; }

template <> NXnumtype getType<double>() { return NXnumtype::FLOAT64; }

template <> NXnumtype getType<int32_t>() { return NXnumtype::INT32; }

template <> NXnumtype getType<int64_t>() { return NXnumtype::INT64; }

template <> NXnumtype getType<uint64_t>() { return NXnumtype::UINT64; }

template <> NXnumtype getType<char>() { return NXnumtype::CHAR; }
} // namespace

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
    TS_ASSERT_EQUALS(fid, nullptr);
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
    char address1[128];
    H5Iget_name(fid->iCurrentG, address1, 128);

    // close the group and check the address
    ASSERT_OKAY(NXclosegroup(fid), "failed to close group");
    std::string address2;
    // NOTE this has to use NXgetpath and not HFIget_name
    // so that it can distinguish the root address from the group ID
    ASSERT_OKAY(NXgetaddress(fid, address2), "did not get path");
    TS_ASSERT_EQUALS(address2, std::string(root));
    TS_ASSERT_DIFFERS(address2, std::string(address1));

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
    DimVector dims{1};
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
    DimVector dims{3};
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
    DimVector dims{3};
    ASSERT_OKAY(NXmakedata64(fid, "data1", type, 1, dims.data()), "failed to make data1");
    ASSERT_OKAY(NXopendata(fid, "data1"), "failed to open data");
    TS_ASSERT_DIFFERS(fid->iCurrentD, 0);
    ASSERT_OKAY(NXclosedata(fid), "");
    TS_ASSERT_EQUALS(fid->iCurrentD, 0);

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
    DimVector dims{3};
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
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    TS_ASSERT_EQUALS(fid->iCurrentD, 0);
    TS_ASSERT_DIFFERS(fid->iCurrentG, 0);
    std::string lastaddress;
    char lastname[128];
    NXgetaddress(fid, lastaddress);
    H5Iget_name(fid->iCurrentG, lastname, 128);
    TS_ASSERT_EQUALS(lastaddress, std::string(lastname));
    TS_ASSERT_EQUALS(lastaddress, "/entry");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  template <typename T> void do_test_data_putget(NexusFile5 *fid, string name, T const &in) {
    T out;
    DimVector dims{1};
    ASSERT_OKAY(NXmakedata64(fid, name.c_str(), getType<T>(), 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, name.c_str()), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &in), "failed to put data");
    ASSERT_OKAY(NXgetdata(fid, &out), "failed to get data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    TS_ASSERT_EQUALS(in, out);
  }

  void test_data_putget_basic() {
    cout << "\ntest dataset read/write\n";

    // open a file
    FileResource resource("test_napi_file_dataRW.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put/get an int
    cout << "\tread/write int...";
    do_test_data_putget<int32_t>(fid, "data_int", 12);
    cout << "done\n";

    // put/get an int64_t
    cout << "\tread/write int64_t...";
    do_test_data_putget<int64_t>(fid, "data_int64", 12);
    cout << "done\n";

    // put/get a size_t
    cout << "\tread/write size_T...";
    do_test_data_putget<uint64_t>(fid, "data_sizet", 12);
    cout << "done\n";

    // put/get a float
    cout << "\tread/write float...";
    do_test_data_putget<float>(fid, "data_float", 1.2f);
    cout << "done\n";

    // put/get double
    cout << "\tread/write double...";
    do_test_data_putget<double>(fid, "data_double", 1.4);
    cout << "done\n";

    // put/get a single char
    cout << "\tread/write char...";
    do_test_data_putget<char>(fid, "data_char", 'x');
    cout << "done\n";

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_put_data_bad() {
    cout << "\ntest put data -- bad\n";
    // open a file
    FileResource resource("test_napi_file_dataRW.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // try to put data when not in a dataset -- should fail
    int data = 1;
    ASSERT_OKAY(NXmakegroup(fid, "a_group", "NXshirt"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "a_group", "NXshirt"), "failed to open group");
    ASSERT_ERROR(NXputdata(fid, &data), "putting data in group didn't fail");

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_data_putget_string() {
    cout << "\ntest dataset read/write -- string\n";

    // open a file
    FileResource resource("test_napi_file_stringrw.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put/get a string
    cout << "\nread/write string...\n";
    // NOTE: whitespace is not stripped, so `out` must have EXACTLY the same length a `in`
    string in("this is a string"), out(in.size(), 'X');
    string name("string_data_2");

    // NOTE: to properly set the DataSpace, should be `dims {in.size(), 1}` and use rank = 2
    // However, that seems to contradict notes inside napi5 about rank for string data
    // Using rank = 1 works, but the DataSpace will register size = 1
    DimVector dims{static_cast<dimsize_t>(in.size())};
    ASSERT_OKAY(NXmakedata64(fid, name.c_str(), NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, name.c_str()), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, in.data()), "failed to put data");
    ASSERT_OKAY(NXgetdata(fid, out.data()), "failed to get data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    TS_ASSERT_EQUALS(in, out);

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_data_putget_array() {
    cout << "\ntest dataset read/write -- arrays\n";

    // open a file
    FileResource resource("test_napi_file_dataRW.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put/get an int
    int in[4] = {12, 7, 2, 3}, out[4];
    DimVector dims{4};
    ASSERT_OKAY(NXmakedata64(fid, "data_int", getType<int>(), 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "data_int"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, in), "failed to put data");
    DimVector dimsout{0, 0, 0, 0};
    int rank;
    NXnumtype datatype;
    ASSERT_OKAY(NXgetinfo64(fid, &rank, dimsout.data(), &datatype), "failed to get info");
    ASSERT_OKAY(NXgetdata(fid, &(out[0])), "failed to get data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    // confirm
    TS_ASSERT_EQUALS(rank, 1);
    TS_ASSERT_EQUALS(dimsout.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(in[i], out[i]);
    }
    TS_ASSERT_EQUALS(datatype, NXnumtype::INT32);

    // put/get double array
    double ind[] = {12.0, 7.22, 2.3, 3.141592}, outd[4];
    dims = {4};
    ASSERT_OKAY(NXmakedata64(fid, "data_double", NXnumtype::FLOAT64, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "data_double"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, ind), "failed to put data");
    ASSERT_OKAY(NXgetinfo64(fid, &rank, dimsout.data(), &datatype), "failed to get info");
    ASSERT_OKAY(NXgetdata(fid, outd), "failed to get data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    // confirm
    TS_ASSERT_EQUALS(rank, 1);
    TS_ASSERT_EQUALS(dimsout.front(), 4);
    for (int i = 0; i < 4; i++) {
      TS_ASSERT_EQUALS(ind[i], outd[i]);
    }
    TS_ASSERT_EQUALS(datatype, NXnumtype::FLOAT64);

    // put/get double 2D array
    double indd[3][2] = {{12.4, 17.89}, {1256.22, 3.141592}, {0.001, 1.0e4}}, outdd[3][2];
    dims = {3, 2};
    ASSERT_OKAY(NXmakedata64(fid, "data_double_2d", NXnumtype::FLOAT64, 2, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "data_double_2d"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, indd), "failed to put data");
    ASSERT_OKAY(NXgetinfo64(fid, &rank, dimsout.data(), &datatype), "failed to get info");
    ASSERT_OKAY(NXgetdata(fid, outdd), "failed to get data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    // confirm
    TS_ASSERT_EQUALS(rank, 2);
    TS_ASSERT_EQUALS(dimsout[0], 3);
    TS_ASSERT_EQUALS(dimsout[1], 2);
    for (dimsize_t i = 0; i < dims[0]; i++) {
      for (dimsize_t j = 0; j < dims[1]; j++) {
        TS_ASSERT_EQUALS(indd[i][j], outdd[i][j]);
      }
    }
    TS_ASSERT_EQUALS(datatype, NXnumtype::FLOAT64);

    // put/get a char array
    char word[] = "silicovolcaniosis";
    char read[30] = {'A'}; // pre-fill with junk data
    dims = {static_cast<dimsize_t>(strlen(word))};
    ASSERT_OKAY(NXmakedata64(fid, "data_char", NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "data_char"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, word), "failed to put data");
    ASSERT_OKAY(NXgetinfo64(fid, &rank, dimsout.data(), &datatype), "failed to get info");
    ASSERT_OKAY(NXgetdata(fid, &(read[0])), "failed to get data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    // confirm
    TS_ASSERT_EQUALS(datatype, NXnumtype::CHAR);
    TS_ASSERT_EQUALS(rank, 1);
    TS_ASSERT_EQUALS(dimsout[0], 17);
    TS_ASSERT_EQUALS(std::string(read), "silicovolcaniosis");
    TS_ASSERT_EQUALS(std::string(read), std::string(word));
    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  // #################################################################################################################
  // TEST ADDRESS METHODS
  // #################################################################################################################

  void test_get_address_groups() {
    cout << "\ntest get address -- groups only\n";
    FileResource resource("test_napi_file_grp.h5");
    std::string filename = resource.fullPath();
    std::string address;

    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");

    // at root, path should be "/"
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get root address");
    TS_ASSERT_EQUALS("/", address);

    // make and open a group -- now at "/abc"
    ASSERT_OKAY(NXmakegroup(fid, "abc", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "abc", "NXentry"), "failed to open group");
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get address /abc");
    TS_ASSERT_EQUALS("/abc", address);

    // make another layer -- at "/acb/def"
    ASSERT_OKAY(NXmakegroup(fid, "def", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "def", "NXentry"), "failed to open group");
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get address /abc/def");
    TS_ASSERT_EQUALS("/abc/def", address);

    // go down a step -- back to "/abc"
    ASSERT_OKAY(NXclosegroup(fid), "failed to open group");
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get address /abc");
    TS_ASSERT_EQUALS("/abc", address);

    // go up a different step -- at "/abc/ghi"
    ASSERT_OKAY(NXmakegroup(fid, "ghi", "NXsample"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "ghi", "NXsample"), "failed to open group");
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get address /abc/ghi");
    TS_ASSERT_EQUALS("/abc/ghi", address);

    // make a group with same name at this level -- what happens?
    ASSERT_OKAY(NXmakegroup(fid, "ghi", "NXsample"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "ghi", "NXsample"), "failed to open group");
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get address /abc/ghi/ghi");
    char actual_address[128];
    H5Iget_name(fid->iCurrentG, actual_address, 128);
    TS_ASSERT_EQUALS("/abc/ghi/ghi", std::string(actual_address));
    TS_ASSERT_EQUALS("/abc/ghi/ghi", address);

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_get_address_data() {
    cout << "\ntest get address -- groups and data!\n";
    FileResource resource("test_napi_file_grpdata.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");

    // make and open a group -- now at "/abc"
    std::string address;
    ASSERT_OKAY(NXmakegroup(fid, "abc", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "abc", "NXentry"), "failed to open group");
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get address /abc");
    TS_ASSERT_EQUALS("/abc", address);

    // make another layer -- at "/abc/def"
    DimVector dims{1};
    ASSERT_OKAY(NXmakedata64(fid, "def", NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "def"), "failed to open data");
    int in = 17;
    ASSERT_OKAY(NXputdata(fid, &in), "failed to put data");
    ASSERT_OKAY(NXgetaddress(fid, address), "could not get address /abc/def");
    TS_ASSERT_EQUALS("/abc/def", address);

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_open_address() {
    cout << "tests for open address\n";

    // make file with path /entry
    FileResource resource("test_napi_openpathtest.nxs");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    DimVector dims{1};

    // make path /entry/data1
    char one = '1';
    ASSERT_OKAY(NXmakedata64(fid, "data1", NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "data1"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &one), "failed to put data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // make path /entry/data2
    char two = '2';
    ASSERT_OKAY(NXmakedata64(fid, "data2", NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "data2"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &two), "failed to put data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // make path /entry/data/more_data
    char three = '3';
    ASSERT_OKAY(NXmakegroup(fid, "data", "NXdata"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "data", "NXdata"), "failed to open group");
    ASSERT_OKAY(NXmakedata64(fid, "more_data", NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "more_data"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &three), "failed to put data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // make path /link
    char four = '4';
    ASSERT_OKAY(NXclosegroup(fid), "failed to close data"); // close /entry/data
    ASSERT_OKAY(NXclosegroup(fid), "failed to close data"); // close /entry
    ASSERT_OKAY(NXmakegroup(fid, "link", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "link", "NXentry"), "failed to open group"); // open /entry/link
    ASSERT_OKAY(NXmakedata64(fid, "data4", NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "data4"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &four), "failed to put data");
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // compare
    char output;
    ASSERT_OKAY(NXclosegroup(fid), "failed to close group");

    ASSERT_OKAY(NXopenaddress(fid, "/entry/data1"), "failed to open address");
    ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('1', output);

    ASSERT_OKAY(NXopenaddress(fid, "/link/data4"), "failed to open address");
    ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('4', output);

    ASSERT_OKAY(NXopenaddress(fid, "/entry/data/more_data"), "failed to open address");
    ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('3', output);

    ASSERT_OKAY(NXopenaddress(fid, "/entry/data2"), "failed to open address");
    ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('2', output);

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
    cout << "NXopenaddress checks OK\n";
  }

  void test_get_info() {
    cout << "\ntest get info -- good\n";

    // open a file
    FileResource resource("test_napi_file_dataRW.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    cout << "opened group\n";
    fflush(stdout);

    // put an integer
    int in = 17;
    DimVector dims{1};
    // NOTE the type of `int` is platform-dependent and may be int32_t or int64_t
    ASSERT_OKAY(NXmakedata64(fid, "int_data", getType<int>(), 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "int_data"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &in), "failed to put data");

    cout << "made and put data\n";
    fflush(stdout);

    // get the info and check
    int rank;
    DimVector dimsout{0};
    NXnumtype datatype = getType<int>();
    ASSERT_OKAY(NXgetinfo64(fid, &rank, dimsout.data(), &datatype), "failed to get info");
    cout << "info got\n";
    fflush(stdout);
    TS_ASSERT_EQUALS(datatype, getType<int>());
    TS_ASSERT_EQUALS(rank, 1);
    TS_ASSERT_EQUALS(dimsout[0], 1);
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // put a double
    double ind = 107.2345;
    ASSERT_OKAY(NXmakedata64(fid, "double_data", NXnumtype::FLOAT64, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "double_data"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &ind), "failed to put data");

    cout << "made and put double data\n";
    fflush(stdout);

    // get the info and check
    datatype = NXnumtype::FLOAT64;
    ASSERT_OKAY(NXgetinfo64(fid, &rank, dimsout.data(), &datatype), "failed to get info");
    cout << "info got\n";
    fflush(stdout);
    TS_ASSERT_EQUALS(datatype, NXnumtype::FLOAT64);
    TS_ASSERT_EQUALS(rank, 1);
    TS_ASSERT_EQUALS(dimsout[0], 1);
  }

  void test_get_info_bad() {
    cout << "\ntest get info -- bad\n";
    // open a file
    FileResource resource("test_napi_file_dataRW.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put an integer
    int in = 17;
    DimVector dims{1};
    ASSERT_OKAY(NXmakedata64(fid, "int_data", getType<int>(), 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "int_data"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, &in), "failed to put data");

    // open a group and try to get info
    int rank;
    DimVector dimsout;
    NXnumtype datatype;
    ASSERT_OKAY(NXmakegroup(fid, "a_group", "NXshorts"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "a_group", "NXshorts"), "failed to open group");
    ASSERT_ERROR(NXgetinfo64(fid, &rank, dimsout.data(), &datatype), "trying to get info in group should give error");
  }

  // ##################################################################################################################
  // TEST ATTRIBUTE METHODS
  // ################################################################################################################

  template <typename T> void do_test_putget_attr(NexusFile5 *fid, string name, T const &data) {
    // test put/get by pointer to data
    T out;
    int len;
    NXnumtype datatype = getType<T>();
    ASSERT_OKAY(NXputattr(fid, name.c_str(), &data, 1, getType<T>()), "failed to put attr");
    ASSERT_OKAY(NXgetattr(fid, name.c_str(), &out, &len, &datatype), "failed to get attribute");
    TS_ASSERT_EQUALS(data, out);
    TS_ASSERT_EQUALS(len, 1);
    TS_ASSERT_EQUALS(datatype, getType<T>());
  }

  void test_putget_attr_basic() {
    cout << "\ntest attribute read/write\n";

    // open a file
    FileResource resource("test_napi_attr.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    // move to an entry to avoid conflict with some root-level attributes
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    std::vector<std::string> expected_names{"int_attr_", "dbl_attr_"};

    // put/get an int attribute
    do_test_putget_attr<int32_t>(fid, expected_names[0], 12);

    // put/get a double attribute
    do_test_putget_attr(fid, expected_names[1], 120.2e6);

    // check attr infos
    int numattr;
    ASSERT_OKAY(NXgetattrinfo(fid, &numattr), "failed to get attr info");
    TS_ASSERT_EQUALS(numattr, 2);
    ASSERT_OKAY(NXinitattrdir(fid), "failed to restart attributes");
    char name[20] = {0};
    int len;
    int dims[] = {0, 0, 0, 0};
    NXnumtype datatype;
    for (int i = 0; i < numattr; i++) {
      ASSERT_OKAY(NXgetnextattra(fid, name, &len, dims, &datatype), "could not get next attribute");
      TS_ASSERT_EQUALS(name, expected_names[i]);
      TS_ASSERT_EQUALS(len, 1);
    }

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_putget_attr_str() {
    cout << "\ntest string attribute read/write\n";

    // open a file
    FileResource resource("test_napi_attr.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    // move to an entry to avoid conflict with some root-level attributes
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put/get a string attribute
    string data = "different string of text";
    ASSERT_OKAY(NXputattr(fid, "str_attr_", data.data(), static_cast<int>(data.size()), NXnumtype::CHAR),
                "failed to put attr");

    // NOTE we MUST pass the size of the string + 1 for thsi to work
    int len = static_cast<int>(data.size() + 1);
    // NOTE we MUST pass the correct variable type (rather than deducing it) for this to work
    NXnumtype datatype = NXnumtype::CHAR;

    // read into a low-level char array
    char cread[30] = {'A'}; // pre-fill with junk
    ASSERT_OKAY(NXgetattr(fid, "str_attr_", cread, &len, &datatype), "failed to get attribute");
    TS_ASSERT_EQUALS(data, cread);
    TS_ASSERT_EQUALS(len, data.size());
    TS_ASSERT_EQUALS(datatype, NXnumtype::CHAR);

    // read into a string through .data()
    // NOTE this requries that the string already be the correct size.
    // If it is too long, the string will contain junk data
    // If too short, the string will not contain all of the data
    string readme(30, 'A'); // pre-fill with junk
    ASSERT_OKAY(NXgetattr(fid, "str_attr_", readme.data(), &len, &datatype), "failed to get attribute");
    TS_ASSERT_DIFFERS(data, readme);
    readme.resize(len);
    // NOTE we must go to length - 1, because read attribute is WRONG
    // using the correct length inside napi will lead to errors elsewhere, which
    // expect the wrong value
    string expected(data.data(), data.size() - 1);
    TS_ASSERT_EQUALS(expected, readme);
    TS_ASSERT_EQUALS(len, data.size() - 1);
    TS_ASSERT_EQUALS(datatype, NXnumtype::CHAR);

    // cleanup
    ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  // ##################################################################################################################
  // TEST LINK METHODS
  // ################################################################################################################

  void test_links() {
    cout << "tests of linkature\n";

    FileResource resource("test_napi_link.nxs");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    ASSERT_OKAY(NXopen(filename.c_str(), NXACC_CREATE5, fid), "failed to open");
    ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // Create some data
    cout << "create entry at /entry/some_data\n";
    string const somedata("this is some data");
    DimVector dims{static_cast<dimsize_t>(somedata.size())};
    ASSERT_OKAY(NXmakedata64(fid, "some_data", NXnumtype::CHAR, 1, dims.data()), "failed to make data");
    ASSERT_OKAY(NXopendata(fid, "some_data"), "failed to open data");
    ASSERT_OKAY(NXputdata(fid, somedata.data()), "failed to put data");

    // create a link target
    NXlink datalink;
    ASSERT_OKAY(NXgetdataID(fid, &datalink), "failed to make link");
    TS_ASSERT_EQUALS(datalink.targetAddress, "/entry/some_data");
    TS_ASSERT_EQUALS(datalink.linkType, NXentrytype::sds);

    // close data
    ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // Create a group, and link it to that data
    cout << "create group at /entry/data to link to the data\n";
    ASSERT_OKAY(NXmakegroup(fid, "data", "NXdata"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "data", "NXdata"), "failed to open group");
    ASSERT_OKAY(NXmakelink(fid, &datalink), "failed to make link");
    ASSERT_OKAY(NXclosegroup(fid), "failed to close");

    // check data link
    ASSERT_OKAY(NXopenaddress(fid, "/entry/data/some_data"), "failed to open linked address");
    NXlink res1;
    ASSERT_OKAY(NXgetdataID(fid, &res1), "failed to get data ID from link");
    TS_ASSERT_EQUALS(datalink.linkType, res1.linkType);
    TS_ASSERT_EQUALS(datalink.targetAddress, res1.targetAddress);
    cout << "data link works\n";
    ASSERT_OKAY(NXclosedata(fid), "failed to close linked data");

    NXopenaddress(fid, "/entry");

    // Create two groups, group1 and group2
    // Make a link inside group2 to group1

    // make group1
    cout << "create group /entry/group1\n";
    ASSERT_OKAY(NXmakegroup(fid, "group1", "NXpants"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "group1", "NXpants"), "failed to open group");
    NXlink grouplink;
    ASSERT_OKAY(NXgetgroupID(fid, &grouplink), "failed to get group ID");
    TS_ASSERT_EQUALS(grouplink.targetAddress, "/entry/group1");
    TS_ASSERT_EQUALS(grouplink.linkType, NXentrytype::group);
    ASSERT_OKAY(NXclosegroup(fid), "failed to close group");

    // make group 2
    cout << "create group /entry/group2/group1\n";
    ASSERT_OKAY(NXmakegroup(fid, "group2", "NXshorts"), "failed to make group");
    ASSERT_OKAY(NXopengroup(fid, "group2", "NXshorts"), "failed to open group");
    ASSERT_OKAY(NXmakelink(fid, &grouplink), "failed to make link");
    ASSERT_OKAY(NXclosegroup(fid), "failed to close");

    // check group link
    ASSERT_OKAY(NXopenaddress(fid, "/entry/group2/group1"), "failed to open linked address");
    NXlink res2;
    ASSERT_OKAY(NXgetgroupID(fid, &res2), "failed to get linked group ID");
    TS_ASSERT_EQUALS(grouplink.linkType, res2.linkType);
    TS_ASSERT_EQUALS(string(grouplink.targetAddress), string(res2.targetAddress));
    cout << "group link works\n";
  }
};
