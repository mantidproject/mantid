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
#include "MantidNexus/NeXusFile.hpp"
#include "MantidNexus/napi.h"
#include "napi_test_util.h"
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for copy and compare
#include <string>

static int testLoadPath();
static int testExternal(const std::string &progName);

using NexusNapiTest::print_data;
using NexusNapiTest::removeFile;
using NexusNapiTest::write_dmc01;
using NexusNapiTest::write_dmc02;

#define ASSERT_NO_ERROR(status, msg)                                                                                   \
  if ((status) != NXstatus::NX_OK)                                                                                     \
    ON_ERROR(msg);

namespace { // anonymous namespace
std::string relativePathOf(const std::string &filename) { return std::filesystem::path(filename).filename().string(); }
} // anonymous namespace

int main(int argc, char *argv[]) {
  std::cout << "determining file type" << std::endl;
  std::string nxFile;
  NXaccess_mode nx_creation_code;
  if (strstr(argv[0], "napi_test_hdf5") != NULL) {
    nx_creation_code = NXACC_CREATE5;
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
  int i, j, NXlen;
  float r;
  const unsigned char i1_array[4] = {1, 2, 3, 4};
  const short int i2_array[4] = {1000, 2000, 3000, 4000};
  const int i4_array[4] = {1000000, 2000000, 3000000, 4000000};
  float r4_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
  double r8_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
  int array_dims[2] = {5, 4};
  int unlimited_dims[1] = {NX_UNLIMITED};
  int chunk_size[2] = {5, 4};
  int slab_start[2], slab_size[2];
  char c1_array[5][4] = {
      {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
  NXhandle fileid;
  NXlink glink, dlink;
  int comp_array[100][20];
  const char *ch_test_data = "NeXus ><}&{'\\&\" Data";

  std::cout << "Creating \"" << nxFile << "\"" << std::endl;
  // create file
  ASSERT_NO_ERROR(NXopen(nxFile.c_str(), nx_creation_code, &fileid), "Failure in NXopen for " + nxFile);
  if (nx_creation_code == NXACC_CREATE5) {
    std::cout << "Trying to reopen the file handle" << std::endl;
    NXhandle clone_fileid;
    ASSERT_NO_ERROR(NXreopen(fileid, &clone_fileid), "Failed to NXreopen " + nxFile);
  }
  // open group entry
  ASSERT_NO_ERROR(NXmakegroup(fileid, "entry", "NXentry"), "NXmakegroup(fileid, \"entry\", \"NXentry\")");
  ASSERT_NO_ERROR(NXopengroup(fileid, "entry", "NXentry"), "NXopengroup(fileid, \"entry\", \"NXentry\")");
  ASSERT_NO_ERROR(NXputattr(fileid, "hugo", "namenlos", static_cast<int>(strlen("namenlos")), NXnumtype::CHAR),
                  "NXputattr(fileid, \"hugo\", \"namenlos\", strlen, NXnumtype::CHAR)");
  ASSERT_NO_ERROR(NXputattr(fileid, "cucumber", "passion", static_cast<int>(strlen("passion")), NXnumtype::CHAR),
                  "NXputattr(fileid, \"cucumber\", \"passion\", strlen, NXnumtype::CHAR)");
  NXlen = static_cast<int>(strlen(ch_test_data));
  ASSERT_NO_ERROR(NXmakedata(fileid, "ch_data", NXnumtype::CHAR, 1, &NXlen), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "ch_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, ch_test_data), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXmakedata(fileid, "c1_data", NXnumtype::CHAR, 2, array_dims), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "c1_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, c1_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXmakedata(fileid, "i1_data", NXnumtype::INT8, 1, &array_dims[1]), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "i1_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, i1_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXmakedata(fileid, "i2_data", NXnumtype::INT16, 1, &array_dims[1]), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "i2_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, i2_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXmakedata(fileid, "i4_data", NXnumtype::INT32, 1, &array_dims[1]), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "i4_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, i4_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXcompmakedata(fileid, "r4_data", NXnumtype::FLOAT32, 2, array_dims, NX_COMP_LZW, chunk_size), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r4_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, r4_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");

  // BEGIN DOUBLE SLAB
  ASSERT_NO_ERROR(NXmakedata(fileid, "r8_data", NXnumtype::FLOAT64, 2, array_dims), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r8_data"), "");
  slab_start[0] = 4;
  slab_start[1] = 0;
  slab_size[0] = 1;
  slab_size[1] = 4;
  // cppcheck-suppress cstyleCast
  ASSERT_NO_ERROR(NXputslab(fileid, (double *)r8_array + 16, slab_start, slab_size), "");
  slab_start[0] = 0;
  slab_start[1] = 0;
  slab_size[0] = 4;
  slab_size[1] = 4;
  ASSERT_NO_ERROR(NXputslab(fileid, r8_array, slab_start, slab_size), "");
  ASSERT_NO_ERROR(
      NXputattr(fileid, "ch_attribute", ch_test_data, static_cast<int>(strlen(ch_test_data)), NXnumtype::CHAR), "");
  i = 42;
  ASSERT_NO_ERROR(NXputattr(fileid, "i4_attribute", &i, 1, NXnumtype::INT32), "");
  r = static_cast<float>(3.14159265);
  ASSERT_NO_ERROR(NXputattr(fileid, "r4_attribute", &r, 1, NXnumtype::FLOAT32), "");
  ASSERT_NO_ERROR(NXgetdataID(fileid, &dlink), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  // END DOUBLE SLAB

  // BEGIN LINK TEST
  // open group entry/data
  ASSERT_NO_ERROR(NXmakegroup(fileid, "data", "NXdata"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "data", "NXdata"), "");
  ASSERT_NO_ERROR(NXmakelink(fileid, &dlink), "");
  int dims[2] = {100, 20};
  for (i = 0; i < 100; i++) {
    for (j = 0; j < 20; j++) {
      comp_array[i][j] = i;
    }
  }
  int cdims[2] = {20, 20};
  ASSERT_NO_ERROR(NXcompmakedata(fileid, "comp_data", NXnumtype::INT32, 2, dims, NX_COMP_LZW, cdims), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "comp_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, comp_array), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXflush(&fileid), "");
  ASSERT_NO_ERROR(NXmakedata(fileid, "flush_data", NXnumtype::INT32, 1, unlimited_dims), "");
  slab_size[0] = 1;
  for (i = 0; i < 7; i++) {
    slab_start[0] = i;
    ASSERT_NO_ERROR(NXopendata(fileid, "flush_data"), "");
    ASSERT_NO_ERROR(NXputslab(fileid, &i, slab_start, slab_size), "");
    ASSERT_NO_ERROR(NXflush(&fileid), "");
  }
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group entry/data
  // open group entry/sample
  ASSERT_NO_ERROR(NXmakegroup(fileid, "sample", "NXsample"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "sample", "NXsample"), "");
  NXlen = 12;
  ASSERT_NO_ERROR(NXmakedata(fileid, "ch_data", NXnumtype::CHAR, 1, &NXlen), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "ch_data"), "");
  ASSERT_NO_ERROR(NXputdata(fileid, "NeXus sample"), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXgetgroupID(fileid, &glink), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group entry/sample
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group entry
  // open group link
  ASSERT_NO_ERROR(NXmakegroup(fileid, "link", "NXentry"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "link", "NXentry"), "");
  ASSERT_NO_ERROR(NXmakelink(fileid, &glink), "");
  ASSERT_NO_ERROR(NXmakenamedlink(fileid, "renLinkGroup", &glink), "");
  ASSERT_NO_ERROR(NXmakenamedlink(fileid, "renLinkData", &dlink), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // close group link
  ASSERT_NO_ERROR(NXclose(&fileid), "");
  // close file
  //  END LINK TEST

  if ((argc >= 2) && !strcmp(argv[1], "-q")) {
    return TEST_SUCCEED; /* create only */
  }

  char name[NX_MAXNAMELEN], char_class[NX_MAXNAMELEN], char_buffer[128];
  char group_name[NX_MAXNAMELEN], class_name[NX_MAXNAMELEN];
  char path[512];

  // read test
  std::cout << "Read/Write to read \"" << nxFile << "\"" << std::endl;
  ASSERT_NO_ERROR(NXopen(nxFile.c_str(), NXACC_RDWR, &fileid), "Failed to open \"" << nxFile << "\" for read/write");
  char filename[256];
  ASSERT_NO_ERROR(NXinquirefile(fileid, filename, 256), "");
  std::cout << "NXinquirefile found: " << relativePathOf(filename) << std::endl;
  NXgetattrinfo(fileid, &i);
  if (i > 0) {
    std::cout << "Number of global attributes: " << i << std::endl;
  }
  NXnumtype NXtype;
  NXstatus entry_status, attr_status;
  int NXrank, NXdims[32];
  do {
    // cppcheck-suppress argumentSize
    attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
    if (attr_status == NXstatus::NX_ERROR)
      return TEST_FAILED;
    if (attr_status == NXstatus::NX_OK) {
      switch (NXtype) {
      case NXnumtype::CHAR:
        NXlen = sizeof(char_buffer);
        ASSERT_NO_ERROR(NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype), "");
        if (strcmp(name, "file_time") && strcmp(name, "HDF_version") && strcmp(name, "HDF5_Version") &&
            strcmp(name, "XML_version")) {
          printf("   %s = %s\n", name, char_buffer);
        }
        break;
      default:
        break;
      }
    }
  } while (attr_status == NXstatus::NX_OK);
  ASSERT_NO_ERROR(NXopengroup(fileid, "entry", "NXentry"), "");
  NXgetattrinfo(fileid, &i);
  std::cout << "Number of group attributes: " << i << std::endl;
  ASSERT_NO_ERROR(NXgetpath(fileid, path, 512), "");
  std::cout << "NXentry path " << path << std::endl;
  do {
    // cppcheck-suppress argumentSize
    attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
    if (attr_status == NXstatus::NX_ERROR)
      return TEST_FAILED;
    if (attr_status == NXstatus::NX_OK) {
      if (NXtype == NXnumtype::CHAR) {
        NXlen = sizeof(char_buffer);
        ASSERT_NO_ERROR(NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype), "");
        printf("   %s = %s\n", name, char_buffer);
      }
    }
  } while (attr_status == NXstatus::NX_OK);
  // cppcheck-suppress argumentSize
  ASSERT_NO_ERROR(NXgetgroupinfo(fileid, &i, group_name, class_name), "");
  std::cout << "Group: " << group_name << "(" << class_name << ") contains " << i << " items\n";
  do {
    // cppcheck-suppress argumentSize
    entry_status = NXgetnextentry(fileid, name, char_class, &NXtype);
    if (entry_status == NXstatus::NX_ERROR)
      return TEST_FAILED;
    if (strcmp(char_class, "SDS") != 0) {
      if (entry_status != NXstatus::NX_EOD) {
        printf("   Subgroup: %s(%s)\n", name, char_class);
        entry_status = NXstatus::NX_OK;
      }
    } else {
      void *data_buffer;
      if (entry_status == NXstatus::NX_OK) {
        ASSERT_NO_ERROR(NXopendata(fileid, name), "");
        ASSERT_NO_ERROR(NXgetpath(fileid, path, 512), "");
        printf("Data path %s\n", path);
        ASSERT_NO_ERROR(NXgetinfo(fileid, &NXrank, NXdims, &NXtype), "");
        printf("   %s(%d)", name, (int)NXtype);
        // cppcheck-suppress cstyleCast
        ASSERT_NO_ERROR(NXmalloc((void **)&data_buffer, NXrank, NXdims, NXtype), "");
        int n = 1;
        for (int k = 0; k < NXrank; k++) {
          n *= NXdims[k];
        }
        if (NXtype == NXnumtype::CHAR) {
          ASSERT_NO_ERROR(NXgetdata(fileid, data_buffer), "");
          print_data(" = ", std::cout, data_buffer, NXtype, n);
        } else if (NXtype != NXnumtype::FLOAT32 && NXtype != NXnumtype::FLOAT64) {
          ASSERT_NO_ERROR(NXgetdata(fileid, data_buffer), "");
          print_data(" = ", std::cout, data_buffer, NXtype, n);
        } else {
          slab_start[0] = 0;
          slab_start[1] = 0;
          slab_size[0] = 1;
          slab_size[1] = 4;
          ASSERT_NO_ERROR(NXgetslab(fileid, data_buffer, slab_start, slab_size), "");
          print_data("\n      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = TEST_FAILED;
          ASSERT_NO_ERROR(NXgetslab(fileid, data_buffer, slab_start, slab_size), "");
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 2;
          ASSERT_NO_ERROR(NXgetslab(fileid, data_buffer, slab_start, slab_size), "");
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 3;
          ASSERT_NO_ERROR(NXgetslab(fileid, data_buffer, slab_start, slab_size), "");
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 4;
          ASSERT_NO_ERROR(NXgetslab(fileid, data_buffer, slab_start, slab_size), "");
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          ASSERT_NO_ERROR(NXgetattrinfo(fileid, &i), "");
          if (i > 0) {
            printf("      Number of attributes : %d\n", i);
          }
          do {
            // cppcheck-suppress argumentSize
            attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
            if (attr_status == NXstatus::NX_ERROR)
              return TEST_FAILED;
            if (attr_status == NXstatus::NX_OK) {
              switch (NXtype) {
              case NXnumtype::INT32:
                NXlen = TEST_FAILED;
                ASSERT_NO_ERROR(NXgetattr(fileid, name, &i, &NXlen, &NXtype), "");
                printf("         %s : %d\n", name, i);
                break;
              case NXnumtype::FLOAT32:
                NXlen = TEST_FAILED;
                ASSERT_NO_ERROR(NXgetattr(fileid, name, &r, &NXlen, &NXtype), "");
                printf("         %s : %f\n", name, r);
                break;
              case NXnumtype::CHAR:
                NXlen = sizeof(char_buffer);
                ASSERT_NO_ERROR(NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype), "");
                printf("         %s : %s\n", name, char_buffer);
                break;
              default:
                continue;
              }
            }
          } while (attr_status == NXstatus::NX_OK);
        }
        ASSERT_NO_ERROR(NXclosedata(fileid), "");
        // cppcheck-suppress cstyleCast
        ASSERT_NO_ERROR(NXfree((void **)&data_buffer), "");
      }
    }
  } while (entry_status == NXstatus::NX_OK);
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  // check links
  std::cout << "check links\n";
  NXlink blink;
  ASSERT_NO_ERROR(NXopengroup(fileid, "entry", "NXentry"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "sample", "NXsample"), "");
  ASSERT_NO_ERROR(NXgetgroupID(fileid, &glink), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "data", "NXdata"), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r8_data"), "");
  ASSERT_NO_ERROR(NXgetdataID(fileid, &dlink), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  ASSERT_NO_ERROR(NXopendata(fileid, "r8_data"), "");
  ASSERT_NO_ERROR(NXgetdataID(fileid, &blink), "");
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  if (NXsameID(fileid, &dlink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (r8_data)\n"
              << "original data\n";
    NXIprintlink(fileid, &dlink);
    std::cout << "linked data\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");

  ASSERT_NO_ERROR(NXopengroup(fileid, "link", "NXentry"), "");
  ASSERT_NO_ERROR(NXopengroup(fileid, "sample", "NXsample"), "");
  ASSERT_NO_ERROR(NXgetpath(fileid, path, 512), "");
  std::cout << "Group path " << path << "\n";
  ASSERT_NO_ERROR(NXgetgroupID(fileid, &blink), "");
  if (NXsameID(fileid, &glink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (sample)\n"
              << "original group\n";
    NXIprintlink(fileid, &glink);
    std::cout << "linked group\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");

  std::cout << "renLinkGroup NXsample test\n";
  ASSERT_NO_ERROR(NXopengroup(fileid, "renLinkGroup", "NXsample"), "");
  if (NXgetgroupID(fileid, &blink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &glink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (renLinkGroup)\n"
              << "original group\n";
    NXIprintlink(fileid, &glink);
    std::cout << "linked group\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");

  std::cout << "renLinkData test\n";
  ASSERT_NO_ERROR(NXopendata(fileid, "renLinkData"), "");
  ASSERT_NO_ERROR(NXgetdataID(fileid, &blink), "");
  if (NXsameID(fileid, &dlink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (renLinkData)\n"
              << "original group\n";
    NXIprintlink(fileid, &glink);
    std::cout << "linked group\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  ASSERT_NO_ERROR(NXclosedata(fileid), "");
  ASSERT_NO_ERROR(NXclosegroup(fileid), "");
  std::cout << "Link check OK\n";

  // tests for NXopenpath
  std::cout << "tests for NXopenpath\n";
  ASSERT_NO_ERROR(NXopenpath(fileid, "/entry/data/comp_data"), "Failure on NXopenpath\n");
  ASSERT_NO_ERROR(NXopenpath(fileid, "/entry/data/comp_data"), "Failure on NXopenpath\n");
  ASSERT_NO_ERROR(NXopenpath(fileid, "../r8_data"), "Failure on NXopenpath\n");
  ASSERT_NO_ERROR(NXopengrouppath(fileid, "/entry/data/comp_data"), "Failure on NXopengrouppath\n");
  ASSERT_NO_ERROR(NXopenpath(fileid, "/entry/data/r8_data"), "Failure on NXopenpath\n");
  std::cout << "NXopenpath checks OK\n";

  ASSERT_NO_ERROR(NXclose(&fileid), "");
#endif // WIN32

  std::cout << "before load path tests\n";
  if (testLoadPath() != TEST_SUCCEED)
    return TEST_FAILED;

  std::cout << "before external link tests\n";
  if (testExternal(argv[0]) != TEST_SUCCEED)
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
    if (NXopen("dmc01.hdf", NXACC_RDWR, &h) != NXstatus::NX_OK) {
      std::cout << "Loading NeXus file dmc01.hdf from path " << getenv("NX_LOAD_PATH") << " FAILED\n";
      return TEST_FAILED;
    } else {
      std::cout << "Success loading NeXus file from path\n";
      NXclose(&h);
      return TEST_SUCCEED;
    }
  } else {
    std::cout << "NX_LOAD_PATH is not defined\n";
  }
  return TEST_SUCCEED;
}

/*---------------------------------------------------------------------*/
static int testExternal(const std::string &progName) {
#ifdef WIN32 // these have issues on windows
  UNUSED_ARG(progName);
  return TEST_SUCCEED;
#else
  const std::string PROTOCOL("nxfile://");
  int dummylen = 1;
  float dummyfloat = 1;
  float temperature;

  if (strstr(progName.c_str(), "hdf4") != NULL) {
    std::cout << "Skipping external linking in hdf4\n";
    return TEST_SUCCEED;
  } else if (strstr(progName.c_str(), "hdf5") != NULL) {
    // this only works for hdf5 backed files
  } else if (strstr(progName.c_str(), "xml") != NULL) {
    std::cout << "XML backend is not supported\n";
    return TEST_FAILED;
  } else {
    std::cout << "Failed to recognise napi_test program in testExternal\n";
    return TEST_FAILED;
  }
  const NXaccess_mode create(NXACC_CREATE5);
  const std::string ext("h5");

  // create external files
  const std::string extFile1 = "dmc01c." + ext;
  removeFile(extFile1); // in case it was left over from previous run
  write_dmc01(extFile1);
  if (!std::filesystem::exists(extFile1)) {
    std::cerr << "Cannot find \"" << extFile1 << "\" to use for external linking\n";
    return TEST_FAILED;
  }
  const std::string extFile2 = "dmc02c." + ext;
  removeFile(extFile2); // in case it was left over from previous run
  write_dmc02(extFile2);
  if (!std::filesystem::exists(extFile2)) {
    std::cerr << "Cannot find \"" << extFile2 << "\" to use for external linking\n";
    return TEST_FAILED;
  }
  std::cout << "using external files: \"" << extFile1 << "\" and \"" << extFile2 << "\"\n";

  const std::string testFile = "nxexternal." + ext;
  std::cout << "Creating testfile \"" << testFile << "\"\n";
  removeFile(testFile); // in case previous run didn't clean up

  // create the test file
  NXhandle hfil;
  ASSERT_NO_ERROR(NXopen(testFile.c_str(), create, &hfil), "Failed to open \"" + testFile + "\" for writing");
  /*if(NXmakegroup(hfil,"entry1","NXentry") != NXstatus::NX_OK){
    return TEST_FAILED;
  }*/
  const std::string extFile1EntryPath = PROTOCOL + extFile1 + "#/entry1";
  ASSERT_NO_ERROR(NXlinkexternal(hfil, "entry1", "NXentry", extFile1EntryPath.c_str()),
                  "Failed to NXlinkexternal(hfil, \"entry1\", \"NXentry\", \"" + extFile1EntryPath + "\")");
  /*if(NXmakegroup(hfil,"entry2","NXentry") != NXstatus::NX_OK){
    return TEST_FAILED;
  }*/
  const std::string extFile2EntryPath = PROTOCOL + extFile2 + "#/entry1";
  ASSERT_NO_ERROR(NXlinkexternal(hfil, "entry2", "NXentry", extFile2EntryPath.c_str()),
                  "Failed to NXlinkexternal(hfil, \"entry2\", \"NXentry\", \"" + extFile2EntryPath + "\")");
  ASSERT_NO_ERROR(NXmakegroup(hfil, "entry3", "NXentry"), "");
  ASSERT_NO_ERROR(NXopengroup(hfil, "entry3", "NXentry"), "");
  /* force create old style external link */
  ASSERT_NO_ERROR(NXmakedata(hfil, "extlinkdata", NXnumtype::FLOAT32, 1, &dummylen), "");
  ASSERT_NO_ERROR(NXopendata(hfil, "extlinkdata"), "");
  ASSERT_NO_ERROR(NXputdata(hfil, &dummyfloat), "");
  std::string temperaturePath(PROTOCOL + extFile1 + "#/entry1/sample/temperature_mean");
  if (NXputattr(hfil, "napimount", temperaturePath.c_str(), static_cast<int>(strlen(temperaturePath.c_str())),
                NXnumtype::CHAR) != NXstatus::NX_OK)
    return TEST_FAILED;
  /* this would segfault because we are tricking the napi stack
  if(NXclosedata(&hfil) != NXstatus::NX_OK){
    return TEST_FAILED;
  }
  */
  ASSERT_NO_ERROR(NXopenpath(hfil, "/entry3"), "Failed to NXopenpath(hfil, \"/entry3\") during write");
  /* create new style external link on hdf5 , equivalent to the above on other backends */
  ASSERT_NO_ERROR(NXlinkexternaldataset(hfil, "extlinknative", temperaturePath.c_str()),
                  "Failed to NXlinkexternaldataset(hfil, \"extlinknative\", \"" + temperaturePath + "\")");

  ASSERT_NO_ERROR(NXclose(&hfil), "");

  // actually test linking
  ASSERT_NO_ERROR(NXopen(testFile.c_str(), NXACC_RDWR, &hfil), "Failed to open \"" + testFile + "\" for read/write");
  ASSERT_NO_ERROR(NXopenpath(hfil, "/entry1/start_time"), "");
  char time[132];
  memset(time, 0, 132);
  ASSERT_NO_ERROR(NXgetdata(hfil, time), "");
  printf("First file time: %s\n", time);

  char filename[256];
  ASSERT_NO_ERROR(NXinquirefile(hfil, filename, 256), "");
  std::cout << "NXinquirefile found: " << relativePathOf(filename) << "\n";

  ASSERT_NO_ERROR(NXopenpath(hfil, "/entry2/sample/sample_name"), "");
  memset(time, 0, 132);
  ASSERT_NO_ERROR(NXgetdata(hfil, time), "");
  printf("Second file sample: %s\n", time);
  ASSERT_NO_ERROR(NXinquirefile(hfil, filename, 256), "");
  std::cout << "NXinquirefile found: " << relativePathOf(filename) << "\n";

  ASSERT_NO_ERROR(NXopenpath(hfil, "/entry2/start_time"), "");
  memset(time, 0, 132);
  ASSERT_NO_ERROR(NXgetdata(hfil, time), "");
  printf("Second file time: %s\n", time);
  NXopenpath(hfil, "/");
  if (NXisexternalgroup(hfil, "entry1", "NXentry", filename, 255) != NXstatus::NX_OK) {
    return TEST_FAILED;
  } else {
    printf("entry1 external URL = %s\n", filename);
  }
  printf("testing link to external data set\n");
  if (NXopenpath(hfil, "/entry3") != NXstatus::NX_OK) {
    std::cerr << "failed to step into external file in \"/entry3\"\n";
    return TEST_FAILED;
  }
  if (NXisexternaldataset(hfil, "extlinkdata", filename, 255) != NXstatus::NX_OK) {
    printf("extlinkdata should be external link\n");
    return TEST_FAILED;
  } else {
    printf("extlinkdata external URL = %s\n", filename);
  }
  ASSERT_NO_ERROR(NXopendata(hfil, "extlinkdata"), "");
  memset(&temperature, 0, 4);
  ASSERT_NO_ERROR(NXgetdata(hfil, &temperature), "");
  printf("value retrieved: %4.2f\n", temperature);

  ASSERT_NO_ERROR(NXopenpath(hfil, "/entry3"), "");
  if (NXisexternaldataset(hfil, "extlinknative", filename, 255) != NXstatus::NX_OK) {
    ON_ERROR("extlinknative should be external link");
  } else {
    printf("extlinknative external URL = %s\n", filename);
  }
  ASSERT_NO_ERROR(NXopendata(hfil, "extlinknative"), "");
  memset(&temperature, 0, 4);
  ASSERT_NO_ERROR(NXgetdata(hfil, &temperature), "");
  printf("value retrieved: %4.2f\n", temperature);
  NXclose(&hfil);
  printf("External File Linking tested OK\n");

  // remove file that was created
  removeFile(testFile);
  removeFile(extFile1);
  removeFile(extFile2);

  return TEST_SUCCEED;
#endif
}
