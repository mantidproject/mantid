#ifndef MANTID_MDALGORITHMS_EVALUATEMDFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_EVALUATEMDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/EvaluateMDFunction.h"
#include "MantidMDAlgorithms/CreateMDHistoWorkspace.h"

using Mantid::MDAlgorithms::EvaluateMDFunction;
using Mantid::MDAlgorithms::CreateMDHistoWorkspace;
using namespace Mantid::API;

class EvaluateMDFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EvaluateMDFunctionTest *createSuite() { return new EvaluateMDFunctionTest(); }
  static void destroySuite( EvaluateMDFunctionTest *suite ) { delete suite; }


  void test_Init()
  {
    EvaluateMDFunction alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("EvaluateMDFunctionTest_OutputWS");

    const size_t nx = 3, ny = 4;
    IMDHistoWorkspace_sptr inputWorkspace = createInputWorkspace(nx,ny);
    TS_ASSERT( inputWorkspace );
    std::string funcStr = "name=UserFunctionMD,Formula=x+y";
  
    EvaluateMDFunction alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWorkspace) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Function", funcStr) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    IMDHistoWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    for(size_t i = 0; i < nx; ++i)
    for(size_t j = 0; j < ny; ++j)
    {
      size_t linearIndex = ws->getLinearIndex(i,j);
      auto v = ws->getCenter(linearIndex);
      auto x = v[0];
      auto y = v[1];
      auto f = ws->getSignalAt(linearIndex);
      TS_ASSERT_DELTA( f, x + y, 1e-15 );
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  IMDHistoWorkspace_sptr createInputWorkspace(size_t nx, size_t ny)
  {

    std::vector<double> values( nx * ny, 1.0 );
    std::vector<int> dims(2);
    dims[0] = static_cast<int>(nx);
    dims[1] = static_cast<int>(ny);

    CreateMDHistoWorkspace alg;
    alg.initialize();
    alg.setProperty("SignalInput", values);
    alg.setProperty("ErrorInput", values);
    alg.setProperty("Dimensionality", 2);
    alg.setProperty("NumberOfBins", dims);
    alg.setPropertyValue("Extents", "-1,1,-1,1");
    alg.setPropertyValue("Names", "A,B");
    alg.setPropertyValue("Units", "U,U");
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    IMDHistoWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>("out");
    AnalysisDataService::Instance().remove("out");
    return ws;
  }

};


#endif /* MANTID_MDALGORITHMS_EVALUATEMDFUNCTIONTEST_H_ */
