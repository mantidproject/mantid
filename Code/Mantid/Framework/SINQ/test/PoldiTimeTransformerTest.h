#ifndef MANTID_SINQ_POLDITIMETRANSFORMERTEST_H_
#define MANTID_SINQ_POLDITIMETRANSFORMERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"

using namespace Mantid::Poldi;

using ::testing::Return;

class PoldiTimeTransformerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiTimeTransformerTest *createSuite() { return new PoldiTimeTransformerTest(); }
  static void destroySuite( PoldiTimeTransformerTest *suite ) { delete suite; }

  PoldiTimeTransformerTest()
  {
      m_detector = boost::shared_ptr<ConfiguredHeliumDetector>(new ConfiguredHeliumDetector);
      m_chopper = boost::shared_ptr<MockChopper>(new MockChopper);

      m_spectrum = PoldiSourceSpectrum_sptr(new ConfiguredSpectrum);

      EXPECT_CALL(*m_chopper, distanceFromSample())
              .WillRepeatedly(Return(11800.0));

      EXPECT_CALL(*m_chopper, zeroOffset())
              .WillRepeatedly(Return(0.15));

      m_instrument = PoldiInstrumentAdapter_sptr(new FakePoldiInstrumentAdapter);
  }

  void testDetectorCharacteristics()
  {
      double distance = 1996.017;
      double tof1A = 4947.990;
      double twoTheta = 1.577358;

      double sinTheta = 0.70942287322834615878;
      double cosTheta = 0.70478307793280472246;

      DetectorElementCharacteristics characteristics(static_cast<int>(m_detector->centralElement()), m_detector, m_chopper);

      TS_ASSERT_DELTA(characteristics.twoTheta, twoTheta, 1e-6);
      TS_ASSERT_DELTA(characteristics.distance, distance, 1e-3);
      TS_ASSERT_DELTA(characteristics.totalDistance, distance + 11800.0, 1e-3);
      TS_ASSERT_DELTA(characteristics.tof1A, tof1A, 1e-3);
      TS_ASSERT_DELTA(characteristics.sinTheta, sinTheta, 1e-6);
      TS_ASSERT_DELTA(characteristics.cosTheta, cosTheta, 1e-6);

      TestablePoldiTimeTransformer transformer;
      DetectorElementCharacteristics center = transformer.getDetectorCenterCharacteristics(m_detector, m_chopper);

      TS_ASSERT_EQUALS(characteristics.twoTheta, center.twoTheta);
      TS_ASSERT_EQUALS(characteristics.distance, center.distance);
      TS_ASSERT_EQUALS(characteristics.totalDistance, center.totalDistance);
      TS_ASSERT_EQUALS(characteristics.tof1A, center.tof1A);
      TS_ASSERT_EQUALS(characteristics.sinTheta, center.sinTheta);
      TS_ASSERT_EQUALS(characteristics.cosTheta, center.cosTheta);
  }

  void testDetectorFactors()
  {
      DetectorElementCharacteristics center(static_cast<int>(m_detector->centralElement()), m_detector, m_chopper);

      DetectorElementData data(102, center, m_detector, m_chopper);

      TS_ASSERT_DELTA(data.intensityFactor(), 1.010685, 1e-6);
      TS_ASSERT_DELTA(data.lambdaFactor(), 2.6941614e-4, 1e-11);
      TS_ASSERT_DELTA(data.timeFactor(), 0.9346730, 1e-7);
  }

  void testGetDetectorElementData()
  {
      TestablePoldiTimeTransformer transformer;
      std::vector<DetectorElementData_const_sptr> elements = transformer.getDetectorElementData(m_detector, m_chopper);
      DetectorElementCharacteristics center = transformer.getDetectorCenterCharacteristics(m_detector, m_chopper);

      DetectorElementData data(102, center, m_detector, m_chopper);

      TS_ASSERT_EQUALS(data.intensityFactor(), elements[102]->intensityFactor());
      TS_ASSERT_DELTA(data.lambdaFactor(), 2.6941614e-4, 1e-11);
      TS_ASSERT_DELTA(data.timeFactor(), 0.9346730, 1e-7);
  }

  void testInitializationFromInstrument()
  {
      TestablePoldiTimeTransformer transformer;

      TS_ASSERT_THROWS_NOTHING(transformer.initializeFromPoldiInstrument(m_instrument));

      PoldiInstrumentAdapter_sptr invalid;
      TS_ASSERT_THROWS(transformer.initializeFromPoldiInstrument(invalid), std::invalid_argument);
  }

  void TestCalculatedTotalIntensity()
  {
      double centre = Conversions::qToD(5.667449);

      TestablePoldiTimeTransformer function;
      function.initializeFromPoldiInstrument(m_instrument);
      function.m_chopperSlits = 8;

      TS_ASSERT_DELTA(fabs(1.0 - function.calculatedTotalIntensity(centre)/8220.165039062), 0.0, 1e-7);
  }


private:
  class TestablePoldiTimeTransformer : PoldiTimeTransformer
  {
      friend class PoldiTimeTransformerTest;

      TestablePoldiTimeTransformer() : PoldiTimeTransformer() {}
  };

  boost::shared_ptr<ConfiguredHeliumDetector> m_detector;
  boost::shared_ptr<MockChopper> m_chopper;
  PoldiSourceSpectrum_sptr m_spectrum;

  PoldiInstrumentAdapter_sptr m_instrument;
};


#endif /* MANTID_SINQ_POLDITIMETRANSFORMERTEST_H_ */
