#ifndef OBSERVATIONTEST_H_
#define OBSERVATIONTEST_H_

#include "MantidMDAlgorithms/Quantification/Observation.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>

using Mantid::MDAlgorithms::Observation;
using Mantid::Kernel::DeltaEMode;
using Mantid::Kernel::V3D;

class ObservationTest : public CxxTest::TestSuite
{

private:
  // Avoiding booleans
  enum TestObjectType { NoChopper, WithChopper, NoAperture, WithAperture, NoDetShape, WithDetShape};

public:

  ObservationTest() : m_test_ei(12.1), m_test_ef(15.5), m_sourcePos(0.0,0.0,-10.0),
    m_chopperPos(0,0,-3.), m_aperturePos(0.,0.,-8.)
  {
  }

  void test_trying_to_construct_object_with_no_instrument_throws_exception()
  {
    TS_ASSERT_THROWS(Observation(createEmptyExptInfo(), g_test_id), std::runtime_error);
  }

  void test_trying_to_construct_object_with_unknown_id_throws_exception()
  {
    TS_ASSERT_THROWS(Observation(createEmptyExptInfo(), 1000), Mantid::Kernel::Exception::NotFoundError);
  }

  void test_efixed_returns_Ei_for_direct_mode()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper, WithAperture, DeltaEMode::Direct);
    TS_ASSERT_EQUALS(event->getEFixed(), m_test_ei);
  }

  void test_efixed_returns_EFixed_for_indirect_mode()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper, WithAperture, DeltaEMode::Indirect);
    TS_ASSERT_EQUALS(event->getEFixed(), m_test_ef);
  }

  void test_theta_angle_from_beam_is_correct()
  {
    boost::shared_ptr<Observation> event = createTestObservation();
    TS_ASSERT_DELTA(event->twoTheta(), 0.440510663, 1e-09);
  }

  void test_phi_angle_from_beam_is_correct()
  {
    boost::shared_ptr<Observation> event = createTestObservation();
    TS_ASSERT_DELTA(event->phi(), M_PI/4., 1e-09);
  }

  void test_sample_to_detector_distance_gives_expected_results()
  {
    boost::shared_ptr<Observation> event = createTestObservation();

    TS_ASSERT_DELTA(event->sampleToDetectorDistance(), std::sqrt(11.), 1e-12);
  }

  void test_moderator_to_first_chopper_distance_throws_without_chopper_present()
  {
    boost::shared_ptr<Observation> event = createTestObservation(NoChopper);

    TS_ASSERT_THROWS(event->moderatorToFirstChopperDistance(), std::invalid_argument);
  }

  void test_moderator_to_first_chopper_distance_gives_expected_result()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper);
    const double expectedDistance = m_chopperPos.distance(m_sourcePos);

    TS_ASSERT_DELTA(event->moderatorToFirstChopperDistance(), expectedDistance, 1e-12);
  }

  void test_first_chopper_to_sample_distance_throws_if_no_chopper_present()
  {
    boost::shared_ptr<Observation> event = createTestObservation(NoChopper);

    TS_ASSERT_THROWS(event->firstChopperToSampleDistance(), std::invalid_argument);
  }

  void test_first_chopper_to_sample_distance_gives_expected_result()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper);
    const double expectedDistance = m_chopperPos.distance(V3D());

    TS_ASSERT_DELTA(event->firstChopperToSampleDistance(), expectedDistance, 1e-12);
  }

  void test_chopper_to_aperture_distance_throws_if_no_aperture_present()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper, NoAperture);

    TS_ASSERT_THROWS(event->firstApertureToFirstChopperDistance(), std::invalid_argument);
  }

  void test_chopper_to_aperture_distance_throws_if_no_chopper_present()
  {
    boost::shared_ptr<Observation> event = createTestObservation(NoChopper, WithAperture);

    TS_ASSERT_THROWS(event->firstApertureToFirstChopperDistance(), std::invalid_argument);
  }

  void test_chopper_to_aperture_distance_throws_if_no_chopper_or_aperture_present()
  {
    boost::shared_ptr<Observation> event = createTestObservation(NoChopper, NoAperture);

    TS_ASSERT_THROWS(event->firstApertureToFirstChopperDistance(), std::invalid_argument);
  }

  void test_first_aperture_to_first_chopper_distance_gives_expected_result()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper, WithAperture);
    const double expectedDistance = m_chopperPos.distance(m_aperturePos);

    TS_ASSERT_DELTA(event->firstApertureToFirstChopperDistance(), expectedDistance, 1e-12);
  }

  void test_sample_over_detector_volume_throws_when_detector_has_no_shape()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper, WithAperture,DeltaEMode::Direct, V3D(1,1,1), NoDetShape);

    TS_ASSERT_THROWS(event->sampleOverDetectorVolume(0.2, 0.15, 0.75), std::invalid_argument);
  }

  void test_sample_over_detector_volume_throws_gives_expected_pos_with_valid_random_numbers()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper, WithAperture,DeltaEMode::Direct, V3D(1,1,1));

    Mantid::Kernel::V3D detectionPoint;
    TS_ASSERT_THROWS_NOTHING(detectionPoint = event->sampleOverDetectorVolume(0.2, 0.15, 0.75));

    TS_ASSERT_DELTA(detectionPoint[Mantid::Geometry::X], 0.006, 1e-8);
    TS_ASSERT_DELTA(detectionPoint[Mantid::Geometry::Y], -0.00350008, 1e-8);
    TS_ASSERT_DELTA(detectionPoint[Mantid::Geometry::Z], -0.0072, 1e-8);

  }
  void test_labToDetTransformation_Yields_Expected_Matrix()
  {
    boost::shared_ptr<Observation> event = createTestObservation(WithChopper, WithAperture, DeltaEMode::Direct, V3D(1,1,1));
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

  Mantid::API::ExperimentInfo_sptr createEmptyExptInfo()
  {
    return boost::make_shared<Mantid::API::ExperimentInfo>();
  }

  boost::shared_ptr<Observation> createTestObservation(
      const TestObjectType addChopper = WithChopper,
      const TestObjectType addAperture = WithAperture,
      const DeltaEMode::Type emode = DeltaEMode::Elastic,
      const V3D & detPos = V3D(1,1,3),
      const TestObjectType addDetShape = WithDetShape)
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    ExperimentInfo_sptr exptInfo = createEmptyExptInfo();

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
      instrument->add(aperture);
    }

    exptInfo->setInstrument(instrument);
    exptInfo->mutableRun().addProperty("deltaE-mode", DeltaEMode::asString(emode));

    if(emode == DeltaEMode::Direct) // Direct
    {
      // Add log entry
      exptInfo->mutableRun().addProperty("Ei", m_test_ei);
    }
    else if(emode ==  DeltaEMode::Indirect) // Indirect
    {
      exptInfo->instrumentParameters().addDouble(det1,"EFixed", m_test_ef);
    }
    else {}

    return boost::make_shared<Observation>(exptInfo, static_cast<Mantid::detid_t>(g_test_id));
  }

  enum { g_test_id = 1 };
  const double m_test_ei;
  const double m_test_ef;
  const V3D m_sourcePos;
  const V3D m_chopperPos;
  const V3D m_aperturePos;
};

#endif /* OBSERVATIONTEST_H_ */
