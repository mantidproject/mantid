#ifndef MANTID_API_INFOCOMPONENTVISITORTEST_H_
#define MANTID_API_INFOCOMPONENTVISITORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/V3D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidAPI/InfoComponentVisitor.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <set>

using Mantid::API::InfoComponentVisitor;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;

class InfoComponentVisitorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InfoComponentVisitorTest *createSuite() {
    return new InfoComponentVisitorTest();
  }
  static void destroySuite(InfoComponentVisitorTest *suite) { delete suite; }

  void test_visitor_basic_sanity_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    // Create the visitor.
    InfoComponentVisitor visitor(1, [](const Mantid::detid_t) { return 0; });

    // Visit everything
    visitee->registerContents(visitor);

    size_t expectedSize = 0;
    ++expectedSize; // source
    ++expectedSize; // sample
    ++expectedSize; // Detector
    ++expectedSize; // instrument

    TSM_ASSERT_EQUALS("Should have registered 4 components", visitor.size(),
                      expectedSize);
  }

  void test_visitor_detector_indexes_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    // Create the visitor.
    const size_t detectorIndex = 0;
    InfoComponentVisitor visitor(
        1, [&](const Mantid::detid_t) { return detectorIndex; });

    // Visit everything
    visitee->registerContents(visitor);

    /*
     * Now lets check the cached contents of our visitor to check
     * it did the job correctly.
    */
    TSM_ASSERT_EQUALS("Single detector should have index of 0",
                      visitor.assemblySortedDetectorIndices(),
                      std::vector<size_t>{detectorIndex});
  }

  void test_visitor_component_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    // Create the visitor.
    InfoComponentVisitor visitor(1, [](const Mantid::detid_t) { return 0; });

    // Visit everything
    visitee->registerContents(visitor);

    std::set<Mantid::Geometry::ComponentID> componentIds(
        visitor.componentIds().begin(), visitor.componentIds().end());
    TSM_ASSERT_EQUALS("Expect 4 component Ids", componentIds.size(), 4);

    TSM_ASSERT_EQUALS("Should contain the instrument id", 1,
                      componentIds.count(visitee->getComponentID()));
    TSM_ASSERT_EQUALS(
        "Should contain the sample id", 1,
        componentIds.count(visitee->getComponentByName("some-surface-holder")
                               ->getComponentID()));
    TSM_ASSERT_EQUALS("Should contain the source id", 1,
                      componentIds.count(visitee->getComponentByName("source")
                                             ->getComponentID()));
    TSM_ASSERT_EQUALS(
        "Should contain the detector id", 1,
        componentIds.count(
            visitee->getComponentByName("point-detector")->getComponentID()));
  }

  void test_visitor_ranges_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    // Create the visitor.
    InfoComponentVisitor visitor(1, [](const Mantid::detid_t) { return 0; });

    // Visit everything
    visitee->registerContents(visitor);

    auto ranges = visitor.componentDetectorRanges();
    TSM_ASSERT_EQUALS("There are 3 non-detector components", ranges.size(), 3);

    /*
     * In this instrument there is only a single assembly (the instrument
     * itself). All other non-detectors are also non-assembly components.
     * We therefore EXPECT that the ranges provided are all from 0 to 0 for
     * those generic components. This is important for subsequent correct
     * working on ComponentInfo.
     */
    // Source has no detectors
    TS_ASSERT_EQUALS(ranges[0].first, 0);
    TS_ASSERT_EQUALS(ranges[0].second, 0);
    // Sample has no detectors
    TS_ASSERT_EQUALS(ranges[1].first, 0);
    TS_ASSERT_EQUALS(ranges[1].second, 0);
    // Instrument has 1 detector.
    TS_ASSERT_EQUALS(ranges[2].first, 0);
    TS_ASSERT_EQUALS(ranges[2].second, 1);
  }

  void test_visitor_drops_detectors_without_id() {
    /*
     We have to go via DetectorInfo::indexOf to get the index of a detector.
     if this throws because the detector has an invalid id, we are forced to
     drop it.

     Some IDFs i.e. SNAP have montiors with detector ids <  0.
    */

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    // Create the visitor. Note any access to the indexOf lambda will throw for
    // detectors.
    InfoComponentVisitor visitor(1, [](const Mantid::detid_t) -> size_t {
      throw std::out_of_range("");
    });

    // Visit everything
    visitee->registerContents(visitor);

    size_t expectedSize = 0;
    ++expectedSize; // source
    ++expectedSize; // sample
    ++expectedSize; // instrument
    // Note no detector counted
    TS_ASSERT_EQUALS(visitor.size(), expectedSize);
  }
};

class InfoComponentVisitorTestPerformance : public CxxTest::TestSuite {
private:
  const int m_nPixels = 1000;
  boost::shared_ptr<Mantid::Geometry::Instrument> m_instrument;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InfoComponentVisitorTestPerformance *createSuite() {
    return new InfoComponentVisitorTestPerformance();
  }
  static void destroySuite(InfoComponentVisitorTestPerformance *suite) {
    delete suite;
  }

  InfoComponentVisitorTestPerformance() {
    m_instrument = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, m_nPixels, 1 /*sample-bank distance*/);
  }

  void test_process_rectangular_instrument() {
    InfoComponentVisitor visitor(
        m_nPixels * m_nPixels,
        [](const Mantid::detid_t id) { return static_cast<size_t>(id); });
    m_instrument->registerContents(visitor);
    TS_ASSERT(visitor.size() >= size_t(m_nPixels * m_nPixels));
  }
};
#endif /* MANTID_API_INFOCOMPONENTVISITORTEST_H_ */
