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
#include "MantidNexusCpp/NeXusFile_fwd.h"
#include "MantidNexusCpp/napi.h"
#include "napi_test_util.h"
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for copy and compare
#include <string>

static int testLoadPath();
static int testExternal(const std::string &progName);

using NexusCppTest::print_data;
using NexusCppTest::removeFile;
using NexusCppTest::write_dmc01;
using NexusCppTest::write_dmc02;

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
  } else if (strstr(argv[0], "napi_test_hdf4") != NULL) {
    nx_creation_code = NXACC_CREATE4;
    nxFile = "NXtest.hdf";
  } else {
    ON_ERROR(std::string(argv[0]) + " is not supported");
  }
  removeFile(nxFile); // in case previous run didn't clean up

#ifdef WIN32 // these have issues on windows
  UNUSED_ARG(nx_creation_code);
  UNUSED_ARG(argc);
#else // WIN32
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
  if (NXopen(nxFile.c_str(), nx_creation_code, &fileid) != NXstatus::NX_OK) {
    std::cerr << "Failed to NXopen(" << nxFile << ", " << nx_creation_code << ", fileid)\n";
    return TEST_FAILED;
  }
  if (nx_creation_code == NXACC_CREATE5) {
    std::cout << "Trying to reopen the file handle" << std::endl;
    NXhandle clone_fileid;
    if (NXreopen(fileid, &clone_fileid) != NXstatus::NX_OK) {
      std::cerr << "Failed to NXreopen " << nxFile << "\n";
      return TEST_FAILED;
    }
  }
  if (NXmakegroup(fileid, "entry", "NXentry") != NXstatus::NX_OK)
    ON_ERROR("NXmakegroup(fileid, \"entry\", \"NXentry\")");
  if (NXopengroup(fileid, "entry", "NXentry") != NXstatus::NX_OK)
    ON_ERROR("NXopengroup(fileid, \"entry\", \"NXentry\")");
  if (NXputattr(fileid, "hugo", "namenlos", static_cast<int>(strlen("namenlos")), NXnumtype::CHAR) != NXstatus::NX_OK)
    ON_ERROR("NXputattr(fileid, \"hugo\", \"namenlos\", strlen, NXnumtype::CHAR)");
  if (NXputattr(fileid, "cucumber", "passion", static_cast<int>(strlen("passion")), NXnumtype::CHAR) != NXstatus::NX_OK)
    ON_ERROR("NXputattr(fileid, \"cucumber\", \"passion\", strlen, NXnumtype::CHAR)");
  NXlen = static_cast<int>(strlen(ch_test_data));
  if (NXmakedata(fileid, "ch_data", NXnumtype::CHAR, 1, &NXlen) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "ch_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, ch_test_data) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "c1_data", NXnumtype::CHAR, 2, array_dims) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "c1_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, c1_array) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "i1_data", NXnumtype::INT8, 1, &array_dims[1]) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "i1_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, i1_array) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "i2_data", NXnumtype::INT16, 1, &array_dims[1]) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "i2_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, i2_array) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "i4_data", NXnumtype::INT32, 1, &array_dims[1]) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "i4_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, i4_array) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXcompmakedata(fileid, "r4_data", NXnumtype::FLOAT32, 2, array_dims, NX_COMP_LZW, chunk_size) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r4_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, r4_array) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "r8_data", NXnumtype::FLOAT64, 2, array_dims) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r8_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  slab_start[0] = 4;
  slab_start[1] = 0;
  slab_size[0] = 1;
  slab_size[1] = 4;
  // cppcheck-suppress cstyleCast
  if (NXputslab(fileid, (double *)r8_array + 16, slab_start, slab_size) != NXstatus::NX_OK)
    return TEST_FAILED;
  slab_start[0] = 0;
  slab_start[1] = 0;
  slab_size[0] = 4;
  slab_size[1] = 4;
  if (NXputslab(fileid, r8_array, slab_start, slab_size) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputattr(fileid, "ch_attribute", ch_test_data, static_cast<int>(strlen(ch_test_data)), NXnumtype::CHAR) !=
      NXstatus::NX_OK)
    return TEST_FAILED;
  i = 42;
  if (NXputattr(fileid, "i4_attribute", &i, 1, NXnumtype::INT32) != NXstatus::NX_OK)
    return TEST_FAILED;
  r = static_cast<float>(3.14159265);
  if (NXputattr(fileid, "r4_attribute", &r, 1, NXnumtype::FLOAT32) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &dlink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (nx_creation_code != NXACC_CREATE4) {
#if HAVE_LONG_LONG_INT
    const int64_t grossezahl[4] = {12, 555555555555LL, 23, 777777777777LL};
#else
    const int64_t grossezahl[4] = {12, 555555, 23, 77777};
#endif /* HAVE_LONG_LONG_INT */
    int dims[1] = {4};
    if (NXmakedata(fileid, "grosse_zahl", NXnumtype::INT64, 1, dims) == NXstatus::NX_OK) {
      if (NXopendata(fileid, "grosse_zahl") != NXstatus::NX_OK)
        return TEST_FAILED;
      if (NXputdata(fileid, grossezahl) != NXstatus::NX_OK)
        return TEST_FAILED;
      if (NXclosedata(fileid) != NXstatus::NX_OK)
        return TEST_FAILED;
    }
  }
  if (NXmakegroup(fileid, "data", "NXdata") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "data", "NXdata") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakelink(fileid, &dlink) != NXstatus::NX_OK)
    return TEST_FAILED;
  int dims[2] = {100, 20};
  for (i = 0; i < 100; i++) {
    for (j = 0; j < 20; j++) {
      comp_array[i][j] = i;
    }
  }
  int cdims[2] = {20, 20};
  if (NXcompmakedata(fileid, "comp_data", NXnumtype::INT32, 2, dims, NX_COMP_LZW, cdims) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "comp_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, comp_array) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXflush(&fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakedata(fileid, "flush_data", NXnumtype::INT32, 1, unlimited_dims) != NXstatus::NX_OK)
    return TEST_FAILED;
  slab_size[0] = 1;
  for (i = 0; i < 7; i++) {
    slab_start[0] = i;
    if (NXopendata(fileid, "flush_data") != NXstatus::NX_OK)
      return TEST_FAILED;
    if (NXputslab(fileid, &i, slab_start, slab_size) != NXstatus::NX_OK)
      return TEST_FAILED;
    if (NXflush(&fileid) != NXstatus::NX_OK)
      return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakegroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
    return TEST_FAILED;
  NXlen = 12;
  if (NXmakedata(fileid, "ch_data", NXnumtype::CHAR, 1, &NXlen) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "ch_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(fileid, "NeXus sample") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetgroupID(fileid, &glink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakegroup(fileid, "link", "NXentry") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "link", "NXentry") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakelink(fileid, &glink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakenamedlink(fileid, "renLinkGroup", &glink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXmakenamedlink(fileid, "renLinkData", &dlink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclose(&fileid) != NXstatus::NX_OK)
    return TEST_FAILED;

  if ((argc >= 2) && !strcmp(argv[1], "-q")) {
    return TEST_SUCCEED; /* create only */
  }

  char name[NX_MAXNAMELEN], char_class[NX_MAXNAMELEN], char_buffer[128];
  char group_name[NX_MAXNAMELEN], class_name[NX_MAXNAMELEN];
  char path[512];

  // read test
  std::cout << "Read/Write to read \"" << nxFile << "\"" << std::endl;
  if (NXopen(nxFile.c_str(), NXACC_RDWR, &fileid) != NXstatus::NX_OK) {
    std::cerr << "Failed to open \"" << nxFile << "\" for read/write" << std::endl;
    return TEST_FAILED;
  }
  char filename[256];
  if (NXinquirefile(fileid, filename, 256) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
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
        if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NXstatus::NX_OK)
          return TEST_FAILED;
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
  if (NXopengroup(fileid, "entry", "NXentry") != NXstatus::NX_OK)
    return TEST_FAILED;
  NXgetattrinfo(fileid, &i);
  std::cout << "Number of group attributes: " << i << std::endl;
  if (NXgetpath(fileid, path, 512) != NXstatus::NX_OK)
    return TEST_FAILED;
  std::cout << "NXentry path " << path << std::endl;
  do {
    // cppcheck-suppress argumentSize
    attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
    if (attr_status == NXstatus::NX_ERROR)
      return TEST_FAILED;
    if (attr_status == NXstatus::NX_OK) {
      if (NXtype == NXnumtype::CHAR) {
        NXlen = sizeof(char_buffer);
        if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NXstatus::NX_OK)
          return TEST_FAILED;
        printf("   %s = %s\n", name, char_buffer);
      }
    }
  } while (attr_status == NXstatus::NX_OK);
  // cppcheck-suppress argumentSize
  if (NXgetgroupinfo(fileid, &i, group_name, class_name) != NXstatus::NX_OK)
    return TEST_FAILED;
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
        if (NXopendata(fileid, name) != NXstatus::NX_OK)
          return TEST_FAILED;
        if (NXgetpath(fileid, path, 512) != NXstatus::NX_OK)
          return TEST_FAILED;
        printf("Data path %s\n", path);
        if (NXgetinfo(fileid, &NXrank, NXdims, &NXtype) != NXstatus::NX_OK)
          return TEST_FAILED;
        printf("   %s(%d)", name, (int)NXtype);
        // cppcheck-suppress cstyleCast
        if (NXmalloc((void **)&data_buffer, NXrank, NXdims, NXtype) != NXstatus::NX_OK)
          return TEST_FAILED;
        int n = 1;
        for (int k = 0; k < NXrank; k++) {
          n *= NXdims[k];
        }
        if (NXtype == NXnumtype::CHAR) {
          if (NXgetdata(fileid, data_buffer) != NXstatus::NX_OK)
            return TEST_FAILED;
          print_data(" = ", std::cout, data_buffer, NXtype, n);
        } else if (NXtype != NXnumtype::FLOAT32 && NXtype != NXnumtype::FLOAT64) {
          if (NXgetdata(fileid, data_buffer) != NXstatus::NX_OK)
            return TEST_FAILED;
          print_data(" = ", std::cout, data_buffer, NXtype, n);
        } else {
          slab_start[0] = 0;
          slab_start[1] = 0;
          slab_size[0] = 1;
          slab_size[1] = 4;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
            return TEST_FAILED;
          print_data("\n      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = TEST_FAILED;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 2;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 3;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          slab_start[0] = 4;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NXstatus::NX_OK)
            return TEST_FAILED;
          print_data("      ", std::cout, data_buffer, NXtype, 4);
          if (NXgetattrinfo(fileid, &i) != NXstatus::NX_OK)
            return TEST_FAILED;
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
                if (NXgetattr(fileid, name, &i, &NXlen, &NXtype) != NXstatus::NX_OK)
                  return TEST_FAILED;
                printf("         %s : %d\n", name, i);
                break;
              case NXnumtype::FLOAT32:
                NXlen = TEST_FAILED;
                if (NXgetattr(fileid, name, &r, &NXlen, &NXtype) != NXstatus::NX_OK)
                  return TEST_FAILED;
                printf("         %s : %f\n", name, r);
                break;
              case NXnumtype::CHAR:
                NXlen = sizeof(char_buffer);
                if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NXstatus::NX_OK)
                  return TEST_FAILED;
                printf("         %s : %s\n", name, char_buffer);
                break;
              default:
                continue;
              }
            }
          } while (attr_status == NXstatus::NX_OK);
        }
        if (NXclosedata(fileid) != NXstatus::NX_OK)
          return TEST_FAILED;
        // cppcheck-suppress cstyleCast
        if (NXfree((void **)&data_buffer) != NXstatus::NX_OK)
          return TEST_FAILED;
      }
    }
  } while (entry_status == NXstatus::NX_OK);
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  // check links
  std::cout << "check links\n";
  NXlink blink;
  if (NXopengroup(fileid, "entry", "NXentry") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetgroupID(fileid, &glink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "data", "NXdata") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r8_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &dlink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(fileid, "r8_data") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &blink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &dlink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (r8_data)\n" << "original data\n";
    NXIprintlink(fileid, &dlink);
    std::cout << "linked data\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;

  if (NXopengroup(fileid, "link", "NXentry") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopengroup(fileid, "sample", "NXsample") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetpath(fileid, path, 512) != NXstatus::NX_OK)
    return TEST_FAILED;
  std::cout << "Group path " << path << "\n";
  if (NXgetgroupID(fileid, &blink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &glink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (sample)\n" << "original group\n";
    NXIprintlink(fileid, &glink);
    std::cout << "linked group\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;

  std::cout << "renLinkGroup NXsample test\n";
  if (NXopengroup(fileid, "renLinkGroup", "NXsample") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetgroupID(fileid, &blink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &glink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (renLinkGroup)\n" << "original group\n";
    NXIprintlink(fileid, &glink);
    std::cout << "linked group\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;

  std::cout << "renLinkData test\n";
  if (NXopendata(fileid, "renLinkData") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXgetdataID(fileid, &blink) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXsameID(fileid, &dlink, &blink) != NXstatus::NX_OK) {
    std::cout << "Link check FAILED (renLinkData)\n" << "original group\n";
    NXIprintlink(fileid, &glink);
    std::cout << "linked group\n";
    NXIprintlink(fileid, &blink);
    return TEST_FAILED;
  }
  if (NXclosedata(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXclosegroup(fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
  std::cout << "Link check OK\n";

  // tests for NXopenpath
  std::cout << "tests for NXopenpath\n";
  if (NXopenpath(fileid, "/entry/data/comp_data") != NXstatus::NX_OK) {
    ON_ERROR("Failure on NXopenpath\n");
  }
  if (NXopenpath(fileid, "/entry/data/comp_data") != NXstatus::NX_OK) {
    ON_ERROR("Failure on NXopenpath\n");
  }
  if (NXopenpath(fileid, "../r8_data") != NXstatus::NX_OK) {
    ON_ERROR("Failure on NXopenpath\n");
  }
  if (NXopengrouppath(fileid, "/entry/data/comp_data") != NXstatus::NX_OK) {
    ON_ERROR("Failure on NXopengrouppath\n");
  }
  if (NXopenpath(fileid, "/entry/data/r8_data") != NXstatus::NX_OK) {
    ON_ERROR("Failure on NXopenpath\n");
  }
  std::cout << "NXopenpath checks OK\n";

  if (NXclose(&fileid) != NXstatus::NX_OK)
    return TEST_FAILED;
#endif // WIN32

  std::cout << "before load path tests\n";
  if (testLoadPath() != TEST_SUCCEED)
    return TEST_FAILED;

  std::cout << "before external link tests\n";
  if (testExternal(argv[0]) != TEST_SUCCEED) {
    return TEST_FAILED;
  }

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
  if (NXopen(testFile.c_str(), create, &hfil) != NXstatus::NX_OK) {
    std::cerr << "Failed to open \"" << testFile << "\" for writing\n";
    return TEST_FAILED;
  }
  /*if(NXmakegroup(hfil,"entry1","NXentry") != NXstatus::NX_OK){
    return TEST_FAILED;
  }*/
  const std::string extFile1EntryPath = PROTOCOL + extFile1 + "#/entry1";
  if (NXlinkexternal(hfil, "entry1", "NXentry", extFile1EntryPath.c_str()) != NXstatus::NX_OK) {
    std::cerr << "Failed to NXlinkexternal(hfil, \"entry1\", \"NXentry\", \"" << extFile1EntryPath << "\")\n";
    return TEST_FAILED;
  }
  /*if(NXmakegroup(hfil,"entry2","NXentry") != NXstatus::NX_OK){
    return TEST_FAILED;
  }*/
  const std::string extFile2EntryPath = PROTOCOL + extFile2 + "#/entry1";
  if (NXlinkexternal(hfil, "entry2", "NXentry", extFile2EntryPath.c_str()) != NXstatus::NX_OK) {
    std::cerr << "Failed to NXlinkexternal(hfil, \"entry2\", \"NXentry\", \"" << extFile2EntryPath << "\")\n";
    return TEST_FAILED;
  }
  if (NXmakegroup(hfil, "entry3", "NXentry") != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  if (NXopengroup(hfil, "entry3", "NXentry") != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  /* force create old style external link */
  if (NXmakedata(hfil, "extlinkdata", NXnumtype::FLOAT32, 1, &dummylen) != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXopendata(hfil, "extlinkdata") != NXstatus::NX_OK)
    return TEST_FAILED;
  if (NXputdata(hfil, &dummyfloat) != NXstatus::NX_OK)
    return TEST_FAILED;
  std::string temperaturePath(PROTOCOL + extFile1 + "#/entry1/sample/temperature_mean");
  if (NXputattr(hfil, "napimount", temperaturePath.c_str(), static_cast<int>(strlen(temperaturePath.c_str())),
                NXnumtype::CHAR) != NXstatus::NX_OK)
    return TEST_FAILED;
  /* this would segfault because we are tricking the napi stack
  if(NXclosedata(&hfil) != NXstatus::NX_OK){
    return TEST_FAILED;
  }
  */
  if (NXopenpath(hfil, "/entry3") != NXstatus::NX_OK) {
    std::cerr << "Failed to NXopenpath(hfil, \"/entry3\") during write\n";
    return TEST_FAILED;
  }
  /* create new style external link on hdf5 , equivalent to the above on other backends */
  if (NXlinkexternaldataset(hfil, "extlinknative", temperaturePath.c_str()) != NXstatus::NX_OK) {
    std::cerr << "Failed to NXlinkexternaldataset(hfil, \"extlinknative\", \"" << temperaturePath << "\")\n";
    return TEST_FAILED;
  }

  if (NXclose(&hfil) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }

  // actually test linking
  if (NXopen(testFile.c_str(), NXACC_RDWR, &hfil) != NXstatus::NX_OK) {
    std::cerr << "Failed to open \"" << testFile << "\" for read/write\n";
    return TEST_FAILED;
  }
  if (NXopenpath(hfil, "/entry1/start_time") != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  char time[132];
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  printf("First file time: %s\n", time);

  char filename[256];
  if (NXinquirefile(hfil, filename, 256) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  std::cout << "NXinquirefile found: " << relativePathOf(filename) << "\n";

  if (NXopenpath(hfil, "/entry2/sample/sample_name") != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  printf("Second file sample: %s\n", time);
  if (NXinquirefile(hfil, filename, 256) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  std::cout << "NXinquirefile found: " << relativePathOf(filename) << "\n";

  if (NXopenpath(hfil, "/entry2/start_time") != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
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
  if (NXopendata(hfil, "extlinkdata") != NXstatus::NX_OK)
    return TEST_FAILED;
  memset(&temperature, 0, 4);
  if (NXgetdata(hfil, &temperature) != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  printf("value retrieved: %4.2f\n", temperature);

  if (NXopenpath(hfil, "/entry3") != NXstatus::NX_OK) {
    return TEST_FAILED;
  }
  if (NXisexternaldataset(hfil, "extlinknative", filename, 255) != NXstatus::NX_OK) {
    printf("extlinknative should be external link\n");
    return TEST_FAILED;
  } else {
    printf("extlinknative external URL = %s\n", filename);
  }
  if (NXopendata(hfil, "extlinknative") != NXstatus::NX_OK)
    return TEST_FAILED;
  memset(&temperature, 0, 4);
  if (NXgetdata(hfil, &temperature) != NXstatus::NX_OK) {
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
#endif
}
