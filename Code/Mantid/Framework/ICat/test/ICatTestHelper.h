#ifndef ICATTESTHELPER_H_
#define ICATTESTHELPER_H_

#include "MantidICat/Login.h"
#include "MantidICat/Session.h"
#include "MantidKernel/ConfigService.h"
#include "MantidICat/ICatExport.h"

namespace ICatTestHelper
{
  /// Skip all unit tests if ICat server is down
  EXPORT_OPT_MANTID_ICAT bool skipTests();
}

#endif
