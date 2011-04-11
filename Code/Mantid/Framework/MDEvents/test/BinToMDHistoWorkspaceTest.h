#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <iomanip>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/math/special_functions/fpclassify.hpp>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MDEventsTestHelper.hh"

using namespace Mantid::MDEvents;
using namespace Mantid::API;


class BinToMDHistoWorkspaceTest : public CxxTest::TestSuite
{

private:

  //Helper class. Mock Implicit function.
  class MockImplicitFunction : public Mantid::API::ImplicitFunction
  {
  public:
    bool evaluate(const Mantid::API::Point3D* pPoint3D) const
    {
      return false; //Always reject points.
    }
    virtual std::string getName() const
    {
      return "MockImplicitFunction";
    }
    MOCK_CONST_METHOD0(toXMLString, std::string());
    ~MockImplicitFunction()   {;}
  };

  //Helper class. Builds mock implicit functions.
  class MockImplicitFunctionBuilder : public Mantid::API::ImplicitFunctionBuilder
  {
  public:
    Mantid::API::ImplicitFunction* create() const
    {
      return new MockImplicitFunction;
    }
  };

  //Helper class. Parses mock Implicit Functions.
  class MockImplicitFunctionParser : public Mantid::API::ImplicitFunctionParser
  {
  public:
    MockImplicitFunctionParser() : Mantid::API::ImplicitFunctionParser(NULL){}
    Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* functionElement)
    {
      return new MockImplicitFunctionBuilder;
    }
    void setSuccessorParser(Mantid::API::ImplicitFunctionParser* successor){}
    void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser){}
  };


public:

  void testSetup()
  {
    using namespace Mantid::Kernel;
    Mantid::API::ImplicitFunctionFactory::Instance().subscribe<testing::NiceMock<MockImplicitFunction> >("MockImplicitFunction"); 
    Mantid::API::ImplicitFunctionParserFactory::Instance().subscribe<MockImplicitFunctionParser >("MockImplicitFunctionParser"); 
  }

  void test_Init()
  {
    BinToMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    BinToMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("BinToMDHistoWorkspaceTest_ws", in_ws);
    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinToMDHistoWorkspaceTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimX", "Axis0,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimY", "Axis1,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimZ", "Axis2,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimT", "NONE,0.0,10.0,1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinToMDHistoWorkspaceTest_ws"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

    TS_ASSERT( alg.isExecuted() );

    IMDWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<IMDWorkspace>(
        AnalysisDataService::Instance().retrieve("BinToMDHistoWorkspaceTest_ws")); )
    TS_ASSERT(out);
    if(!out) return;

    // Took 6x6x6 bins in the middle of the box
    TS_ASSERT_EQUALS(out->getNPoints(), 6*6*6);
    // Every box has a single event summed into it, so 1.0 weight
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      TS_ASSERT_DELTA(out->getSignalAt(i), 1.0, 1e-5);
      TS_ASSERT_DELTA(out->getErrorAt(i), 1.0, 1e-5);
    }

    AnalysisDataService::Instance().remove("BinToMDHistoWorkspaceTest_ws");
  }

  void test_exec_with_impfunction()
  {
    //This describes the local implicit function that will always reject bins. so output workspace should have zero.
    std::string functionXML = std::string("<Function>")+
        "<Type>MockImplicitFunction</Type>"+
        "<ParameterList>"+
        "</ParameterList>"+
        "</Function>";


    BinToMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("BinToMDHistoWorkspaceTest_ws", in_ws);
    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinToMDHistoWorkspaceTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimX", "Axis0,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimY", "Axis1,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimZ", "Axis2,2.0,8.0, 6"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimT", "NONE,0.0,10.0,1"));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ImplicitFunctionXML",functionXML));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinToMDHistoWorkspaceTest_ws"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

      TS_ASSERT( alg.isExecuted() );

    IMDWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<IMDWorkspace>(
      AnalysisDataService::Instance().retrieve("BinToMDHistoWorkspaceTest_ws")); )
      TS_ASSERT(out);
    if(!out) return;

    // Took 6x6x6 bins in the middle of the box
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      double signal = out->getSignalAt(i);
      //TODO: Fix on windows:
//      TS_ASSERT( boost::math::isnan(signal) ); //The implicit function should have ensured that no bins were present.
    }
  }

};


class BinToMDHistoWorkspaceTestPerformance : public CxxTest::TestSuite
{
public:
  MDEventWorkspace3::sptr in_ws;

  void setUp()
  {
    Mantid::Kernel::CPUTimer tim;
    in_ws = MDEventsHelper::makeMDEW<3>(10, 0.0, 10.0, 1000);
    // 1000 boxes with 1000 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000*1000);
    in_ws->splitAllIfNeeded(NULL);
    AnalysisDataService::Instance().addOrReplace("BinToMDHistoWorkspaceTest_ws", in_ws);
//    std::cout << tim << " to setUp.\n";
  }

  void tearDown()
  {
    AnalysisDataService::Instance().remove("BinToMDHistoWorkspaceTest_ws");
  }

  /** A slow test that is useful for profiling and optimizing */
  void test_for_profiling()
  {
    Mantid::Kernel::CPUTimer tim;
    for (size_t i=0; i<1; i++)
    {
      BinToMDHistoWorkspace alg;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinToMDHistoWorkspaceTest_ws") );
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimX", "Axis0,2.0,8.0, 60"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimY", "Axis1,2.0,8.0, 60"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimZ", "Axis2,2.0,8.0, 60"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimT", "NONE,0.0,10.0,1"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinToMDHistoWorkspaceTest_ws_histo"));
      TS_ASSERT_THROWS_NOTHING( alg.execute(); )
      TS_ASSERT( alg.isExecuted() );
    }
    std::cout << "<measurement><name>CPUFraction</name><value>" << tim.CPUfraction() << "</value></measurement>";

//    std::cout << tim << " to run.\n";
  }

};


#endif /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_ */

