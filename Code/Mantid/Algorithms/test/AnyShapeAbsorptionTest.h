#ifndef ANYSHAPEABSORPTIONTEST_H_
#define ANYSHAPEABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/AnyShapeAbsorption.h"
#include "MantidAlgorithms/FlatPlateAbsorption.h"
#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidKernel/UnitFactory.h"
#include "WorkspaceCreationHelper.hh"

class AnyShapeAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( atten.name(), "AbsorptionCorrection" );
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

  void testAgainstFlatPlate()
  {
    if ( !atten.isInitialized() ) atten.initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::FlatPlateAbsorption flat;
    flat.initialize();
    TS_ASSERT_THROWS_NOTHING( flat.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
    std::string flatWS("flat");
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("OutputWorkspace",flatWS) );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleHeight","2.3") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleWidth","1.8") );
    TS_ASSERT_THROWS_NOTHING( flat.setPropertyValue("SampleThickness","1.5") );
    TS_ASSERT_THROWS_NOTHING( flat.execute() );
    TS_ASSERT( flat.isExecuted() );

    // Using the output of the FlatPlateAbsorption algorithm is convenient because
    // it adds the sample object to the workspace
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("InputWorkspace",flatWS) );
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( atten.execute() );
    TS_ASSERT( atten.isExecuted() );
    
    Mantid::API::MatrixWorkspace_sptr flatws;
    TS_ASSERT_THROWS_NOTHING( flatws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(flatWS)) );
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    // These should be extremely close to one another (a fraction of a %)
    TS_ASSERT_DELTA( result->readY(0).front(), flatws->readY(0).front(), 0.00001 );
    TS_ASSERT_DELTA( result->readY(0).back(), flatws->readY(0).back(), 0.00001 );
    TS_ASSERT_DELTA( result->readY(0)[8], flatws->readY(0)[8], 0.00001 );
    // Check a few actual numbers as well
    TS_ASSERT_DELTA( result->readY(0).front(), 0.4852, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0).back(), 0.0665, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0)[4], 0.1731, 0.0001 );
    
    Mantid::API::AnalysisDataService::Instance().remove(flatWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testAgainstCylinder()
  {
    Mantid::Algorithms::AnyShapeAbsorption atten2;
    atten2.initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CylinderAbsorption cyl;
    cyl.initialize();
    TS_ASSERT_THROWS_NOTHING( cyl.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
//    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("InputWorkspace",inputWS) );
    std::string cylWS("cyl");
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("OutputWorkspace",cylWS) );
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("CylinderSampleHeight","4") );
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("CylinderSampleRadius","0.4") );
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("NumberOfSlices","10") );
    TS_ASSERT_THROWS_NOTHING( cyl.setPropertyValue("NumberOfAnnuli","6") );
    TS_ASSERT_THROWS_NOTHING( cyl.execute() );
    TS_ASSERT( cyl.isExecuted() );

    // Using the output of the CylinderAbsorption algorithm is convenient because
    // it adds the sample object to the workspace
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("InputWorkspace",cylWS) );
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( atten2.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( atten2.execute() );
    TS_ASSERT( atten2.isExecuted() );
    
    Mantid::API::MatrixWorkspace_sptr cylws;
    TS_ASSERT_THROWS_NOTHING( cylws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(cylWS)) );
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    // These should be somewhat close to one another (within a couple of %)
    Mantid::MantidVec y0 = result->readY(0);
    TS_ASSERT_DELTA( y0.front()/cylws->readY(0).front(), 1.0, 0.02 );
    TS_ASSERT_DELTA( y0[4]/cylws->readY(0)[4], 1.0, 0.02 );
    TS_ASSERT_DELTA( y0[7]/cylws->readY(0)[7], 1.0, 0.02 );
    // Check a few actual numbers as well
    TS_ASSERT_DELTA( y0.front(), 0.7357, 0.0001 );
    TS_ASSERT_DELTA( y0.back(), 0.2698, 0.0001 );
    TS_ASSERT_DELTA( y0[5], 0.4054, 0.0001 );

    // Now test with a gauge volume used.
    // Create a small cylinder to be the gauge volume
    std::string cylinder = "<cylinder id=\"shape\"> ";
    cylinder += "<centre-of-bottom-base x=\"0.0\" y=\"-0.01\" z=\"0.0\" /> " ;
    cylinder += "<axis x=\"0.0\" y=\"0.0\" z=\"1\" /> " ;
    cylinder += "<radius val=\"0.1\" /> " ;
    cylinder += "<height val=\"0.02\" /> " ;
    cylinder += "</cylinder>";

    cylws->mutableRun().addProperty("GaugeVolume",cylinder);

    // Re-run the algorithm
    Mantid::Algorithms::AnyShapeAbsorption atten3;
    atten3.initialize();
    TS_ASSERT_THROWS_NOTHING( atten3.setPropertyValue("InputWorkspace",cylWS) );
    TS_ASSERT_THROWS_NOTHING( atten3.setPropertyValue("OutputWorkspace","gauge") );
    TS_ASSERT_THROWS_NOTHING( atten3.setPropertyValue("AttenuationXSection","5.08") );
    TS_ASSERT_THROWS_NOTHING( atten3.setPropertyValue("ScatteringXSection","5.1") );
    TS_ASSERT_THROWS_NOTHING( atten3.setPropertyValue("SampleNumberDensity","0.07192") );
    TS_ASSERT_THROWS_NOTHING( atten3.execute() );
    TS_ASSERT( atten3.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve("gauge")) );
    TS_ASSERT_LESS_THAN( result->readY(0).front(), y0.front() );
    TS_ASSERT_LESS_THAN( result->readY(0).back(), y0.back() );
    TS_ASSERT_LESS_THAN( result->readY(0)[1], y0[1] );
    TS_ASSERT_LESS_THAN( result->readY(0).back(), result->readY(0).front() );

    Mantid::API::AnalysisDataService::Instance().remove(cylWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
    Mantid::API::AnalysisDataService::Instance().remove("gauge");
  }

private:
  Mantid::Algorithms::AnyShapeAbsorption atten;
};

#endif /*ANYSHAPEABSORPTIONTEST_H_*/
