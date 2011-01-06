#ifndef CuboidGaugeVolumeAbsorptionTEST_H_
#define CuboidGaugeVolumeAbsorptionTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CuboidGaugeVolumeAbsorption.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Geometry::Object_sptr;

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

  void testFailsIfNoInstrument()
  {
    // Create a simple test workspace that has no instrument
    Workspace2D_sptr testWS = WorkspaceCreationHelper::Create2DWorkspace(5, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    TS_ASSERT_THROWS( atten.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS),
         std::invalid_argument );
  }

  void testFailsIfNoSampleShape()
  {
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(9, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CuboidGaugeVolumeAbsorption abs;
    abs.initialize();
    TS_ASSERT_THROWS_NOTHING( abs.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
    // None of the below values matter - they just have to be set to something
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("OutputWorkspace","out") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleHeight","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleWidth","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleThickness","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("AttenuationXSection","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("ScatteringXSection","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleNumberDensity","1") );
    TS_ASSERT( ! abs.execute() );
  }

  void testFailsIfSampleSmallerThanGaugeVolume()
  {
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(9, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    // Define a sample shape
    Object_sptr sampleShape = ComponentCreationHelper::createCuboid(0.005,0.003,0.002);
    testWS->mutableSample().setShape(*sampleShape);

    Mantid::Algorithms::CuboidGaugeVolumeAbsorption abs;
    abs.initialize();
    TS_ASSERT_THROWS_NOTHING( abs.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
    // None of the below values matter - they just have to be set to something
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("OutputWorkspace","out") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleHeight","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleWidth","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleThickness","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("AttenuationXSection","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("ScatteringXSection","1") );
    TS_ASSERT_THROWS_NOTHING( abs.setPropertyValue("SampleNumberDensity","1") );
    TS_ASSERT( ! abs.execute() );
  }

  void testExec()
  {
    if ( !atten.isInitialized() ) atten.initialize();

    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    // Define a sample shape
    Object_sptr sampleShape = ComponentCreationHelper::createCuboid(0.025,0.03,0.02);
    testWS->mutableSample().setShape(*sampleShape);

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
    // This test cut and paste from FlatPlateAbsorption. Since we have a larger sample now, but the
    // same integration volume, the numbers have to be smaller.
    TS_ASSERT_LESS_THAN( result->readY(0).front(), 0.7235 );
    TS_ASSERT_LESS_THAN( result->readY(0)[1], 0.6888 );
    TS_ASSERT_LESS_THAN( result->readY(0).back(), 0.4603 );
    TS_ASSERT_LESS_THAN( result->readY(1).front(), 0.7235 );
    TS_ASSERT_LESS_THAN( result->readY(1)[5], 0.5616 );
    TS_ASSERT_LESS_THAN( result->readY(1).back(), 0.4603 );
    
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::CuboidGaugeVolumeAbsorption atten;
};

#endif /*CuboidGaugeVolumeAbsorptionTEST_H_*/
