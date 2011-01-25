#ifndef HRPDSLABCANABSORPTIONTEST_H_
#define HRPDSLABCANABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/HRPDSlabCanAbsorption.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Geometry;
using Mantid::API::MatrixWorkspace_sptr;

class HRPDSlabCanAbsorptionTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( atten.name(), "HRPDSlabCanAbsorption" );
  }

  void testVersion()
  {
    TS_ASSERT_EQUALS( atten.version(), 1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( atten.category(), "Diffraction" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( atten.initialize() );
    TS_ASSERT( atten.isInitialized() );
  }

  void testExec()
  {
    if ( !atten.isInitialized() ) atten.initialize();

    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 0.25, 0.5);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    boost::shared_ptr<Instrument> testInst(new Instrument("testInst"));
    testWS->setInstrument(testInst);
    testWS->mutableSpectraMap().populateSimple(0, 3);

    // Define a source and sample position
    //Define a source component
    ObjComponent *source = new ObjComponent("moderator", Object_sptr(), testInst.get());
    source->setPos(V3D(0.0, 0.0, -95.0));
    testInst->add(source);
    testInst->markAsSource(source);

    // Define a sample as a simple sphere
    ObjComponent *sample = new ObjComponent("samplePos", Object_sptr(), testInst.get());
    testInst->setPos(0.0, 0.0, 0.0);
    testInst->add(sample);
    testInst->markAsSamplePos(sample);

    // Add three detectors - one for each bank of HRPD
    Detector * det1 = new Detector("2101",0,testInst.get());
    det1->setPos(V3D(0.04528,0.04528,-0.887693));
    testInst->add(det1);
    testInst->markAsDetector(det1);
    Detector * det2 = new Detector("911000",1,testInst.get());
    det2->setPos(V3D(-1.60016,0.770105,0.293987));
    testInst->add(det2);
    testInst->markAsDetector(det2);
    Detector * det3 = new Detector("10101",2,testInst.get());
    det3->setPos(V3D(1.98194,0.0990971,3.19728));
    testInst->add(det3);
    testInst->markAsDetector(det3);

    TS_ASSERT_THROWS_NOTHING( atten.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS) );
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("OutputWorkspace",outputWS) );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("Thickness","1.5") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleAttenuationXSection","6.52") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleScatteringXSection","19.876") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("SampleNumberDensity","0.0093") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("NumberOfWavelengthPoints","3") );
    TS_ASSERT_THROWS_NOTHING( atten.setPropertyValue("ExpMethod","Normal") );
    TS_ASSERT_THROWS_NOTHING( atten.execute() );
    TS_ASSERT( atten.isExecuted() );

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
                                (Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)) );
    TS_ASSERT_DELTA( result->readY(0).front(), 0.7423, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0)[1], 0.7244, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0).back(), 0.5964, 0.0001 );
    TS_ASSERT_DELTA( result->readY(1).front(), 0.7033, 0.0001 );
    TS_ASSERT_DELTA( result->readY(1)[5], 0.5939, 0.0001 );
    TS_ASSERT_DELTA( result->readY(1).back(), 0.5192, 0.0001 );
    TS_ASSERT_DELTA( result->readY(2).front(), 0.7337, 0.0001 );
    TS_ASSERT_DELTA( result->readY(2)[5], 0.6404, 0.0001 );
    TS_ASSERT_DELTA( result->readY(2).back(), 0.5741, 0.0001 );

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::HRPDSlabCanAbsorption atten;
};

#endif /*HRPDSLABCANABSORPTIONTEST_H_*/
