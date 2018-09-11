#ifndef MANTID_GEOMETRY_REFLECTIONGENERATORTEST_H_
#define MANTID_GEOMETRY_REFLECTIONGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/ReflectionGenerator.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class ReflectionGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectionGeneratorTest *createSuite() {
    return new ReflectionGeneratorTest();
  }
  static void destroySuite(ReflectionGeneratorTest *suite) { delete suite; }

  void test_getUniqueHKLs() {
    double dMin = 0.55;
    double dMax = 4.0;

    ReflectionGenerator generator(
        CrystalStructure("4.126 4.126 4.126", "P m -3 m", "Si 0 0 0 1.0 0.01"),
        ReflectionConditionFilter::Centering);

    TS_ASSERT_THROWS_NOTHING(generator.getUniqueHKLs(dMin, dMax));

    std::vector<V3D> peaks = generator.getUniqueHKLs(dMin, dMax);

    TS_ASSERT_EQUALS(peaks.size(), 68);
    TS_ASSERT_EQUALS(peaks[0], V3D(1, 1, 0));
    TS_ASSERT_EQUALS(peaks[11], V3D(3, 2, 0));
    TS_ASSERT_EQUALS(peaks[67], V3D(7, 2, 1));

    // make d-value list and check that peaks are within limits
    std::vector<double> peaksD = generator.getDValues(peaks);

    std::sort(peaksD.begin(), peaksD.end());

    TS_ASSERT_LESS_THAN_EQUALS(dMin, peaksD.front());
    TS_ASSERT_LESS_THAN_EQUALS(peaksD.back(), dMax);
  }

  void test_getHKLs() {
    double dMin = 0.55;
    double dMax = 4.0;

    // make a structure with P-1
    ReflectionGenerator generator(
        CrystalStructure("4.126 4.126 4.126", "P -1", "Si 0 0 0 1.0 0.01"),
        ReflectionConditionFilter::Centering);

    std::vector<V3D> unique = generator.getUniqueHKLs(dMin, dMax);
    std::vector<V3D> peaks = generator.getHKLs(dMin, dMax);

    // Because of symmetry -1, each reflection has multiplicity 2.
    TS_ASSERT_EQUALS(peaks.size(), 2 * unique.size());
  }

  void test_getDValues() {
    std::vector<V3D> hkls{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    ReflectionGenerator generator(
        CrystalStructure("2 3 5", "P -1", "Si 0 0 0 1.0 0.01"));
    std::vector<double> dValues = generator.getDValues(hkls);

    TS_ASSERT_EQUALS(dValues.size(), hkls.size());
    TS_ASSERT_EQUALS(dValues[0], 2.0);
    TS_ASSERT_EQUALS(dValues[1], 3.0);
    TS_ASSERT_EQUALS(dValues[2], 5.0);
  }

  void test_getUniqueHKLsStructureFactor() {
    CrystalStructure si("5.43 5.43 5.43", "F m -3 m",
                        "Si 0.3 0.3 0.3 1.0 0.05");

    ReflectionGenerator generator(si,
                                  ReflectionConditionFilter::StructureFactor);

    std::vector<V3D> hklsCentering = generator.getUniqueHKLs(
        0.6, 10.0, boost::make_shared<HKLFilterCentering>(si.centering()));

    std::vector<V3D> hklsStructureFactors = generator.getUniqueHKLs(0.6, 10.0);

    TS_ASSERT_EQUALS(hklsCentering.size(), hklsStructureFactors.size());

    for (size_t i = 0; i < hklsCentering.size(); ++i) {
      TS_ASSERT_EQUALS(hklsCentering[i], hklsStructureFactors[i]);
    }
  }

  void test_getUniqueHKLsHexagonal() {
    ReflectionGenerator generator(
        CrystalStructure("3.2094 3.2094 5.2108 90.0 90.0 120.0", "P 63/m m c",
                         "Mg 1/3 2/3 1/4 1.0 0.005"),
        ReflectionConditionFilter::StructureFactor);

    std::vector<V3D> hkls = generator.getUniqueHKLs(0.5, 10.0);

    TS_ASSERT_EQUALS(hkls.size(), 88);

    std::vector<double> dValues = generator.getDValues(hkls);
    for (size_t i = 0; i < hkls.size(); ++i) {
      TS_ASSERT_LESS_THAN(0.5, dValues[i]);
    }
  }

  void test_getUniqueHKLsTrigonal() {
    ReflectionGenerator generator(
        CrystalStructure("4.759355 4.759355 12.99231 90.0 90.0 120.0", "R -3 c",
                         "Al 0 0 0.35217 1.0 0.005; O 0.69365 0 1/4 1.0 0.005"),
        ReflectionConditionFilter::StructureFactor);

    std::vector<V3D> hkls = generator.getUniqueHKLs(0.885, 10.0);

    TS_ASSERT_EQUALS(hkls.size(), 44);
  }
};

#endif /* MANTID_GEOMETRY_REFLECTIONGENERATORTEST_H_ */
