#ifndef MANTID_ALGORITHMS_DETECTORGRIDDEFINITIONTEST_H_
#define MANTID_ALGORITHMS_DETECTORGRIDDEFINITIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"

#include <algorithm>

using Mantid::Algorithms::DetectorGridDefinition;

class DetectorGridDefinitionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorGridDefinitionTest *createSuite() {
    return new DetectorGridDefinitionTest();
  }
  static void destroySuite(DetectorGridDefinitionTest *suite) { delete suite; }

  void test_latitudeAt() {
    const auto def = makeTestDefinition();
    for (size_t i = 0; i < nLat(); ++i) {
      const auto dLat = (maxLat() - minLat()) / static_cast<double>(nLat() - 1);
      const auto lat = static_cast<double>(i) * dLat + minLat();
      TS_ASSERT_EQUALS(def.latitudeAt(i), lat)
    }
  }

  void test_longitudeAt() {
    const auto def = makeTestDefinition();
    for (size_t i = 0; i < nLong(); ++i) {
      const auto dLong =
          (maxLong() - minLong()) / static_cast<double>(nLong() - 1);
      const auto lon = static_cast<double>(i) * dLong + minLong();
      TS_ASSERT_EQUALS(def.longitudeAt(i), lon)
    }
  }

  void test_nearestNeighbourIndices() {
    const auto def = makeTestDefinition();
    auto indices = def.nearestNeighbourIndices(minLat(), minLong());
    TS_ASSERT(inArray(indices, 0))
    TS_ASSERT(inArray(indices, 1))
    TS_ASSERT(inArray(indices, nLat()))
    TS_ASSERT(inArray(indices, nLat() + 1))
    indices = def.nearestNeighbourIndices(maxLat(), maxLong());
    TS_ASSERT(inArray(indices, nLat() * (nLong() - 2) + nLat() - 2))
    TS_ASSERT(inArray(indices, nLat() * (nLong() - 2) + nLat() - 1))
    TS_ASSERT(inArray(indices, nLat() * (nLong() - 1) + nLat() - 2))
    TS_ASSERT(inArray(indices, nLat() * (nLong() - 1) + nLat() - 1))
    const auto dLat = (maxLat() - minLat()) / static_cast<double>(nLat() - 1);
    const auto lat = (maxLat() + minLat() - dLat) / 2.0;
    const auto dLong =
        (maxLong() - minLong()) / static_cast<double>(nLong() - 1);
    const auto lon = (maxLong() + minLong() - dLong) / 2.0;
    indices = def.nearestNeighbourIndices(lat, lon);
    TS_ASSERT(inArray(indices, 37))
    TS_ASSERT(inArray(indices, 38))
    TS_ASSERT(inArray(indices, 44))
    TS_ASSERT(inArray(indices, 45))
  }

  void test_size() {
    const auto def = makeTestDefinition();
    TS_ASSERT_EQUALS(def.numberColumns(), nLong())
    TS_ASSERT_EQUALS(def.numberRows(), nLat())
  }

  void test_latitudes_have_zero_gap() {
    DetectorGridDefinition def(minLat(), minLat(), 2, minLong(), maxLong(), 4);
    const auto indices =
        def.nearestNeighbourIndices(minLat(), (minLong() + maxLong()) / 2.0);
    TS_ASSERT(inArray(indices, 2))
    TS_ASSERT(inArray(indices, 3))
    TS_ASSERT(inArray(indices, 4))
    TS_ASSERT(inArray(indices, 5))
  }

  void test_longitudes_have_zero_gap() {
    DetectorGridDefinition def(minLat(), maxLat(), 4, minLong(), minLong(), 2);
    const auto indices =
        def.nearestNeighbourIndices((minLat() + maxLat()) / 2.0, minLong());
    TS_ASSERT(inArray(indices, 1))
    TS_ASSERT(inArray(indices, 2))
    TS_ASSERT(inArray(indices, 5))
    TS_ASSERT(inArray(indices, 6))
  }

private:
  static double minLat() { return -0.23; }
  static double maxLat() { return 1.36; }
  static size_t nLat() { return 7; }
  static double minLong() { return -1.09; }
  static double maxLong() { return 2.71; }
  static size_t nLong() { return 13; }
  static DetectorGridDefinition makeTestDefinition() {
    return DetectorGridDefinition(minLat(), maxLat(), nLat(), minLong(),
                                  maxLong(), nLong());
  }

  static bool inArray(const std::array<size_t, 4> &a, const size_t i) {
    return std::count(a.cbegin(), a.cend(), i) == 1;
  }
};

#endif /* MANTID_ALGORITHMS_DETECTORGRIDDEFINITIONTEST_H_ */
