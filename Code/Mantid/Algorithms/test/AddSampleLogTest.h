#ifndef ADDSAMPLELOGTEST_H_
#define ADDSAMPLELOGTEST_H_

#include <cxxtest/TestSuite.h>

#include <string>

#include "MantidDataObjects/Workspace2D.h"
#include "MantidAlgorithms/AddSampleLog.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Algorithms;

class AddSampleLogTest : public CxxTest::TestSuite
{
public:

	void testInsertion()
	{
		AddSampleLog alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize());
		TS_ASSERT( alg.isInitialized() )

		Workspace2D_sptr testWS = makeDummyWorkspace2D();

    alg.setPropertyValue("Workspace", "AddSampleLogTest_Temporary");
		alg.setPropertyValue("LogName", "my name");
		alg.setPropertyValue("LogText", "my data");

    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT( alg.isExecuted() )

    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(alg.getProperty("Workspace")));
    
    Sample wSpaceSam = output->sample();
    PropertyWithValue<std::string> *testProp =
      dynamic_cast<PropertyWithValue<std::string>*>(wSpaceSam.getLogData("my name"));
    
    TS_ASSERT(testProp)
    TS_ASSERT_EQUALS(testProp->value(), "my data")
    
	}

	Workspace2D_sptr makeDummyWorkspace2D()
	{
		Workspace2D_sptr testWorkspace(new Workspace2D);
    AnalysisDataService::Instance().add("AddSampleLogTest_Temporary", testWorkspace);
		return testWorkspace;
	}


};

#endif /*ADDSAMPLELOGTEST_H_*/
