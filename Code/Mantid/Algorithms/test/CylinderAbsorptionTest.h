#ifndef CYLINDERABSORPTIONTEST_H_
#define CYLINDERABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/ConvertUnits.h"

class CylinderAbsorptionTest : public CxxTest::TestSuite
{
public:
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

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/AutoTestData/GEM40979.raw");
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
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfWavelengthPoints","255") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") )
    TS_ASSERT_THROWS_NOTHING( atten.execute() )
    TS_ASSERT( atten.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( result->readY(0).front(), 0.7717, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0).back(), 0.4281, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0)[2000], 0.7347, 0.0001 )
    TS_ASSERT_DELTA( result->readY(5).front(), 0.7717, 0.0001 )
    TS_ASSERT_DELTA( result->readY(5).back(), 0.4297, 0.0001 )
    TS_ASSERT_DELTA( result->readY(5)[1234], 0.7526, 0.0001 )
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testInelastic()
  {
    Mantid::Algorithms::CylinderAbsorption atten2;
    atten2.initialize();

    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/AutoTestData/IRS38633.raw");
    const std::string inputWS = "rawWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumList","10,100");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",inputWS);
    convert.setPropertyValue("OutputWorkspace",inputWS);
    convert.setPropertyValue("Target","Wavelength");
    convert.execute();

    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("InputWorkspace",inputWS) )
    const std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("OutputWorkspace",outputWS) )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("CylinderSampleHeight","4") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("CylinderSampleRadius","0.4") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("AttenuationXSection","5.08") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("ScatteringXSection","5.1") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("SampleNumberDensity","0.07192") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("NumberOfSlices","2") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("NumberOfAnnuli","2") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("NumberOfWavelengthPoints","101") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("EMode","Indirect") )
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("EFixed","10.0") )
    TS_ASSERT_THROWS_NOTHING( atten2.execute() )
    TS_ASSERT( atten2.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( result->readY(0).front(), 0.3442, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0).back(), 0.2996, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0)[1111], 0.3184, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1).front(), 0.4766, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1).back(), 0.4253, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1)[555], 0.4616, 0.0001 )
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::CylinderAbsorption atten;
  std::string inputWS;
};

#endif /*CYLINDERABSORPTIONTEST_H_*/
