#ifndef UNGROUPWORKSPACESTEST_H_
#define UNGROUPWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/UnGroupWorkspaces.h"
#include "MantidDataHandling/LoadRaw3.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class UnGroupWorkspacesTest : public CxxTest::TestSuite
{
public:
	void testName()
	{
		UnGroupWorkspace ungrpwsalg;
		TS_ASSERT_EQUALS( ungrpwsalg.name(), "UnGroupWorkspace" );
	}

	void testVersion()
	{
		UnGroupWorkspace ungrpwsalg;
		TS_ASSERT_EQUALS( ungrpwsalg.version(), 1 );
	}

	void testCategory()
	{
		UnGroupWorkspace ungrpwsalg;
		TS_ASSERT_EQUALS( ungrpwsalg.category(), "DataHandling" );
	}

	void testInit()
	{
	    UnGroupWorkspace alg2;
		TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
		TS_ASSERT( alg2.isInitialized() );

		const std::vector<Property*> props = alg2.getProperties();
		TS_ASSERT_EQUALS( props.size(), 1 );

		TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspaces" );
		TS_ASSERT( props[0]->isDefault() );
		
	}

	void testExecUnGroupSingleGroupWorkspace()
	{
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/Data/EVS13895.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","EVS13895"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );
		
		UnGroupWorkspace ungrpwsalg;
		ungrpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("EVS13895");
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.setProperty("InputWorkspaces",input));
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.execute());
		TS_ASSERT( ungrpwsalg.isExecuted() );
		//EVS13895 gets deleted,so test it
		WorkspaceGroup_sptr result;
		TS_ASSERT_THROWS( result = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("EVS13895")),std::runtime_error );

		Workspace_sptr result1;
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_1")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_2")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_3")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_4")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_5")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_6")) );

		
	}
	void testExecUnGroupTwoGroupWorkspaces()
	{
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/Data/EVS13895.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","EVS13895"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );

		LoadRaw3 alg1;
		alg1.initialize();
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("FileName","../../../../Test/Data/EVS13908.raw"));
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("OutputWorkspace","EVS13908"));
		TS_ASSERT_THROWS_NOTHING( alg1.execute());
		TS_ASSERT( alg1.isExecuted() );
		
		UnGroupWorkspace ungrpwsalg;
		ungrpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("EVS13895");
		input.push_back("EVS13908");
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.setProperty("InputWorkspaces",input));
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.execute());
		TS_ASSERT( ungrpwsalg.isExecuted() );
		//EVS13895,EVS13908 gets deleted,so test it
		WorkspaceGroup_sptr result;
		TS_ASSERT_THROWS( result = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("EVS13895")),std::runtime_error );
		TS_ASSERT_THROWS( result = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("EVS13908")),std::runtime_error );

		Workspace_sptr result1;
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_1")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_2")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_3")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_4")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_5")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_6")) );

		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13908_1")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13908_2")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13908_3")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13908_4")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13908_5")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13908_6")) );
	
	}
	void testExecUnGroupOneNormalWorkspace()
	{
	  	LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/Data/LOQ48098.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","LOQ48098"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );
		
		
		UnGroupWorkspace ungrpwsalg;
		ungrpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("LOQ48098");
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.setProperty("InputWorkspaces",input));
		//this throws exception as selcted workspace is not a group ws
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.execute());
		TS_ASSERT(! ungrpwsalg.isExecuted() );
							
	}
	
};
#endif
