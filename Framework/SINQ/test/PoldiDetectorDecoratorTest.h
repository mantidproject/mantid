// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDIDETECTORDECORATORTEST_H_
#define MANTID_SINQ_POLDIDETECTORDECORATORTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidSINQ/PoldiUtilities/PoldiDetectorDecorator.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"

using namespace Mantid::Poldi;
using ::testing::Return;
using ::testing::_;

class PoldiDetectorDecoratorTest : public CxxTest::TestSuite {
  boost::shared_ptr<MockDetector> m_detector;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiDetectorDecoratorTest *createSuite() {
    return new PoldiDetectorDecoratorTest();
  }
  static void destroySuite(PoldiDetectorDecoratorTest *suite) { delete suite; }

  PoldiDetectorDecoratorTest() {
    m_detector = boost::make_shared<MockDetector>();
  }

  void testInitialization() {
    PoldiDetectorDecorator decorator(m_detector);

    TS_ASSERT_EQUALS(decorator.decoratedDetector(), m_detector);

    decorator.setDecoratedDetector(boost::shared_ptr<PoldiAbstractDetector>());

    TS_ASSERT(!decorator.decoratedDetector());
  }

  void testForwardMethods() {
    PoldiDetectorDecorator decorator(m_detector);

    EXPECT_CALL(*m_detector, twoTheta(_)).WillOnce(Return(1.5));
    TS_ASSERT_EQUALS(decorator.twoTheta(0), 1.5);

    EXPECT_CALL(*m_detector, distanceFromSample(_)).WillOnce(Return(1999.9));
    TS_ASSERT_EQUALS(decorator.distanceFromSample(0), 1999.9);

    EXPECT_CALL(*m_detector, elementCount()).WillOnce(Return(400));
    TS_ASSERT_EQUALS(decorator.elementCount(), 400);

    EXPECT_CALL(*m_detector, centralElement()).WillOnce(Return(199));
    TS_ASSERT_EQUALS(decorator.centralElement(), 199);
    std::vector<int> forwardedAvailableElements = decorator.availableElements();
    TS_ASSERT_EQUALS(forwardedAvailableElements,
                     m_detector->availableElements());

    EXPECT_CALL(*m_detector, qLimits(_, _))
        .WillOnce(Return(std::make_pair(1.0, 5.0)));
    std::pair<double, double> forwardedQLimits = decorator.qLimits(1.1, 5.0);
    TS_ASSERT_EQUALS(forwardedQLimits.first, 1.0);
    TS_ASSERT_EQUALS(forwardedQLimits.second, 5.0);
  }

  void testForwardMethodsInvalidDetector() {
    PoldiDetectorDecorator decorator;

    TS_ASSERT_THROWS(decorator.twoTheta(0), const std::runtime_error &);
    TS_ASSERT_THROWS(decorator.distanceFromSample(0), const std::runtime_error &);
    TS_ASSERT_THROWS(decorator.elementCount(), const std::runtime_error &);
    TS_ASSERT_THROWS(decorator.centralElement(), const std::runtime_error &);
    TS_ASSERT_THROWS(decorator.availableElements(), const std::runtime_error &);
    TS_ASSERT_THROWS(decorator.qLimits(1.0, 5.0), const std::runtime_error &);
  }
};

#endif /* MANTID_SINQ_POLDIDETECTORDECORATORTEST_H_ */
