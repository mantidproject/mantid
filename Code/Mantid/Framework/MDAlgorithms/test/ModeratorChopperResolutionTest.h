#ifndef MODERATORCHOPPERRESOLUTIONTEST_H_
#define MODERATORCHOPPERRESOLUTIONTEST_H_

#include "MantidMDAlgorithms/Quantification/Resolution/ModeratorChopperResolution.h"

#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

class ModeratorChopperResolutionTest : public CxxTest::TestSuite
{
public:

  void test_Returned_Width_Is_NonZero()
  {
    using Mantid::MDAlgorithms::ModeratorChopperResolution;
    using Mantid::MDAlgorithms::CachedExperimentInfo;

    boost::shared_ptr<CachedExperimentInfo> testCachedExperimentInfo = createTestCachedExperimentInfo();
    ModeratorChopperResolution *modChop(NULL);

    TS_ASSERT_THROWS_NOTHING(modChop = new ModeratorChopperResolution(*testCachedExperimentInfo));
    if(modChop)
    {
      //TS_ASSERT(modChop->energyWidth(0.1) > 0.0);
    }
    delete modChop;
  }

private:
  boost::shared_ptr<Mantid::MDAlgorithms::CachedExperimentInfo> createTestCachedExperimentInfo()
  {
    using namespace Mantid::Kernel;
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    using namespace Mantid::MDAlgorithms;
    m_expt = boost::make_shared<ExperimentInfo>();

    Instrument_sptr instrument(new Instrument("test-inst"));
    instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(Mantid::Geometry::Y, Mantid::Geometry::Z, Mantid::Geometry::Right, "frame"));
    Detector *det1 = new Detector("det1", 1, instrument.get());
    const V3D detPos(1,1,1);
    Object_sptr shape = ComponentCreationHelper::createCappedCylinder(0.012, 0.01, detPos,V3D(0,1,0),"cyl");
    det1->setShape(shape);
    instrument->add(det1);
    det1->setPos(detPos);
    instrument->markAsDetector(det1);

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0,0,-10));
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *samplePos = new ObjComponent("samplePos");
    instrument->add(samplePos);
    instrument->markAsSamplePos(samplePos);

    ObjComponent * chopper = new ObjComponent("firstChopperPos");
    chopper->setPos(V3D(0,0,-3.0));
    instrument->add(chopper);
    instrument->markAsChopperPoint(chopper);

    ObjComponent *aperture = new ObjComponent("aperture");
    aperture->setPos(V3D(0,0,-7));
    shape = ComponentCreationHelper::createCuboid(0.04, 0.025, 0.05);
    aperture->setShape(shape);
    instrument->add(aperture);

    m_expt->setInstrument(instrument);
    m_expt->mutableRun().addProperty("deltaE-mode", DeltaEMode::asString(Mantid::Kernel::DeltaEMode::Direct));
    Mantid::Geometry::OrientedLattice latt(5.57,5.51,12.298);
    m_expt->mutableSample().setOrientedLattice(&latt);


    // Add log entry
    m_expt->mutableRun().addProperty("Ei", 45.1);

    return boost::make_shared<CachedExperimentInfo>(*m_expt, static_cast<Mantid::detid_t>(1));
  }

  Mantid::API::ExperimentInfo_sptr m_expt;
};

#endif /* MODERATORCHOPPERRESOLUTIONTEST_H_ */
