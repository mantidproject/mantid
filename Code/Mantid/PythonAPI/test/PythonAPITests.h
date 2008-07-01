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

public:

	PythonAPITest()
	{
	}

	void testLoadIsisRaw()
	{
		Mantid::API::Workspace_sptr ws = LoadIsisRawFile(
				"../../../../Test/Data/HET15869.RAW", "TestWorkspace1");

		TS_ASSERT(ws.use_count() > 0);
	}
	
	void testGetWorkspaceNames()
	{
		std::vector<std::string> temp = GetWorkspaceNames();

		TS_ASSERT(!temp.empty());
	}
	
	void testGetAlgorithmNames()
	{
		std::vector<std::string> temp = GetAlgorithmNames();

		TS_ASSERT(!temp.empty());
	}

};

#endif /*MANTID_PYTHONAPITESTS_H_*/
