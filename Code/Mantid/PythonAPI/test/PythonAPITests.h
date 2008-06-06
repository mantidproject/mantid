#ifndef MANTID_PYTHONAPITESTS_H_
#define MANTID_PYTHONAPITESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidAPI/Workspace.h"

using namespace Mantid::PythonAPI;

class PythonAPITest : public CxxTest::TestSuite
{

private:
	PythonInterface* inter;

public:

	PythonAPITest()
	{
		inter = new PythonInterface();
	}

	//void testFrameworkInitialise()
	//{
	//	TS_ASSERT_THROWS_NOTHING(inter->InitialiseFrameworkManager());
	//}

	void testCreateAlgorithm()
	{
		TS_ASSERT(inter->CreateAlgorithm("HelloWorldAlgorithm"));
	}

	void testCreateAlgorithmNotFoundThrows()
	{
		TS_ASSERT_THROWS_ANYTHING(inter->CreateAlgorithm("Rubbish!"));
	}

	void testLoadIsisRaw()
	{
		Mantid::API::Workspace_sptr ws = inter->LoadIsisRawFile(
				"../../../../Test/Data/HET15869.RAW", "TestWorkspace1");

		TS_ASSERT(ws.use_count() > 0);
	}
	
	void testGetHistogramNumber()
	{
		Mantid::API::Workspace_sptr ws = inter->RetrieveWorkspace("TestWorkspace1");
		TS_ASSERT_EQUALS(ws->getHistogramNumber(), 2584);
	}
	
	void testGetBinNumber()
	{
		Mantid::API::Workspace_sptr ws = inter->RetrieveWorkspace("TestWorkspace1");

		TS_ASSERT_EQUALS(ws->blocksize(), 1675);
	}
	
	void testGetWorkspaceNames()
	{
		std::vector<std::string> temp = inter->GetWorkspaceNames();

		TS_ASSERT(!temp.empty());
	}
	
	void testGetAlgorithmNames()
	{
		std::vector<std::string> temp = inter->GetAlgorithmNames();

		TS_ASSERT(!temp.empty());
	}

	void testGetXdata()
	{
		std::vector<double>* data = inter->GetXData("TestWorkspace1", 0);

		TS_ASSERT(!data->empty());
	}

	void testGetYdata()
	{
		std::vector<double>* data = inter->GetYData("TestWorkspace1", 0);

		TS_ASSERT(!data->empty());
	}

	void testGetEdata()
	{
		std::vector<double>* data = inter->GetEData("TestWorkspace1", 0);

		TS_ASSERT(!data->empty());
	}
	
	void testTryDuplicatingWorkspaceName()
	{
		Mantid::API::Workspace_sptr ws = inter->LoadIsisRawFile(
				"../../../../Test/Data/HET15869.RAW", "TestWorkspace1");

		TS_ASSERT_EQUALS(ws.use_count(), 0);
	}
	
	void testDeleteWorkspace()
	{
		inter->DeleteWorkspace("TestWorkspace1");

		std::vector<std::string> temp = inter->GetWorkspaceNames();

		TS_ASSERT(temp.empty());
	}

};

#endif /*MANTID_PYTHONAPITESTS_H_*/
