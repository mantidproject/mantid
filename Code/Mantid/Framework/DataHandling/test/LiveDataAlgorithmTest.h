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

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_makeAlgorithm()
  {
    FrameworkManager::Instance();
    AlgorithmManager::Instance();
    for (int post=0; post<1; post++)
    {
      // Try both the regular and the Post-Processing algorithm
      std::string prefix="";
      if (bool(post))
        prefix = "Post";

      Workspace2D_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(5, 10);
      AnalysisDataService::Instance().addOrReplace("first", ws);
      AnalysisDataService::Instance().remove("second");

      LiveDataAlgorithmImpl alg;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )

      IAlgorithm * procAlg;
      procAlg = alg.makeAlgorithm( bool(post) );
      TSM_ASSERT("NULL algorithm pointer returned if nothing is specified.", !procAlg);

      TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(prefix + "ProcessingAlgorithm", "RenameWorkspace") );
      TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(prefix + "ProcessingProperties",
          "InputWorkspace=first;OutputWorkspace=second") );

      procAlg = alg.makeAlgorithm( bool(post) );
      TSM_ASSERT("Non-NULL algorithm pointer", procAlg);
      TS_ASSERT( procAlg->isInitialized() );
      TS_ASSERT_EQUALS( procAlg->getPropertyValue("InputWorkspace"), "first" );

      // Run the algorithm and check that it was done correctly
      procAlg->execute();
      TS_ASSERT( !AnalysisDataService::Instance().doesExist("first") );
      TS_ASSERT( AnalysisDataService::Instance().doesExist("second") );
    }

  }


};


#endif /* MANTID_DATAHANDLING_LIVEDATAALGORITHMTEST_H_ */
