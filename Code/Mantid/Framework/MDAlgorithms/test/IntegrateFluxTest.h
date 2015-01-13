#ifndef MANTID_MDALGORITHMS_INTEGRATEFLUXTEST_H_
#define MANTID_MDALGORITHMS_INTEGRATEFLUXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/IntegrateFlux.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <numeric>

using Mantid::MDAlgorithms::IntegrateFlux;
using namespace Mantid::API;

namespace
{
  enum WorkspaceType { Tof, /*Weighted,*/ WeightedNoTime, Histogram, HistogramNonUniform, Distribution, PointData, PointDataNonUniform };

  struct TestingFunction
  {
    const MatrixWorkspace& workspace;
    WorkspaceType type;
    const double dx;
    TestingFunction(const MatrixWorkspace& ws, WorkspaceType t):
      workspace(ws),
      type(t),
      dx((ws.getXMax()-ws.getXMin())/ static_cast<double>(ws.blocksize()))
    {}
    double operator()(double x) const
    {
      switch(type)
      {
      case PointData: 
      case PointDataNonUniform:
          return x * x + x;
      case Distribution:
          return x * x / dx;
      case HistogramNonUniform:
        {
          double res = 0.0;
          auto& X = workspace.readX(0);
          auto& Y = workspace.readY(0);
          auto ix = std::lower_bound( X.begin(), X.end(), x );
          if ( ix != X.end() )
          {
            if ( x < *ix )
            {
              --ix;
              auto i = static_cast<size_t>(std::distance( X.begin(), ix ));
              res = Y[i] * (x - *ix) / (*(ix+1) - *ix);
            }
          }
          else
          {
            --ix;
          }
          auto i = static_cast<size_t>(std::distance( X.begin(), ix ));
          res += std::accumulate( Y.begin(), Y.begin() + i, 0.0 );
          return res;
        }
      default:
          x /= dx;
          return x * x;
      }
      throw std::logic_error("Cannot test this workspace type.");
    }
  };
}

class IntegrateFluxTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateFluxTest *createSuite() { return new IntegrateFluxTest(); }
  static void destroySuite( IntegrateFluxTest *suite ) 
  { 
    delete suite; 
  }

  void test_Init()
  {
    IntegrateFlux alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_weighted_no_time()
  {
    const size_t expectedNormalInterpolationSize = 98;
    do_test_all(WeightedNoTime,expectedNormalInterpolationSize);
  }

  void test_tof()
  {
    const size_t expectedNormalInterpolationSize = 1000;
    do_test_all(Tof,expectedNormalInterpolationSize);
  }

  void test_histogram()
  {
    size_t expectedNormalInterpolationSize = 100;
    const double tolerance = 1e-3;
    do_test_all(Histogram,expectedNormalInterpolationSize,tolerance);
    expectedNormalInterpolationSize = do_test_normal_case(Histogram,tolerance,99);
    TS_ASSERT_EQUALS( expectedNormalInterpolationSize, 99 );
    expectedNormalInterpolationSize = do_test_normal_case(Histogram,tolerance,30);
    TS_ASSERT_EQUALS( expectedNormalInterpolationSize, 30 );
  }

  void test_histogram_non_uniform()
  {
    size_t expectedNormalInterpolationSize = 100;
    const double tolerance = 1e-3;
    do_test_all(HistogramNonUniform,expectedNormalInterpolationSize,tolerance);
    expectedNormalInterpolationSize = do_test_normal_case(HistogramNonUniform,tolerance,99);
    TS_ASSERT_EQUALS( expectedNormalInterpolationSize, 99 );
    expectedNormalInterpolationSize = do_test_normal_case(HistogramNonUniform,tolerance,30);
    TS_ASSERT_EQUALS( expectedNormalInterpolationSize, 30 );
  }

  void test_distribution()
  {
    const size_t expectedNormalInterpolationSize = 100;
    const double tolerance = 1e-3;
    do_test_all(Distribution,expectedNormalInterpolationSize,tolerance);
  }

  void test_point_data()
  {
    size_t expectedNormalInterpolationSize = 100;
    const double tolerance = 1e-5;
    do_test_all(PointData,expectedNormalInterpolationSize,tolerance);
    expectedNormalInterpolationSize = do_test_normal_case(PointData,tolerance,99);
    TS_ASSERT_EQUALS( expectedNormalInterpolationSize, 99 );
  }

  void test_point_data_non_uniform()
  {
    size_t expectedNormalInterpolationSize = 100;
    const double tolerance = 1e-5;
    do_test_all(PointDataNonUniform,expectedNormalInterpolationSize,tolerance);
    expectedNormalInterpolationSize = do_test_normal_case(PointData,tolerance,99);
    TS_ASSERT_EQUALS( expectedNormalInterpolationSize, 99 );
    expectedNormalInterpolationSize = do_test_normal_case(PointData,tolerance,30);
    TS_ASSERT_EQUALS( expectedNormalInterpolationSize, 30 );
  }

private:

  void do_test_all(WorkspaceType type, size_t normalInterpolationSize, double tolerance = 0.1)
  {
    do_test_one_interpolation_point(type);
    auto n = do_test_normal_case(type,tolerance);
    TS_ASSERT_EQUALS( n, normalInterpolationSize );
    n = do_test_normal_case(type,tolerance,2);
    TS_ASSERT_EQUALS( n, 2 );
  }

  size_t do_test_normal_case(WorkspaceType wsType, double tolerance, int nPoints = 1000)
  {
    // Name of the input workspace.
    std::string inWSName("IntegrateFluxTest_InputWS");
    // Name of the output workspace.
    std::string outWSName("IntegrateFluxTest_OutputWS");

    // Create an input workspace
    createInputWorkspace(inWSName,wsType);

    IntegrateFlux alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("NPoints", nPoints) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return -1;

    auto inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>( inWSName );
    
    TS_ASSERT( ws->getAxis(0)->unit() == inWS->getAxis(0)->unit() );
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 4 );

    auto &x = ws->readX(0);
    auto &y = ws->readY(0);
    
    size_t n = x.size();
    TS_ASSERT_EQUALS( n, y.size() );
    TS_ASSERT_EQUALS( y.front(), 0.0 );

    TestingFunction fun(*inWS,wsType);
    size_t i0 = n * 20 / 100;
    if ( i0 == 0 ) i0 = 1;
    for(size_t i = i0; i < n; ++i)
    {
      TS_ASSERT_DELTA( y[i] / fun(x[i]), 1.0, tolerance );
      //std::cerr << x[i] << ' ' << fun(x[i])  << ' ' <<  y[i] << ' ' << std::endl;
    }


    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();

    return n;
  }
  
  void do_test_one_interpolation_point(WorkspaceType type)
  {
    // Name of the input workspace.
    std::string inWSName("IntegrateFluxTest_InputWS");
    // Name of the output workspace.
    std::string outWSName("IntegrateFluxTest_OutputWS");

    // Create an input workspace
    createInputWorkspace(inWSName,type);

    IntegrateFlux alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inWSName);
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS( alg.setProperty("NPoints", 1), std::invalid_argument );

    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
  
private:

  void createInputWorkspace(const std::string& wsName, WorkspaceType type)
  {
    switch(type)
    {
    case WeightedNoTime:
      createInputWorkspaceWeightedNoTime(wsName);
      return;
    case Tof:
      createInputWorkspaceTOF(wsName);
      return;
    case Histogram:
      createInputWorkspaceHistogram(wsName);
      return;
    case HistogramNonUniform:
      createInputWorkspaceHistogramNonUniform(wsName);
      return;
    case Distribution:
      createInputWorkspaceDistribution(wsName);
      return;
    case PointData:
      createInputWorkspacePointData(wsName);
      return;
    case PointDataNonUniform:
      createInputWorkspacePointDataNonUniform(wsName);
      return;
    };
  }

  void createInputWorkspaceWeightedNoTime(const std::string& wsName)
  {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    alg->initialize();
    alg->setPropertyValue("WorkspaceType","Event");
    alg->setPropertyValue("Function","User Defined");
    alg->setPropertyValue("UserDefinedFunction","name=LinearBackground,A0=1,A1=2");
    alg->setProperty("NumEvents",10000);
    alg->setProperty("NumBanks",1);
    alg->setProperty("BankPixelWidth",2);
    alg->setProperty("XMin",0.0);
    alg->setProperty("XMax",100.0);
    alg->setPropertyValue("XUnit","Momentum");
    alg->setProperty("BinWidth",1.0);
    alg->setProperty("OutputWorkspace",wsName);
    alg->execute();

    alg = Mantid::API::AlgorithmManager::Instance().create("CompressEvents");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace",wsName);
    alg->setPropertyValue("OutputWorkspace",wsName);
    alg->setProperty("Tolerance",1.0);
    alg->execute();
  }

  void createInputWorkspaceTOF(const std::string& wsName)
  {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    alg->initialize();
    alg->setPropertyValue("WorkspaceType","Event");
    alg->setPropertyValue("Function","User Defined");
    alg->setPropertyValue("UserDefinedFunction","name=LinearBackground,A0=1,A1=2");
    alg->setProperty("NumEvents",10000);
    alg->setProperty("NumBanks",1);
    alg->setProperty("BankPixelWidth",2);
    alg->setProperty("XMin",0.0);
    alg->setProperty("XMax",100.0);
    alg->setPropertyValue("XUnit","Momentum");
    alg->setProperty("BinWidth",1.0);
    alg->setProperty("OutputWorkspace",wsName);
    alg->execute();

  }

  void createInputWorkspaceHistogram(const std::string& wsName)
  {
    auto ws = Mantid::API::WorkspaceFactory::Instance().create( "Workspace2D", 4, 101, 100 );
    auto& x = ws->dataX(0);
    x[0] = 0.0;
    for(auto i = x.begin()+1; i != x.end(); ++i)
    {
      *i = *(i-1) + 0.3;
    }
    for(size_t spec = 0; spec != ws->getNumberHistograms(); ++spec)
    {
      ws->setX(spec,x);
      auto& y = ws->dataY(spec);
      for(auto j = y.begin(); j != y.end(); ++j)
      {
        auto i = std::distance( y.begin(), j );
        *j = double(2 * i) + 1.0;
      }
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName,ws);
  }

  void createInputWorkspaceHistogramNonUniform(const std::string& wsName)
  {
    auto ws = Mantid::API::WorkspaceFactory::Instance().create( "Workspace2D", 4, 101, 100 );
    auto& x = ws->dataX(0);
    x[0] = 0.0;
    for(auto i = x.begin()+1; i != x.end(); ++i)
    {
      double tmp = *(i-1);
      *i = tmp * (1.0 + 0.0001 * tmp) + 0.3;
    }
    for(size_t spec = 0; spec != ws->getNumberHistograms(); ++spec)
    {
      ws->setX(spec,x);
      auto& y = ws->dataY(spec);
      for(auto j = y.begin(); j != y.end(); ++j)
      {
        auto i = std::distance( y.begin(), j );
        *j = double(2 * i) + 1.0;
      }
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName,ws);
  }

  void createInputWorkspaceDistribution(const std::string& wsName)
  {
    auto ws = Mantid::API::WorkspaceFactory::Instance().create( "Workspace2D", 4, 101, 100 );
    auto& x = ws->dataX(0);
    x[0] = 0.0;
    for(auto i = x.begin()+1; i != x.end(); ++i)
    {
      *i = *(i-1) + 0.3;
    }
    for(size_t spec = 0; spec != ws->getNumberHistograms(); ++spec)
    {
      ws->setX(spec,x);
      auto& y = ws->dataY(spec);
      for(auto j = y.begin(); j != y.end(); ++j)
      {
        auto i = std::distance( y.begin(), j );
        *j = double(2 * i) + 1.0;
      }
      //std::cerr << std::accumulate( y.begin(), y.end(), 0.0 ) << std::endl;
    }
    ws->isDistribution(true);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName,ws);
  }

  void createInputWorkspacePointData(const std::string& wsName)
  {
    auto ws = Mantid::API::WorkspaceFactory::Instance().create( "Workspace2D", 4, 100, 100 );
    auto& x = ws->dataX(0);
    x[0] = 0.0;
    for(auto i = x.begin()+1; i != x.end(); ++i)
    {
      *i = *(i-1) + 0.3;
    }
    for(size_t spec = 0; spec != ws->getNumberHistograms(); ++spec)
    {
      ws->setX(spec,x);
      auto& y = ws->dataY(spec);
      for(auto j = y.begin(); j != y.end(); ++j)
      {
        auto i = std::distance( y.begin(), j );
        *j = 2 * x[i] + 1.0;
      }
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName,ws);
  }

  void createInputWorkspacePointDataNonUniform(const std::string& wsName)
  {
    auto ws = Mantid::API::WorkspaceFactory::Instance().create( "Workspace2D", 4, 100, 100 );
    auto& x = ws->dataX(0);
    x[0] = 0.0;
    for(auto i = x.begin()+1; i != x.end(); ++i)
    {
      double tmp = *(i-1);
      *i = tmp * (1.0 + 0.0001 * tmp) + 0.3;
    }
    for(size_t spec = 0; spec != ws->getNumberHistograms(); ++spec)
    {
      ws->setX(spec,x);
      auto& y = ws->dataY(spec);
      for(auto j = y.begin(); j != y.end(); ++j)
      {
        auto i = std::distance( y.begin(), j );
        *j = 2 * x[i] + 1.0;
      }
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(wsName,ws);
  }

};


#endif /* MANTID_MDALGORITHMS_INTEGRATEFLUXTEST_H_ */