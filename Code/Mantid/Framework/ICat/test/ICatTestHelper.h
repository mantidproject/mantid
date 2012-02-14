#ifndef ICATTESTHELPER_H_
#define ICATTESTHELPER_H_

#include "MantidICat/CatalogLogin.h"
#include "MantidICat/Session.h"
#include "MantidKernel/ConfigService.h"

namespace ICatTestHelper
{
  /// Skip all unit tests if ICat server is down
  bool skipTests();
}

#endif
