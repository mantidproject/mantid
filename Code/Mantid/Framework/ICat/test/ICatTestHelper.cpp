#include "ICatTestHelper.h"
#include "MantidICat/ICatExport.h"

namespace ICatTestHelper
{
  /// Skip all unit tests if ICat server is down
  EXPORT_OPT_MANTID_ICAT bool skipTests()
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");
    Mantid::ICat::Login loginobj;
    loginobj.initialize();
    loginobj.setPropertyValue("Username", "mantid_test");
    loginobj.setPropertyValue("Password", "mantidtestuser");
    loginobj.execute();
    if (!loginobj.isExecuted())
    {
      std::cerr << "ICat server seems to be down. Skipping tests" << std::endl;
      return true;
    }
    else
      return false;
  }

}
