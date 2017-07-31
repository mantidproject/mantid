#ifndef MANTID_BEAMLINE_BEAMLINETEST_H_
#define MANTID_BEAMLINE_BEAMLINETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidBeamline/Beamline.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"

using namespace Mantid::Beamline;

class BeamlineTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BeamlineTest *createSuite() { return new BeamlineTest(); }
  static void destroySuite(BeamlineTest *suite) { delete suite; }

  void test_default_construct_empty() {
    Beamline beamline;
    TS_ASSERT(beamline.empty());
  }

  void test_cross_referencing_applied() {

    ComponentInfo componentInfo{};
    DetectorInfo detectorInfo{};

    // Sanity check no initial referencing
    TS_ASSERT(!componentInfo.hasDetectorInfo());
    TS_ASSERT(!detectorInfo.hasComponentInfo());

    Beamline beamline(std::move(componentInfo), std::move(detectorInfo));

    TSM_ASSERT("ComponentInfo should ref DetectorInfo",
               beamline.componentInfo().hasDetectorInfo());
    TSM_ASSERT("DetectorInfo should ref ComponentInfo",
               beamline.detectorInfo().hasComponentInfo());
    TS_ASSERT(!beamline.empty());
  }

  void test_copyable() {

    Beamline a(ComponentInfo{}, DetectorInfo{});
    Beamline b(a);

    // Copying should yield new detector info
    TS_ASSERT_DIFFERS(&a.detectorInfo(), &b.detectorInfo());
    // Copying should yield new component info
    TS_ASSERT_DIFFERS(&a.componentInfo(), &b.componentInfo());
    TS_ASSERT(!a.empty());
    TS_ASSERT(!b.empty());
  }

  void test_assignable() {

    Beamline a(ComponentInfo{}, DetectorInfo{});
    Beamline b{};
    b = a;

    // Should yield new detector info
    TS_ASSERT_DIFFERS(&a.detectorInfo(), &b.detectorInfo());
    // Should yield new component info
    TS_ASSERT_DIFFERS(&a.componentInfo(), &b.componentInfo());
    TS_ASSERT(!a.empty());
    TS_ASSERT(!b.empty());
  }

  void test_moveable() {
    Beamline a(ComponentInfo{}, DetectorInfo{});
    auto *detInfo = &a.detectorInfo();
    auto *compInfo = &a.componentInfo();
    // Should NOT yield new infos
    Beamline b(std::move(a));
    TS_ASSERT_EQUALS(&b.detectorInfo(), detInfo);
    TS_ASSERT_EQUALS(&b.componentInfo(), compInfo);
    TS_ASSERT(!b.empty());
  }

  void test_move_assignment() {
    Beamline a(ComponentInfo{}, DetectorInfo{});
    auto *detInfo = &a.detectorInfo();
    auto *compInfo = &a.componentInfo();
    // Should NOT yield new infos
    Beamline b{};
    b = std::move(a);
    TS_ASSERT_EQUALS(&b.detectorInfo(), detInfo);
    TS_ASSERT_EQUALS(&b.componentInfo(), compInfo);
    TS_ASSERT(!b.empty());
  }

  void test_alias() {
    Beamline a(ComponentInfo{}, DetectorInfo{});
    auto *detInfo = &a.detectorInfo();
    auto *compInfo = &a.componentInfo();
    // Should NOT yield new infos
    Beamline b = a.alias();
    TS_ASSERT_EQUALS(&b.detectorInfo(), detInfo);
    TS_ASSERT_EQUALS(&b.componentInfo(), compInfo);
    TS_ASSERT(!b.empty());
  }

  void test_alias_ownership() {
    Beamline b;
    {
      // alias temporary
      b = Beamline(ComponentInfo{}, DetectorInfo{}).alias();
    }
    // This will throw if not shared ownership
    b.detectorInfo().size();
    // This will throw if not shared ownership
    b.componentInfo().size();
  }
};

#endif /* MANTID_BEAMLINE_BEAMLINETEST_H_ */
