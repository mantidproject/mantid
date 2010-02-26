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
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("QAxisBinning","0.5,0.25,2") )
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EMode","Indirect") )
    TS_ASSERT_THROWS_NOTHING( sqw.setPropertyValue("EFixed","1.84") )
    
    TS_ASSERT_THROWS_NOTHING( sqw.execute() )
    TS_ASSERT( sqw.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )

    TS_ASSERT_EQUALS( result->getAxis(0)->length(), 1904 )
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "DeltaE" )
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(0), -0.5590, 0.0001 )
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(999), -0.0971, 0.0001 )
    TS_ASSERT_DELTA( (*(result->getAxis(0)))(1900), 0.5728, 0.0001 )
    
    TS_ASSERT_EQUALS( result->getAxis(1)->length(), 7 )
    TS_ASSERT_EQUALS( result->getAxis(1)->unit()->unitID(), "MomentumTransfer" )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(0), 0.5 )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(3), 1.25 )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(6), 2.0 )
    
    TS_ASSERT_DELTA( result->readY(0)[1160], 0.0144, 0.00001)
    TS_ASSERT_DELTA( result->readE(0)[1160], 1.619E-4, 1.0E-7)
    TS_ASSERT_EQUALS( result->readY(1)[1145], 0)
    TS_ASSERT_EQUALS( result->readE(1)[1145], 0)
    TS_ASSERT_DELTA( result->readY(2)[1200], 1.303E-3, 1.0E-6)
    TS_ASSERT_DELTA( result->readE(2)[1200], 4.843E-5, 1.0E-8)
    TS_ASSERT_DELTA( result->readY(3)[99], 1.792E-5, 1.0E-8)
    TS_ASSERT_DELTA( result->readE(3)[99], 9.782E-6, 1.0E-9)
    TS_ASSERT_DELTA( result->readY(4)[1654], 1.374E-5, 1.0E-8)
    TS_ASSERT_DELTA( result->readE(4)[1654], 4.667E-06, 1.0E-9)
    TS_ASSERT_DELTA( result->readY(5)[1025], 4.756E-05, 1.0E-8)
    TS_ASSERT_DELTA( result->readE(5)[1025], 9.481E-06, 1.0E-9)
    
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);    
  }

private:
  Mantid::Algorithms::SofQW sqw;
};

#endif /*SOFQWTEST_H_*/
