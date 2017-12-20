#ifndef MANTID_GEOMETRY_COMPONENTVISITORHELPERTEST_H_
#define MANTID_GEOMETRY_COMPONENTVISITORHELPERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ComponentVisitorHelper.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
using namespace Mantid::Geometry;

std::ostream& operator<<(std::ostream& out, const Mantid::Geometry::IComponent& component)
{
    component.printSelf(out);
    return out;
}

namespace {
class MockComponentVisitor : public ComponentVisitor {
 public:
  MOCK_METHOD1(registerComponentAssembly,
      size_t(const Mantid::Geometry::ICompAssembly &assembly));
  MOCK_METHOD1(registerGenericComponent,
      size_t(const Mantid::Geometry::IComponent &component));
  MOCK_METHOD1(registerGenericObjComponent,
      size_t(const Mantid::Geometry::IObjComponent &objComponent));
  MOCK_METHOD1(registerDetector,
      size_t(const Mantid::Geometry::IDetector &detector));
  MOCK_METHOD1(registerStructuredBank,
      size_t(const Mantid::Geometry::ICompAssembly &bank));
  MOCK_METHOD1(registerBankOfTubes,
      size_t(const Mantid::Geometry::ICompAssembly &bank));
  MOCK_METHOD1(registerTube,
      size_t(const Mantid::Geometry::ICompAssembly &tube));
};

}

class ComponentVisitorHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentVisitorHelperTest *createSuite() { return new ComponentVisitorHelperTest(); }
  static void destroySuite( ComponentVisitorHelperTest *suite ) { delete suite; }


  void test_visit_as_tube()
  {
    MockComponentVisitor visitor;
    CompAssembly assembly;

    // Test unsuccesful match. Match not close enough, should call generic registeration for assembly instead
    EXPECT_CALL(visitor, registerComponentAssembly(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "bankoftube");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));

    // Test unsuccessful match if not followed by digits, should call generic registration for assembly instead
    EXPECT_CALL(visitor, registerComponentAssembly(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "tube12x");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));

    // Test successful match
    EXPECT_CALL(visitor, registerTube(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "tube123");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));

    // Test successful ignore case
    EXPECT_CALL(visitor, registerTube(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "Tube1");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));
  }

  void test_visit_as_bank_of_tubes()
  {
    MockComponentVisitor visitor;
    CompAssembly assembly;

    // Match not close enough, should end with PACK. Should call generic registeration for assembly instead
    EXPECT_CALL(visitor, registerComponentAssembly(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "somepackz");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));

    // Match not close enough, alphabetical characters preceding only. Should call generic registeration for assembly instead
    EXPECT_CALL(visitor, registerComponentAssembly(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "16pack");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));

    // Test successful match
    EXPECT_CALL(visitor, registerBankOfTubes(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "sixteenpack");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));

    // Test successful ignore case
    EXPECT_CALL(visitor, registerBankOfTubes(testing::_)).Times(1);
    ComponentVisitorHelper::visitAssembly(visitor, assembly, "EightPack");
    TS_ASSERT(testing::Mock::VerifyAndClearExpectations(&visitor));
  }

};


#endif /* MANTID_GEOMETRY_COMPONENTVISITORHELPERTEST_H_ */
