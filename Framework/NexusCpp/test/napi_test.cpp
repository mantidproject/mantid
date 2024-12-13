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
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for copy and compare
#include <string>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "MantidNexusCpp/napi.h"
#include "napi_test_util.h"

static int testLoadPath();
static int testExternal(const std::string &progName);

using NexusCppTest::print_data;
using NexusCppTest::removeFile;
using NexusCppTest::write_dmc01;
using NexusCppTest::write_dmc02;

namespace { // anonymous namespace
// return to system when any test failed
constexpr int TEST_FAILED{1};
// return to system when all tests succeed
constexpr int TEST_SUCCEED{0};

static const char *relativePathOf(const char *filename) {
  char cwd[1024];

  getcwd(cwd, sizeof(cwd));

  if (strncmp(filename, cwd, strlen(cwd)) == 0) {
    return filename + strlen(cwd) + 1;
  } else {
    return filename;
  }
}
} // anonymous namespace

int main(int argc, char *argv[]) {
  int i, j, k, n, NXtype, NXlen, entry_status, attr_status;
  int NXrank, NXdims[32];
  float r;
  void *data_buffer;
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
  char name[NX_MAXNAMELEN], char_class[NX_MAXNAMELEN], char_buffer[128];
  char group_name[NX_MAXNAMELEN], class_name[NX_MAXNAMELEN];
  char c1_array[5][4] = {
      {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
  int unlimited_cdims[2] = {NX_UNLIMITED, 4};
  NXhandle fileid;
  NXlink glink, dlink, blink;
  int comp_array[100][20];
  int dims[2];
  int cdims[2];
  int64_t grossezahl[4];
  const char *ch_test_data = "NeXus ><}&{'\\&\" Data";
  char path[512];

  grossezahl[0] = 12;
  grossezahl[2] = 23;
#if HAVE_LONG_LONG_INT
  grossezahl[1] = (int64_t)555555555555LL;
  grossezahl[3] = (int64_t)777777777777LL;
#else
  grossezahl[1] = (int64_t)555555555555;
  grossezahl[3] = (int64_t)777777777777;
#endif /* HAVE_LONG_LONG_INT */

  std::string nxFile;
  NXaccess_mode nx_creation_code;
  if (strstr(argv[0], "napi_test_hdf5") != NULL) {
    nx_creation_code = NXACC_CREATE5;
    nxFile = "NXtest.h5";
  } else if (strstr(argv[0], "napi_c_test") != NULL) {
    nx_creation_code = NXACC_CREATE5;
    nxFile = "NXtest.h5";
  } else if (strstr(argv[0], "napi_test-xml-table") != NULL) {
    return TEST_FAILED; // xml is not supported
  } else if (strstr(argv[0], "napi_test-xml") != NULL) {
    return TEST_FAILED; // xml is not supported
  } else {
    nx_creation_code = NXACC_CREATE4;
    nxFile = "NXtest.hdf";
  }
  removeFile(nxFile); // in case previous run didn't clean up

  std::cout << "Creating \"" << nxFile << "\"\n";
  // create file
  if (NXopen(nxFile.c_str(), nx_creation_code, &fileid) != NX_OK) {
    std::cerr << "Failed to NXopen(" << nxFile << ", " << nx_creation_code << ", fileid)\n";
    return TEST_FAILED;
  }
  if (nx_creation_code == NXACC_CREATE5) {
    NXhandle clone_fileid;
    if (NXreopen(fileid, &clone_fileid) != NX_OK) {
      std::cerr << "Failed to NXreopen " << nxFile << "\n";
      return TEST_FAILED;
    }
  }
  if (NXmakegroup(fileid, "entry", "NXentry") != NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
    return TEST_FAILED;
  if (NXputattr(fileid, "hugo", "namenlos", strlen("namenlos"), NX_CHAR) != NX_OK)
    return TEST_FAILED;
  if (NXputattr(fileid, "cucumber", "passion", strlen("passion"), NX_CHAR) != NX_OK)
    return TEST_FAILED;
  NXlen = static_cast<int>(strlen(ch_test_data));
  if (NXmakedata(fileid, "ch_data", NX_CHAR, 1, &NXlen) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "ch_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, ch_test_data) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "c1_data", NX_CHAR, 2, array_dims) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "c1_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, c1_array) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "i1_data", NX_INT8, 1, &array_dims[1]) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "i1_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, i1_array) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "i2_data", NX_INT16, 1, &array_dims[1]) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "i2_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, i2_array) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "i4_data", NX_INT32, 1, &array_dims[1]) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "i4_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, i4_array) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXcompmakedata(fileid, "r4_data", NX_FLOAT32, 2, array_dims, NX_COMP_LZW, chunk_size) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r4_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, r4_array) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "r8_data", NX_FLOAT64, 2, array_dims) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r8_data") != NX_OK)
    return TEST_FAILED;
  slab_start[0] = 4;
  slab_start[1] = 0;
  slab_size[0] = 1;
  slab_size[1] = 4;
  // cppcheck-suppress cstyleCast
  if (NXputslab(fileid, (double *)r8_array + 16, slab_start, slab_size) != NX_OK)
    return TEST_FAILED;
  slab_start[0] = 0;
  slab_start[1] = 0;
  slab_size[0] = 4;
  slab_size[1] = 4;
  if (NXputslab(fileid, r8_array, slab_start, slab_size) != NX_OK)
    return TEST_FAILED;
  if (NXputattr(fileid, "ch_attribute", ch_test_data, static_cast<int>(strlen(ch_test_data)), NX_CHAR) != NX_OK)
    return TEST_FAILED;
  i = 42;
  if (NXputattr(fileid, "i4_attribute", &i, 1, NX_INT32) != NX_OK)
    return TEST_FAILED;
  r = static_cast<float>(3.14159265);
  if (NXputattr(fileid, "r4_attribute", &r, 1, NX_FLOAT32) != NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &dlink) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  dims[0] = 4;
  if (nx_creation_code != NXACC_CREATE4) {
    if (NXmakedata(fileid, "grosse_zahl", NX_INT64, 1, dims) == NX_OK) {
      if (NXopendata(fileid, "grosse_zahl") != NX_OK)
        return TEST_FAILED;
      if (NXputdata(fileid, grossezahl) != NX_OK)
        return TEST_FAILED;
      if (NXclosedata(fileid) != NX_OK)
        return TEST_FAILED;
    }
  }
  if (NXmakegroup(fileid, "data", "NXdata") != NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "data", "NXdata") != NX_OK)
    return TEST_FAILED;
  if (NXmakelink(fileid, &dlink) != NX_OK)
    return TEST_FAILED;
  dims[0] = 100;
  dims[1] = 20;
  for (i = 0; i < 100; i++) {
    for (j = 0; j < 20; j++) {
      comp_array[i][j] = i;
    }
  }
  cdims[0] = 20;
  cdims[1] = 20;
  if (NXcompmakedata(fileid, "comp_data", NX_INT32, 2, dims, NX_COMP_LZW, cdims) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "comp_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, comp_array) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXflush(&fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "flush_data", NX_INT32, 1, unlimited_dims) != NX_OK)
    return TEST_FAILED;
  slab_size[0] = 1;
  for (i = 0; i < 7; i++) {
    slab_start[0] = i;
    if (NXopendata(fileid, "flush_data") != NX_OK)
      return TEST_FAILED;
    if (NXputslab(fileid, &i, slab_start, slab_size) != NX_OK)
      return TEST_FAILED;
    if (NXflush(&fileid) != NX_OK)
      return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakegroup(fileid, "sample", "NXsample") != NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "sample", "NXsample") != NX_OK)
    return TEST_FAILED;
  NXlen = 12;
  if (NXmakedata(fileid, "ch_data", NX_CHAR, 1, &NXlen) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "ch_data") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, "NeXus sample") != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXgetgroupID(fileid, &glink) != NX_OK)
    return TEST_FAILED;
  if ((nx_creation_code & NXACC_CREATEXML) == 0) {
    if (NXmakedata(fileid, "cdata_unlimited", NX_CHAR, 2, unlimited_cdims) != NX_OK)
      return TEST_FAILED;
    if (NXopendata(fileid, "cdata_unlimited") != NX_OK)
      return TEST_FAILED;
    slab_size[0] = 1;
    slab_size[1] = 4;
    slab_start[1] = 0;
    for (i = 0; i < 5; i++) {
      slab_start[0] = i;
      if (NXputslab(fileid, &(c1_array[i][0]), slab_start, slab_size) != NX_OK)
        return TEST_FAILED;
    }
    if (NXclosedata(fileid) != NX_OK)
      return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXmakegroup(fileid, "link", "NXentry") != NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "link", "NXentry") != NX_OK)
    return TEST_FAILED;
  if (NXmakelink(fileid, &glink) != NX_OK)
    return TEST_FAILED;
  if (NXmakenamedlink(fileid, "renLinkGroup", &glink) != NX_OK)
    return TEST_FAILED;
  if (NXmakenamedlink(fileid, "renLinkData", &dlink) != NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXclose(&fileid) != NX_OK)
    return TEST_FAILED;
  std::cout << "END OF WRITE\n"; // TODO REMOVE
  if ((argc >= 2) && !strcmp(argv[1], "-q")) {
    return TEST_SUCCEED; /* create only */
  }

  // read test
  std::cout << "Read/Write to read \"" << nxFile << "\"\n";
  if (NXopen(nxFile.c_str(), NXACC_RDWR, &fileid) != NX_OK) {
    std::cerr << "Failed to open \"" << nxFile << "\" for read/write\n";
    return TEST_FAILED;
  }
  char filename[256];
  if (NXinquirefile(fileid, filename, 256) != NX_OK) {
    return TEST_FAILED;
  }
  printf("NXinquirefile found: %s\n", relativePathOf(filename));
  NXgetattrinfo(fileid, &i);
  if (i > 0) {
    printf("Number of global attributes: %d\n", i);
  }
  do {
    // cppcheck-suppress argumentSize
    attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
    if (attr_status == NX_ERROR)
      return TEST_FAILED;
    if (attr_status == NX_OK) {
      switch (NXtype) {
      case NX_CHAR:
        NXlen = sizeof(char_buffer);
        if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK)
          return TEST_FAILED;
        if (strcmp(name, "file_time") && strcmp(name, "HDF_version") && strcmp(name, "HDF5_Version") &&
            strcmp(name, "XML_version")) {
          printf("   %s = %s\n", name, char_buffer);
        }
        break;
      }
    }
  } while (attr_status == NX_OK);
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
    return TEST_FAILED;
  NXgetattrinfo(fileid, &i);
  printf("Number of group attributes: %d\n", i);
  if (NXgetpath(fileid, path, 512) != NX_OK)
    return TEST_FAILED;
  printf("NXentry path %s\n", path);
  do {
    // cppcheck-suppress argumentSize
    attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
    if (attr_status == NX_ERROR)
      return TEST_FAILED;
    if (attr_status == NX_OK) {
      switch (NXtype) {
      case NX_CHAR:
        NXlen = sizeof(char_buffer);
        if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK)
          return TEST_FAILED;
        printf("   %s = %s\n", name, char_buffer);
      }
    }
  } while (attr_status == NX_OK);
  // cppcheck-suppress argumentSize
  if (NXgetgroupinfo(fileid, &i, group_name, class_name) != NX_OK)
    return TEST_FAILED;
  printf("Group: %s(%s) contains %d items\n", group_name, class_name, i);
  do {
    // cppcheck-suppress argumentSize
    entry_status = NXgetnextentry(fileid, name, char_class, &NXtype);
    if (entry_status == NX_ERROR)
      return TEST_FAILED;
    if (strcmp(char_class, "SDS") != 0) {
      if (entry_status != NX_EOD) {
        printf("   Subgroup: %s(%s)\n", name, char_class);
        entry_status = NX_OK;
      }
    } else {
      if (entry_status == NX_OK) {
        if (NXopendata(fileid, name) != NX_OK)
          return TEST_FAILED;
        if (NXgetpath(fileid, path, 512) != NX_OK)
          return TEST_FAILED;
        printf("Data path %s\n", path);
        if (NXgetinfo(fileid, &NXrank, NXdims, &NXtype) != NX_OK)
          return TEST_FAILED;
        printf("   %s(%d)", name, NXtype);
        // cppcheck-suppress cstyleCast
        if (NXmalloc((void **)&data_buffer, NXrank, NXdims, NXtype) != NX_OK)
          return TEST_FAILED;
        n = 1;
        for (k = 0; k < NXrank; k++) {
          n *= NXdims[k];
        }
        if (NXtype == NX_CHAR) {
          if (NXgetdata(fileid, data_buffer) != NX_OK)
            return TEST_FAILED;
          print_data(" = ", std::cout, data_buffer, NXtype, n);
        } else if (NXtype != NX_FLOAT32 && NXtype != NX_FLOAT64) {
          if (NXgetdata(fileid, data_buffer) != NX_OK)
            return TEST_FAILED;
          print_data(" = ", std::cout, data_buffer, NXtype, n);
        } else {
          slab_start[0] = 0;
          slab_start[1] = 0;
          slab_size[0] = 1;
          slab_size[1] = 4;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return TEST_FAILED;
          print_data("\n      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = TEST_FAILED;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 2;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 3;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 4;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          if (NXgetattrinfo(fileid, &i) != NX_OK)
            return TEST_FAILED;
          if (i > 0) {
            printf("      Number of attributes : %d\n", i);
          }
          do {
            // cppcheck-suppress argumentSize
            attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
            if (attr_status == NX_ERROR)
              return TEST_FAILED;
            if (attr_status == NX_OK) {
              switch (NXtype) {
              case NX_INT32:
                NXlen = TEST_FAILED;
                if (NXgetattr(fileid, name, &i, &NXlen, &NXtype) != NX_OK)
                  return TEST_FAILED;
                printf("         %s : %d\n", name, i);
                break;
              case NX_FLOAT32:
                NXlen = TEST_FAILED;
                if (NXgetattr(fileid, name, &r, &NXlen, &NXtype) != NX_OK)
                  return TEST_FAILED;
                printf("         %s : %f\n", name, r);
                break;
              case NX_CHAR:
                NXlen = sizeof(char_buffer);
                if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK)
                  return TEST_FAILED;
                printf("         %s : %s\n", name, char_buffer);
                break;
              }
            }
          } while (attr_status == NX_OK);
        }
        if (NXclosedata(fileid) != NX_OK)
          return TEST_FAILED;
        // cppcheck-suppress cstyleCast
        if (NXfree((void **)&data_buffer) != NX_OK)
          return TEST_FAILED;
      }
    }
  } while (entry_status == NX_OK);
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  /*
   * check links
   */
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "sample", "NXsample") != NX_OK)
    return TEST_FAILED;
  if (NXgetgroupID(fileid, &glink) != NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "data", "NXdata") != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r8_data") != NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &dlink) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r8_data") != NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &blink) != NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &dlink, &blink) != NX_OK) {
    printf("Link check FAILED (r8_data)\n");
    printf("original data\n");
    NXIprintlink(fileid, &dlink);
    printf("linked data\n");
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;

  if (NXopengroup(fileid, "link", "NXentry") != NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "sample", "NXsample") != NX_OK)
    return TEST_FAILED;
  if (NXgetpath(fileid, path, 512) != NX_OK)
    return TEST_FAILED;
  printf("Group path %s\n", path);
  if (NXgetgroupID(fileid, &blink) != NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &glink, &blink) != NX_OK) {
    printf("Link check FAILED (sample)\n");
    printf("original group\n");
    NXIprintlink(fileid, &glink);
    printf("linked group\n");
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;

  if (NXopengroup(fileid, "renLinkGroup", "NXsample") != NX_OK)
    return TEST_FAILED;
  if (NXgetgroupID(fileid, &blink) != NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &glink, &blink) != NX_OK) {
    printf("Link check FAILED (renLinkGroup)\n");
    printf("original group\n");
    NXIprintlink(fileid, &glink);
    printf("linked group\n");
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;

  if (NXopendata(fileid, "renLinkData") != NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &blink) != NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &dlink, &blink) != NX_OK) {
    printf("Link check FAILED (renLinkData)\n");
    printf("original group\n");
    NXIprintlink(fileid, &glink);
    printf("linked group\n");
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosedata(fileid) != NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NX_OK)
    return TEST_FAILED;
  printf("Link check OK\n");

  /*
    tests for NXopenpath
  */
  if (NXopenpath(fileid, "/entry/data/comp_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return TEST_FAILED;
  }
  if (NXopenpath(fileid, "/entry/data/comp_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return TEST_FAILED;
  }
  if (NXopenpath(fileid, "../r8_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return TEST_FAILED;
  }
  if (NXopengrouppath(fileid, "/entry/data/comp_data") != NX_OK) {
    printf("Failure on NXopengrouppath\n");
    return TEST_FAILED;
  }
  if (NXopenpath(fileid, "/entry/data/r8_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return TEST_FAILED;
  }
  printf("NXopenpath checks OK\n");

  if (NXclose(&fileid) != NX_OK)
    return TEST_FAILED;

  printf("before load path tests\n");
  if (testLoadPath() != TEST_SUCCEED)
    return TEST_FAILED;

  printf("before external link tests\n");
  if (testExternal(argv[0]) != TEST_SUCCEED) {
    return TEST_FAILED;
  }

  // cleanup and return
  printf("all ok - done\n");
  removeFile(nxFile);
  return TEST_SUCCEED;
}
/*---------------------------------------------------------------------*/
int testLoadPath() {
  NXhandle h;

  if (getenv("NX_LOAD_PATH") != NULL) {
    // TODO create file and cleanup
    // std::string filename("data/dmc01.h5");
    if (NXopen("dmc01.hdf", NXACC_RDWR, &h) != NX_OK) {
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
  if (NXopen(testFile.c_str(), create, &hfil) != NX_OK) {
    std::cerr << "Failed to open \"" << testFile << "\" for writing\n";
    return TEST_FAILED;
  }
  /*if(NXmakegroup(hfil,"entry1","NXentry") != NX_OK){
    return TEST_FAILED;
  }*/
  const std::string extFile1EntryPath = PROTOCOL + extFile1 + "#/entry1";
  if (NXlinkexternal(hfil, "entry1", "NXentry", extFile1EntryPath.c_str()) != NX_OK) {
    std::cerr << "Failed to NXlinkexternal(hfil, \"entry1\", \"NXentry\", \"" << extFile1EntryPath << "\")\n";
    return TEST_FAILED;
  }
  /*if(NXmakegroup(hfil,"entry2","NXentry") != NX_OK){
    return TEST_FAILED;
  }*/
  const std::string extFile2EntryPath = PROTOCOL + extFile2 + "#/entry1";
  if (NXlinkexternal(hfil, "entry2", "NXentry", extFile2EntryPath.c_str()) != NX_OK) {
    std::cerr << "Failed to NXlinkexternal(hfil, \"entry2\", \"NXentry\", \"" << extFile2EntryPath << "\")\n";
    return TEST_FAILED;
  }
  if (NXmakegroup(hfil, "entry3", "NXentry") != NX_OK) {
    return TEST_FAILED;
  }
  if (NXopengroup(hfil, "entry3", "NXentry") != NX_OK) {
    return TEST_FAILED;
  }
  /* force create old style external link */
  if (NXmakedata(hfil, "extlinkdata", NX_FLOAT32, 1, &dummylen) != NX_OK)
    return TEST_FAILED;
  if (NXopendata(hfil, "extlinkdata") != NX_OK)
    return TEST_FAILED;
  if (NXputdata(hfil, &dummyfloat) != NX_OK)
    return TEST_FAILED;
  std::string temperaturePath(PROTOCOL + extFile1 + "#/entry1/sample/temperature_mean");
  if (NXputattr(hfil, "napimount", temperaturePath.c_str(), static_cast<int>(strlen(temperaturePath.c_str())),
                NX_CHAR) != NX_OK)
    return TEST_FAILED;
  /* this would segfault because we are tricking the napi stack
  if(NXclosedata(&hfil) != NX_OK){
    return TEST_FAILED;
  }
  */
  if (NXopenpath(hfil, "/entry3") != NX_OK) {
    std::cerr << "Failed to NXopenpath(hfil, \"/entry3\") during write\n";
    return TEST_FAILED;
  }
  /* create new style external link on hdf5 , equivalent to the above on other backends */
  if (NXlinkexternaldataset(hfil, "extlinknative", temperaturePath.c_str()) != NX_OK) {
    std::cerr << "Failed to NXlinkexternaldataset(hfil, \"extlinknative\", \"" << temperaturePath << "\")\n";
    return TEST_FAILED;
  }

  if (NXclose(&hfil) != NX_OK) {
    return TEST_FAILED;
  }

  // actually test linking
  if (NXopen(testFile.c_str(), NXACC_RDWR, &hfil) != NX_OK) {
    std::cerr << "Failed to open \"" << testFile << "\" for read/write\n";
    return TEST_FAILED;
  }
  if (NXopenpath(hfil, "/entry1/start_time") != NX_OK) {
    return TEST_FAILED;
  }
  char time[132];
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NX_OK) {
    return TEST_FAILED;
  }
  printf("First file time: %s\n", time);

  char filename[256];
  if (NXinquirefile(hfil, filename, 256) != NX_OK) {
    return TEST_FAILED;
  }
  printf("NXinquirefile found: %s\n", relativePathOf(filename));

  if (NXopenpath(hfil, "/entry2/sample/sample_name") != NX_OK) {
    return TEST_FAILED;
  }
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NX_OK) {
    return TEST_FAILED;
  }
  printf("Second file sample: %s\n", time);
  if (NXinquirefile(hfil, filename, 256) != NX_OK) {
    return TEST_FAILED;
  }
  printf("NXinquirefile found: %s\n", relativePathOf(filename));

  if (NXopenpath(hfil, "/entry2/start_time") != NX_OK) {
    return TEST_FAILED;
  }
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NX_OK) {
    return TEST_FAILED;
  }
  printf("Second file time: %s\n", time);
  NXopenpath(hfil, "/");
  if (NXisexternalgroup(hfil, "entry1", "NXentry", filename, 255) != NX_OK) {
    return TEST_FAILED;
  } else {
    printf("entry1 external URL = %s\n", filename);
  }
  printf("testing link to external data set\n");
  if (NXopenpath(hfil, "/entry3") != NX_OK) {
    std::cerr << "failed to step into external file in \"/entry3\"\n";
    return TEST_FAILED;
  }
  if (NXisexternaldataset(hfil, "extlinkdata", filename, 255) != NX_OK) {
    printf("extlinkdata should be external link\n");
    return TEST_FAILED;
  } else {
    printf("extlinkdata external URL = %s\n", filename);
  }
  if (NXopendata(hfil, "extlinkdata") != NX_OK)
    return TEST_FAILED;
  memset(&temperature, 0, 4);
  if (NXgetdata(hfil, &temperature) != NX_OK) {
    return TEST_FAILED;
  }
  printf("value retrieved: %4.2f\n", temperature);

  if (NXopenpath(hfil, "/entry3") != NX_OK) {
    return TEST_FAILED;
  }
  if (NXisexternaldataset(hfil, "extlinknative", filename, 255) != NX_OK) {
    printf("extlinknative should be external link\n");
    return TEST_FAILED;
  } else {
    printf("extlinknative external URL = %s\n", filename);
  }
  if (NXopendata(hfil, "extlinknative") != NX_OK)
    return TEST_FAILED;
  memset(&temperature, 0, 4);
  if (NXgetdata(hfil, &temperature) != NX_OK) {
    return TEST_FAILED;
  }
  printf("value retrieved: %4.2f\n", temperature);

  NXclose(&hfil);
  printf("External File Linking tested OK\n");

  // remove file that was created
  removeFile(testFile);
  removeFile(extFile1);
  removeFile(extFile2);

  return TEST_SUCCEED;
}
