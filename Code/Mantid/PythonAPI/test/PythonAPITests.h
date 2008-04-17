#ifndef MANTID_PYTHONAPITESTS_H_
#define MANTID_PYTHONAPITESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>

#include "MantidPythonAPI/PythonInterface.h"

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

	void testFrameworkInitialise()
	{
		TS_ASSERT_THROWS_NOTHING(inter->InitialiseFrameworkManager());
	}

	void testCreateAlgorithm()
	{
		TS_ASSERT(inter->CreateAlgorithm("HelloWorldAlgorithm"));
	}

	void testCreateAlgorithmNotFoundThrows()
	{
		TS_ASSERT_THROWS_ANYTHING(inter->CreateAlgorithm("Rubbish!"));
	}

	void testExecuteAlgorithm()
	{
		TS_ASSERT(inter->ExecuteAlgorithm("HelloWorldAlgorithm", ""));
	}

	void testExecuteAlgorithmNotFound()
	{
		TS_ASSERT_THROWS_ANYTHING(inter->ExecuteAlgorithm("Rubbish!", ""));
	}

	void testLoadIsisRaw()
	{
		int hists = inter->LoadIsisRawFile(
				"../../../../Test/Data/HET15869.RAW", "TestWorkspace1");

		TS_ASSERT_EQUALS(hists, 2584);
	}
	
	void testGetHistogramNumber()
	{
		int hists = inter->GetHistogramNumber("TestWorkspace1");

		TS_ASSERT_EQUALS(hists, 2584);
	}
	
	void testGetBinNumber()
	{
		int bins = inter->GetBinNumber("TestWorkspace1");

		TS_ASSERT_EQUALS(bins, 1676);
	}
	
	void testGetWorkspaceNames()
	{
		std::vector<std::string> temp = inter->GetWorkspaceNames();

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
		int hists = inter->LoadIsisRawFile(
				"../../../../Test/Data/HET15869.RAW", "TestWorkspace1");

		TS_ASSERT_EQUALS(hists, -1);
	}

};

#endif /*MANTID_PYTHONAPITESTS_H_*/
