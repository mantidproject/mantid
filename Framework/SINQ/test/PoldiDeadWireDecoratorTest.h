// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SINQ_POLDIDEADWIREDECORATORTEST_H_
#define MANTID_SINQ_POLDIDEADWIREDECORATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"

using namespace Mantid::Poldi;

using ::testing::Return;
using ::testing::_;

class PoldiDeadWireDecoratorTest : public CxxTest::TestSuite {
private:
  boost::shared_ptr<MockDetector> m_detector;
  std::set<int> m_validDeadWires;
  std::set<int> m_invalidDeadWires;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiDeadWireDecoratorTest *createSuite() {
    return new PoldiDeadWireDecoratorTest();
  }
  static void destroySuite(PoldiDeadWireDecoratorTest *suite) { delete suite; }

  PoldiDeadWireDecoratorTest() {
    m_detector = boost::make_shared<MockDetector>();

    int valid[] = {0, 1, 2, 5, 99, 299, 399};
    int invalid[] = {0, 1, 400};

    m_validDeadWires = std::set<int>(valid, valid + 7);
    m_invalidDeadWires = std::set<int>(invalid, invalid + 3);
  }

  void testInitialization() {
    PoldiDeadWireDecorator decorator(m_validDeadWires, m_detector);

    TS_ASSERT_EQUALS(decorator.deadWires(), m_validDeadWires);
  }

  void testAssignment() {
    PoldiDeadWireDecorator decorator(std::set<int>(), m_detector);

    decorator.setDeadWires(m_validDeadWires);
    TS_ASSERT_EQUALS(decorator.deadWires(), m_validDeadWires);
  }

  void testelementCount() {
    PoldiDeadWireDecorator decorator(m_validDeadWires, m_detector);

    TS_ASSERT_EQUALS(decorator.elementCount(), 393);
  }

  void testavailableElements() {
    PoldiDeadWireDecorator decorator(m_validDeadWires, m_detector);

    const std::vector<int> goodElements = decorator.availableElements();

    TS_ASSERT_EQUALS(goodElements.front(), 3);
    TS_ASSERT_EQUALS(goodElements.back(), 398);
  }

  void testinvalid() {
    PoldiDeadWireDecorator decorator(std::set<int>(), m_detector);

    TS_ASSERT_THROWS(decorator.setDeadWires(m_invalidDeadWires),
                     const std::runtime_error &);
  }
};

#endif /* MANTID_SINQ_POLDIDEADWIREDECORATORTEST_H_ */
