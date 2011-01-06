#ifndef CYLINDERABSORPTIONTEST_H_
#define CYLINDERABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class CylinderAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( atten.name(), "CylinderAbsorption" );
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

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_THROWS_NOTHING( atten.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("CylinderSampleHeight","4") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("CylinderSampleRadius","0.4") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfSlices","2") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfAnnuli","2") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfWavelengthPoints","5") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") );
    TS_ASSERT_THROWS_NOTHING( atten.execute() );
    TS_ASSERT( atten.isExecuted() );
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    TS_ASSERT_DELTA( result->readY(0).front(), 0.7260, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0).back(), 0.2427, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0)[8], 0.2709, 0.0001 );
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testInelastic()
  {
    Mantid::Algorithms::CylinderAbsorption atten2;
    atten2.initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_THROWS_NOTHING( atten2.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
    const std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("CylinderSampleHeight","4") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("CylinderSampleRadius","0.4") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("NumberOfSlices","2") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("NumberOfAnnuli","2") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("NumberOfWavelengthPoints","5") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("EMode","Indirect") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("EFixed","1.845") );
    TS_ASSERT_THROWS_NOTHING( atten2.execute() );
    TS_ASSERT( atten2.isExecuted() );
    
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    TS_ASSERT_DELTA( result->readY(0).front(), 0.4920, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0).back(), 0.2847, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0)[2], 0.4313, 0.0001 );
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::CylinderAbsorption atten;
};

#endif /*CYLINDERABSORPTIONTEST_H_*/
