#ifndef CuboidGaugeVolumeAbsorptionTEST_H_
#define CuboidGaugeVolumeAbsorptionTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CuboidGaugeVolumeAbsorption.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/ConvertUnits.h"

class CuboidGaugeVolumeAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testBasics()
  {
    TS_ASSERT_EQUALS( atten.name(), "CuboidGaugeVolumeAbsorption" );
    TS_ASSERT_EQUALS( atten.version(), 1 );
    TS_ASSERT_EQUALS( atten.category(), "Absorption Corrections" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( atten.initialize() );
    TS_ASSERT( atten.isInitialized() );
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

    Mantid::Algorithms::FlatPlateAbsorption flat;
    flat.initialize();
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("InputWorkspace",inputWS) );
    std::string flatWS("flat");
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("OutputWorkspace",inputWS) );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleHeight","3.0") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleWidth","2.5") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleThickness","2.0") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("NumberOfWavelengthPoints","1") );
    TS_ASSERT_THROWS_NOTHING( flat.execute() );
    TS_ASSERT( flat.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("InputWorkspace",inputWS) );
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleHeight","2.3") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleWidth","1.8") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleThickness","1.5") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("AttenuationXSection","6.52") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ScatteringXSection","19.876") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.0093") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfWavelengthPoints","100") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") );
    TS_ASSERT_THROWS_NOTHING( atten.execute() );
    TS_ASSERT( atten.isExecuted() );
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    // This test cut and paste from FlatPlateAbsorption. Since we have a larger sample now, but the
    // same integration volume, the numbers have to be smaller.
    TS_ASSERT_LESS_THAN( result->readY(0).front(), 0.7536 );
    TS_ASSERT_LESS_THAN( result->readY(0)[9453], 0.7318 );
    TS_ASSERT_LESS_THAN( result->readY(0).back(), 0.6288 );
    TS_ASSERT_LESS_THAN( result->readY(1).front(), 0.7323 );
    TS_ASSERT_LESS_THAN( result->readY(1)[18439], 0.6553 );
    TS_ASSERT_LESS_THAN( result->readY(1).back(), 0.5952 );
    TS_ASSERT_LESS_THAN( result->readY(2).front(), 0.7467 );
    TS_ASSERT_LESS_THAN( result->readY(2)[1234], 0.7447 );
    TS_ASSERT_LESS_THAN( result->readY(2).back(), 0.6134 );
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::CuboidGaugeVolumeAbsorption atten;
  std::string inputWS;
};

#endif /*CuboidGaugeVolumeAbsorptionTEST_H_*/
