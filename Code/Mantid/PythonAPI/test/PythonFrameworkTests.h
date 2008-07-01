#ifndef  MANTID_PYTHONFRAMEWORKTESTS_H_
#define  MANTID_PYTHONFRAMEWORKTESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>

#include "MantidPythonAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"

using namespace Mantid;
using namespace Mantid::PythonAPI;

class PythonFrameworkTest : public CxxTest::TestSuite
{

private:
	Mantid::PythonAPI::FrameworkManager* mgr;

public:

	PythonFrameworkTest()
	{
		mgr = new Mantid::PythonAPI::FrameworkManager;
	}

	void testCreateAlgorithm()
	{
		API::IAlgorithm* alg = mgr->createAlgorithm("HelloWorldAlgorithm");
		API::Algorithm* temp = dynamic_cast<API::Algorithm*>(alg);
		TS_ASSERT_EQUALS(temp->name(), "HelloWorldAlgorithm");
	}

	void testCreateAlgorithmNotFoundThrows()
	{
		TS_ASSERT_THROWS_ANYTHING(mgr->createAlgorithm("Rubbish!"));
	}

	void testLoadIsisRaw()
	{
		API::IAlgorithm* alg = mgr->createAlgorithm("LoadRaw");
		TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("Filename", "../../../../Test/Data/HET15869.RAW"));
		TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", "TestWorkspace1"));

		TS_ASSERT_THROWS_NOTHING(alg->execute());
	}
	
	void testGetWorkspace()
	{
		API::Workspace* ws = mgr->getWorkspace("TestWorkspace1");

		TS_ASSERT_EQUALS(ws->getHistogramNumber(), 2584);
	}
	
	void testDeleteWorkspace()
	{
		TS_ASSERT(mgr->deleteWorkspace("TestWorkspace1"));
	}

};

#endif /*MANTID_PYTHONFRAMEWORKTESTS_H_*/

