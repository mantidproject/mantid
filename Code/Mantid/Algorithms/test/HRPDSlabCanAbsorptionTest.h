#ifndef HRPDSLABCANABSORPTIONTEST_H_
#define HRPDSLABCANABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/HRPDSlabCanAbsorption.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/ConvertUnits.h"

class HRPDSlabCanAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( atten.name(), "HRPDSlabCanAbsorption" )
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( atten.version(), 1 )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( atten.category(), "Diffraction" )
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
    loader.setPropertyValue("SpectrumList","1,194,322");
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
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("Thickness","1.5") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleAttenuationXSection","6.52") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleScatteringXSection","19.876") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.0093") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfWavelengthPoints","100") )
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") )
    TS_ASSERT_THROWS_NOTHING( atten.execute() )
    TS_ASSERT( atten.isExecuted() )
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) )
    TS_ASSERT_DELTA( result->readY(0).front(), 0.7451, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0)[9453], 0.7212, 0.0001 )
    TS_ASSERT_DELTA( result->readY(0).back(), 0.6089, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1).front(), 0.6522, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1)[18439], 0.5699, 0.0001 )
    TS_ASSERT_DELTA( result->readY(1).back(), 0.5070, 0.0001 )
    TS_ASSERT_DELTA( result->readY(2).front(), 0.7376, 0.0001 )
    TS_ASSERT_DELTA( result->readY(2)[1234], 0.7355, 0.0001 )
    TS_ASSERT_DELTA( result->readY(2).back(), 0.5927, 0.0001 )
    
    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::HRPDSlabCanAbsorption atten;
  std::string inputWS;
};

#endif /*HRPDSLABCANABSORPTIONTEST_H_*/
