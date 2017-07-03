#ifndef MANTID_KERNEL_INVISIBLEPROPERTYTEST_H_
#define MANTID_KERNEL_INVISIBLEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/InvisibleProperty.h"
#include "MantidKernel/PropertyManagerOwner.h"

#include <memory>

using namespace Mantid::Kernel;

class InvisiblePropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InvisiblePropertyTest *createSuite() {
    return new InvisiblePropertyTest();
  }
  static void destroySuite(InvisiblePropertyTest *suite) { delete suite; }

  void test_construct() { TS_ASSERT_THROWS_NOTHING(InvisibleProperty{}); }

  void test_isVisible() {
    InvisibleProperty prop;
    TS_ASSERT(!prop.isVisible(&m_owner));
  }

  void test_clone() {
    InvisibleProperty prop;
    std::unique_ptr<IPropertySettings> clone(prop.clone());
    TS_ASSERT(!clone->isVisible(&m_owner));
  }

private:
  const PropertyManagerOwner m_owner;
};

#endif /* MANTID_KERNEL_INVISIBLEPROPERTYTEST_H_ */
