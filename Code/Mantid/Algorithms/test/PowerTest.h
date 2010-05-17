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

	void testInit()
	{
		Power alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize());
		TS_ASSERT(alg.isInitialized());
		//Setting properties to input workspaces that don't exist throws
		TSM_ASSERT_THROWS("Base must be numeric", alg.setPropertyValue("LHSWorkspace","n"), std::invalid_argument )
		TSM_ASSERT_THROWS("Base must be numeric", alg.setPropertyValue("RHSWorkspace","n"), std::invalid_argument )
	}

	void testPowerCalculation()
	{
		WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
		WorkspaceSingleValue_sptr exponentWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
		AnalysisDataService::Instance().add("baseWsp",baseWs);
		AnalysisDataService::Instance().add("exponentWsp",exponentWs);

		Power alg;
		alg.initialize();

		alg.setPropertyValue("OutputWorkspace","OutputWsp");
		alg.setPropertyValue("RHSWorkspace", "exponentWsp");
		alg.setPropertyValue("LHSWorkspace", "baseWsp");
		alg.execute();

		WorkspaceSingleValue_sptr output =  boost::dynamic_pointer_cast<WorkspaceSingleValue>(AnalysisDataService::Instance().retrieve("OutputWsp"));
		Mantid::MantidVec expectedValue(1,4);
		TSM_ASSERT_EQUALS("Power has not been determined correctly", expectedValue, output->dataY());


		AnalysisDataService::Instance().remove("exponentWs");
		AnalysisDataService::Instance().remove("baseWs");
		AnalysisDataService::Instance().remove("OutputWsp");
	}

	void testSignedExponent()
	{
		WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
		WorkspaceSingleValue_sptr exponentWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(-2);
		AnalysisDataService::Instance().add("baseWs",baseWs);
		AnalysisDataService::Instance().add("exponentWs",exponentWs);

		Power alg;
		alg.initialize();

		alg.setPropertyValue("OutputWorkspace","OutputWs");
		alg.setPropertyValue("RHSWorkspace", "exponentWs");
		alg.setPropertyValue("LHSWorkspace", "baseWs");
		alg.execute();
		TSM_ASSERT_EQUALS("Algorithm should not run with a negative exponent", alg.isExecuted(),false);


		AnalysisDataService::Instance().remove("exponentWs");
		AnalysisDataService::Instance().remove("baseWs");
		AnalysisDataService::Instance().remove("OutputWs");
	}

	void testMultivaluedWorkspaceAsExponent()
	{
		WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
		Workspace1D_sptr exponentWs = WorkspaceCreationHelper::Create1DWorkspaceRand(10);
		AnalysisDataService::Instance().add("baseWs",baseWs);
		AnalysisDataService::Instance().add("exponentWs",exponentWs);

		Power alg;
		alg.initialize();

		alg.setPropertyValue("OutputWorkspace","OutputWs");
		alg.setPropertyValue("RHSWorkspace", "exponentWs");
		alg.setPropertyValue("LHSWorkspace", "baseWs");
		alg.execute();
		TSM_ASSERT_EQUALS("Exponent must be a single valued workspace", alg.isExecuted(),false);


		AnalysisDataService::Instance().remove("exponentWs");
		AnalysisDataService::Instance().remove("baseWs");
		AnalysisDataService::Instance().remove("OutputWs");
	}

	void testPowerErrorCalculation()
	{
		//Workspace creation helper creates input error as sqrt of input value. So input error = 2.

		//if x = p ^ y, then err_x = y * x * err_p / p

		WorkspaceSingleValue_sptr baseWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4);
		WorkspaceSingleValue_sptr exponentWs = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
		AnalysisDataService::Instance().add("baseWs",baseWs);
		AnalysisDataService::Instance().add("exponentWs",exponentWs);

		Power alg;
		alg.initialize();

		alg.setPropertyValue("OutputWorkspace","OutputWs");
		alg.setPropertyValue("RHSWorkspace", "exponentWs");
		alg.setPropertyValue("LHSWorkspace", "baseWs");
		alg.execute();

		WorkspaceSingleValue_sptr output =  boost::dynamic_pointer_cast<WorkspaceSingleValue>(AnalysisDataService::Instance().retrieve("OutputWs"));
		Mantid::MantidVec expectedErrorValue(1,16);
		TSM_ASSERT_EQUALS("Power algorithm error has not been determined properly.", expectedErrorValue, output->dataE());

		AnalysisDataService::Instance().remove("exponentWs");
		AnalysisDataService::Instance().remove("baseWs");
		AnalysisDataService::Instance().remove("OutputWs");
	}
};

#endif
