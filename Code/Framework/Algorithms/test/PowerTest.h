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

class PowerTest: public CxxTest::TestSuite
{
public:

  void testName()
  {
    Mantid::Algorithms::Power power;
TSM_ASSERT_EQUALS  ("Algorithm name should be Power", power.name(), "Power" )
}

void testVersion()
{
  Mantid::Algorithms::Power power;
  TSM_ASSERT_EQUALS("Expected version is 1", power.version(), 1 )
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

  TSM_ASSERT_THROWS_NOTHING("InputWorkspace should be settable", power.setPropertyValue("InputWorkspace","InputWS") )
  TSM_ASSERT_THROWS_NOTHING("OutputWorkspace should be settable", power.setPropertyValue("OutputWorkspace","WSCor") )
  TSM_ASSERT_THROWS_NOTHING("Exponent should be settable", power.setPropertyValue("Exponent","2.0") )

  AnalysisDataService::Instance().remove("InputWS");
  AnalysisDataService::Instance().remove("WSCor");
}

void testNonNumericExponent()
{
  Power power;
  power.initialize();
  TSM_ASSERT_THROWS("Exponent cannot be non-numeric", power.setPropertyValue("Exponent","x"), std::invalid_argument )
}

void testNegativeExponent()
{
  Power power;
  power.initialize();
  TSM_ASSERT_THROWS_NOTHING("Negative exponents are allowed.", power.setPropertyValue("Exponent","-1"))
}

void testdefaultExponent()
{
  Power power;
  power.initialize();
  std::string sz_InitalValue = power.getPropertyValue("Exponent");
  TSM_ASSERT_EQUALS("The default exponent value should be 1", "1", sz_InitalValue);
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
  TSM_ASSERT("The Power algorithm did not finish executing", power.isExecuted());

  WorkspaceSingleValue_sptr output = boost::dynamic_pointer_cast<WorkspaceSingleValue>(AnalysisDataService::Instance().retrieve("WSCor"));

  Mantid::MantidVec expectedValue(1,4);
  TSM_ASSERT_EQUALS("Power has not been determined correctly", expectedValue, output->dataY());

  AnalysisDataService::Instance().remove("InputWS");
  AnalysisDataService::Instance().remove("WSCor");
}

void testPowerCalculationWithNegativeExponent()
{
  WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
  AnalysisDataService::Instance().add("InputWS", baseWs);

  Power power;
  power.initialize();

  power.setPropertyValue("InputWorkspace","InputWS");
  power.setPropertyValue("OutputWorkspace","WSCor");
  power.setPropertyValue("Exponent","-2.0");

  power.execute();
  TSM_ASSERT("The Power algorithm did not finish executing", power.isExecuted());

  WorkspaceSingleValue_sptr output = boost::dynamic_pointer_cast<WorkspaceSingleValue>(AnalysisDataService::Instance().retrieve("WSCor"));

  Mantid::MantidVec expectedValue(1, 0.25);
  TSM_ASSERT_EQUALS("Power has not been determined correctly", expectedValue, output->dataY());

  Mantid::MantidVec expectedError(1, 0.35355);
  TS_ASSERT_DELTA(0.353553391, output->dataE()[0], 0.001);

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

  Mantid::MantidVec expectedError(1,16);
  TSM_ASSERT_EQUALS("Error has not been determined correctly", expectedError, output->dataE());

  AnalysisDataService::Instance().remove("InputWS");
  AnalysisDataService::Instance().remove("WSCor");
}
};

#endif
