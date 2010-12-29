#ifndef FLATPLATEABSORPTIONTEST_H_
#define FLATPLATEABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FlatPlateAbsorption.h"
#include "MantidKernel/UnitFactory.h"
#include "WorkspaceCreationHelper.hh"

class FlatPlateAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( atten.name(), "FlatPlateAbsorption" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( atten.version(), 1 );
  }

  void testCategory()
  {
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

    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_THROWS_NOTHING( atten.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleHeight","2.3") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleWidth","1.8") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleThickness","1.5") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("AttenuationXSection","6.52") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ScatteringXSection","19.876") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.0093") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfWavelengthPoints","3") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") );
    TS_ASSERT_THROWS_NOTHING( atten.execute() );
    TS_ASSERT( atten.isExecuted() );
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    TS_ASSERT_DELTA( result->readY(0).front(), 0.7235, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0)[1], 0.6888, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0).back(), 0.4603, 0.0001 );
    TS_ASSERT_DELTA( result->readY(1).front(), 0.7235, 0.0001 );
    TS_ASSERT_DELTA( result->readY(1)[5], 0.5616, 0.0001 );
    TS_ASSERT_DELTA( result->readY(1).back(), 0.4603, 0.0001 );
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::FlatPlateAbsorption atten;
  std::string inputWS;
};

#endif /*FLATPLATEABSORPTIONTEST_H_*/
