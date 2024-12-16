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
#include <filesystem>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "MantidNexusCpp/napi.h"
#include "napi_test_util.h"

using NexusCppTest::print_data;

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

  if (NXputattr(file, "old_style_int_attribute", &i, 1, NX_INT32) != NX_OK) {
    std::cerr << "Failed to NXputattr(handle, \"old_style_int_attribute\", ...)\n";
    return NX_ERROR;
  }
  if (NXputattr(file, "oldstylestrattr", "i:wq!<ESC><ESC>", static_cast<int>(strlen("i:wq!<ESC><ESC>")), NX_CHAR) !=
      NX_OK)
    return NX_ERROR;
  return NX_OK;
}

int main(int argc, char *argv[]) {
  if (argc != 1) {
    fprintf(stderr, "failed to specify backend\n");
    return 1;
  }

  int i, n, level, NXrank, NXrank2, NXdims[32], NXdims2[32], NXtype, NXtype2, NXlen, attr_status;
  void *data_buffer;

  const int i4_array[4] = {1000000, 2000000, 3000000, 4000000};

  char name[NX_MAXNAMELEN], char_buffer[128];

  int nx_creation_code;
  std::string filename("napi_attra.");
  if (strstr(argv[0], "hdf4") != NULL) {
    nx_creation_code = NXACC_CREATE;
    filename += "hdf";
  } else if (strstr(argv[0], "xml") != NULL) {
    fprintf(stderr, "\nxml not supported\n");
    return 1;
  } else {
    nx_creation_code = NXACC_CREATE5;
    filename += "h5";
  }

  // cleanup from previous run
  if (std::filesystem::exists(filename))
    std::filesystem::remove(filename);

  /* make sure to test strings (we might not support vlen or only support that) and numbers */

  std::cout << "\nstarting napi_attra_test\n";
  std::cout << "creating file \"" << filename << "\"\n";

  NXhandle fileid;
  if (NXopen(filename.c_str(), nx_creation_code, &fileid) != NX_OK) {
    std::cerr << "NXopen(" << filename << ", " << nx_creation_code << ", handle)\n failed";
    return 1;
  }

  /* create global attributes */
  fprintf(stderr, "creating global attributes\n");

  if (createAttrs(fileid) != NX_OK && nx_creation_code == NXACC_CREATE5) {
    fprintf(stderr, "unexpected problem creating attributes\n");
    return 1;
  }

  /* create group attributes */
  if (NXmakegroup(fileid, "entry", "NXentry") != NX_OK) {
    std::cerr << "Failed to create /entry\n";
    return 1;
  }
  if (NXopengroup(fileid, "entry", "NXentry") != NX_OK) {
    std::cerr << "Failed to open /entry\n";
    return 1;
  }

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

  if (NXopen(filename.c_str(), NXACC_READ, &fileid) != NX_OK)
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
      // cppcheck-suppress argumentSize
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
        // cppcheck-suppress argumentSize
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
        print_data("", std::cerr, NXdims, NX_INT32, NXrank);
        // cppcheck-suppress cstyleCast
        if (NXmalloc((void **)&data_buffer, NXrank, NXdims, NXtype) != NX_OK) {
          fprintf(stderr, "CANNOT GET MEMORY FOR %s\n", name);
          return 1;
        }
        if (NXgetattra(fileid, name, data_buffer) != NX_OK) {
          fprintf(stderr, "CANNOT get data for %s\n", name);
          return 1;
        }
        print_data("\t\t", std::cerr, data_buffer, NXtype, n);
        // cppcheck-suppress cstyleCast
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
          print_data("\t\t", std::cerr, char_buffer, NXtype, 1);

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
      // cppcheck-suppress argumentSize
      attr_status = NXgetnextattra(fileid, name, &NXrank, NXdims, &NXtype);
      if (attr_status == NX_EOD) {
        fprintf(stderr, "BANG! We've seen no error iterating through array attributes with old api\n");
        break;
      }
    } while (attr_status != NX_ERROR);
  }

  const int result = (NXclose(&fileid) == NX_OK) ? 0 : 1;

  // remove file that was created
  if (std::filesystem::exists(filename))
    std::filesystem::remove(filename);

  if (result == 0)
    std::cout << "we reached the end - this looks good\n";

  return result;
}
