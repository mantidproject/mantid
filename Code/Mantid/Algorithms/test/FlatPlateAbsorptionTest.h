#ifndef FLATPLATEABSORPTIONTEST_H_
#define FLATPLATEABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FlatPlateAbsorption.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/ConvertUnits.h"

class FlatPlateAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( atten.name(), "FlatPlateAbsorption" )
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
    loader.setPropertyValue("Filename","../../../../Test/AutoTestData/HRP39191.raw");
    inputWS = "rawWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("SpectrumList","1,66,322");
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
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleHeight","2.3") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleWidth","1.8") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleThickness","1.5") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("AttenuationXSection","6.52") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ScatteringXSection","19.876") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.0093") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfWavelengthPoints","100") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") )
    TS_ASSERT_THROWS_NOTHING( atten.execute() )
    TS_ASSERT( atten.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( result->readY(0).front(), 0.7536, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0)[9453], 0.7318, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0).back(), 0.6288, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1).front(), 0.7323, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1)[18439], 0.6553, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1).back(), 0.5952, 0.0001 )
    TS_ASSERT_DELTA( result->readY(2).front(), 0.7467, 0.0001 )
    TS_ASSERT_DELTA( result->readY(2)[1234], 0.7447, 0.0001 )
    TS_ASSERT_DELTA( result->readY(2).back(), 0.6134, 0.0001 )
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::FlatPlateAbsorption atten;
  std::string inputWS;
};

#endif /*FLATPLATEABSORPTIONTEST_H_*/
