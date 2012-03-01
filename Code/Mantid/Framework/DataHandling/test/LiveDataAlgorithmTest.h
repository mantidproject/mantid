#ifndef MANTID_DATAHANDLING_LIVEDATAALGORITHMTEST_H_
#define MANTID_DATAHANDLING_LIVEDATAALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidDataHandling/LiveDataAlgorithm.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

//------------------------------------------------------------------------------------------------
/** Concrete declaration of LiveDataAlgorithm for testing */
class LiveDataAlgorithmImpl : public LiveDataAlgorithm
{
  // Make all the members public so I can test them.
  friend class LiveDataAlgorithmTest;
public:
  virtual const std::string name() const { return "LiveDataAlgorithmImpl";};
  virtual int version() const { return 1;};
  virtual const std::string category() const { return "Testing";}
  void init()
  { this->initProps(); }
  void exec() {}
};


class LiveDataAlgorithmTest : public CxxTest::TestSuite
{
public:

  void test_initProps()
  {
    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING( alg.initProps() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("LiveDataAlgorithmTest_OutputWS");

    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("StartTime", "2010-09-14T04:20:12.95") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );

    TS_ASSERT( !alg.hasPostProcessing() );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PostProcessingAlgorithm", "RenameWorkspace") );
    TS_ASSERT( alg.hasPostProcessing() );

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_validateInputs()
  {
    LiveDataAlgorithmImpl alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT( !alg.hasPostProcessing() );

    TSM_ASSERT_THROWS_ANYTHING("No OutputWorkspace",  alg.validateInputs() );
    alg.setPropertyValue("OutputWorkspace", "out_ws");
    TSM_ASSERT_THROWS_NOTHING("Is OK now",  alg.validateInputs() );

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PostProcessingScript", "Pause(1)") );
    TS_ASSERT( alg.hasPostProcessing() );

    TSM_ASSERT_THROWS_ANYTHING("No AccumulationWorkspace",  alg.validateInputs() );
    alg.setPropertyValue("AccumulationWorkspace", "accum_ws");
    TSM_ASSERT_THROWS_NOTHING("Is OK now",  alg.validateInputs() );

    alg.setPropertyValue("AccumulationWorkspace", "out_ws");
    TSM_ASSERT_THROWS_ANYTHING("AccumulationWorkspace == OutputWorkspace",  alg.validateInputs() );
  }

  /** Test creating the processing algorithm.
   * NOTE: RunPythonScript is not available from unit tests, so
   * this is tested in LoadLiveDataTest.py
   */
  void test_makeAlgorithm()
  {
    FrameworkManager::Instance();
    AlgorithmManager::Instance();
    for (int post=0; post<2; post++)
    {
      // Try both the regular and the Post-Processing algorithm
      std::string prefix="";
      if (bool(post))
        prefix = "Post";
      std::cout << prefix << "Processing algo" << std::endl;

      Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(5, 10);
      AnalysisDataService::Instance().addOrReplace("first", ws);
      AnalysisDataService::Instance().remove("second");

      LiveDataAlgorithmImpl alg;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )

      IAlgorithm_sptr procAlg;
      procAlg = alg.makeAlgorithm( bool(post) );
      TSM_ASSERT("NULL algorithm pointer returned if nothing is specified.", !procAlg);

      TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(prefix + "ProcessingAlgorithm", "RenameWorkspace") );
      TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(prefix + "ProcessingProperties",
          "InputWorkspace=first;OutputWorkspace=second") );

      procAlg = alg.makeAlgorithm( bool(post) );
      TSM_ASSERT("Non-NULL algorithm pointer", procAlg);
      TS_ASSERT( procAlg->isInitialized() );
      TS_ASSERT_EQUALS( procAlg->getPropertyValue("InputWorkspace"), "first" );
      TS_ASSERT_EQUALS( procAlg->getPropertyValue("OutputWorkspace"), "second" );

      // Just so the ADS gets updated properly.
      procAlg->setChild(false);
      // Run the algorithm and check that it was done correctly
      procAlg->execute();
      TS_ASSERT( !AnalysisDataService::Instance().doesExist("first") );
      TS_ASSERT( AnalysisDataService::Instance().doesExist("second") );
    }
  }


};


#endif /* MANTID_DATAHANDLING_LIVEDATAALGORITHMTEST_H_ */
