#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_

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
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::MDEvents;
using namespace Mantid::API;


class BinToMDHistoWorkspaceTest : public CxxTest::TestSuite
{

private:

  //Helper class. Mock Implicit function.
  class MockImplicitFunction : public Mantid::API::ImplicitFunction
  {
  public:
    bool evaluate(const Mantid::coord_t*, const bool *, const size_t) const
    {
      return false;
    }
    bool evaluate(const Mantid::API::Point3D* /*pPoint3D*/) const
    {
      return false; //Always reject points.
    }
    bool evaluate(const Mantid::coord_t*, const bool *)
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
    Mantid::API::ImplicitFunctionBuilder* createFunctionBuilder(Poco::XML::Element* /*functionElement*/)
    {
      return new MockImplicitFunctionBuilder;
    }
    void setSuccessorParser(Mantid::API::ImplicitFunctionParser* /*successor*/){}
    void setParameterParser(Mantid::API::ImplicitFunctionParameterParser* /*parser*/){}
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
  

  /** Test the algo
  * @param nameX : name of the axis
  * @param expected_signal :: how many events in each resulting bin
  * @param expected_numBins :: how many points/bins in the output
  */
  void do_test_exec(std::string functionXML,
      std::string name1, std::string name2, std::string name3, std::string name4,
      double expected_signal,
      size_t expected_numBins,
      bool IterateEvents=false)
  {
    BinToMDHistoWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    IMDEventWorkspace_sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1);
    AnalysisDataService::Instance().addOrReplace("BinToMDHistoWorkspaceTest_ws", in_ws);
    // 1000 boxes with 1 event each
    TS_ASSERT_EQUALS( in_ws->getNPoints(), 1000);

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinToMDHistoWorkspaceTest_ws") );
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimX", name1));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimY", name2));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimZ", name3));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimT", name4));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ImplicitFunctionXML",functionXML));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IterateEvents", IterateEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinToMDHistoWorkspaceTest_ws"));

    TS_ASSERT_THROWS_NOTHING( alg.execute(); )

    TS_ASSERT( alg.isExecuted() );

    IMDWorkspace_sptr out ;
    TS_ASSERT_THROWS_NOTHING( out = boost::dynamic_pointer_cast<IMDWorkspace>(
        AnalysisDataService::Instance().retrieve("BinToMDHistoWorkspaceTest_ws")); )
    TS_ASSERT(out);
    if(!out) return;

    // Took 6x6x6 bins in the middle of the box
    TS_ASSERT_EQUALS(out->getNPoints(), expected_numBins);
    // Every box has a single event summed into it, so 1.0 weight
    for (size_t i=0; i < out->getNPoints(); i++)
    {
      if (functionXML == "")
      {
        // Nothing rejected
        TS_ASSERT_DELTA(out->getSignalAt(i), expected_signal, 1e-5);
        TS_ASSERT_DELTA(out->getErrorAt(i), expected_signal, 1e-5);
      }
      else
      {
        // All NAN cause of implicit function
        TS_ASSERT( boost::math::isnan( out->getSignalAt(i) ) ); //The implicit function should have ensured that no bins were present.
      }
    }
    AnalysisDataService::Instance().remove("BinToMDHistoWorkspaceTest_ws");
  }

  void test_exec_3D()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_3D_scrambled_order()
  { do_test_exec("", "Axis1,2.0,8.0, 6", "Axis0,2.0,8.0, 6", "", "Axis2,2.0,8.0, 6", 1.0 /*signal*/, 6*6*6 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_3D_unevenSizes()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 6", "", 2.0 /*signal*/, 6*6*3 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_with_impfunction()
  {
    //This describes the local implicit function that will always reject bins. so output workspace should have zero.
    std::string functionXML = std::string("<Function>")+
        "<Type>MockImplicitFunction</Type>"+
        "<ParameterList>"+
        "</ParameterList>"+
        "</Function>";
    do_test_exec(functionXML,  "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_2D()
  { // Integrate over the 3rd dimension
    do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "", "", 1.0*10.0 /*signal*/, 6*6 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_2D_largeBins()
  {
    do_test_exec("", "Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "", "", 4.0*10.0 /*signal*/, 3*3 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_2D_scrambledAndUnevent()
  { do_test_exec("", "Axis0,2.0,8.0, 3", "", "Axis2,2.0,8.0, 6", "", 2.0*10.0 /*signal*/, 3*6 /*# of bins*/, false /*IterateEvents*/ );
  }

  void test_exec_1D()
  { // Integrate over 2 dimensions
    do_test_exec("", "Axis2,2.0,8.0, 6", "", "", "", 1.0*100.0 /*signal*/, 6 /*# of bins*/, false /*IterateEvents*/ );
  }




  void test_exec_3D_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_3D_scrambled_order_IterateEvents()
  { do_test_exec("", "Axis1,2.0,8.0, 6", "Axis0,2.0,8.0, 6", "", "Axis2,2.0,8.0, 6", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_3D_unevenSizes_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 3", "Axis2,2.0,8.0, 6", "", 2.0 /*signal*/, 6*6*3 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_with_impfunction_IterateEvents()
  { //This describes the local implicit function that will always reject bins. so output workspace should have zero.
    std::string functionXML = std::string("<Function>")+
        "<Type>MockImplicitFunction</Type>"+
        "<ParameterList>"+
        "</ParameterList>"+
        "</Function>";
    do_test_exec(functionXML,  "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "Axis2,2.0,8.0, 6", "", 1.0 /*signal*/, 6*6*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_2D_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 6", "Axis1,2.0,8.0, 6", "", "", 1.0*10.0 /*signal*/, 6*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_2D_largeBins_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 3", "Axis1,2.0,8.0, 3", "", "", 4.0*10.0 /*signal*/, 3*3 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_2D_scrambledAndUnevent_IterateEvents()
  { do_test_exec("", "Axis0,2.0,8.0, 3", "", "Axis2,2.0,8.0, 6", "", 2.0*10.0 /*signal*/, 3*6 /*# of bins*/, true /*IterateEvents*/ );
  }

  void test_exec_1D_IterateEvents()
  { do_test_exec("", "Axis2,2.0,8.0, 6", "", "", "", 1.0*100.0 /*signal*/, 6 /*# of bins*/, true /*IterateEvents*/ );
  }


};


class BinToMDHistoWorkspaceTestPerformance : public CxxTest::TestSuite
{
public:
  MDEventWorkspace3::sptr in_ws;

  void setUp()
  {
    Mantid::Kernel::CPUTimer tim;
    in_ws = MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 1000);
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
    for (size_t i=0; i<10; i++)
    {
      BinToMDHistoWorkspace alg;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "BinToMDHistoWorkspaceTest_ws") );
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimX", "Axis0,2.0,8.0, 60"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimY", "Axis1,2.0,8.0, 60"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimZ", "Axis2,2.0,8.0, 60"));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DimT", ""));
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "BinToMDHistoWorkspaceTest_ws_histo"));
      TS_ASSERT_THROWS_NOTHING( alg.execute(); )
      TS_ASSERT( alg.isExecuted() );
    }
    std::cout << "<measurement><name>CPUFraction</name><value>" << tim.CPUfraction() << "</value></measurement>";

//    std::cout << tim << " to run.\n";
  }

};


#endif /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACETEST_H_ */

