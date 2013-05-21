#ifndef UNGROUPWORKSPACESTEST_H_
#define UNGROUPWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/UnGroupWorkspace.h"
#include "MantidDataHandling/LoadRaw3.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class UnGroupWorkspaceTest : public CxxTest::TestSuite
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

	void testInit()
	{
        // some other test didn't clean after itself :(
        AnalysisDataService::Instance().clear();
	    UnGroupWorkspace alg2;
		TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
		TS_ASSERT( alg2.isInitialized() );

		const std::vector<Property*> props = alg2.getProperties();
		TS_ASSERT_EQUALS( props.size(), 1 );

		TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" );
		TS_ASSERT( props[0]->isDefault() );
		
	}

	void testExecUnGroupSingleGroupWorkspace()
	{
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","CSP79590.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","CSP79590"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );
		
		UnGroupWorkspace ungrpwsalg;
		ungrpwsalg.initialize();
		//std::vector<std::string >input;
		//input.push_back("CSP79590");
		std::string input="CSP79590";
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.setProperty("InputWorkspace",input));
		TS_ASSERT_THROWS_NOTHING( ungrpwsalg.execute());
		TS_ASSERT( ungrpwsalg.isExecuted() );
		//EVS13895 gets deleted,so test it
		WorkspaceGroup_sptr result;
		TS_ASSERT_THROWS( result = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("CSP79590"),std::runtime_error );

		Workspace_sptr result1;
		TS_ASSERT_THROWS_NOTHING( result1 = AnalysisDataService::Instance().retrieveWS<Workspace>("CSP79590_1") );
		TS_ASSERT_THROWS_NOTHING( result1 = AnalysisDataService::Instance().retrieveWS<Workspace>("CSP79590_2") );
		
	}
	
	void testExecUnGroupOneNormalWorkspace()
	{
  	LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","LOQ48098.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","LOQ48098"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );
		
		
		UnGroupWorkspace ungrpwsalg;
		ungrpwsalg.initialize();
		/*std::vector<std::string >input;
		input.push_back("LOQ48098");*/
		std::string input="LOQ48098";
		TS_ASSERT_THROWS( ungrpwsalg.setProperty("InputWorkspace",input), std::invalid_argument );
		//this throws exception as selcted workspace is not a group ws
		TS_ASSERT_THROWS( ungrpwsalg.execute(), std::runtime_error );
		TS_ASSERT(! ungrpwsalg.isExecuted() );
							
	}
	
};
#endif
