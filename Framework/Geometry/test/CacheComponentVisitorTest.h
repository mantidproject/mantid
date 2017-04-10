#ifndef MANTID_GEOMETRY_CACHECOMPONENTVISITORTEST_H_
#define MANTID_GEOMETRY_CACHECOMPONENTVISITORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/CacheComponentVisitor.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/ObjComponent.h"

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
};

#endif /* MANTID_API_CACHECOMPONENTVISITORTEST_H_ */
