#ifndef MANTID_GEOMETRY_CACHECOMPONENTVISITORTEST_H_
#define MANTID_GEOMETRY_CACHECOMPONENTVISITORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/CacheComponentVisitor.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/Detector.h"

using namespace Mantid::Geometry;

class CacheComponentVisitorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CacheComponentVisitorTest *createSuite() {
    return new CacheComponentVisitorTest();
  }
  static void destroySuite(CacheComponentVisitorTest *suite) { delete suite; }

  void test_simple_visit() {
    CompAssembly assembly;
    CacheComponentVisitor visitor;
    TS_ASSERT_EQUALS(visitor.componentIds().size(), 0);
    assembly.registerContents(visitor);
    TS_ASSERT_EQUALS(visitor.componentIds().size(), 1);
    TS_ASSERT_EQUALS(visitor.componentIds()[0], &assembly);
  }

  void test_compound_visit() {

    auto *source = new ObjComponent("source");
    auto *sample = new ObjComponent("sample");
    CompAssembly assembly;
    assembly.add(source);
    assembly.add(sample);

    CacheComponentVisitor visitor;
    TS_ASSERT_EQUALS(visitor.componentIds().size(), 0);
    assembly.registerContents(visitor);
    TS_ASSERT_EQUALS(visitor.componentIds().size(), 3);
    TS_ASSERT_EQUALS(visitor.componentIds()[0], source);
    TS_ASSERT_EQUALS(visitor.componentIds()[1], sample);
    TS_ASSERT_EQUALS(visitor.componentIds()[2], &assembly);
  }

  void test_indexing_scheme() {
    /* Note this is testing internals. Client code should only be
     * calling the register.. methods in a few special places.
     */
    CacheComponentVisitor visitor;
    Detector detector1("det", 1, nullptr);
    TSM_ASSERT_EQUALS("Should have Index 0", 0,
                      visitor.registerDetector(detector1));

    ObjComponent comp("some_comp");
    TSM_ASSERT_EQUALS("Should have Index 1", 1,
                      visitor.registerGenericComponent(comp));

    // Detector now "steals" lower index previously allocated above. Detector
    // always first indexes.
    Detector detector2("det", 2, nullptr);
    TSM_ASSERT_EQUALS("Should have Index 1", 1,
                      visitor.registerDetector(detector2));
  }
};

#endif /* MANTID_API_CACHECOMPONENTVISITORTEST_H_ */
