#ifndef MANTID_PYTHONAPI_RUNPYTHONSCRIPTTEST_H_
#define MANTID_PYTHONAPI_RUNPYTHONSCRIPTTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidPythonAPI/RunPythonScript.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::PythonAPI;
using namespace Mantid::API;

class RunPythonScriptTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RunPythonScriptTest *createSuite() { return new RunPythonScriptTest(); }
  static void destroySuite( RunPythonScriptTest *suite ) { delete suite; }


  void test_Init()
  {
    RunPythonScript alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  MatrixWorkspace_sptr doRun(std::string Code, std::string outputName="outputName")
  {
    // Make an input workspace
    AnalysisDataService::Instance().clear();
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(10, 20);
    AnalysisDataService::Instance().addOrReplace("inputName", ws);

    // Run with that code
    RunPythonScript alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "inputName") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Code", Code) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outputName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr outWs;
    TS_ASSERT_THROWS_NOTHING( outWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputName) );
    TS_ASSERT(outWs);
    return outWs;
  }

  void test_doNothing()
  {
    // Empty code string.
    MatrixWorkspace_sptr ws = doRun("", "inputName");
  }
  

};


#endif /* MANTID_PYTHONAPI_RUNPYTHONSCRIPTTEST_H_ */
