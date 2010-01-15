#ifndef CYLINDERABSORPTIONTEST_H_
#define CYLINDERABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Property.h"
#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidAlgorithms/ConvertUnits.h"

class CylinderAbsorptionTest : public CxxTest::TestSuite
{
public:
  CylinderAbsorptionTest()
  {
    Mantid::DataHandling::LoadRaw2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/GEM40979.raw");
    inputWS = "rawWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumMin","1000");
    loader.setPropertyValue("SpectrumMax","1010");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",inputWS);
    convert.setPropertyValue("OutputWorkspace",inputWS);
    convert.setPropertyValue("Target","Wavelength");
    convert.execute();
  }

  void testName()
  {
    TS_ASSERT_EQUALS( atten.name(), "CylinderAbsorption" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( atten.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( atten.category(), "General" )
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( atten.initialize() )
    TS_ASSERT( atten.isInitialized() )
  }

  void testExec()
  {
    if ( !atten.isInitialized() ) atten.initialize();

    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("InputWorkspace",inputWS) )
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("CylinderSampleHeight","4") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("CylinderSampleRadius","0.4") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("AttenuationXSection","5.08") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ScatteringXSection","5.1") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.07192") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfSlices","2") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfAnnuli","2") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") )
    TS_ASSERT_THROWS_NOTHING( atten.execute() )
    TS_ASSERT( atten.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( result->readY(0).front(), 0.7718, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0).back(), 0.4282, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0)[2000], 0.7348, 0.0001 )
  }

private:
  Mantid::Algorithms::CylinderAbsorption atten;
  std::string inputWS;
};

#endif /*CYLINDERABSORPTIONTEST_H_*/
