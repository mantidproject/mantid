#ifndef MANTID_GEOMETRY_INFOCOMPONENTVISITORTEST_H_
#define MANTID_GEOMETRY_INFOCOMPONENTVISITORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/V3D.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/InfoComponentVisitor.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include <set>
#include <algorithm>
#include <boost/make_shared.hpp>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
using Mantid::detid_t;

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
    Mantid::Geometry::ParameterMap pmap;
    // Create the visitor.
    InfoComponentVisitor visitor(std::vector<detid_t>{1} /*detector ids*/, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

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

  void test_visitor_purges_parameter_map_basic_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/,
                                           V3D(11, 0, 0) /*detector position*/);
    Mantid::Geometry::ParameterMap pmap;
    auto detector = visitee->getDetector(visitee->getDetectorIDs()[0]);
    pmap.addV3D(detector->getComponentID(), "pos",
                Mantid::Kernel::V3D{12, 0, 0});
    pmap.addV3D(visitee->getComponentID(), "pos",
                Mantid::Kernel::V3D{13, 0, 0});

    TS_ASSERT_EQUALS(pmap.size(), 2);

    // Create the visitor.
    InfoComponentVisitor visitor(std::vector<detid_t>{0}, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

    // Visit everything. Purging should happen.
    visitee->registerContents(visitor);

    TSM_ASSERT_EQUALS(
        "Detectors positions are NOT purged by visitor at present", pmap.size(),
        1);
  }

  void test_visitor_purges_parameter_map_safely() {
    /* We need to check that the purging process does not actually result in
     *things
     * that are subsequently read being misoriented or mislocated.
     *
     * In detail: Purging must be depth-first because of the way that lower
     *level
     * components calculate their positions/rotations from their parents.
     */
    using namespace ComponentHelper;

    const V3D sourcePos(0, 0, 0);
    const V3D samplePos(10, 0, 0);
    const V3D detectorPos(11, 0, 0);
    // Create a very basic instrument to visit
    auto baseInstrument = ComponentCreationHelper::createMinimalInstrument(
        sourcePos, samplePos, detectorPos);
    auto paramMap = boost::make_shared<Mantid::Geometry::ParameterMap>();
    auto parInstrument = boost::make_shared<Mantid::Geometry::Instrument>(
        baseInstrument, paramMap);

    TSM_ASSERT_EQUALS("Expect 0 items in the parameter map to start with",
                      paramMap->size(), 0);
    auto source = parInstrument->getComponentByName("source");
    const V3D newInstrumentPos(-10, 0, 0);
    ComponentHelper::moveComponent(*parInstrument, *paramMap, newInstrumentPos,
                                   TransformType::Absolute);
    const V3D newSourcePos(-1, 0, 0);
    ComponentHelper::moveComponent(*source, *paramMap, newSourcePos,
                                   TransformType::Absolute);

    // Test the moved things are where we expect them to be an that the
    // parameter map is populated.
    TS_ASSERT_EQUALS(newSourcePos,
                     parInstrument->getComponentByName("source")->getPos());
    TS_ASSERT_EQUALS(newInstrumentPos, parInstrument->getPos());
    TSM_ASSERT_EQUALS("Expect 2 items in the parameter map", paramMap->size(),
                      2);

    const detid_t detectorId = 0;
    InfoComponentVisitor visitor(std::vector<detid_t>{detectorId}, *paramMap,
                                 parInstrument->getSource()->getComponentID(),
                                 parInstrument->getSample()->getComponentID());
    parInstrument->registerContents(visitor);

    TSM_ASSERT_EQUALS("Expect 0 items in the purged parameter map",
                      paramMap->size(), 0);

    // Now we check that thing are located where we expect them to be.
    auto positions = visitor.positions();
    TSM_ASSERT(
        "Check source position",
        (*positions)[0].isApprox(Mantid::Kernel::toVector3d(newSourcePos)));
    TSM_ASSERT(
        "Check instrument position",
        (*positions)[2].isApprox(Mantid::Kernel::toVector3d(newInstrumentPos)));
  }

  void test_visitor_detector_indexes_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    Mantid::Geometry::ParameterMap pmap;
    // Create the visitor.
    const size_t detectorIndex =
        0; // Internally we expect detector index to start at 0
    InfoComponentVisitor visitor(std::vector<detid_t>{1} /*detector ids*/, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

    // Visit everything
    visitee->registerContents(visitor);

    /*
     * Now lets check the cached contents of our visitor to check
     * it did the job correctly.
    */
    TSM_ASSERT_EQUALS("Single detector should have index of 0",
                      *visitor.assemblySortedDetectorIndices(),
                      std::vector<size_t>{detectorIndex});
  }

  void test_visitor_component_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    Mantid::Geometry::ParameterMap pmap;
    // Create the visitor.
    InfoComponentVisitor visitor(std::vector<detid_t>{1} /*detector ids*/, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

    // Visit everything
    visitee->registerContents(visitor);

    std::set<Mantid::Geometry::ComponentID> componentIds(
        visitor.componentIds()->begin(), visitor.componentIds()->end());

    auto componentIdToIndexMap = visitor.componentIdToIndexMap();

    TSM_ASSERT_EQUALS("Expect 4 component Ids", componentIds.size(), 4);
    TSM_ASSERT_EQUALS("Expect 4 component Ids in map",
                      componentIdToIndexMap->size(), 4);

    TSM_ASSERT_EQUALS("Should contain the instrument id", 1,
                      componentIds.count(visitee->getComponentID()));
    TSM_ASSERT_EQUALS(
        "Should contain the sample id", 1,
        componentIds.count(visitee->getComponentByName("some-surface-holder")
                               ->getComponentID()));
    TSM_ASSERT_EQUALS("Should contain the source id", 1,
                      componentIds.count(visitee->getComponentByName("source")
                                             ->getComponentID()));

    auto detectorComponentId =
        visitee->getComponentByName("point-detector")->getComponentID();
    TSM_ASSERT_EQUALS("Should contain the detector id", 1,
                      componentIds.count(detectorComponentId));
    TSM_ASSERT_EQUALS(
        "Detectors are guaranteed to occupy the lowest component range",
        componentIdToIndexMap->at(detectorComponentId), 0);

    std::set<size_t> uniqueIndices;
    for (auto id : componentIds) {
      uniqueIndices.insert(componentIdToIndexMap->at(id));
    }
    TSM_ASSERT_EQUALS("We should have unique index values in our map",
                      uniqueIndices.size(), componentIds.size());
    TSM_ASSERT_EQUALS(
        "Indices are out of range",
        *std::max_element(uniqueIndices.begin(), uniqueIndices.end()),
        componentIds.size() - 1);
  }

  void test_visitor_detector_ranges_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    Mantid::Geometry::ParameterMap pmap;
    // Create the visitor.
    InfoComponentVisitor visitor(std::vector<detid_t>{1} /*detector ids*/, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

    // Visit everything
    visitee->registerContents(visitor);

    auto detectorRanges = visitor.componentDetectorRanges();
    TSM_ASSERT_EQUALS("There are 3 non-detector components",
                      detectorRanges->size(), 3);

    /*
     * In this instrument there is only a single assembly (the instrument
     * itself). All other non-detectors are also non-assembly components.
     * We therefore EXPECT that the ranges provided are all from 0 to 0 for
     * those generic components. This is important for subsequent correct
     * working on ComponentInfo.
     */
    // Source has no detectors
    TS_ASSERT_EQUALS((*detectorRanges)[0].first, 0);
    TS_ASSERT_EQUALS((*detectorRanges)[0].second, 0);
    // Sample has no detectors
    TS_ASSERT_EQUALS((*detectorRanges)[1].first, 0);
    TS_ASSERT_EQUALS((*detectorRanges)[1].second, 0);
    // Instrument has 1 detector
    TS_ASSERT_EQUALS((*detectorRanges)[2].first, 0);
    TS_ASSERT_EQUALS((*detectorRanges)[2].second, 1);
  }

  void test_visitor_component_ranges_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    Mantid::Geometry::ParameterMap pmap;
    // Create the visitor.
    InfoComponentVisitor visitor(std::vector<detid_t>{1} /*detector ids*/, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

    // Visit everything
    visitee->registerContents(visitor);

    auto componentRanges = visitor.componentChildComponentRanges();
    TSM_ASSERT_EQUALS("There are 3 non-detector components",
                      componentRanges->size(), 3);

    /*
     * In this instrument there is only a single assembly (the instrument
     * itself). We therefore EXPECT that the ranges provided are all from 0 to 0
     * for
     * those non-assembly components. This is important for subsequent correct
     * working on ComponentInfo.
     */
    // Source has no sub-components, range includes only itself
    TS_ASSERT_EQUALS((*componentRanges)[0].first, 0);
    TS_ASSERT_EQUALS((*componentRanges)[0].second, 1);
    // Sample has no sub-components, range includes only itself
    TS_ASSERT_EQUALS((*componentRanges)[1].first, 1);
    TS_ASSERT_EQUALS((*componentRanges)[1].second, 2);
    // Instrument has 1 detector.
    TS_ASSERT_EQUALS((*componentRanges)[2].first, 0);
    TS_ASSERT_EQUALS((*componentRanges)[2].second, 4);
  }
  void test_visitor_collects_detector_id_to_index_mappings() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    Mantid::Geometry::ParameterMap pmap;
    InfoComponentVisitor visitor(std::vector<detid_t>{1} /*detector ids*/, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

    // Visit everything
    visitee->registerContents(visitor);

    TS_ASSERT_EQUALS(visitor.detectorIdToIndexMap()->size(), 1);
    TS_ASSERT_EQUALS(visitor.detectorIdToIndexMap()->at(1),
                     0); // ID 1 to index 0

    TS_ASSERT_EQUALS(visitor.detectorIds()->size(), 1);
    TS_ASSERT_EQUALS(visitor.detectorIds()->at(0), 1); // index 0 is ID 1
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

    Mantid::Geometry::ParameterMap pmap;
    // Create the visitor. Note any access to the indexOf lambda will throw for
    // detectors.
    InfoComponentVisitor visitor(
        std::vector<detid_t>{
            0 /*sized just 1 to invoke out of range exception*/},
        pmap);

    // Visit everything
    visitee->registerContents(visitor);

    size_t expectedSize = 0;
    ++expectedSize; // source
    ++expectedSize; // sample
    ++expectedSize; // instrument
    // Note no detector counted
    TS_ASSERT_EQUALS(visitor.size(), expectedSize);
  }

  void test_visitation_of_rectangular_detector() {

    // Need confidence that this works properly for RectangularDetectors
    const int nPixelsWide = 10; // Gives 10*10 detectors in total
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixelsWide, 1 /*sample-bank distance*/);
    Mantid::Geometry::ParameterMap pmap;
    InfoComponentVisitor visitor(instrument->getDetectorIDs(), pmap);
    instrument->registerContents(visitor);

    TSM_ASSERT_EQUALS("Wrong number of detectors registered",
                      visitor.detectorIds()->size(), nPixelsWide * nPixelsWide);
  }

  void test_parent_indices() {

    const int nPixelsWide = 10; // Gives 10*10 detectors in total
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixelsWide, 1 /*sample-bank distance*/);

    Mantid::Geometry::ParameterMap pmap;
    InfoComponentVisitor visitor(instrument->getDetectorIDs() /*detector ids*/,
                                 pmap,
                                 instrument->getSource()->getComponentID(),
                                 instrument->getSample()->getComponentID());

    // Visit everything
    instrument->registerContents(visitor);

    const auto parentComponentIndices = visitor.parentComponentIndices();

    size_t testIndex =
        visitor.size() -
        2; // One component down from root. (Has parent of the root itself)
    TS_ASSERT_EQUALS((*parentComponentIndices)[testIndex], visitor.size() - 1);
    size_t root = visitor.size() - 1;
    TS_ASSERT_EQUALS((*parentComponentIndices)[root], root);

    testIndex = 0; // Check a detector
    const auto rowAssemblyIndex = (*parentComponentIndices)[testIndex];
    const auto bankIndex = (*parentComponentIndices)[rowAssemblyIndex];
    const auto instrumentIndex = (*parentComponentIndices)[bankIndex];
    // Walk all the way up to the instrument
    TS_ASSERT_EQUALS(instrumentIndex, visitor.size() - 1);
  }

  void test_source_and_sample() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
                                           V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    Mantid::Geometry::ParameterMap pmap;
    InfoComponentVisitor visitor(std::vector<detid_t>{1} /*detector ids*/, pmap,
                                 visitee->getSource()->getComponentID(),
                                 visitee->getSample()->getComponentID());

    // Visit everything
    visitee->registerContents(visitor);

    // Detector has component index of 0
    TS_ASSERT_EQUALS(1, visitor.source());
    TS_ASSERT_EQUALS(2, visitor.sample());
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
    Mantid::Geometry::ParameterMap pmap;
    InfoComponentVisitor visitor(m_instrument->getDetectorIDs(), pmap,
                                 m_instrument->getSource()->getComponentID(),
                                 m_instrument->getSample()->getComponentID());
    m_instrument->registerContents(visitor);
    TS_ASSERT(visitor.size() >= size_t(m_nPixels * m_nPixels));
  }
};
#endif /* MANTID_API_INFOCOMPONENTVISITORTEST_H_ */
