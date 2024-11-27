/*---------------------------------------------------------------------------
  NeXus - Neutron & X-ray Common Data Format

  Test program for C API

  Copyright (C) 1997-2009 Freddie Akeroyd

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

  $Id: napi_test.c 1178 2009-01-21 12:28:55Z Freddie Akeroyd $

----------------------------------------------------------------------------*/
#include "MantidNexusCpp/napi.h"
#include "MantidNexusCpp/napiconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_SIZE 200000

int test_unlimited(int file_type, const char *filename) {
  const static double d[DATA_SIZE];
  int dims[2] = {NX_UNLIMITED, DATA_SIZE};
  int i, slab_start[2], slab_size[2];
  NXhandle file_id = NULL;
  remove(filename);
  NXopen(filename, file_type, &file_id);
  NXmakegroup(file_id, "entry1", "NXentry");
  NXopengroup(file_id, "entry1", "NXentry");
  NXmakedata(file_id, "data", NX_FLOAT64, 2, dims);
  NXopendata(file_id, "data");
  slab_start[1] = 0;
  slab_size[0] = 1;
  slab_size[1] = DATA_SIZE;

  for (i = 0; i < 2; i++) {
    slab_start[0] = i;
    NXputslab(file_id, d, slab_start, slab_size);
  }

  NXclosedata(file_id);
  NXclosegroup(file_id);
  NXclose(&file_id);
  return 0;
}

int main(int argc, char *argv[]) {
  time_t tim;
#ifdef WITH_HDF4
  printf("Testing HDF4\n");
  time(&tim);
  test_unlimited(NXACC_CREATE4, "test_unlimited.nx4");
  printf("Took %u seconds\n", (unsigned)(time(NULL) - tim));
#endif

#ifdef WITH_HDF5
  printf("Testing HDF5\n");
  time(&tim);
  test_unlimited(NXACC_CREATE5, "test_unlimited.nx5");
  printf("Took %u seconds\n", (unsigned)(time(NULL) - tim));
#endif
  return 0;
}
