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
#include "MantidNexus/napi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_SIZE 200000

int test_unlimited(NXaccess file_type, const char *filename) {
  // cppcheck-suppress constVariable
  static double d[DATA_SIZE];
  Mantid::Nexus::DimVector dims{NX_UNLIMITED, DATA_SIZE};
  NXhandle file_id = NULL;
  remove(filename);
  NXopen(filename, file_type, file_id);
  NXmakegroup(file_id, "entry1", "NXentry");
  NXopengroup(file_id, "entry1", "NXentry");
  NXcompmakedata64(file_id, "data", NXnumtype::FLOAT64, 2, dims, NXcompression::NONE, dims);
  NXopendata(file_id, "data");

  Mantid::Nexus::DimSizeVector slab_start{0, 0}, slab_size{1, DATA_SIZE};
  for (Mantid::Nexus::dimsize_t i = 0; i < 2; i++) {
    slab_start[0] = i;
    NXputslab64(file_id, d, slab_start, slab_size);
  }

  NXclosedata(file_id);
  NXclosegroup(file_id);
  NXclose(file_id);
  return 0;
}

int main() {
  time_t tim;

  printf("Testing HDF5\n");
  time(&tim);
  test_unlimited(NXaccess::CREATE5, "test_unlimited.nx5");
  printf("Took %u seconds\n", (unsigned)(time(NULL) - tim));

  return 0;
}
