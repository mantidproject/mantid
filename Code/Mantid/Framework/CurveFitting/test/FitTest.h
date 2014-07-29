#ifndef CURVEFITTING_FITTEST_H_
#define CURVEFITTING_FITTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/FakeObjects.h"

#include "MantidCurveFitting/Fit.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/FuncMinimizerFactory.h"

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

namespace
{
    class TestMinimizer : public API::IFuncMinimizer
    {
    public:
      /// Constructor setting a value for the relative error acceptance (default=0.01)
      TestMinimizer()
      {
          declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("SomeOutput","abc",Kernel::Direction::Output),
              "Name of the output Workspace holding some output.");
      }

      /// Overloading base class methods.
      std::string name()const{return "TestMinimizer";}
      /// Do one iteration.
      bool iterate(size_t iter)
      {
          m_data[iter] = iter;

          if ( iter >= m_data.size() - 1 )
          {
              API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D",1,m_data.size(),m_data.size());
              auto & Y = ws->dataY(0);
              for(size_t i = 0; i < Y.size(); ++i)
              {
                Y[i] = static_cast<double>(m_data[i]);
              }
              setProperty("SomeOutput",ws);
              return false;
          }
          return true;
      }

      /// Return current value of the cost function
      double costFunctionVal() {return 0.0;}
      /// Initialize minimizer.
      virtual void initialize(API::ICostFunction_sptr, size_t maxIterations = 0)
      {
            m_data.resize(maxIterations);
      }

    private:
      std::vector<size_t> m_data;
    };

    DECLARE_FUNCMINIMIZER(TestMinimizer,TestMinimizer)
}

class FitTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitTest *createSuite() { return new FitTest(); }
  static void destroySuite( FitTest *suite ) { delete suite; }
  
  FitTest()
  {
    // need to have DataObjects loaded
    FrameworkManager::Instance();
  }


  // Test that Fit copies minimizer's output properties to Fit
  // Test that minimizer's iterate(iter) method is called maxIteration times
  //  and iter passed to iterate() has values within 0 <= iter < maxIterations
  void test_minimizer_output()
  {
      API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
      Fit fit;
      fit.initialize();

      fit.setProperty("Function","name=LinearBackground");
      fit.setProperty("InputWorkspace",ws);
      fit.setProperty("MaxIterations",99);
      fit.setProperty("Minimizer","TestMinimizer,SomeOutput=MinimizerOutput");
      fit.setProperty("CreateOutput",true);

      fit.execute();
      TS_ASSERT( fit.existsProperty("SomeOutput") );
      TS_ASSERT_EQUALS( fit.getPropertyValue("SomeOutput"), "MinimizerOutput");
      TS_ASSERT( API::AnalysisDataService::Instance().doesExist("MinimizerOutput") );

      API::MatrixWorkspace_sptr outWS = API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("MinimizerOutput");
      TS_ASSERT( outWS );
      auto &y = outWS->readY(0);
      TS_ASSERT_EQUALS( y.size(), 99 );
      for(size_t iter = 0; iter < 99; ++iter)
      {
          TS_ASSERT_EQUALS( y[iter], static_cast<double>(iter) );
      }

      API::AnalysisDataService::Instance().clear();
  }

  // Test that minimizer's output is'n passed to Fit if no other output is created.
  // Other output are: fitting parameters table, calculated values.
  // To create output either CreateOutput must be set to true or Output be set to non-empty string
  void test_minimizer_output_not_passed_to_Fit()
  {
      API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
      Fit fit;
      fit.initialize();

      fit.setProperty("Function","name=LinearBackground");
      fit.setProperty("InputWorkspace",ws);
      fit.setProperty("MaxIterations",99);
      fit.setProperty("Minimizer","TestMinimizer,SomeOutput=MinimizerOutput");

      fit.execute();
      TS_ASSERT( !fit.existsProperty("SomeOutput") );
      TS_ASSERT( !API::AnalysisDataService::Instance().doesExist("MinimizerOutput") );

  }

};

#endif /*CURVEFITTING_FITMWTEST_H_*/
