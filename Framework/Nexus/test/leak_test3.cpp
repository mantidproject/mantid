#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "MantidNexus/napi.h"

#define PSZ(s) (s).c_str()

using namespace std;

const int nFiles = 10;
const int nEntry = 2;
const int nData = 2;
Mantid::Nexus::DimVector array_dims{512, 512};
std::string szFile("leak_test.nxs");
std::size_t const iBinarySize(512 * 512);
int aiBinaryData[iBinarySize];

#define ON_ERROR(msgstr)                                                                                               \
  {                                                                                                                    \
    std::cerr << msgstr << std::endl;                                                                                  \
    return 1;                                                                                                          \
  }

int main() {
  int iFile, iEntry, iData, iNXdata;

  for (std::size_t i = 0; i < iBinarySize; i++) {
    aiBinaryData[i] = rand();
  }

  for (iFile = 0; iFile < nFiles; iFile++) {
    printf("file %d\n", iFile);

    NXhandle fileid;
    NXlink aLink;
    if (NXopen(szFile, NXaccess::CREATE5, fileid) != NXstatus::NX_OK)
      ON_ERROR("NXopen_failed")

    for (iEntry = 0; iEntry < nEntry; iEntry++) {
      ostringstream oss;
      oss << "entry_" << iEntry;

      if (NXmakegroup(fileid, PSZ(oss.str()), "NXentry") != NXstatus::NX_OK)
        ON_ERROR("NXmakegroup failed!")

      if (NXopengroup(fileid, PSZ(oss.str()), "NXentry") != NXstatus::NX_OK)
        ON_ERROR("NXopengroup failed!")

      for (iNXdata = 0; iNXdata < nData; iNXdata++) {
        ostringstream oss2;
        oss2 << "data_" << iNXdata;

        if (NXmakegroup(fileid, PSZ(oss2.str()), "NXdata") != NXstatus::NX_OK)
          ON_ERROR("NXmakegroup failed!")

        if (NXopengroup(fileid, PSZ(oss2.str()), "NXdata") != NXstatus::NX_OK)
          ON_ERROR("NXopengroup failed!")

        NXgetgroupID(fileid, aLink);
        for (iData = 0; iData < nData; iData++) {
          ostringstream oss3;
          oss3 << "i2_data_" << iData;

          if (NXcompmakedata64(fileid, PSZ(oss3.str()), NXnumtype::INT16, 2, array_dims, NXcompression::LZW,
                               array_dims) != NXstatus::NX_OK)
            ON_ERROR("NXcompmakedata failed!")

          if (NXopendata(fileid, PSZ(oss3.str())) != NXstatus::NX_OK)
            ON_ERROR("NXopendata failed!")

          if (NXputdata(fileid, aiBinaryData) != NXstatus::NX_OK)
            ON_ERROR("NXputdata failed!")

          if (NXclosedata(fileid) != NXstatus::NX_OK)
            ON_ERROR("NXclosedata failed!")
        }

        if (NXclosegroup(fileid) != NXstatus::NX_OK)
          ON_ERROR("NXclosegroup failed!")
      }

      if (NXclosegroup(fileid) != NXstatus::NX_OK)
        ON_ERROR("NXclosegroup failed!")
    }

    if (NXclose(fileid) != NXstatus::NX_OK)
      ON_ERROR("NXclose failed!")

    // Delete file
    remove(szFile.c_str());
  }

  return 0;
}
