/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format

  Test program for C API

  Copyright (C) 1997-2011 Freddie Akeroyd

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  For further information, see <http://www.nexusformat.org>

  $Id$

----------------------------------------------------------------------------*/

#include "MantidNexus/NexusFile.h"
#include "MantidNexus/napi.h"
#include "napi_test_util.h"
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for copy and compare
#include <string>

static int testLoadPath();

using NexusNapiTest::print_data;
using NexusNapiTest::removeFile;
using NexusNapiTest::write_dmc01;
using NexusNapiTest::write_dmc02;

#define ASSERT_NO_ERROR(status, msg)                                                                                   \
  if ((status) != NXstatus::NX_OK)                                                                                     \
    ON_ERROR(msg);

int main(int argc, char *argv[]) {
  std::cout << "determining file type" << std::endl;
  std::string nxFile;
  NXaccess nx_creation_code;
  if (strstr(argv[0], "napi_test_hdf5") != NULL) {
    nx_creation_code = NXaccess::CREATE5;
    nxFile = "NXtest.h5";
  } else {
    ON_ERROR(std::string(argv[0]) + " is not supported");
  }
  removeFile(nxFile); // in case previous run didn't clean up

#ifdef WIN32 // these have issues on windows
  UNUSED_ARG(nx_creation_code);
  UNUSED_ARG(argc);
#else  // WIN32
  // ------------------------------------------> TODO fine up to here "nexuscpptest-c-hdf5-test"
  float r;
  const unsigned char i1_array[4] = {1, 2, 3, 4};
  const short int i2_array[4] = {1000, 2000, 3000, 4000};
  const int i4_array[4] = {1000000, 2000000, 3000000, 4000000};
  float r4_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
  double r8_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
  Mantid::Nexus::DimVector array_dims{5, 4}, chunk_size{5, 4};
  Mantid::Nexus::DimVector slab_start(2, 0), slab_size(2, 0);
  char c1_array[5][4] = {
      {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
  NXhandle fileid;
  NXlink glink, dlink;
  const char *ch_test_data = "NeXus ><}&{'\\&\" Data";

  std::cout << "Creating \"" << nxFile << "\"" << std::endl;
  // create file
  ASSERT_NO_ERROR(NXopen(nxFile, nx_creation_code, fileid), "Failure in NXopen for " + nxFile);
  // open group entry
  ASSERT_NO_ERROR(NXmakegroup(fileid, "entry", "NXentry"), "NXmakegroup(fileid, \"entry\", \"NXentry\")");
  ASSERT_NO_ERROR(NXopengroup(fileid, "entry", "NXentry"), "NXopengroup(fileid, \"entry\", \"NXentry\")");
  ASSERT_NO_ERROR(NXputattr(fileid, "hugo", "namenlos", static_cast<int>(strlen("namenlos")), NXnumtype::CHAR),
                  "NXputattr(fileid, \"hugo\", \"namenlos\", strlen, NXnumtype::CHAR)");
  ASSERT_NO_ERROR(NXputattr(fileid, "cucumber", "passion", static_cast<int>(strlen("passion")), NXnumtype::CHAR),
                  "NXputattr(fileid, \"cucumber\", \"passion\", strlen, NXnumtype::CHAR)");
  Mantid::Nexus::DimVector NXlen64{static_cast<int64_t>(strlen(ch_test_data))};
  ASSERT_NO_ERROR(NXcompmakedata64(fileid, "ch_data", NXnumtype::CHAR, 1, NXlen64, NXcompression::NONE, NXlen64), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "ch_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, ch_test_data), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXcompmakedata64(fileid, "c1_data", NXnumtype::CHAR, 2, array_dims, NXcompression::NONE, array_dims),
                  "");
  ASSERT_NO_ERROR(NXopendata(fileid, "c1_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, c1_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXcompmakedata64(fileid, "i1_data", NXnumtype::INT8, 1, array_dims, NXcompression::NONE, array_dims),
                  "");
  ASSERT_NO_ERROR(NXopendata(fileid, "i1_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, i1_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXcompmakedata64(fileid, "i2_data", NXnumtype::INT16, 1, array_dims, NXcompression::NONE, array_dims),
                  "");
  ASSERT_NO_ERROR(NXopendata(fileid, "i2_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, i2_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXcompmakedata64(fileid, "i4_data", NXnumtype::INT32, 1, array_dims, NXcompression::NONE, array_dims),
                  "");
  ASSERT_NO_ERROR(NXopendata(fileid, "i4_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, i4_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(
      NXcompmakedata64(fileid, "r4_data", NXnumtype::FLOAT32, 2, array_dims, NXcompression::LZW, chunk_size), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r4_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, r4_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");

  std::cout << "BEGIN DOUBLE SLAB\n";

  ASSERT_NO_ERROR(
      NXcompmakedata64(fileid, "r8_data", NXnumtype::FLOAT64, 2, array_dims, NXcompression::NONE, array_dims), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r8_data"), "");
  slab_start[0] = 4;
  slab_start[1] = 0;
  slab_size[0] = 1;
  slab_size[1] = 4;
  // cppcheck-suppress cstyleCast
  ASSERT_NO_ERROR(NXputslab64(fileid, (double *)r8_array + 16, slab_start, slab_size), "");
  slab_start[0] = 0;
  slab_start[1] = 0;
  slab_size[0] = 4;
  slab_size[1] = 4;
  ASSERT_NO_ERROR(NXputslab64(fileid, r8_array, slab_start, slab_size), "");
  ASSERT_NO_ERROR(
      NXputattr(fileid, "ch_attribute", ch_test_data, static_cast<int>(strlen(ch_test_data)), NXnumtype::CHAR), "");
  int intdata = 42;
  ASSERT_NO_ERROR(NXputattr(fileid, "i4_attribute", &intdata, 1, NXnumtype::INT32), "");
  r = static_cast<float>(3.14159265);
  ASSERT_NO_ERROR(NXputattr(fileid, "r4_attribute", &r, 1, NXnumtype::FLOAT32), "");
  ASSERT_NO_ERROR(NXgetdataID(fileid, dlink), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  // END DOUBLE SLAB

  std::cout << "BEGIN LINK TEST\n";

  // open group entry/data
  ASSERT_NO_ERROR(NXmakegroup(fileid, "data", "NXdata"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "data", "NXdata"), "");
  ASSERT_NO_ERROR(NXmakelink(fileid, dlink), "");
  Mantid::Nexus::DimVector dims{100, 20};
  int comp_array[100][20];
  for (int64_t i = 0; i < dims[0]; i++) {
    for (int64_t j = 0; j < dims[1]; j++) {
      comp_array[i][j] = static_cast<int>(i);
    }
  }
  Mantid::Nexus::DimVector cdims{20, 20};
  ASSERT_NO_ERROR(NXcompmakedata64(fileid, "comp_data", NXnumtype::INT32, 2, dims, NXcompression::LZW, cdims),
                  "NXcompmakedata64 comp_data");
  ASSERT_NO_ERROR(NXopendata(fileid, "comp_data"), "NXopendata comp_data");
  ASSERT_NO_ERROR(NXputdata(fileid, comp_array), "NXputdata comp_data");
  ASSERT_NO_ERROR(NXclosedata(fileid), "NXclosedata comp_data");
  ASSERT_NO_ERROR(NXflush(fileid), "NXflush comp_data");
  Mantid::Nexus::DimVector unlimited_dims{NX_UNLIMITED};
  // NXcompmakedata64 has a hard time with unlimited dimensions
  ASSERT_NO_ERROR(NXmakedata64(fileid, "flush_data", NXnumtype::INT32, 1, unlimited_dims), "NXmakedata64 flush_data");
  slab_size[0] = 1;
  for (int i = 0; i < 7; i++) {
    slab_start[0] = i;
    ASSERT_NO_ERROR(NXopendata(fileid, "flush_data"), "");
    ASSERT_NO_ERROR(NXputslab64(fileid, &i, slab_start, slab_size), "");
    ASSERT_NO_ERROR(NXflush(fileid), "");
  }
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group entry/data
  // open group entry/sample
  ASSERT_NO_ERROR(NXmakegroup(fileid, "sample", "NXsample"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "sample", "NXsample"), "");
  NXlen64 = {12};
  ASSERT_NO_ERROR(NXcompmakedata64(fileid, "ch_data", NXnumtype::CHAR, 1, NXlen64, NXcompression::NONE, NXlen64), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "ch_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, "NeXus sample"), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXgetgroupID(fileid, glink), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group entry/sample
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group entry
  // open group link
  ASSERT_NO_ERROR(NXmakegroup(fileid, "link", "NXentry"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "link", "NXentry"), "");
  ASSERT_NO_ERROR(NXmakelink(fileid, glink), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group link
  ASSERT_NO_ERROR(NXclose(fileid), "");
  // close file
  //  END LINK TEST

  if ((argc >= 2) && !strcmp(argv[1], "-q")) {
    return TEST_SUCCEED; /* create only */
  }

  std::string address;

  // read test
  std::cout << "Read/Write to read \"" << nxFile << "\"" << std::endl;
  ASSERT_NO_ERROR(NXopen(nxFile, NXaccess::RDWR, fileid), "Failed to open \"" << nxFile << "\" for read/write");
  NXnumtype NXtype;
  Mantid::Nexus::DimVector NXdims(32);
  // used to be test of NXgetnextattra here
  ASSERT_NO_ERROR(NXopengroup(fileid, "entry", "NXentry"), "");
  ASSERT_NO_ERROR(NXgetaddress(fileid, address), "");
  std::cout << "NXentry address " << address << std::endl << std::flush;

  // NOTE used to be test of NXgetnextattra here

  // NOTE used to be test of NXgetnextentry here

  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // check links
  std::cout << "check links\n";
  NXlink blink;
  ASSERT_NO_ERROR(NXopengroup(fileid, "entry", "NXentry"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "sample", "NXsample"), "");
  ASSERT_NO_ERROR(NXgetgroupID(fileid, glink), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "data", "NXdata"), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r8_data"), "");
  ASSERT_NO_ERROR(NXgetdataID(fileid, dlink), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r8_data"), "");
  ASSERT_NO_ERROR(NXgetdataID(fileid, blink), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  if (dlink.targetAddress != blink.targetAddress) {
    std::cout << "Link check FAILED (r8_data)\n";
    return TEST_FAILED;
  }
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");

  ASSERT_NO_ERROR(NXopengroup(fileid, "link", "NXentry"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "sample", "NXsample"), "");
  ASSERT_NO_ERROR(NXgetaddress(fileid, address), "");
  std::cout << "Group address " << address << "\n";
  ASSERT_NO_ERROR(NXgetgroupID(fileid, blink), "");
  if (glink.targetAddress != blink.targetAddress) {
    std::cout << "Link check FAILED (sample)\n";
    return TEST_FAILED;
  }
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");

  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  std::cout << "Link check OK\n";

  // tests for NXopenaddress
  std::cout << "tests for NXopenaddress\n" << std::flush;
  ASSERT_NO_ERROR(NXopenaddress(fileid, "/entry/data/comp_data"), "Failed to open /entry/data/comp_data 1st\n");
  ASSERT_NO_ERROR(NXopenaddress(fileid, "/entry/data/comp_data"), "Failed to open /entry/data/comp_data 2nd\n");
  ASSERT_NO_ERROR(NXopenaddress(fileid, "../r8_data"), "Failed to open ../r8_data\n");
  ASSERT_NO_ERROR(NXopengroupaddress(fileid, "/entry/data/comp_data"), "Failed to open /entry/data/comp_data group\n");
  ASSERT_NO_ERROR(NXopenaddress(fileid, "/entry/data/r8_data"), "Failed to open /entry/r8_data\n");
  std::cout << "NXopenaddress checks OK\n";

  ASSERT_NO_ERROR(NXclose(fileid), "");
#endif // WIN32

  std::cout << "before load path tests\n";
  if (testLoadPath() != TEST_SUCCEED)
    return TEST_FAILED;

  // cleanup and return
  std::cout << "all ok - done\n";
  removeFile(nxFile);
  return TEST_SUCCEED;
}

/*---------------------------------------------------------------------*/
int testLoadPath() {
  if (getenv("NX_LOAD_PATH") != NULL) {
    // TODO create file and cleanup
    // std::string filename("data/dmc01.h5");
    NXhandle h;
    if (NXopen("dmc01.hdf", NXaccess::RDWR, h) != NXstatus::NX_OK) {
      std::cout << "Loading NeXus file dmc01.hdf from path " << getenv("NX_LOAD_PATH") << " FAILED\n";
      return TEST_FAILED;
    } else {
      std::cout << "Success loading NeXus file from path\n";
      NXclose(h);
      return TEST_SUCCEED;
    }
  } else {
    std::cout << "NX_LOAD_PATH is not defined\n";
  }
  return TEST_SUCCEED;
}
