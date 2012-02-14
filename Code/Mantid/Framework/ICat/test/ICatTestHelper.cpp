#include "ICatTestHelper.h"

namespace ICatTestHelper
{
  /// Skip all unit tests if ICat server is down
  bool skipTests()
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");
    Mantid::ICat::CatalogLogin loginobj;
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
