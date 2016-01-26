#ifndef MANTID_API_IMDLEANGEOMETRYTEST_H_
#define MANTID_API_IMDLEANGEOMETRYTEST_H_

#include "MantidAPI/IMDLeanGeometry.h"

#include <cxxtest/TestSuite.h>

/// This class is pure abstract - nothing can be tested here.
class IMDLeanGeometryTest : public CxxTest::TestSuite {

public:
  void test_noop() {
    IMDLeanGeometry *geom1;

    boost::shared_ptr<IMDLeanGeometry> geom2;
  }
};

#endif /* MANTID_API_IMDLEANGEOMETRYTEST_H_ */
