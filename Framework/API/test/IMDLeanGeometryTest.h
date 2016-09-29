#ifndef MANTID_API_IMDLEANGEOMETRYTEST_H_
#define MANTID_API_IMDLEANGEOMETRYTEST_H_

#include "MantidAPI/IMDLeanGeometry.h"

#include <cxxtest/TestSuite.h>

/// This class is pure abstract - nothing can be tested here.
class IMDLeanGeometryTest : public CxxTest::TestSuite {

public:
  void test_noop() {
    Mantid::API::IMDLeanGeometry *geom1 = nullptr;
    TSM_ASSERT_EQUALS("nullptr should be nullptr", nullptr, geom1);

    boost::shared_ptr<Mantid::API::IMDLeanGeometry> geom2;
    TSM_ASSERT_EQUALS("Null shared pointer should be nullptr", nullptr,
                      geom2.get());
  }
};

#endif /* MANTID_API_IMDLEANGEOMETRYTEST_H_ */
