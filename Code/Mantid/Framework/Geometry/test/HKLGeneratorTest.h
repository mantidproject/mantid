#ifndef MANTID_GEOMETRY_HKLGENERATORTEST_H_
#define MANTID_GEOMETRY_HKLGENERATORTEST_H_

#include <cxxtest/TestSuite.h>
#include <iostream>

#include "MantidGeometry/Crystal/HKLGenerator.h"
#include "MantidGeometry/Crystal/HKLFilter.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

#include "MantidKernel/Timer.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class HKLGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLGeneratorTest *createSuite() { return new HKLGeneratorTest(); }
  static void destroySuite(HKLGeneratorTest *suite) { delete suite; }

  void test_HKLGeneratorReturnsCorrectSizeSymmetricInt() {
    HKLGenerator gen(2, 2, 2);

    TS_ASSERT_EQUALS(gen.size(), 125);
  }

  void test_HKLGeneratorReturnsCorrectSizeSymmetricV3D() {
    HKLGenerator gen(V3D(2, 2, 2));

    TS_ASSERT_EQUALS(gen.size(), 125);
  }

  void test_HKLGeneratorReturnsCorrectSizeAsymmetricV3D() {
    HKLGenerator gen(V3D(-2, -1, -5), V3D(3, 4, -2));

    TS_ASSERT_EQUALS(gen.size(), 144);
  }

  void testSpeed() {
    Timer t;
    size_t N = 1;

    SpaceGroup_const_sptr sg =
        SpaceGroupFactory::Instance().createSpaceGroup("F d -3 m");

    HKLFilterSpaceGroup filter(sg);
    HKLGenerator gen(10, 10, 10);

    for (size_t i = 0; i < N; ++i) {
      std::vector<V3D> hkls;
      hkls.reserve(gen.size());
      std::copy_if(gen.begin(), gen.end(), std::back_inserter(hkls), filter);

      TS_ASSERT_LESS_THAN(0, hkls.size());
      std::cout << hkls.size() << std::endl;
      // TS_ASSERT_EQUALS(hkls.size(), 61 * 61 * 61);
    }

    std::cout << t.elapsed() / N << std::endl;
  }

  void testSpeed_old() {
    Timer t;
    size_t N = 100;

    SpaceGroup_const_sptr sg =
        SpaceGroupFactory::Instance().createSpaceGroup("C m c m");

    for (size_t i = 0; i < N; ++i) {
      std::vector<V3D> hkls;
      hkls.reserve(61 * 61 * 61);
      for (int h = -30; h <= 30; ++h) {
        for (int k = -30; k <= 30; ++k) {
          for (int l = -30; l <= 30; ++l) {
            V3D hkl(h, k, l);

            if (sg->isAllowedReflection(hkl)) {
              hkls.push_back(hkl);
            }
          }
        }
      }

      TS_ASSERT_LESS_THAN(0, hkls.size());
      // TS_ASSERT_EQUALS(hkls.size(), 61 * 61 * 61);
    }

    std::cout << t.elapsed() / N << std::endl;
  }
};

#endif /* MANTID_GEOMETRY_HKLGENERATORTEST_H_ */
