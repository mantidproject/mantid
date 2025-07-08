#include "MantidNexus/napi.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#ifndef WIN32
#include <unistd.h>
#endif

const int nFiles = 10;
const int nEntry = 10;
const int nData = 10;
Mantid::Nexus::DimVector vec_dims{4};
short int i2_array[4] = {1000, 2000, 3000, 4000};
int iFile, iReOpen, iEntry, iData, iNXdata, iSimpleArraySize = 4;

int main() {
  printf("Running for %d iterations\n", nFiles);
  NXaccess access_mode = NXaccess::CREATE5;
  std::string filename;

  for (iFile = 0; iFile < nFiles; iFile++) {
    filename = "leak_test2_" + std::to_string(iFile) + ".nxs";
    remove(filename.c_str());
    NXhandle fileid;
    if (NXopen(filename, access_mode, fileid) != NXstatus::NX_OK) {
      std::cerr << "NXopen failed!" << std::endl;
      return 1;
    }

    for (iEntry = 0; iEntry < nEntry; iEntry++) {
      std::ostringstream oss;
      oss << "entry_" << iEntry;
      if (NXmakegroup(fileid, oss.str(), "NXentry") != NXstatus::NX_OK) {
        std::cerr << "NXmakegroup failed!" << std::endl;
        return 1;
      }

      if (NXopengroup(fileid, oss.str(), "NXentry") != NXstatus::NX_OK) {
        std::cerr << "NXopengroup failed!" << std::endl;
        return 1;
      }

      for (iNXdata = 0; iNXdata < nData; iNXdata++) {
        std::ostringstream oss2;
        oss2 << "data_" << iNXdata;
        if (NXmakegroup(fileid, oss2.str(), "NXdata") != NXstatus::NX_OK) {
          std::cerr << "NXmakegroup failed!" << std::endl;
          return 1;
        }

        if (NXopengroup(fileid, oss2.str(), "NXdata") != NXstatus::NX_OK) {
          std::cerr << "NXopengroup failed!" << std::endl;
          return 1;
        }

        for (iData = 0; iData < nData; iData++) {
          std::ostringstream oss3;
          oss3 << "i2_data_" << iData;
          if (NXcompmakedata64(fileid, oss3.str(), NXnumtype::INT16, 1, vec_dims, NXcompression::NONE, vec_dims) !=
              NXstatus::NX_OK) {
            std::cerr << "NXmakedata failed!" << std::endl;
            return 1;
          }

          if (NXopendata(fileid, oss3.str()) != NXstatus::NX_OK) {
            std::cerr << "NXopendata failed!" << std::endl;
            return 1;
          }

          if (NXputdata(fileid, i2_array) != NXstatus::NX_OK) {
            std::cerr << "NXputdata failed!" << std::endl;
            return 1;
          }

          if (NXclosedata(fileid) != NXstatus::NX_OK) {
            std::cerr << "NXclosedata failed!" << std::endl;
            return 1;
          }
        }

        if (NXclosegroup(fileid) != NXstatus::NX_OK) {
          std::cerr << "NXclosegroup failed!" << std::endl;
          return 1;
        }
      }

      if (NXclosegroup(fileid) != NXstatus::NX_OK) {
        std::cerr << "NXclosegroup failed!" << std::endl;
        return 1;
      }
    }

    if (NXclose(fileid) != NXstatus::NX_OK) {
      std::cerr << "NXclose failed!" << std::endl;
      return 1;
    }
    fileid = NULL;

    // Delete file
    remove(filename.c_str());
  }
  return 0;
}
