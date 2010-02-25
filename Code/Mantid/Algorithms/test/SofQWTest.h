#ifndef SOFQWTEST_H_
#define SOFQWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQW.h"
#include "MantidNexus/LoadNexusProcessed.h"

using namespace Mantid::API;
//using namespace Mantid::Kernel;

class SofQWTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( sqw.name(), "SofQW" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( sqw.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( sqw.category(), "General" )
  }
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( sqw.initialize() )
    TS_ASSERT( sqw.isInitialized() )
  }
  
  void testExec()
  {
    if (!sqw.isInitialized()) sqw.initialize();

    Mantid::NeXus::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","../../../../Test/Data/irs26173_ipg.nxs");
    const std::string inputWS = "inputWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();
    
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("InputWorkspace",inputWS) )
    const std::string outputWS = "result";
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("QAxisBinning","1,0.1,2") )
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EMode","Indirect") )
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EFixed","1.84") )
    
    TS_ASSERT_THROWS_NOTHING( sqw.execute() )
    TS_ASSERT( sqw.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )

    TS_ASSERT_EQUALS( result->getAxis(0)->length(), 1904 )
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "DeltaE" )
    TS_ASSERT_EQUALS( result->getAxis(1)->length(), 11 )
    TS_ASSERT_EQUALS( result->getAxis(1)->unit()->unitID(), "MomentumTransfer" )
    
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);    
  }

private:
  Mantid::Algorithms::SofQW sqw;
};

#endif /*SOFQWTEST_H_*/
