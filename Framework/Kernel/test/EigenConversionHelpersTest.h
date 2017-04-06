#ifndef MANTID_KERNEL_EIGENCONVERSIONHELPERSTEST_H_
#define MANTID_KERNEL_EIGENCONVERSIONHELPERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/EigenConversionHelpers.h"

using namespace Mantid::Kernel;

class EigenConversionHelpersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EigenConversionHelpersTest *createSuite() {
    return new EigenConversionHelpersTest();
  }
  static void destroySuite(EigenConversionHelpersTest *suite) { delete suite; }

  void test_toV3D() {
    const auto result = toV3D(Eigen::Vector3d(0.1, 0.2, 0.4));
    TS_ASSERT_EQUALS(result[0], 0.1);
    TS_ASSERT_EQUALS(result[1], 0.2);
    TS_ASSERT_EQUALS(result[2], 0.4);
  }

  void test_toQuat() {
    const Eigen::Vector3d start(0.1, 0.2, 0.4);
    const Eigen::Vector3d end(0.2, -1.234, 10.1);
    const auto rot = Eigen::Quaterniond::FromTwoVectors(start, end);
    auto probe = toV3D(start);
    toQuat(rot).rotate(probe);
    const auto expected = toV3D(rot * start);
    TS_ASSERT_DELTA(probe[0], expected[0], 1e-15);
    TS_ASSERT_DELTA(probe[1], expected[1], 1e-15);
    TS_ASSERT_DELTA(probe[2], expected[2], 1e-15);
  }

  void test_toVector3d() {
    const auto result = toVector3d(V3D(0.1, 0.2, 0.4));
    TS_ASSERT_EQUALS(result[0], 0.1);
    TS_ASSERT_EQUALS(result[1], 0.2);
    TS_ASSERT_EQUALS(result[2], 0.4);
  }

  void test_toQuaterniond() {
    const Eigen::Vector3d start(0.1, 0.2, 0.4);
    const Eigen::Vector3d end(0.2, -1.234, 10.1);
    const auto rot = Eigen::Quaterniond::FromTwoVectors(start, end);
    TS_ASSERT(toQuaterniond(toQuat(rot)).isApprox(rot, 1e-15));
  }
};

#endif /* MANTID_KERNEL_EIGENCONVERSIONHELPERSTEST_H_ */
