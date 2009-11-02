#ifndef RENAMEWORKSPACETEST_H_
#define RENAMEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/RenameWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class RenameWorkspaceTest : public CxxTest::TestSuite
{
public:
	void testName()
	{
		TS_ASSERT_EQUALS( alg.name(), "RenameWorkspace" );
	}

	void testVersion()
	{
		TS_ASSERT_EQUALS( alg.version(), 1 );
	}

	void testCategory()
	{
		TS_ASSERT_EQUALS( alg.category(), "DataHandling" );
	}

	void testInit()
	{
		Mantid::Algorithms::RenameWorkspace alg2;
		TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
		TS_ASSERT( alg2.isInitialized() );

		const std::vector<Property*> props = alg2.getProperties();
		TS_ASSERT_EQUALS( props.size(), 2 );

		TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" );
		TS_ASSERT( props[0]->isDefault() );
		TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[0]) );

		TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" );
		TS_ASSERT( props[1]->isDefault() );
		TS_ASSERT( dynamic_cast<WorkspaceProperty<Workspace>* >(props[1]) );

	
	}

	void testExec()
	{
		MatrixWorkspace_sptr inputWS = createWorkspace();
		AnalysisDataService::Instance().add("InputWS",inputWS);

		Mantid::Algorithms::RenameWorkspace alg3;
		alg3.initialize();
		TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS") );
		TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","WSRenamed") );

		TS_ASSERT_THROWS_NOTHING( alg3.execute() );
		TS_ASSERT( alg3.isExecuted() );

		Workspace_sptr result;
		TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("WSRenamed")) );
		TS_ASSERT( result );

		TS_ASSERT_THROWS_ANYTHING( result = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("InputWS")) );

    AnalysisDataService::Instance().remove("WSRenamed");
	}
	
	void testExecSameNames()
	{
		MatrixWorkspace_sptr inputWS = createWorkspace();
		AnalysisDataService::Instance().add("InputWS",inputWS);

		Mantid::Algorithms::RenameWorkspace alg3;
		alg3.initialize();
		TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("InputWorkspace","InputWS") );
		TS_ASSERT_THROWS_NOTHING( alg3.setPropertyValue("OutputWorkspace","InputWS") );

		TS_ASSERT_THROWS_NOTHING( alg3.execute() );
		TS_ASSERT( !alg3.isExecuted() );

		Workspace_sptr result;
		TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("InputWS")) );
		TS_ASSERT( result );

		AnalysisDataService::Instance().remove("InputWS");
	}

	MatrixWorkspace_sptr createWorkspace()
	{
		MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(4,4,0.5);
		return inputWS;
	}

private:
	Mantid::Algorithms::RenameWorkspace alg;

};

#endif /*RENAMEWORKSPACETEST_H_*/
