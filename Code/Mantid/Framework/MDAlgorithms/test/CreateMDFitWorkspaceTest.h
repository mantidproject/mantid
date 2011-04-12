#ifndef CREATEMDFITWORKSPACE_TEST_H_
#define CREATEMDFITWORKSPACE_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/CreateMDFitWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFitFunction.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;

class CreateMDFitWorkspaceTest : public CxxTest::TestSuite
{
public:

    void testCreate()
    {
      CreateMDFitWorkspace maker;
      maker.initialize();
      maker.setPropertyValue("OutputWorkspace","CreateMDFitWorkspaceTest_ws");
      maker.setPropertyValue("Dimensions","id=x,xmin=0,xmax=1,n=100");
      maker.setPropertyValue("Formula","exp(-((x-0.52)^2/0.2^2))");
      maker.setPropertyValue("MaxPoints","10");
      maker.execute();
      TS_ASSERT(maker.isExecuted());

      IFitFunction* fun = FunctionFactory::Instance().createInitialized("name=UserFunctionMD,Formula=h*exp(-a*(x-c)^2),Workspace=CreateMDFitWorkspaceTest_ws");
      TS_ASSERT(fun);
      TS_ASSERT(fun->getWorkspace());


      if (fun){delete fun;}
    }

};



#endif // CREATEMDFITWORKSPACE_TEST_H_
