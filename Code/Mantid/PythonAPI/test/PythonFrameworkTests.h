#ifndef  MANTID_PYTHONFRAMEWORKTESTS_H_
#define  MANTID_PYTHONFRAMEWORKTESTS_H_

#include <vector>

#include <cxxtest/TestSuite.h>
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"

#include "MantidPythonAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
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

	void testCreateAlgorithmMethod1()
	{
		API::IAlgorithm* alg = mgr->createAlgorithm("HelloWorldAlgorithm");
		API::Algorithm* temp = dynamic_cast<API::Algorithm*>(alg);
		TS_ASSERT_EQUALS(temp->name(), "HelloWorldAlgorithm");
	}

	void testCreateAlgorithmNotFoundThrows()
	{
		TS_ASSERT_THROWS_ANYTHING(mgr->createAlgorithm("Rubbish!"));
	}

	void testGetDeleteWorkspace()
	{
    API::AnalysisDataService::Instance().add("TestWorkspace1",WorkspaceCreationHelper::Create2DWorkspace123(10,22,1));
    API::MatrixWorkspace* ws = dynamic_cast<API::MatrixWorkspace*>(mgr->getMatrixWorkspace("TestWorkspace1"));

		TS_ASSERT_EQUALS(ws->getNumberHistograms(), 22);
		TS_ASSERT(mgr->deleteWorkspace("TestWorkspace1"));
	}
	
	void testCreateAlgorithmMethod2()
	{
		API::IAlgorithm* alg = mgr->createAlgorithm("PropertyAlgorithm", "10;9.99");

    TS_ASSERT( alg->isInitialized() )
    TS_ASSERT( !alg->isExecuted() )
    TS_ASSERT_EQUALS( alg->getPropertyValue("IntValue"), "10" )
    TS_ASSERT_EQUALS( alg->getPropertyValue("DoubleValue"), "9.99" )
	}
	
	void testExecuteAlgorithmMethod()
	{
		API::IAlgorithm* alg = mgr->execute("PropertyAlgorithm", "8;8.88");
    TS_ASSERT_EQUALS( alg->getPropertyValue("IntValue"), "8" )
    TS_ASSERT( alg->isExecuted() )
	}

};

#endif /*MANTID_PYTHONFRAMEWORKTESTS_H_*/

