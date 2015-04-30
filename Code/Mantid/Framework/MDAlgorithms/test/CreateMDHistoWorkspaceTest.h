#ifndef MANTID_MDEVENTS_CREATEMDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_CREATEMDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/CreateMDHistoWorkspace.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;

class CreateMDHistoWorkspaceTest : public CxxTest::TestSuite
{

private:

  /**
  Helper method to make a standard algorithm. Properties on this algorithm can be overriden in individual tests.
  */
  IAlgorithm_sptr make_standard_algorithm(const std::string& outWSName)
  {
    IAlgorithm_sptr alg = boost::make_shared<CreateMDHistoWorkspace>();
    alg->initialize();
    alg->setRethrows(true);
    alg->setPropertyValue("SignalInput", "1,2,3");
    alg->setPropertyValue("ErrorInput", "0,0.1,0.2");
    alg->setProperty("Dimensionality", 1);
    alg->setPropertyValue("NumberOfBins", "3");
    alg->setPropertyValue("Extents", "-1,1");
    alg->setPropertyValue("Names", "A");
    alg->setPropertyValue("Units", "U");
    alg->setPropertyValue("OutputWorkspace", outWSName);
    return alg;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateMDHistoWorkspaceTest *createSuite() { return new CreateMDHistoWorkspaceTest(); }
  static void destroySuite( CreateMDHistoWorkspaceTest *suite ) { delete suite; }

  void test_catagory()
  {
    CreateMDHistoWorkspace alg;
    TS_ASSERT_EQUALS("MDAlgorithms", alg.category());
  }

  void test_name()
  {
    CreateMDHistoWorkspace alg;
    TS_ASSERT_EQUALS("CreateMDHistoWorkspace", alg.name());
  }

  void test_Init()
  {
    CreateMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_throws_if_wrong_number_of_signal_values()
  {
    std::string outWSName = "test_ws";
    IAlgorithm_sptr alg = make_standard_algorithm(outWSName);
    alg->setProperty("SignalInput", "1"); //Only one signal value provided, but NumberOfBins set to 5!
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_throws_if_wrong_number_of_error_values()
  {
    std::string outWSName = "test_ws";
    IAlgorithm_sptr alg = make_standard_algorithm(outWSName);
    alg->setProperty("ErrorInput", "1"); //Only one error value provided, but NumberOfBins set to 5!
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_1D()
  {
    // Name of the output workspace.
    std::string outWSName("CreateMDHistoWorkspaceTest_OutputWS");
  
    CreateMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("SignalInput", "1,2,3,4,5") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ErrorInput", "0,0.1,0.2,0.3,0.4") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Dimensionality", 1) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("NumberOfBins", "5") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Extents", "-1,1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Names", "A") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Units", "U") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    MDHistoWorkspace_sptr outWs;
    TS_ASSERT_THROWS_NOTHING( outWs = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(outWSName) );
    TS_ASSERT(outWs);
    if (!outWs) return;
    
     // Check the dimensionality
    TS_ASSERT_EQUALS(1, outWs->getNumDims());
    auto dim1 = outWs->getDimension(0);

    TS_ASSERT_EQUALS("A", dim1->getName());
    TS_ASSERT_EQUALS("A", dim1->getDimensionId());
    TS_ASSERT_EQUALS("U", dim1->getUnits().ascii());
    TS_ASSERT_EQUALS(1, dim1->getMaximum());
    TS_ASSERT_EQUALS(-1, dim1->getMinimum());
    TS_ASSERT_EQUALS(5, dim1->getNBins());

    // Check the data
    double* signals = outWs->getSignalArray();
    TS_ASSERT_DELTA(1, signals[0], 0.0001); // Check the first signal value
    TS_ASSERT_DELTA(2, signals[1], 0.0001); // Check the second signal value
    double* errorsSQ = outWs->getErrorSquaredArray();
    TS_ASSERT_DELTA(0, errorsSQ[0], 0.0001); // Check the first error sq value
    TS_ASSERT_DELTA(0.01, errorsSQ[1], 0.0001); // Check the second error sq value
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec_2D()
  {
    // Name of the output workspace.
    std::string outWSName("CreateMDHistoWorkspaceTest_OutputWS");
  
    CreateMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("SignalInput", "1,2,3,4,5,6") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ErrorInput", "0,0.1,0.2,0.3,0.4,0.5") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Dimensionality", 2) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("NumberOfBins", "2,3") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Extents", "-1,1,-1,1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Names", "A,B") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Units", "U,U") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    MDHistoWorkspace_sptr outWs;
    TS_ASSERT_THROWS_NOTHING( outWs = AnalysisDataService::Instance().retrieveWS<MDHistoWorkspace>(outWSName) );
    TS_ASSERT(outWs);
    if (!outWs) return;
    
     // Check the dimensionality
    TS_ASSERT_EQUALS(2, outWs->getNumDims());
    auto dim1 = outWs->getDimension(0);
    auto dim2 = outWs->getDimension(1);

    TS_ASSERT_EQUALS(2, dim1->getNBins());
    TS_ASSERT_EQUALS(3, dim2->getNBins());

    // Check the data
    double* signals = outWs->getSignalArray();
    TS_ASSERT_DELTA(1, signals[0], 0.0001); // Check the first signal value
    TS_ASSERT_DELTA(2, signals[1], 0.0001); // Check the second signal value
    double* errorsSQ = outWs->getErrorSquaredArray();
    TS_ASSERT_DELTA(0, errorsSQ[0], 0.0001); // Check the first error sq value
    TS_ASSERT_DELTA(0.01, errorsSQ[1], 0.0001); // Check the second error sq value
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  


};


#endif /* MANTID_MDEVENTS_CREATEMDHISTOWORKSPACETEST_H_ */
