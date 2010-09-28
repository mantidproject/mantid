#ifndef SOFQWTEST_H_
#define SOFQWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQW.h"
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadInstrument.h"
//#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::API;

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
    loader.setProperty("Filename","../../../../Test/AutoTestData/IRS26173_ipg.nxs");
    const std::string inputWS = "inputWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.execute();

    Mantid::API::MatrixWorkspace_sptr inWS;
    TS_ASSERT_THROWS_NOTHING( inWS = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(inputWS)) )
    WorkspaceHelpers::makeDistribution(inWS);

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
    
    TS_ASSERT_DELTA( result->readY(0)[1160], 91.4270, 0.0001)
    TS_ASSERT_DELTA( result->readE(0)[1160], 1.0275, 0.0001)
    TS_ASSERT_EQUALS( result->readY(1)[1145], 0)
    TS_ASSERT_EQUALS( result->readE(1)[1145], 0)
    TS_ASSERT_DELTA( result->readY(2)[1200], 8.0998, 0.0001)
    TS_ASSERT_DELTA( result->readE(2)[1200], 0.3011, 0.0001)
    TS_ASSERT_DELTA( result->readY(3)[99], 0.1876, 0.0001)
    TS_ASSERT_DELTA( result->readE(3)[99], 0.1024, 0.0001)
    TS_ASSERT_DELTA( result->readY(4)[1654], 0.0668, 0.0001)
    TS_ASSERT_DELTA( result->readE(4)[1654], 0.0227, 0.0001)
    TS_ASSERT_DELTA( result->readY(5)[1025], 0.3232, 0.0001)
    TS_ASSERT_DELTA( result->readE(5)[1025], 0.0644, 0.0001)
    
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);    
  }

private:
  Mantid::Algorithms::SofQW sqw;
};

#endif /*SOFQWTEST_H_*/
