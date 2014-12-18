#ifndef OBSERVATIONTEST_H_
#define OBSERVATIONTEST_H_

#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>

using Mantid::MDAlgorithms::CachedExperimentInfo;
using Mantid::Kernel::DeltaEMode;
using Mantid::Kernel::V3D;

class CachedExperimentInfoTest : public CxxTest::TestSuite
{

private:
  // Avoiding booleans
  enum TestObjectType { NoChopper, WithChopper, NoAperture, WithAperture, NoDetShape, WithDetShape};

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CachedExperimentInfoTest *createSuite() { return new CachedExperimentInfoTest(); }
  static void destroySuite( CachedExperimentInfoTest *suite ) { delete suite; }
  
  CachedExperimentInfoTest() : m_test_ei(12.1), m_test_ef(15.5), m_sourcePos(0.0,0.0,-10.0),
    m_chopperPos(0,0,-3.), m_aperturePos(0.,0.,-8.)
  {
  }

  void test_trying_to_construct_object_with_unknown_id_throws_exception()
  {
    //createEmptyExptInfo();
    auto expt = boost::make_shared<Mantid::API::ExperimentInfo>();
    TS_ASSERT_THROWS(CachedExperimentInfo(*expt, 1000), Mantid::Kernel::Exception::NotFoundError);
  }

  void test_trying_to_construct_object_with_no_chopper_throws()
  {
    TS_ASSERT_THROWS(createTestCachedExperimentInfo(NoChopper), std::invalid_argument);
  }

  void test_trying_to_construct_object_with_no_aperature_throws()
  {
    TS_ASSERT_THROWS(createTestCachedExperimentInfo(WithChopper, NoAperture), std::invalid_argument);
  }

  void test_trying_to_construct_object_with_no_det_shape_throws()
  {
    TS_ASSERT_THROWS(createTestCachedExperimentInfo(WithChopper, WithAperture,DeltaEMode::Direct, V3D(1,1,1), NoDetShape),
                     std::invalid_argument);
  }

  void test_efixed_returns_Ei_for_direct_mode()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper, WithAperture, DeltaEMode::Direct);
    TS_ASSERT_EQUALS(event->getEFixed(), m_test_ei);
  }

  void test_efixed_returns_EFixed_for_indirect_mode()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper, WithAperture, DeltaEMode::Indirect);
    TS_ASSERT_EQUALS(event->getEFixed(), m_test_ef);
  }

  void test_theta_angle_from_beam_is_correct()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo();
    TS_ASSERT_DELTA(event->twoTheta(), 0.440510663, 1e-09);
  }

  void test_phi_angle_from_beam_is_correct()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo();
    TS_ASSERT_DELTA(event->phi(), M_PI/4., 1e-09);
  }

  void test_sample_to_detector_distance_gives_expected_results()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo();

    TS_ASSERT_DELTA(event->sampleToDetectorDistance(), std::sqrt(11.), 1e-12);
  }

  void test_moderator_to_first_chopper_distance_gives_expected_result()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper);
    const double expectedDistance = m_chopperPos.distance(m_sourcePos);

    TS_ASSERT_DELTA(event->moderatorToFirstChopperDistance(), expectedDistance, 1e-12);
  }

  void test_first_chopper_to_sample_distance_gives_expected_result()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper);
    const double expectedDistance = m_chopperPos.distance(V3D());

    TS_ASSERT_DELTA(event->firstChopperToSampleDistance(), expectedDistance, 1e-12);
  }

  void test_first_aperture_to_first_chopper_distance_gives_expected_result()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper, WithAperture);
    const double expectedDistance = m_chopperPos.distance(m_aperturePos);

    TS_ASSERT_DELTA(event->firstApertureToFirstChopperDistance(), expectedDistance, 1e-12);
  }

  void test_aperture_size_is_expected()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper, WithAperture);
    const double expectedWidth(0.08), expectedHeight(0.05);
    
    std::pair<double,double> apSize = event->apertureSize();
    TS_ASSERT_DELTA(apSize.first, expectedWidth, 1e-4);
    TS_ASSERT_DELTA(apSize.second, expectedHeight, 1e-4);
  }

  void test_sample_widths_are_expected()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper, WithAperture);

    const Mantid::Kernel::V3D & sampleWidths = event->sampleCuboid();
    TS_ASSERT_DELTA(sampleWidths.X(), 0.2, 1e-5);
    TS_ASSERT_DELTA(sampleWidths.Y(), 0.4, 1e-5);
    TS_ASSERT_DELTA(sampleWidths.Z(), 0.6, 1e-5);

  }

  void test_detector_volume_gives_expected_pos()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper, WithAperture,DeltaEMode::Direct, V3D(1,1,1));

    Mantid::Kernel::V3D volume;
    TS_ASSERT_THROWS_NOTHING(volume = event->detectorVolume());

    TS_ASSERT_DELTA(volume[0], 0.0240, 1e-6);
    TS_ASSERT_DELTA(volume[1], 0.0100, 1e-6);
    TS_ASSERT_DELTA(volume[2], 0.0240, 1e-6);
  }

  void test_labToDetTransformation_Yields_Expected_Matrix()
  {
    boost::shared_ptr<CachedExperimentInfo> event = createTestCachedExperimentInfo(WithChopper, WithAperture, DeltaEMode::Direct, V3D(1,1,1));
    const double sintheta = std::sqrt(2./3.);
    const double costheta = 1./std::sqrt(3.);
    const double sinphi = 0.5*std::sqrt(2.);
    const double cosphi = 0.5*std::sqrt(2.);
    double expectedMatrix[3][3] =
      {{costheta*cosphi,-sinphi,costheta}, \
       {costheta*cosphi,cosphi, costheta}, \
       {-sintheta,0.0,costheta} \
      };

    Mantid::Kernel::DblMatrix labToDet = event->labToDetectorTransform();
    for(int i = 0; i < 3; ++i)
    {
      for(int j = 0; j < 3; ++j)
      {
        std::ostringstream os;
        os << "Mismatch at row=" << i << ", col=" << j;
        TSM_ASSERT_DELTA(os.str(), labToDet[i][j], expectedMatrix[i][j], 1e-12);
      }
    }
  }

private:

  void createEmptyExptInfo()
  {
    m_expt = boost::make_shared<Mantid::API::ExperimentInfo>();
  }

  boost::shared_ptr<CachedExperimentInfo> createTestCachedExperimentInfo(
      const TestObjectType addChopper = WithChopper,
      const TestObjectType addAperture = WithAperture,
      const DeltaEMode::Type emode = DeltaEMode::Direct,
      const V3D & detPos = V3D(1,1,3),
      const TestObjectType addDetShape = WithDetShape)
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    createEmptyExptInfo();

    Instrument_sptr instrument(new Instrument("test-inst"));
    instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(Mantid::Geometry::Y, Mantid::Geometry::Z, Mantid::Geometry::Right, "frame"));
    Detector *det1 = new Detector("det1", g_test_id, instrument.get());
    if(addDetShape == WithDetShape)
    {
      Object_sptr shape = ComponentCreationHelper::createCappedCylinder(0.012, 0.01, detPos,V3D(0,1,0),"cyl");
      det1->setShape(shape);
    }
    instrument->add(det1);
    det1->setPos(detPos);
    instrument->markAsDetector(det1);


    ObjComponent *source = new ObjComponent("source");
    source->setPos(m_sourcePos);
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *samplePos = new ObjComponent("samplePos");
    instrument->add(samplePos);
    instrument->markAsSamplePos(samplePos);
    Object_sptr sampleShape = ComponentCreationHelper::createCuboid(0.1,0.2,0.3);
    m_expt->mutableSample().setShape(*sampleShape);

    if(addChopper == WithChopper)
    {
      ObjComponent* chopper = new ObjComponent("firstChopperPos");
      chopper->setPos(m_chopperPos);
      instrument->add(chopper);
      instrument->markAsChopperPoint(chopper);
    }

    if(addAperture == WithAperture)
    {
      ObjComponent *aperture = new ObjComponent("aperture");
      aperture->setPos(m_aperturePos);
      Object_sptr shape = ComponentCreationHelper::createCuboid(0.04, 0.025, 0.05);
      aperture->setShape(shape);
      instrument->add(aperture);
    }

    m_expt->setInstrument(instrument);
    m_expt->mutableRun().addProperty("deltaE-mode", DeltaEMode::asString(emode));
    Mantid::Geometry::OrientedLattice latt(5.57,5.51,12.298);
    m_expt->mutableSample().setOrientedLattice(&latt);

    if(emode == DeltaEMode::Direct) // Direct
    {
      // Add log entry
      m_expt->mutableRun().addProperty("Ei", m_test_ei);
    }
    else if(emode ==  DeltaEMode::Indirect) // Indirect
    {
      m_expt->instrumentParameters().addDouble(det1,"EFixed", m_test_ef);
    }
    else {}

    return boost::make_shared<CachedExperimentInfo>(*m_expt, static_cast<Mantid::detid_t>(g_test_id));
  }

  enum { g_test_id = 1 };
  const double m_test_ei;
  const double m_test_ef;
  const V3D m_sourcePos;
  const V3D m_chopperPos;
  const V3D m_aperturePos;

  Mantid::API::ExperimentInfo_sptr m_expt;
};

#endif /* OBSERVATIONTEST_H_ */
