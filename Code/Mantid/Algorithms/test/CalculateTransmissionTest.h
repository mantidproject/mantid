#ifndef CALCULATETRANSMISSIONTEST_H_
#define CALCULATETRANSMISSIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalculateTransmission.h"
#include "MantidDataHandling/LoadRaw2.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CropWorkspace.h"
#include "MantidCurveFitting/Linear.h"

using namespace Mantid::API;

class CalculateTransmissionTest : public CxxTest::TestSuite
{
public:
  CalculateTransmissionTest() : trans(), inputWS("LOQWS")
  {
    Mantid::DataHandling::LoadRaw2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/LOQ trans configuration/LOQ48127.raw");
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setPropertyValue("spectrum_min","2");
    loader.setPropertyValue("spectrum_max","4");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",inputWS);
    convert.setPropertyValue("OutputWorkspace",inputWS);
    convert.setPropertyValue("Target","Wavelength");
    convert.setPropertyValue("AlignBins","1");
    convert.execute();    
  }
  
	void testName()
	{
    TS_ASSERT_EQUALS( trans.name(), "CalculateTransmission" )
	}

	void testVersion()
	{
    TS_ASSERT_EQUALS( trans.version(), 1 )
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( trans.category(), "SANS" )
	}

	void testInit()
	{
    TS_ASSERT_THROWS_NOTHING( trans.initialize() )
    TS_ASSERT( trans.isInitialized() )	  
	}
	
	void testExec()
	{
    if ( !trans.isInitialized() ) trans.initialize();

    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("SampleRunWorkspace",inputWS) )
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("DirectRunWorkspace",inputWS) )
    std::string outputWS("outputWS");
    TS_ASSERT_THROWS_NOTHING( trans.setPropertyValue("OutputWorkspace",outputWS) )

    TS_ASSERT_THROWS_NOTHING( trans.execute() )
    TS_ASSERT( trans.isExecuted() )
    
    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS)) )
    const std::vector<double> &Y = output->readY(0);
    // Should all be 1 because I used the same workspace twice as the input
    for (unsigned int i = 0; i < Y.size(); ++i)
    {
      TS_ASSERT_DELTA( Y[i], 1.0, 0.005 )
    }
	}
	
private:
  Mantid::Algorithms::CalculateTransmission trans;
  std::string inputWS;
};

#endif /*CALCULATETRANSMISSIONTEST_H_*/
