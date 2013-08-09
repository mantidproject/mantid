#ifndef CUSTOM_INTERFACES_CREATEMDWSALG_TEST_H_
#define CUSTOM_INTERFACES_CREATEMDWSALG_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h"
#include "MantidAPI/FrameworkManager.h"

using namespace MantidQt::CustomInterfaces;

class CreateMDWorkspaceAlgDialogTest : public CxxTest::TestSuite
{
public:
  void test_construction()
  {
    int argc(0);
    QApplication app(argc, NULL);
    Mantid::API::FrameworkManager::Instance();
    // Mainly to test that the fetching of stuff from ConvertToMD keeps working OK
    TS_ASSERT_THROWS_NOTHING( CreateMDWorkspaceAlgDialog() );
  }
};

#endif
