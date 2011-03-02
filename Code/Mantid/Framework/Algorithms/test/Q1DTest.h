#ifndef Q1DTEST_H_
#define Q1DTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/Q1D.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/ConvertUnits.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class Q1DTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( Q1D.name(), "Q1D" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( Q1D.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( Q1D.category(), "SANS" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( Q1D.initialize() )
    TS_ASSERT( Q1D.isInitialized() )
  }

  void testExec()
  {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","LOQ48127.raw");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumList","3");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",inputWS);
    convert.setPropertyValue("OutputWorkspace",inputWS);
    convert.setPropertyValue("Target","Wavelength");
    convert.execute();

    if (!Q1D.isInitialized()) Q1D.initialize();

    TS_ASSERT_THROWS_NOTHING( Q1D.setPropertyValue("InputWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( Q1D.setPropertyValue("InputForErrors",inputWS) )
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING( Q1D.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( Q1D.setPropertyValue("OutputBinning","0.1,0.02,0.5") )
    TS_ASSERT_THROWS_NOTHING( Q1D.setPropertyValue("AccountForGravity","1") )

    TS_ASSERT_THROWS_NOTHING( Q1D.execute() )
    TS_ASSERT( Q1D.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_EQUALS( result->getNumberHistograms(), 1 )
    TS_ASSERT_EQUALS( result->blocksize(), 20 )
    TS_ASSERT_EQUALS( result->getAxis(0)->unit()->unitID(), "MomentumTransfer" )
    TS_ASSERT_EQUALS( (*(result->getAxis(1)))(0), 3 )
    
    TS_ASSERT_EQUALS( result->readX(0).size(), 21 )
    TS_ASSERT_EQUALS( result->readX(0).front(), 0.1 )
    TS_ASSERT_DELTA( result->readX(0)[6], 0.22, 0.01 )
    TS_ASSERT_EQUALS( result->readX(0).back(), 0.5 )
    TS_ASSERT_DELTA( result->readY(0).front(), 3323038348.6, 1 )
    TS_ASSERT_DELTA( result->readY(0)[13], 8747222731.8, 1 )
    TS_ASSERT_DELTA( result->readY(0).back(), 203744814, 1 )
    TS_ASSERT_DELTA( result->readE(0).front(), 17742755.2, 1 )
    TS_ASSERT_DELTA( result->readE(0)[10], 54140676.3, 1 )
    TS_ASSERT_DELTA( result->readE(0).back(), 9187621.9, 1 )
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::Q1D Q1D;
  std::string inputWS;
};

#endif /*Q1DTEST_H_*/
