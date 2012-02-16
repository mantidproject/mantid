#ifndef MANTID_DATAHANDLING_LIVEDATAALGORITHMTEST_H_
#define MANTID_DATAHANDLING_LIVEDATAALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/LiveDataAlgorithm.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

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
  
  void test_Something()
  {
  }


};


#endif /* MANTID_DATAHANDLING_LIVEDATAALGORITHMTEST_H_ */
