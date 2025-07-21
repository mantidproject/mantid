// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidNexus/napi.h"
#include "test_helper.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <hdf5.h>
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

#define NX_ASSERT_OKAY(status, msg)                                                                                    \
  if ((status) != NXstatus::NX_OK) {                                                                                   \
    NXclose(fid);                                                                                                      \
    std::cerr << msg << std::flush;                                                                                    \
    TS_FAIL(msg);                                                                                                      \
  }

#define NX_ASSERT_ERROR(status, msg)                                                                                   \
  if ((status) != NXstatus::NX_ERROR) {                                                                                \
    NXclose(fid);                                                                                                      \
    std::cerr << msg << std::flush;                                                                                    \
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

  // all moved to NexusFileTest.h

  // #################################################################################################################
  // TEST MAKE / OPEN / CLOSE GROUP
  // #################################################################################################################

  // all moved to NexusFileTest.h

  // #################################################################################################################
  // TEST MAKE / OPEN / PUT / CLOSE DATASET
  // #################################################################################################################

  void test_data_putget_string() {
    cout << "\ntest dataset read/write -- string\n" << std::flush;

    // open a file
    FileResource resource("test_napi_file_stringrw.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    NX_ASSERT_OKAY(NXopen(filename.c_str(), NXaccess::CREATE5, fid), "failed to open");
    NX_ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    NX_ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put/get a string
    cout << "\nread/write string...\n";
    // NOTE: whitespace is not stripped, so `out` must have EXACTLY the same length a `in`
    string in("this is a string"), out(in.size(), 'X');
    string name("string_data_2");

    // NOTE: to properly set the DataSpace, should be `dims {in.size(), 1}` and use rank = 2
    // However, that seems to contradict notes inside napi5 about rank for string data
    // Using rank = 1 works, but the DataSpace will register size = 1
    DimVector dims{static_cast<dimsize_t>(in.size())};
    NX_ASSERT_OKAY(NXmakedata64(fid, name.c_str(), NXnumtype::CHAR, 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, name.c_str()), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, in.data()), "failed to put data");
    NX_ASSERT_OKAY(NXgetdata(fid, out.data()), "failed to get data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    TS_ASSERT_EQUALS(in, out);

    // cleanup
    NX_ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  void test_data_putget_array() {
    cout << "\ntest dataset read/write -- arrays\n" << std::flush;

    // open a file
    FileResource resource("test_napi_file_dataRW.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    NX_ASSERT_OKAY(NXopen(filename.c_str(), NXaccess::CREATE5, fid), "failed to open");
    NX_ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    NX_ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put/get an int
    int in[4] = {12, 7, 2, 3}, out[4];
    DimVector dims{4};
    NX_ASSERT_OKAY(NXmakedata64(fid, "data_int", getType<int>(), 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "data_int"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, in), "failed to put data");
    DimVector dimsout{0, 0, 0, 0};
    std::size_t rank;
    NXnumtype datatype;
    NX_ASSERT_OKAY(NXgetinfo64(fid, rank, dimsout, datatype), "failed to get info");
    NX_ASSERT_OKAY(NXgetdata(fid, &(out[0])), "failed to get data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");
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
    NX_ASSERT_OKAY(NXmakedata64(fid, "data_double", NXnumtype::FLOAT64, 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "data_double"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, ind), "failed to put data");
    NX_ASSERT_OKAY(NXgetinfo64(fid, rank, dimsout, datatype), "failed to get info");
    NX_ASSERT_OKAY(NXgetdata(fid, outd), "failed to get data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");
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
    NX_ASSERT_OKAY(NXmakedata64(fid, "data_double_2d", NXnumtype::FLOAT64, 2, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "data_double_2d"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, indd), "failed to put data");
    NX_ASSERT_OKAY(NXgetinfo64(fid, rank, dimsout, datatype), "failed to get info");
    NX_ASSERT_OKAY(NXgetdata(fid, outdd), "failed to get data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");
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
    NX_ASSERT_OKAY(NXmakedata64(fid, "data_char", NXnumtype::CHAR, 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "data_char"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, word), "failed to put data");
    NX_ASSERT_OKAY(NXgetinfo64(fid, rank, dimsout, datatype), "failed to get info");
    NX_ASSERT_OKAY(NXgetdata(fid, &(read[0])), "failed to get data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");
    // confirm
    TS_ASSERT_EQUALS(datatype, NXnumtype::CHAR);
    TS_ASSERT_EQUALS(rank, 1);
    TS_ASSERT_EQUALS(dimsout[0], 17);
    TS_ASSERT_EQUALS(std::string(read), "silicovolcaniosis");
    TS_ASSERT_EQUALS(std::string(read), std::string(word));
    // cleanup
    NX_ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  // #################################################################################################################
  // TEST ADDRESS METHODS
  // #################################################################################################################

  void test_open_address() {
    cout << "tests for open address\n" << std::flush;

    // make file with path /entry
    FileResource resource("test_napi_openpathtest.nxs");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    NX_ASSERT_OKAY(NXopen(filename.c_str(), NXaccess::CREATE5, fid), "failed to open");
    NX_ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    NX_ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    DimVector dims{1};

    // make path /entry/data1
    char one = '1';
    NX_ASSERT_OKAY(NXmakedata64(fid, "data1", NXnumtype::CHAR, 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "data1"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, &one), "failed to put data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // make path /entry/data2
    char two = '2';
    NX_ASSERT_OKAY(NXmakedata64(fid, "data2", NXnumtype::CHAR, 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "data2"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, &two), "failed to put data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // make path /entry/data/more_data
    char three = '3';
    NX_ASSERT_OKAY(NXmakegroup(fid, "data", "NXdata"), "failed to make group");
    NX_ASSERT_OKAY(NXopengroup(fid, "data", "NXdata"), "failed to open group");
    NX_ASSERT_OKAY(NXmakedata64(fid, "more_data", NXnumtype::CHAR, 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "more_data"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, &three), "failed to put data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // make path /link
    char four = '4';
    NX_ASSERT_OKAY(NXclosegroup(fid), "failed to close data"); // close /entry/data
    NX_ASSERT_OKAY(NXclosegroup(fid), "failed to close data"); // close /entry
    NX_ASSERT_OKAY(NXmakegroup(fid, "link", "NXentry"), "failed to make group");
    NX_ASSERT_OKAY(NXopengroup(fid, "link", "NXentry"), "failed to open group"); // open /entry/link
    NX_ASSERT_OKAY(NXmakedata64(fid, "data4", NXnumtype::CHAR, 1, dims), "failed to make data");
    NX_ASSERT_OKAY(NXopendata(fid, "data4"), "failed to open data");
    NX_ASSERT_OKAY(NXputdata(fid, &four), "failed to put data");
    NX_ASSERT_OKAY(NXclosedata(fid), "failed to close data");

    // compare
    char output;
    NX_ASSERT_OKAY(NXclosegroup(fid), "failed to close group");

    NX_ASSERT_OKAY(NXopenaddress(fid, "/entry/data1"), "failed to open address");
    NX_ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('1', output);

    NX_ASSERT_OKAY(NXopenaddress(fid, "/link/data4"), "failed to open address");
    NX_ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('4', output);

    NX_ASSERT_OKAY(NXopenaddress(fid, "/entry/data/more_data"), "failed to open address");
    NX_ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('3', output);

    NX_ASSERT_OKAY(NXopenaddress(fid, "/entry/data2"), "failed to open address");
    NX_ASSERT_OKAY(NXgetdata(fid, &output), "failed to get data by opening address");
    TS_ASSERT_EQUALS('2', output);

    // cleanup
    NX_ASSERT_OKAY(NXclose(fid), "failed to close");
    cout << "NXopenaddress checks OK\n";
  }

  // ##################################################################################################################
  // TEST ATTRIBUTE METHODS
  // ################################################################################################################

  void test_putget_attr_str() {
    cout << "\ntest string attribute read/write\n" << std::flush;

    // open a file
    FileResource resource("test_napi_attr.h5");
    std::string filename = resource.fullPath();
    NexusFile5 *fid;
    NX_ASSERT_OKAY(NXopen(filename.c_str(), NXaccess::CREATE5, fid), "failed to open");
    // move to an entry to avoid conflict with some root-level attributes
    NX_ASSERT_OKAY(NXmakegroup(fid, "entry", "NXentry"), "failed to make group");
    NX_ASSERT_OKAY(NXopengroup(fid, "entry", "NXentry"), "failed to open group");

    // put/get a string attribute
    string data = "different string of text";
    NX_ASSERT_OKAY(NXputattr(fid, "str_attr_", data.data(), static_cast<int>(data.size()), NXnumtype::CHAR),
                   "failed to put attr");

    // NOTE we MUST pass the size of the string + 1 for this to work
    std::size_t len = data.size() + 1;
    // NOTE we MUST pass the correct variable type (rather than deducing it) for this to work
    NXnumtype datatype = NXnumtype::CHAR;

    // read into a low-level char array
    char cread[30] = {'A'}; // pre-fill with junk
    NX_ASSERT_OKAY(NXgetattr(fid, "str_attr_", cread, len, datatype), "failed to get attribute");
    TS_ASSERT_EQUALS(data, cread);
    TS_ASSERT_EQUALS(len, data.size());
    TS_ASSERT_EQUALS(datatype, NXnumtype::CHAR);

    // read into a string through .data()
    // NOTE this requries that the string already be the correct size.
    // If it is too long, the string will contain junk data
    // If too short, the string will not contain all of the data
    string readme(30, 'A'); // pre-fill with junk
    NX_ASSERT_OKAY(NXgetattr(fid, "str_attr_", readme.data(), len, datatype), "failed to get attribute");
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
    NX_ASSERT_OKAY(NXclose(fid), "failed to close");
  }

  // ##################################################################################################################
  // TEST LINK METHODS
  // ################################################################################################################

  // all moved to NexusFileReadWriteTest.h
};
