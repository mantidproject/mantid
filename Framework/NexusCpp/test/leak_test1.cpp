#include "MantidNexusCpp/napi.h"
#include "napi_test_util.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

using NexusCppTest::removeFile;

int main() {
  NXaccess access_mode = NXACC_CREATE5;
  const int nReOpen = 1000;
  printf("Running for %d iterations\n", nReOpen);
  int iReOpen;
  const std::string szFile("leak_test1.nxs");

  NXhandle fileid;

  removeFile(szFile); // in case it was left over from previous run
  if (NXopen(szFile.c_str(), access_mode, &fileid) != NXstatus::OKAY)
    ON_ERROR("NXopen failed!\n")

  if (NXclose(&fileid) != NXstatus::OKAY)
    ON_ERROR("NXclose failed!\n")

  for (iReOpen = 0; iReOpen < nReOpen; iReOpen++) {
    if (0 == iReOpen % 100)
      printf("loop count %d\n", iReOpen);

    if (NXopen(szFile.c_str(), NXACC_RDWR, &fileid) != NXstatus::OKAY)
      ON_ERROR("NXopen failed!\n");

    if (NXclose(&fileid) != NXstatus::OKAY)
      ON_ERROR("NXclose failed!\n");
  }

  removeFile(szFile); // cleanup
  fileid = NULL;
  return 0;
}
