#ifndef MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_
#define MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CalculateChiSquared.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/EmptyValues.h"

using Mantid::CurveFitting::CalculateChiSquared;
using namespace Mantid;
using namespace Mantid::API;

class CalculateChiSquaredTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateChiSquaredTest *createSuite() { return new CalculateChiSquaredTest(); }
  static void destroySuite( CalculateChiSquaredTest *suite ) { delete suite; }


  void test_Init()
  {
    CalculateChiSquared alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    Tester tester;
    tester.set1DFunction();
    tester.set1DSpectrumEmpty();
    tester.runAlgorithm();
  }
  
private:

  class Tester{

    // input parameters
    size_t nParams;
    size_t nData;
    bool isHisto;
    double xMin;
    double xMax;
    std::vector<double> xBins;
    std::vector<double> xValues;

    // values for algorithm input properties
    IFunction_sptr function;
    Workspace_sptr workspace;
    int workspaceIndex;
    double StartX;
    double EndX;

    // algorithm output
    double chiSquared;

    void makeXValues(){
      size_t dlt = isHisto ? 1 : 0;
      xBins.resize(nData + dlt);
      double dx = (xMax - xMin) / double(xBins.size() - 1);
      for(size_t i = 0; i < xBins.size(); ++i)
      {
        xBins[i] = xMin + i * dx;
      }
      if (isHisto){
        xValues.resize(nData);
        std::transform(xBins.begin(),xBins.end()-1, xValues.begin(), std::bind2nd(std::plus<double>(),dx/2));
      } else {
        xValues = xBins;
      }
    }

  public:
    Tester(size_t np = 3, size_t nd = 10, bool histo = true)
        : nParams(np), nData(nd), isHisto(histo), workspaceIndex(0), xMin(-10),
          xMax(10), StartX(EMPTY_DBL()), EndX(EMPTY_DBL()) {}

    void runAlgorithm(){

      CalculateChiSquared alg;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )
      TS_ASSERT_THROWS_NOTHING( alg.setProperty("Function", function) );
      TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", workspace) );
      TS_ASSERT_THROWS_NOTHING( alg.execute(); );
      TS_ASSERT( alg.isExecuted() );

      chiSquared = alg.getProperty("ChiSquared");
    }

    void set1DFunction(){
      const std::string fun = "name=UserFunction,Formula=a+b*x+c*x^2,a=1,b=1,c=1";
      function = FunctionFactory::Instance().createInitialized(fun);
      TS_ASSERT_EQUALS(function->nParams(), nParams);
    }

    void set1DSpectrumEmpty(){
      makeXValues();
      const size_t nSpec = 1;
      auto space = WorkspaceFactory::Instance().create("Workspace2D", nSpec, nData+1, nData);
      space->dataX(0).assign(xBins.begin(),xBins.end());
      workspace = space;
    }

  };

};


#endif /* MANTID_CURVEFITTING_CALCULATECHISQUAREDTEST_H_ */