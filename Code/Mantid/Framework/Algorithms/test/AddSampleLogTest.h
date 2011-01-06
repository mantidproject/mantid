#ifndef ADDSAMPLELOGTEST_H_
#define ADDSAMPLELOGTEST_H_

#include <cxxtest/TestSuite.h>

#include <string>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/AddSampleLog.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class AddSampleLogTest : public CxxTest::TestSuite
{
public:

  void testInsertion2D()
  {
    ExecuteAlgorithm(WorkspaceCreationHelper::Create2DWorkspace(10,10));
  }

  void testInsertionEvent()
  {
    ExecuteAlgorithm(WorkspaceCreationHelper::CreateEventWorkspace(10,10));
  }

  void ExecuteAlgorithm(MatrixWorkspace_sptr testWS)
  {
    //add the workspace to the ADS
    AnalysisDataService::Instance().add("AddSampleLogTest_Temporary", testWS);

    //execute algorithm
    AddSampleLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() )

    alg.setPropertyValue("Workspace", "AddSampleLogTest_Temporary");
    alg.setPropertyValue("LogName", "my name");
    alg.setPropertyValue("LogText", "my data");

    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT( alg.isExecuted() )

    //check output
    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(alg.getProperty("Workspace")));
    
    const Run& wSpaceRun = output->run();
    PropertyWithValue<std::string> *testProp = dynamic_cast<PropertyWithValue<std::string>*>(wSpaceRun.getLogData("my name"));
    
    TS_ASSERT(testProp)
    TS_ASSERT_EQUALS(testProp->value(), "my data")
    //cleanup
    AnalysisDataService::Instance().remove(output->getName());
    
  }





};

#endif /*ADDSAMPLELOGTEST_H_*/
