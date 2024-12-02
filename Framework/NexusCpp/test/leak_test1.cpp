#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "MantidNexusCpp/napi.h"
#include <stdlib.h>

#define ON_ERROR(msgstr)                                                                                               \
  {                                                                                                                    \
    fprintf(stderr, msgstr);                                                                                           \
    return 1;                                                                                                          \
  }

int main() {
  NXaccess access_mode = NXACC_CREATE5;
  const int nReOpen = 1000;
  printf("Running for %d iterations\n", nReOpen);
  int iReOpen;
  const char *szFile = "leak_test1.nxs";

  NXhandle fileid;
  unlink(szFile);
  if (NXopen(szFile, access_mode, &fileid) != NX_OK)
    ON_ERROR("NXopen failed!\n")

  if (NXclose(&fileid) != NX_OK)
    ON_ERROR("NXclose failed!\n")

  for (iReOpen = 0; iReOpen < nReOpen; iReOpen++) {
    if (0 == iReOpen % 100)
      printf("loop count %d\n", iReOpen);

    if (NXopen(szFile, NXACC_RDWR, &fileid) != NX_OK)
      ON_ERROR("NXopen failed!\n");

    if (NXclose(&fileid) != NX_OK)
      ON_ERROR("NXclose failed!\n");
  }

  unlink(szFile);
  fileid = NULL;
  return 0;
}
