#ifndef GROUPWORKSPACESTEST_H_
#define GROUPWORKSPACESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidAlgorithms/FindPeaks.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidCurveFitting/GaussianLinearBG1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class GroupWorkspacesTest : public CxxTest::TestSuite
{
public:
	void testName()
	{
		GroupWorkspaces grpwsalg;
		TS_ASSERT_EQUALS( grpwsalg.name(), "GroupWorkspaces" );
	}

	void testVersion()
	{
		GroupWorkspaces grpwsalg;
		TS_ASSERT_EQUALS( grpwsalg.version(), 1 );
	}

	void testCategory()
	{
		GroupWorkspaces grpwsalg;
		TS_ASSERT_EQUALS( grpwsalg.category(), "DataHandling" );
	}

	void testInit()
	{
		Mantid::Algorithms::GroupWorkspaces alg2;
		TS_ASSERT_THROWS_NOTHING( alg2.initialize() );
		TS_ASSERT( alg2.isInitialized() );

		const std::vector<Property*> props = alg2.getProperties();
		TS_ASSERT_EQUALS( props.size(), 2 );

		TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspaces" );
		TS_ASSERT( props[0]->isDefault() );
		
		TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" );
		TS_ASSERT( props[1]->isDefault() );
		TS_ASSERT( dynamic_cast<WorkspaceProperty<WorkspaceGroup>* >(props[1]) );

	
	}

	void testExecGroupOneNormalWorkspace()
	{
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48097.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","LOQ48097"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );
		
        GroupWorkspaces grpwsalg;
		grpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("LOQ48097");
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("InputWorkspaces",input));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("OutputWorkspace","NewGroup"));
		//only one workspace selected.,so it would throw
		TS_ASSERT_THROWS_NOTHING( grpwsalg.execute());
		TS_ASSERT( !grpwsalg.isExecuted() );//fail
	}
	void testExecGroupTwoNormalWorkspaces()
	{
		std::string s;
		std::getline(std::cin,s);
		
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48097.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","LOQ48097"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );

		LoadRaw3 alg1;
		alg1.initialize();
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48098.raw"));
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("OutputWorkspace","LOQ48098"));
		TS_ASSERT_THROWS_NOTHING( alg1.execute());
		TS_ASSERT( alg1.isExecuted() );

        GroupWorkspaces grpwsalg;
		grpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("LOQ48097");
		input.push_back("LOQ48098");
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("InputWorkspaces",input));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("OutputWorkspace","NewGroup"));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.execute());
		TS_ASSERT( grpwsalg.isExecuted() );
		WorkspaceGroup_sptr result;
		TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("NewGroup")) );
		std::vector<std::string> grpVec=result->getNames();
		TS_ASSERT_EQUALS(grpVec.size(),2);

		Workspace_sptr result1;
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("LOQ48097")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("LOQ48098")) );
		AnalysisDataService::Instance().remove("NewGroup");
		AnalysisDataService::Instance().remove("LOQ48097");
		AnalysisDataService::Instance().remove("LOQ48098");
	}
	void testExecGroupThreeNormalWorkspaces()
	{
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48094.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","LOQ48094"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );

		LoadRaw3 alg1;
		alg1.initialize();
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48098.raw"));
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("OutputWorkspace","LOQ48098"));
		TS_ASSERT_THROWS_NOTHING( alg1.execute());
		TS_ASSERT( alg1.isExecuted() );

		LoadRaw3 alg2;
		alg2.initialize();
		TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48097.raw"));
		TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("OutputWorkspace","LOQ48097"));
		TS_ASSERT_THROWS_NOTHING( alg2.execute());
		TS_ASSERT( alg2.isExecuted() );

		GroupWorkspaces grpwsalg;
		grpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("LOQ48094");
		input.push_back("LOQ48098");
		input.push_back("LOQ48097");
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("InputWorkspaces",input));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("OutputWorkspace","NewGroup"));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.execute());
		TS_ASSERT( grpwsalg.isExecuted() );
		WorkspaceGroup_sptr result;
		TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("NewGroup")) );
		std::vector<std::string> grpVec=result->getNames();
		TS_ASSERT_EQUALS(grpVec.size(),4);
		Workspace_sptr result1;
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("LOQ48094")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("LOQ48098")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("LOQ48097")) );

		AnalysisDataService::Instance().remove("NewGroup");
		AnalysisDataService::Instance().remove("LOQ48094");
		AnalysisDataService::Instance().remove("LOQ48098");
		AnalysisDataService::Instance().remove("LOQ48097");
	}
	void testExecGroupNormalWorkspaceandGroupWorkspace()
	{
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/AutoTestData/EVS13895.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","EVS13895"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );

		LoadRaw3 alg1;
	    alg1.initialize();
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48098.raw"));
		TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("OutputWorkspace","LOQ48098"));
		TS_ASSERT_THROWS_NOTHING( alg1.execute());
		TS_ASSERT( alg1.isExecuted() );

		GroupWorkspaces grpwsalg;
		grpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("EVS13895_1");
		input.push_back("EVS13895_2");
		input.push_back("EVS13895_3");
		input.push_back("EVS13895_4");
		input.push_back("EVS13895_5");
		input.push_back("EVS13895_6");
		input.push_back("LOQ48098");

		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("InputWorkspaces",input));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("OutputWorkspace","NewGroup"));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.execute());
		TS_ASSERT( grpwsalg.isExecuted() );
		WorkspaceGroup_sptr result;
		TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("NewGroup")) );
		std::vector<std::string> grpVec=result->getNames();
		TS_ASSERT_EQUALS(grpVec.size(),8);
		Workspace_sptr result1;
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_1")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_2")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_3")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_4")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_5")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("EVS13895_6")) );
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<Workspace>(AnalysisDataService::Instance().retrieve("LOQ48098")) );

		AnalysisDataService::Instance().remove("NewGroup");
		AnalysisDataService::Instance().remove("EVS13895_1");
		AnalysisDataService::Instance().remove("EVS13895_2");
		AnalysisDataService::Instance().remove("EVS13895_3");
		AnalysisDataService::Instance().remove("EVS13895_4");
		AnalysisDataService::Instance().remove("EVS13895_5");
		AnalysisDataService::Instance().remove("EVS13895_6");
		AnalysisDataService::Instance().remove("LOQ48098");
	}
	void testExecGroupTwoIncompatibleWorkspaces()
	{
		/*std::string s;
		std::getline(std::cin,s);*/
		LoadRaw3 alg;
		alg.initialize();
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("FileName","../../../../Test/AutoTestData/LOQ48094.raw"));
		TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace","LOQ48094"));
		TS_ASSERT_THROWS_NOTHING( alg.execute());
		TS_ASSERT( alg.isExecuted() );

		Mantid::NeXus::LoadNexusProcessed loader;
		loader.initialize();
		loader.setProperty("Filename","../../../../Test/AutoTestData/focussed.nxs");
		loader.setProperty("OutputWorkspace","peaksWS");
		TS_ASSERT_THROWS_NOTHING(loader.execute());

		Mantid::Algorithms::FindPeaks finder;
		finder.initialize();
		TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("InputWorkspace","peaksWS"));
		TS_ASSERT_THROWS_NOTHING(finder.setPropertyValue("PeaksList","foundpeaks"));
		TS_ASSERT_THROWS_NOTHING( finder.execute());
		ITableWorkspace_sptr result1;
		TS_ASSERT_THROWS_NOTHING( result1 = boost::dynamic_pointer_cast<ITableWorkspace>(AnalysisDataService::Instance().retrieve("foundpeaks")) );
		TS_ASSERT( finder.isExecuted() );

        GroupWorkspaces grpwsalg;
		grpwsalg.initialize();
		std::vector<std::string >input;
		input.push_back("LOQ48094");
		input.push_back("foundpeaks");
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("InputWorkspaces",input));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.setProperty("OutputWorkspace","NewGroup"));
		TS_ASSERT_THROWS_NOTHING( grpwsalg.execute());
		TS_ASSERT( !grpwsalg.isExecuted() );
				
	}

	private:
	//Mantid::Algorithms::GroupWorkspaces grpwsalg;
	
};
#endif
