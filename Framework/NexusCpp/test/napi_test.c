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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napiconfig.h"

static void print_data(const char *prefix, void *data, int type, int num);
static int testLoadPath();
static int testExternal(char *progName);

static const char *relativePathOf(const char *filename) {
  char cwd[1024];

  getcwd(cwd, sizeof(cwd));

  if (strncmp(filename, cwd, strlen(cwd)) == 0) {
    return filename + strlen(cwd) + 1;
  } else {
    return filename;
  }
}

int main(int argc, char *argv[]) {
  int i, j, k, n, NXrank, NXdims[32], NXtype, NXlen, entry_status, attr_status;
  float r;
  void *data_buffer;
  unsigned char i1_array[4] = {1, 2, 3, 4};
  short int i2_array[4] = {1000, 2000, 3000, 4000};
  int i4_array[4] = {1000000, 2000000, 3000000, 4000000};
  float r4_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
  double r8_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};
  int array_dims[2] = {5, 4};
  int unlimited_dims[1] = {NX_UNLIMITED};
  int chunk_size[2] = {5, 4};
  int slab_start[2], slab_size[2];
  char name[64], char_class[64], char_buffer[128];
  char group_name[64], class_name[64];
  char c1_array[5][4] = {
      {'a', 'b', 'c', 'd'}, {'e', 'f', 'g', 'h'}, {'i', 'j', 'k', 'l'}, {'m', 'n', 'o', 'p'}, {'q', 'r', 's', 't'}};
  int unlimited_cdims[2] = {NX_UNLIMITED, 4};
  NXhandle fileid, clone_fileid;
  NXlink glink, dlink, blink;
  int comp_array[100][20];
  int dims[2];
  int cdims[2];
  int nx_creation_code;
  char nxFile[80];
  char filename[256];
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

  if (strstr(argv[0], "napi_test_hdf5") != NULL) {
    nx_creation_code = NXACC_CREATE5;
    strcpy(nxFile, "NXtest.h5");
  } else if (strstr(argv[0], "napi_c_test") != NULL) {
    nx_creation_code = NXACC_CREATE5;
    strcpy(nxFile, "NXtest.h5");
  } else if (strstr(argv[0], "napi_test-xml-table") != NULL) {
    nx_creation_code = NXACC_CREATEXML | NXACC_TABLE;
    strcpy(nxFile, "NXtest-table.xml");
  } else if (strstr(argv[0], "napi_test-xml") != NULL) {
    nx_creation_code = NXACC_CREATEXML;
    strcpy(nxFile, "NXtest.xml");
  } else {
    nx_creation_code = NXACC_CREATE;
    strcpy(nxFile, "NXtest.hdf");
  }

  /* create file */
  if (NXopen(nxFile, nx_creation_code, &fileid) != NX_OK)
    return 1;
  if (nx_creation_code == NXACC_CREATE5) {
    if (NXreopen(fileid, &clone_fileid) != NX_OK)
      return 1;
  }
  NXsetnumberformat(fileid, NX_FLOAT32, "%9.3f");
  if (NXmakegroup(fileid, "entry", "NXentry") != NX_OK)
    return 1;
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
    return 1;
  if (NXputattr(fileid, "hugo", "namenlos", strlen("namenlos"), NX_CHAR) != NX_OK)
    return 1;
  if (NXputattr(fileid, "cucumber", "passion", strlen("passion"), NX_CHAR) != NX_OK)
    return 1;
  NXlen = strlen(ch_test_data);
  if (NXmakedata(fileid, "ch_data", NX_CHAR, 1, &NXlen) != NX_OK)
    return 1;
  if (NXopendata(fileid, "ch_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, ch_test_data) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXmakedata(fileid, "c1_data", NX_CHAR, 2, array_dims) != NX_OK)
    return 1;
  if (NXopendata(fileid, "c1_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, c1_array) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXmakedata(fileid, "i1_data", NX_INT8, 1, &array_dims[1]) != NX_OK)
    return 1;
  if (NXopendata(fileid, "i1_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, i1_array) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXmakedata(fileid, "i2_data", NX_INT16, 1, &array_dims[1]) != NX_OK)
    return 1;
  if (NXopendata(fileid, "i2_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, i2_array) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXmakedata(fileid, "i4_data", NX_INT32, 1, &array_dims[1]) != NX_OK)
    return 1;
  if (NXopendata(fileid, "i4_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, i4_array) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXcompmakedata(fileid, "r4_data", NX_FLOAT32, 2, array_dims, NX_COMP_LZW, chunk_size) != NX_OK)
    return 1;
  if (NXopendata(fileid, "r4_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, r4_array) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXmakedata(fileid, "r8_data", NX_FLOAT64, 2, array_dims) != NX_OK)
    return 1;
  if (NXopendata(fileid, "r8_data") != NX_OK)
    return 1;
  slab_start[0] = 4;
  slab_start[1] = 0;
  slab_size[0] = 1;
  slab_size[1] = 4;
  if (NXputslab(fileid, (double *)r8_array + 16, slab_start, slab_size) != NX_OK)
    return 1;
  slab_start[0] = 0;
  slab_start[1] = 0;
  slab_size[0] = 4;
  slab_size[1] = 4;
  if (NXputslab(fileid, r8_array, slab_start, slab_size) != NX_OK)
    return 1;
  if (NXputattr(fileid, "ch_attribute", ch_test_data, strlen(ch_test_data), NX_CHAR) != NX_OK)
    return 1;
  i = 42;
  if (NXputattr(fileid, "i4_attribute", &i, 1, NX_INT32) != NX_OK)
    return 1;
  r = 3.14159265;
  if (NXputattr(fileid, "r4_attribute", &r, 1, NX_FLOAT32) != NX_OK)
    return 1;
  if (NXgetdataID(fileid, &dlink) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  dims[0] = 4;
  if (nx_creation_code != NXACC_CREATE) {
    if (NXmakedata(fileid, "grosse_zahl", NX_INT64, 1, dims) == NX_OK) {
      if (NXopendata(fileid, "grosse_zahl") != NX_OK)
        return 1;
      if (NXputdata(fileid, grossezahl) != NX_OK)
        return 1;
      if (NXclosedata(fileid) != NX_OK)
        return 1;
    }
  }
  if (NXmakegroup(fileid, "data", "NXdata") != NX_OK)
    return 1;
  if (NXopengroup(fileid, "data", "NXdata") != NX_OK)
    return 1;
  if (NXmakelink(fileid, &dlink) != NX_OK)
    return 1;
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
    return 1;
  if (NXopendata(fileid, "comp_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, comp_array) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXflush(&fileid) != NX_OK)
    return 1;
  if (NXmakedata(fileid, "flush_data", NX_INT32, 1, unlimited_dims) != NX_OK)
    return 1;
  slab_size[0] = 1;
  for (i = 0; i < 7; i++) {
    slab_start[0] = i;
    if (NXopendata(fileid, "flush_data") != NX_OK)
      return 1;
    if (NXputslab(fileid, &i, slab_start, slab_size) != NX_OK)
      return 1;
    if (NXflush(&fileid) != NX_OK)
      return 1;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  if (NXmakegroup(fileid, "sample", "NXsample") != NX_OK)
    return 1;
  if (NXopengroup(fileid, "sample", "NXsample") != NX_OK)
    return 1;
  NXlen = 12;
  if (NXmakedata(fileid, "ch_data", NX_CHAR, 1, &NXlen) != NX_OK)
    return 1;
  if (NXopendata(fileid, "ch_data") != NX_OK)
    return 1;
  if (NXputdata(fileid, "NeXus sample") != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXgetgroupID(fileid, &glink) != NX_OK)
    return 1;
  if ((nx_creation_code & NXACC_CREATEXML) == 0) {
    if (NXmakedata(fileid, "cdata_unlimited", NX_CHAR, 2, unlimited_cdims) != NX_OK)
      return 1;
    if (NXopendata(fileid, "cdata_unlimited") != NX_OK)
      return 1;
    slab_size[0] = 1;
    slab_size[1] = 4;
    slab_start[1] = 0;
    for (i = 0; i < 5; i++) {
      slab_start[0] = i;
      if (NXputslab(fileid, &(c1_array[i][0]), slab_start, slab_size) != NX_OK)
        return 1;
    }
    if (NXclosedata(fileid) != NX_OK)
      return 1;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  if (NXmakegroup(fileid, "link", "NXentry") != NX_OK)
    return 1;
  if (NXopengroup(fileid, "link", "NXentry") != NX_OK)
    return 1;
  if (NXmakelink(fileid, &glink) != NX_OK)
    return 1;
  if (NXmakenamedlink(fileid, "renLinkGroup", &glink) != NX_OK)
    return 1;
  if (NXmakenamedlink(fileid, "renLinkData", &dlink) != NX_OK)
    return 1;
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  if (NXclose(&fileid) != NX_OK)
    return 1;

  if ((argc >= 2) && !strcmp(argv[1], "-q")) {
    return 0; /* create only */
  }
  /*
    read test
  */
  if (NXopen(nxFile, NXACC_RDWR, &fileid) != NX_OK)
    return 1;
  if (NXinquirefile(fileid, filename, 256) != NX_OK) {
    return 1;
  }
  printf("NXinquirefile found: %s\n", relativePathOf(filename));
  NXgetattrinfo(fileid, &i);
  if (i > 0) {
    printf("Number of global attributes: %d\n", i);
  }
  do {
    attr_status = NXgetnextattr(fileid, name, NXdims, &NXtype);
    if (attr_status == NX_ERROR)
      return 1;
    if (attr_status == NX_OK) {
      switch (NXtype) {
      case NX_CHAR:
        NXlen = sizeof(char_buffer);
        if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK)
          return 1;
        if (strcmp(name, "file_time") && strcmp(name, "HDF_version") && strcmp(name, "HDF5_Version") &&
            strcmp(name, "XML_version")) {
          printf("   %s = %s\n", name, char_buffer);
        }
        break;
      }
    }
  } while (attr_status == NX_OK);
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
    return 1;
  NXgetattrinfo(fileid, &i);
  printf("Number of group attributes: %d\n", i);
  if (NXgetpath(fileid, path, 512) != NX_OK)
    return 1;
  printf("NXentry path %s\n", path);
  do {
    attr_status = NXgetnextattr(fileid, name, NXdims, &NXtype);
    if (attr_status == NX_ERROR)
      return 1;
    if (attr_status == NX_OK) {
      switch (NXtype) {
      case NX_CHAR:
        NXlen = sizeof(char_buffer);
        if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK)
          return 1;
        printf("   %s = %s\n", name, char_buffer);
      }
    }
  } while (attr_status == NX_OK);
  if (NXgetgroupinfo(fileid, &i, group_name, class_name) != NX_OK)
    return 1;
  printf("Group: %s(%s) contains %d items\n", group_name, class_name, i);
  do {
    entry_status = NXgetnextentry(fileid, name, char_class, &NXtype);
    if (entry_status == NX_ERROR)
      return 1;
    if (strcmp(char_class, "SDS") != 0) {
      if (entry_status != NX_EOD) {
        printf("   Subgroup: %s(%s)\n", name, char_class);
        entry_status = NX_OK;
      }
    } else {
      if (entry_status == NX_OK) {
        if (NXopendata(fileid, name) != NX_OK)
          return 1;
        if (NXgetpath(fileid, path, 512) != NX_OK)
          return 1;
        printf("Data path %s\n", path);
        if (NXgetinfo(fileid, &NXrank, NXdims, &NXtype) != NX_OK)
          return 1;
        printf("   %s(%d)", name, NXtype);
        if (NXmalloc((void **)&data_buffer, NXrank, NXdims, NXtype) != NX_OK)
          return 1;
        n = 1;
        for (k = 0; k < NXrank; k++) {
          n *= NXdims[k];
        }
        if (NXtype == NX_CHAR) {
          if (NXgetdata(fileid, data_buffer) != NX_OK)
            return 1;
          print_data(" = ", data_buffer, NXtype, n);
        } else if (NXtype != NX_FLOAT32 && NXtype != NX_FLOAT64) {
          if (NXgetdata(fileid, data_buffer) != NX_OK)
            return 1;
          print_data(" = ", data_buffer, NXtype, n);
        } else {
          slab_start[0] = 0;
          slab_start[1] = 0;
          slab_size[0] = 1;
          slab_size[1] = 4;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return 1;
          print_data("\n      ", data_buffer, NXtype, 4);
          slab_start[0] = 1;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return 1;
          print_data("      ", data_buffer, NXtype, 4);
          slab_start[0] = 2;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return 1;
          print_data("      ", data_buffer, NXtype, 4);
          slab_start[0] = 3;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return 1;
          print_data("      ", data_buffer, NXtype, 4);
          slab_start[0] = 4;
          if (NXgetslab(fileid, data_buffer, slab_start, slab_size) != NX_OK)
            return 1;
          print_data("      ", data_buffer, NXtype, 4);
          if (NXgetattrinfo(fileid, &i) != NX_OK)
            return 1;
          if (i > 0) {
            printf("      Number of attributes : %d\n", i);
          }
          do {
            attr_status = NXgetnextattr(fileid, name, NXdims, &NXtype);
            if (attr_status == NX_ERROR)
              return 1;
            if (attr_status == NX_OK) {
              switch (NXtype) {
              case NX_INT32:
                NXlen = 1;
                if (NXgetattr(fileid, name, &i, &NXlen, &NXtype) != NX_OK)
                  return 1;
                printf("         %s : %d\n", name, i);
                break;
              case NX_FLOAT32:
                NXlen = 1;
                if (NXgetattr(fileid, name, &r, &NXlen, &NXtype) != NX_OK)
                  return 1;
                printf("         %s : %f\n", name, r);
                break;
              case NX_CHAR:
                NXlen = sizeof(char_buffer);
                if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK)
                  return 1;
                printf("         %s : %s\n", name, char_buffer);
                break;
              }
            }
          } while (attr_status == NX_OK);
        }
        if (NXclosedata(fileid) != NX_OK)
          return 1;
        if (NXfree((void **)&data_buffer) != NX_OK)
          return 1;
      }
    }
  } while (entry_status == NX_OK);
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  /*
   * check links
   */
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
    return 1;
  if (NXopengroup(fileid, "sample", "NXsample") != NX_OK)
    return 1;
  if (NXgetgroupID(fileid, &glink) != NX_OK)
    return 1;
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  if (NXopengroup(fileid, "data", "NXdata") != NX_OK)
    return 1;
  if (NXopendata(fileid, "r8_data") != NX_OK)
    return 1;
  if (NXgetdataID(fileid, &dlink) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  if (NXopendata(fileid, "r8_data") != NX_OK)
    return 1;
  if (NXgetdataID(fileid, &blink) != NX_OK)
    return 1;
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXsameID(fileid, &dlink, &blink) != NX_OK) {
    printf("Link check FAILED (r8_data)\n");
    printf("original data\n");
    NXIprintlink(fileid, &dlink);
    printf("linked data\n");
    NXIprintlink(fileid, &blink);
    return 1;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return 1;

  if (NXopengroup(fileid, "link", "NXentry") != NX_OK)
    return 1;
  if (NXopengroup(fileid, "sample", "NXsample") != NX_OK)
    return 1;
  if (NXgetpath(fileid, path, 512) != NX_OK)
    return 1;
  printf("Group path %s\n", path);
  if (NXgetgroupID(fileid, &blink) != NX_OK)
    return 1;
  if (NXsameID(fileid, &glink, &blink) != NX_OK) {
    printf("Link check FAILED (sample)\n");
    printf("original group\n");
    NXIprintlink(fileid, &glink);
    printf("linked group\n");
    NXIprintlink(fileid, &blink);
    return 1;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return 1;

  if (NXopengroup(fileid, "renLinkGroup", "NXsample") != NX_OK)
    return 1;
  if (NXgetgroupID(fileid, &blink) != NX_OK)
    return 1;
  if (NXsameID(fileid, &glink, &blink) != NX_OK) {
    printf("Link check FAILED (renLinkGroup)\n");
    printf("original group\n");
    NXIprintlink(fileid, &glink);
    printf("linked group\n");
    NXIprintlink(fileid, &blink);
    return 1;
  }
  if (NXclosegroup(fileid) != NX_OK)
    return 1;

  if (NXopendata(fileid, "renLinkData") != NX_OK)
    return 1;
  if (NXgetdataID(fileid, &blink) != NX_OK)
    return 1;
  if (NXsameID(fileid, &dlink, &blink) != NX_OK) {
    printf("Link check FAILED (renLinkData)\n");
    printf("original group\n");
    NXIprintlink(fileid, &glink);
    printf("linked group\n");
    NXIprintlink(fileid, &blink);
    return 1;
  }
  if (NXclosedata(fileid) != NX_OK)
    return 1;
  if (NXclosegroup(fileid) != NX_OK)
    return 1;
  printf("Link check OK\n");

  /*
    tests for NXopenpath
  */
  if (NXopenpath(fileid, "/entry/data/comp_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return 0;
  }
  if (NXopenpath(fileid, "/entry/data/comp_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return 0;
  }
  if (NXopenpath(fileid, "../r8_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return 0;
  }
  if (NXopengrouppath(fileid, "/entry/data/comp_data") != NX_OK) {
    printf("Failure on NXopengrouppath\n");
    return 0;
  }
  if (NXopenpath(fileid, "/entry/data/r8_data") != NX_OK) {
    printf("Failure on NXopenpath\n");
    return 0;
  }
  printf("NXopenpath checks OK\n");

  if (NXclose(&fileid) != NX_OK)
    return 1;

  printf("before load path tests\n");
  if (testLoadPath() != 0)
    return 1;

  printf("before external link tests\n");
  if (testExternal(argv[0]) != 0) {
    return 1;
  }

  printf("all ok - done\n");
  return 0;
}
/*---------------------------------------------------------------------*/
static int testLoadPath() {
  NXhandle h;

  if (getenv("NX_LOAD_PATH") != NULL) {
    if (NXopen("dmc01.hdf", NXACC_RDWR, &h) != NX_OK) {
      printf("Loading NeXus file dmc01.hdf from path %s FAILED\n", getenv("NX_LOAD_PATH"));
      return 1;
    } else {
      printf("Success loading NeXus file from path\n");
      NXclose(&h);
      return 0;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------*/
static int testExternal(char *progName) {
  char nxfile[255], ext[5], testFile[80], time[132], filename[256];
  int create;
  NXhandle hfil;
  int dummylen = 1;
  float dummyfloat = 1;
  float temperature;

  if (strstr(progName, "hdf4") != NULL) {
    strcpy(ext, "hdf");
    create = NXACC_CREATE;
  } else if (strstr(progName, "hdf5") != NULL) {
    strcpy(ext, "h5");
    create = NXACC_CREATE5;
  } else if (strstr(progName, "xml") != NULL) {
    strcpy(ext, "xml");
    create = NXACC_CREATEXML;
  } else {
    printf("Failed to recognise napi_test program in testExternal\n");
    return 1;
  }

  sprintf(testFile, "nxext.%s", ext);

  /*
    create the test file
  */
  if (NXopen(testFile, create, &hfil) != NX_OK) {
    return 1;
  }
  /*if(NXmakegroup(hfil,"entry1","NXentry") != NX_OK){
    return 1;
  }*/
  sprintf(nxfile, "nxfile://data/dmc01.%s#/entry1", ext);
  if (NXlinkexternal(hfil, "entry1", "NXentry", nxfile) != NX_OK) {
    return 1;
  }
  /*if(NXmakegroup(hfil,"entry2","NXentry") != NX_OK){
    return 1;
  }*/
  sprintf(nxfile, "nxfile://data/dmc02.%s#/entry1", ext);
  if (NXlinkexternal(hfil, "entry2", "NXentry", nxfile) != NX_OK) {
    return 1;
  }
  if (NXmakegroup(hfil, "entry3", "NXentry") != NX_OK) {
    return 1;
  }
  if (NXopengroup(hfil, "entry3", "NXentry") != NX_OK) {
    return 1;
  }
  /* force create old style external link */
  if (NXmakedata(hfil, "extlinkdata", NX_FLOAT32, 1, &dummylen) != NX_OK)
    return 1;
  if (NXopendata(hfil, "extlinkdata") != NX_OK)
    return 1;
  if (NXputdata(hfil, &dummyfloat) != NX_OK)
    return 1;
  sprintf(nxfile, "nxfile://data/dmc01.%s#/entry1/sample/temperature_mean", ext);
  if (NXputattr(hfil, "napimount", nxfile, strlen(nxfile), NX_CHAR) != NX_OK)
    return 1;
  /* this would segfault because we are tricking the napi stack
  if(NXclosedata(&hfil) != NX_OK){
    return 1;
  }
  */
  if (NXopenpath(hfil, "/entry3") != NX_OK) {
    return 1;
  }
  /* create new style external link on hdf5 , equivalent to the above on other backends */
  if (NXlinkexternaldataset(hfil, "extlinknative", nxfile) != NX_OK)
    return 1;

  if (NXclose(&hfil) != NX_OK) {
    return 1;
  }

  /*
    actually test linking
  */
  if (NXopen(testFile, NXACC_RDWR, &hfil) != NX_OK) {
    return 1;
  }
  if (NXopenpath(hfil, "/entry1/start_time") != NX_OK) {
    return 1;
  }
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NX_OK) {
    return 1;
  }
  printf("First file time: %s\n", time);

  if (NXinquirefile(hfil, filename, 256) != NX_OK) {
    return 1;
  }
  printf("NXinquirefile found: %s\n", relativePathOf(filename));

  if (NXopenpath(hfil, "/entry2/sample/sample_name") != NX_OK) {
    return 1;
  }
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NX_OK) {
    return 1;
  }
  printf("Second file sample: %s\n", time);
  if (NXinquirefile(hfil, filename, 256) != NX_OK) {
    return 1;
  }
  printf("NXinquirefile found: %s\n", relativePathOf(filename));

  if (NXopenpath(hfil, "/entry2/start_time") != NX_OK) {
    return 1;
  }
  memset(time, 0, 132);
  if (NXgetdata(hfil, time) != NX_OK) {
    return 1;
  }
  printf("Second file time: %s\n", time);
  NXopenpath(hfil, "/");
  if (NXisexternalgroup(hfil, "entry1", "NXentry", filename, 255) != NX_OK) {
    return 1;
  } else {
    printf("entry1 external URL = %s\n", filename);
  }

  printf("testing link to external data set\n");
  if (NXopenpath(hfil, "/entry3") != NX_OK) {
    return 1;
  }
  if (NXisexternaldataset(hfil, "extlinkdata", filename, 255) != NX_OK) {
    printf("extlinkdata should be external link\n");
    return 1;
  } else {
    printf("extlinkdata external URL = %s\n", filename);
  }
  if (NXopendata(hfil, "extlinkdata") != NX_OK)
    return 1;
  memset(&temperature, 0, 4);
  if (NXgetdata(hfil, &temperature) != NX_OK) {
    return 1;
  }
  printf("value retrieved: %4.2f\n", temperature);

  if (NXopenpath(hfil, "/entry3") != NX_OK) {
    return 1;
  }
  if (NXisexternaldataset(hfil, "extlinknative", filename, 255) != NX_OK) {
    printf("extlinknative should be external link\n");
    return 1;
  } else {
    printf("extlinknative external URL = %s\n", filename);
  }
  if (NXopendata(hfil, "extlinknative") != NX_OK)
    return 1;
  memset(&temperature, 0, 4);
  if (NXgetdata(hfil, &temperature) != NX_OK) {
    return 1;
  }
  printf("value retrieved: %4.2f\n", temperature);

  NXclose(&hfil);
  printf("External File Linking tested OK\n");
  return 0;
}
/*----------------------------------------------------------------------*/
static void print_data(const char *prefix, void *data, int type, int num) {
  int i;
  printf("%s", prefix);
  for (i = 0; i < num; i++) {
    switch (type) {
    case NX_CHAR:
      printf("%c", ((char *)data)[i]);
      break;

    case NX_INT8:
      printf(" %d", ((unsigned char *)data)[i]);
      break;

    case NX_INT16:
      printf(" %d", ((short *)data)[i]);
      break;

    case NX_INT32:
      printf(" %d", ((int *)data)[i]);
      break;

    case NX_INT64:
      printf(" %lld", (long long)((int64_t *)data)[i]);
      break;

    case NX_UINT64:
      printf(" %llu", (unsigned long long)((uint64_t *)data)[i]);
      break;

    case NX_FLOAT32:
      printf(" %f", ((float *)data)[i]);
      break;

    case NX_FLOAT64:
      printf(" %f", ((double *)data)[i]);
      break;

    default:
      printf("print_data: invalid type");
      break;
    }
  }
  printf("\n");
}
