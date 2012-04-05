#ifndef MANTID_API_ENABLEDWHENWORKSPACEISTYPETEST_H_
#define MANTID_API_ENABLEDWHENWORKSPACEISTYPETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAPI/EnabledWhenWorkspaceIsType.h"
#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class EnabledWhenWorkspaceIsTypeTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EnabledWhenWorkspaceIsTypeTest *createSuite() { return new EnabledWhenWorkspaceIsTypeTest(); }
  static void destroySuite( EnabledWhenWorkspaceIsTypeTest *suite ) { delete suite; }

  class WorkspaceTesterSubClass: public WorkspaceTester
  {
  public:
    int m_someMember;
  };

  void test_enable_disable()
  {
    boost::shared_ptr<WorkspaceTester> ws1(new WorkspaceTester);
    boost::shared_ptr<WorkspaceTesterSubClass> ws2(new WorkspaceTesterSubClass);
    AnalysisDataService::Instance().addOrReplace("tester", ws1);
    AnalysisDataService::Instance().addOrReplace("testersub", ws2);

    PropertyManagerOwner alg;

    // Start with a regular property
    alg.declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input));

    // Make a property with its validator. Will be enabled when that other one is NOT the default
    alg.declareProperty("MyValidatorProp", 456);
    alg.setPropertySettings("MyValidatorProp", new EnabledWhenWorkspaceIsType<WorkspaceTesterSubClass>("InputWorkspace", true));

    alg.declareProperty("MyValidatorProp2", 456);
    alg.setPropertySettings("MyValidatorProp2", new EnabledWhenWorkspaceIsType<WorkspaceTesterSubClass>("InputWorkspace", false));

    Property * prop = alg.getPointerToProperty("MyValidatorProp");
    Property * prop2 = alg.getPointerToProperty("MyValidatorProp2");
    TS_ASSERT(prop); if (!prop) return;
    TSM_ASSERT( "Property always returns visible.", prop->getSettings()->isVisible(&alg) )
    TSM_ASSERT( "Property always returns valid.", prop->isValid().empty() )

    TSM_ASSERT( "Starts off enabled because empty",  prop->getSettings()->isEnabled(&alg));
    alg.setProperty("InputWorkspace", "tester");
    TSM_ASSERT( "Becomes disabled when the workspace is the wrong type", !prop->getSettings()->isEnabled(&alg));
    alg.setProperty("InputWorkspace", "testersub");
    TSM_ASSERT( "Becomes enabled when the workspace is correct type",  prop->getSettings()->isEnabled(&alg));

    TSM_ASSERT( "Starts disabled when the workspace is correct type",  !prop2->getSettings()->isEnabled(&alg));
    alg.setProperty("InputWorkspace", "tester");
    TSM_ASSERT( "Becomes enabled when the workspace is the wrong type", prop2->getSettings()->isEnabled(&alg));

  }


};


#endif /* MANTID_API_ENABLEDWHENWORKSPACEISTYPETEST_H_ */

