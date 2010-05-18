#ifndef POWERTEST_H_
#define POWERTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/Power.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class PowerTest : public CxxTest::TestSuite
{
public:

void testName()
{
    Mantid::Algorithms::Power power;
    TS_ASSERT_EQUALS  ( power.name(), "Power" )
}

void testVersion()
{
  Mantid::Algorithms::Power power;
  TS_ASSERT_EQUALS( power.version(), 1 )
}

void testInit()
{
  Mantid::Algorithms::Power power;
  TS_ASSERT_THROWS_NOTHING( power.initialize() )
  TS_ASSERT( power.isInitialized() )

  const std::vector<Property*> props = power.getProperties();
  TSM_ASSERT_EQUALS("There should only be 3 properties for this power algorithm", props.size(), 3);

  TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" )
  TS_ASSERT( props[0]->isDefault() )
  TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) )

  TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" )
  TS_ASSERT( props[1]->isDefault() )
  TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) )

  TS_ASSERT_EQUALS( props[2]->name(), "Exponent" )
  TS_ASSERT( props[2]->isDefault() )
  TS_ASSERT( dynamic_cast<PropertyWithValue<double>* >(props[2]) )
}

void testSetProperties()
{
  WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
  AnalysisDataService::Instance().add("InputWS", baseWs);

  Power power;
  power.initialize();

  TS_ASSERT_THROWS_NOTHING( power.setPropertyValue("InputWorkspace","InputWS") )
  TS_ASSERT_THROWS_NOTHING( power.setPropertyValue("OutputWorkspace","WSCor") )
  TS_ASSERT_THROWS_NOTHING( power.setPropertyValue("Exponent","2.0") )

  AnalysisDataService::Instance().remove("InputWS");
  AnalysisDataService::Instance().remove("WSCor");
}

void testNonNumericExponent()
{

  Power power;
  power.initialize();

  TS_ASSERT_THROWS( power.setPropertyValue("Exponent","x"), std::invalid_argument )

}

void testNegativeExponent()
{

  Power power;
  power.initialize();

  TS_ASSERT_THROWS( power.setPropertyValue("Exponent","-1"), std::invalid_argument )
}

void testPowerCalculation()
{
  WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
  AnalysisDataService::Instance().add("InputWS", baseWs);

  Power power;
  power.initialize();

  power.setPropertyValue("InputWorkspace","InputWS");
  power.setPropertyValue("OutputWorkspace","WSCor");
  power.setPropertyValue("Exponent","2.0");

  power.execute();
  TS_ASSERT( power.isExecuted());

  WorkspaceSingleValue_sptr output = boost::dynamic_pointer_cast<WorkspaceSingleValue>(AnalysisDataService::Instance().retrieve("WSCor"));

  Mantid::MantidVec expectedValue(1,4);
  TSM_ASSERT_EQUALS("Power has not been determined correctly", expectedValue, output->dataY());

  AnalysisDataService::Instance().remove("InputWS");
  AnalysisDataService::Instance().remove("WSCor");
}

void testPowerErrorCalculation()
{
  //Workspace creation helper creates input error as sqrt of input value. So input error = 2.

  //if x = p ^ y, then err_x = y * x * err_p / p

  WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4);
  AnalysisDataService::Instance().add("InputWS", baseWs);

  Power power;
  power.initialize();

  power.setPropertyValue("InputWorkspace","InputWS");
  power.setPropertyValue("OutputWorkspace","WSCor");
  power.setPropertyValue("Exponent","2.0");

  power.execute();

  WorkspaceSingleValue_sptr output = boost::dynamic_pointer_cast<WorkspaceSingleValue>(AnalysisDataService::Instance().retrieve("WSCor"));

  Mantid::MantidVec expectedValue(1,16);
  TSM_ASSERT_EQUALS("Error has not been determined correctly", expectedValue, output->dataY());

  AnalysisDataService::Instance().remove("InputWS");
  AnalysisDataService::Instance().remove("WSCor");
}
};

#endif
