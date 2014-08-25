#ifndef MANTID_MDALGORITHMS_TOBYFITYMATRIX_H_
#define MANTID_MDALGORITHMS_TOBYFITYMATRIX_H_

#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FermiChopperModel.h"
#include "MantidAPI/IkedaCarpenterModerator.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <cxxtest/TestSuite.h>


class TobyFitYVectorTest : public CxxTest::TestSuite
{
public:

  void test_object_construction_does_not_throw()
  {
    using namespace Mantid::MDAlgorithms;

    TobyFitYVector *yVector(NULL); // TS_ macro doesn't work with stack construction & no default constructor
    TS_ASSERT_THROWS_NOTHING(yVector = new TobyFitYVector);

    delete yVector;
  }

  void test_Values_Vector_Is_Same_Size_As_Number_Of_Attributes()
  {
    using namespace Mantid::MDAlgorithms;
    TobyFitYVector yVector;

    TS_ASSERT_EQUALS(yVector.values().size(), TobyFitYVector::length());
  }

  void test_Values_Are_Not_Used_If_Inactive()
  {
    using Mantid::API::IFunction;
    using namespace Mantid::MDAlgorithms;

    const char * attrs[8] = {"Moderator", "Aperture", "Chopper", "ChopperJitter", "SampleVolume", "DetectorDepth", "DetectorArea", "DetectionTime"};

    TobyFitYVector yVector;
    for(unsigned int i = 0; i < 8; ++i)
    {
      yVector.setAttribute(attrs[i], IFunction::Attribute(false));
    }

    std::vector<double> randNums(yVector.requiredRandomNums(), 0.5);
    auto testObs = createTestCachedExperimentInfo();
    const double deltaE = 300.0;
    QOmegaPoint qOmega(1.0,2.0,3.0,deltaE);
    yVector.recalculate(randNums, *testObs, qOmega);

    const std::vector<double> & values = yVector.values();
    for(size_t i = 0; i < TobyFitYVector::length(); ++i)
    {
      TSM_ASSERT_DELTA(std::string("Value at index ") + boost::lexical_cast<std::string>(i) + " should be zero.", values[i], 0.0, 1e-10);
    }
  }

private:

  boost::shared_ptr<Mantid::MDAlgorithms::CachedExperimentInfo>
  createTestCachedExperimentInfo()
  {
    using namespace Mantid::API;
    using namespace Mantid::MDAlgorithms;
    m_expt = createTestExperiment();

    return boost::make_shared<CachedExperimentInfo>(*m_expt, (Mantid::detid_t)TEST_DET_ID);
  }

  Mantid::API::ExperimentInfo_const_sptr createTestExperiment()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    ExperimentInfo_sptr expt = boost::make_shared<ExperimentInfo>();
    Instrument_sptr testInst = createTestInstrument();
    expt->setInstrument(testInst);

    expt->mutableRun().addProperty<std::string>("deltaE-mode", "direct");
    const double ei = 447.0;
    expt->mutableRun().addProperty<double>("Ei", ei);
    std::vector<double> bins(3,0.0);
    bins[0] = 290;
    bins[1] = 310;
    bins[2] = 330;
    expt->mutableRun().storeHistogramBinBoundaries(bins);

    // Chopper
    FermiChopperModel *chopper = new FermiChopperModel;
    chopper->setAngularVelocityInHz(600.0);
    chopper->setChopperRadius(49.0/1000.);
    chopper->setSlitRadius(1300./1000.);
    chopper->setSlitThickness(2.28/1000.);
    chopper->setIncidentEnergy(ei);

    expt->setChopperModel(chopper, 0);

    // Moderator
    IkedaCarpenterModerator *sourceDescr = new IkedaCarpenterModerator;
    sourceDescr->setTiltAngleInDegrees(0.5585*180.0/M_PI);
    expt->setModeratorModel(sourceDescr);

    // Sample size
    Mantid::Geometry::Object_sptr sampleShape = ComponentCreationHelper::createCuboid(0.04, 0.025, 0.05);
    expt->mutableSample().setShape(*sampleShape);

    // OrientedLattice
    OrientedLattice * latticeRotation = new OrientedLattice;
    expt->mutableSample().setOrientedLattice(latticeRotation);
    delete latticeRotation;
    return expt;
  }


  Mantid::Geometry::Instrument_sptr createTestInstrument()
  {
    using namespace Mantid::Geometry;
    using Mantid::Kernel::V3D;

    Instrument_sptr instrument = boost::make_shared<Instrument>();
    const PointingAlong beamDir = Mantid::Geometry::Z;
    const PointingAlong upDir = Mantid::Geometry::Y;
    boost::shared_ptr<ReferenceFrame> reference =
        boost::make_shared<ReferenceFrame>(upDir, beamDir, Mantid::Geometry::Right, "frame");

    instrument->setReferenceFrame(reference);

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0.0,0.0, -12.0));
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *aperture = new ObjComponent("aperture");
    aperture->setPos(V3D(0.0,0.0, -10.01));
    Object_sptr shape = ComponentCreationHelper::createCuboid(0.047,0.047,0.001);
    aperture->setShape(shape);
    instrument->add(aperture);

    ObjComponent *chopperPos = new ObjComponent("chopperPos");
    chopperPos->setPos(V3D(0.0,0.0,-1.9));
    instrument->add(chopperPos);
    instrument->markAsChopperPoint(chopperPos);

    ObjComponent *sample = new ObjComponent("samplePos");
    sample->setPos(V3D());
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector *det1 = new Detector("det1", TEST_DET_ID, instrument.get());
    V3D detPos;
    detPos.spherical_rad(6.0340, 0.37538367018968838, 2.618430210304493);
    det1->setPos(detPos);
    shape = ComponentCreationHelper::createCappedCylinder(0.011,0.005,V3D(),V3D(0,1,0),"cyl");
    det1->setShape(shape);
    instrument->add(det1);
    instrument->markAsDetector(det1);

    return instrument;
  }

  /// Test ID value
  enum { TEST_DET_ID = 1 };
  ///Test experiment
  Mantid::API::ExperimentInfo_const_sptr m_expt;

};

#endif // MANTID_MDALGORITHMS_TOBYFITYMATRIX_H_
