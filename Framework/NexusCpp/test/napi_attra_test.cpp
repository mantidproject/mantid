/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Data Format

  Test program for attribute array C API

  Copyright (C) 2014 NIAC

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

----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napiconfig.h"

static void print_data(const char *prefix, const void *data, const int type, const int num);

int createAttrs(const NXhandle file) {
  int array_dims[2] = {5, 4};
  static int i = 2014;

  i++;

  float r4_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};

  double r8_array[5][4] = {
      {1., 2., 3., 4.}, {5., 6., 7., 8.}, {9., 10., 11., 12.}, {13., 14., 15., 16.}, {17., 18., 19., 20.}};

  if (NXputattra(file, "attribute_0d", r4_array, 0, array_dims, NX_FLOAT32) != NX_OK)
    return NX_ERROR;
  if (NXputattra(file, "attribute_1d", r4_array, 1, array_dims, NX_FLOAT32) != NX_OK)
    return NX_ERROR;
  if (NXputattra(file, "attribute_2d", r8_array, 2, array_dims, NX_FLOAT64) != NX_OK)
    return NX_ERROR;

  if (NXputattr(file, "old_style_int_attribute", &i, 1, NX_INT32) != NX_OK)
    return NX_ERROR;
  if (NXputattr(file, "oldstylestrattr", "i:wq!<ESC><ESC>", strlen("i:wq!<ESC><ESC>"), NX_CHAR) != NX_OK)
    return NX_ERROR;
  return NX_OK;
}

int main(int argc, char *argv[]) {
  int i, n, level, NXrank, NXrank2, NXdims[32], NXdims2[32], NXtype, NXtype2, NXlen, attr_status;
  void *data_buffer;

  const int i4_array[4] = {1000000, 2000000, 3000000, 4000000};

  char name[64], char_buffer[128];
  NXhandle fileid;
  int nx_creation_code;
  char nxFile[80];

  if (strstr(argv[0], "hdf4") != NULL) {
    nx_creation_code = NXACC_CREATE;
    strcpy(nxFile, "attra.h4");
  } else if (strstr(argv[0], "xml") != NULL) {
    fprintf(stderr, "\nxml not supported\n");
    return 1;
  } else {
    nx_creation_code = NXACC_CREATE5;
    strcpy(nxFile, "attra.h5");
  }

  /* make sure to test strings (we might not support vlen or only support that) and numbers */

  fprintf(stderr, "\nstarting attra napi test\n");
  fprintf(stderr, "creating file\n");

  if (NXopen(nxFile, nx_creation_code, &fileid) != NX_OK)
    return 1;
  NXsetnumberformat(fileid, NX_FLOAT32, "%9.3f");

  /* create global attributes */
  fprintf(stderr, "creating global attributes\n");

  if (createAttrs(fileid) != NX_OK && nx_creation_code == NXACC_CREATE5) {
    fprintf(stderr, "unexpected problem creating attributes\n");
    return 1;
  }

  /* create group attributes */
  if (NXmakegroup(fileid, "entry", "NXentry") != NX_OK)
    return 1;
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
    return 1;

  fprintf(stderr, "creating group attributes\n");
  if (createAttrs(fileid) != NX_OK && nx_creation_code == NXACC_CREATE5) {
    fprintf(stderr, "unexpected problem creating attributes\n");
    return 1;
  }

  /* create dataset attributes */
  NXlen = 4;
  if (NXmakedata(fileid, "dataset", NX_INT32, 1, &NXlen) != NX_OK)
    return 1;
  if (NXopendata(fileid, "dataset") != NX_OK)
    return 1;
  if (NXputdata(fileid, i4_array) != NX_OK)
    return 1;

  fprintf(stderr, "creating dataset attributes\n");
  if (createAttrs(fileid) != NX_OK && nx_creation_code == NXACC_CREATE5) {
    fprintf(stderr, "unexpected problem creating attributes\n");
    return 1;
  }

  if (NXclosedata(fileid) != NX_OK)
    return 1;

  if (NXclosegroup(fileid) != NX_OK)
    return 1;

  if (NXclose(&fileid) != NX_OK)
    return 1;

  fprintf(stderr, "file closed - reopening for testing reads\n");

  if (NXopen(nxFile, NXACC_READ, &fileid) != NX_OK)
    return 1;

  for (level = 0; level < 3; level++) {
    switch (level) {
    case 0:
      fprintf(stderr, "=== at root level\n");
      break;
    case 1:
      if (NXopengroup(fileid, "entry", "NXentry") != NX_OK)
        return 1;
      fprintf(stderr, "=== at entry level\n");
      break;
    case 2:
      if (NXopendata(fileid, "dataset") != NX_OK)
        return 1;
      fprintf(stderr, "=== at dataset level\n");
      break;
    default:
      fprintf(stderr, "=== in unexpected code path\n");
      break;
    }
    /* interate over attributes */
    fprintf(stderr, "iterating over attributes\n");

    if (NXgetattrinfo(fileid, &i) != NX_OK)
      return 1;
    if (i > 0) {
      fprintf(stderr, "\tNumber of attributes : %d\n", i);
    }
    NXinitattrdir(fileid);
    do {
      attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);

      if (attr_status == NX_ERROR)
        return 1;

      if (strcmp(name, "file_time") && strcmp(name, "NeXus_version") && strcmp(name, "HDF_version") &&
          strcmp(name, "HDF5_Version") && strcmp(name, "XML_version")) {
        // do nothing
      } else {
        fprintf(stderr, "\tskipping over %s as the value is not controlled by this test!\n", name);
        continue;
      }

      if (attr_status == NX_OK) {
        /* cross checking against info retrieved by name */
        if (NXgetattrainfo(fileid, name, &NXrank2, NXdims2, &NXtype2) != NX_OK)
          return 1;
        if (NXrank != NXrank2) {
          fprintf(stderr, "attributes ranks disagree!\n");
          return 1;
        }
        if (NXtype != NXtype2) {
          fprintf(stderr, "attributes ranks disagree!\n");
          return 1;
        }
        for (i = 0, n = 1; i < NXrank; i++) {
          n *= NXdims[i];
          if (NXdims[i] != NXdims2[i]) {
            fprintf(stderr, "attributes dimensions disagree!\n");
            return 1;
          }
        }

        fprintf(stderr, "\tfound attribute named %s of type %d, rank %d and dimensions ", name, NXtype, NXrank);
        print_data("", NXdims, NX_INT32, NXrank);
        if (NXmalloc((void **)&data_buffer, NXrank, NXdims, NXtype) != NX_OK) {
          fprintf(stderr, "CANNOT GET MEMORY FOR %s\n", name);
          return 1;
        }
        if (NXgetattra(fileid, name, data_buffer) != NX_OK) {
          fprintf(stderr, "CANNOT get data for %s\n", name);
          return 1;
        }
        print_data("\t\t", data_buffer, NXtype, n);
        if (NXfree((void **)&data_buffer) != NX_OK)
          return 1;

        /* If 0 dim number or single string, read old-style and print */
        /* otherwise attempt to read and expect that to fail */
        /* make sure old api fails correctly */
        if (NXrank == 1 && NXtype == NX_CHAR) {
          fprintf(stderr, "\treading 1d string the old way should produce similar result\n");
          NXlen = sizeof(char_buffer);
          if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK)
            return 1;
          fprintf(stderr, "\t%s = %s\n", name, char_buffer);
        } else if (NXrank == 0 || (NXrank == 1 && NXdims[0] == 1)) {
          fprintf(stderr, "\treading scalar attributes the old way should produce similar result\n");
          if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_OK) {
            fprintf(stderr, "\tbut fails\n");
            return 1;
          }
          print_data("\t\t", char_buffer, NXtype, 1);

        } else {
          fprintf(stderr, "\treading array attributes the old way should produce an error\n");
          NXlen = sizeof(char_buffer);
          if (NXgetattr(fileid, name, char_buffer, &NXlen, &NXtype) != NX_ERROR)
            fprintf(stderr, "\t\t- but does not yet\n");
          else
            fprintf(stderr, "\t\t- it does!\n");
        }
      }
    } while (attr_status == NX_OK);

    fprintf(stderr, "Next we are expecting a failure iterating with the old api\n");
    NXinitattrdir(fileid);
    do {
      attr_status = NXgetnextattr(fileid, name, NXdims, &NXtype);
      if (attr_status == NX_EOD) {
        fprintf(stderr, "BANG! We've seen no error iterating through array attributes with old api\n");
        break;
      }
    } while (attr_status != NX_ERROR);
  }

  if (NXclose(&fileid) != NX_OK)
    return 1;

  fprintf(stderr, "we reached the end - this looks good\n");
  return 0;
}

/*----------------------------------------------------------------------*/
static void print_data(const char *prefix, const void *data, const int type, const int num) {
  int i;
  fprintf(stderr, "%s", prefix);
  for (i = 0; i < num; i++) {
    switch (type) {
    case NX_CHAR:
      fprintf(stderr, "%c", ((const char *)data)[i]);
      break;
    case NX_INT8:
      fprintf(stderr, " %d", ((const unsigned char *)data)[i]);
      break;
    case NX_INT16:
      fprintf(stderr, " %d", ((const short *)data)[i]);
      break;
    case NX_INT32:
      fprintf(stderr, " %d", ((const int *)data)[i]);
      break;
    case NX_INT64:
      fprintf(stderr, " %lld", (const long long)((const int64_t *)data)[i]);
      break;
    case NX_UINT64:
      fprintf(stderr, " %llu", (const unsigned long long)((const uint64_t *)data)[i]);
      break;
    case NX_FLOAT32:
      fprintf(stderr, " %f", ((const float *)data)[i]);
      break;
    case NX_FLOAT64:
      fprintf(stderr, " %f", ((const double *)data)[i]);
      break;
    default:
      fprintf(stderr, " (print_data: invalid type %d)", type);
      break;
    }
  }
  fprintf(stderr, "\n");
}
