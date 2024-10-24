// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/V3D.h"
#include <algorithm>
#include <memory>
#include <set>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
using Mantid::detid_t;

namespace {

std::shared_ptr<const Instrument> makeParameterized(std::shared_ptr<const Instrument> baseInstrument) {
  return std::make_shared<const Instrument>(baseInstrument, std::make_shared<ParameterMap>());
}
} // namespace

class InstrumentVisitorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentVisitorTest *createSuite() { return new InstrumentVisitorTest(); }
  static void destroySuite(InstrumentVisitorTest *suite) { delete suite; }

  void test_visitor_basic_sanity_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);
    // Create the visitor.
    InstrumentVisitor visitor(visitee);
    // Visit everything
    visitee->registerContents(visitor);

    size_t expectedSize = 0;
    ++expectedSize; // source
    ++expectedSize; // sample
    ++expectedSize; // Detector
    ++expectedSize; // instrument

    TSM_ASSERT_EQUALS("Should have registered 4 components", visitor.size(), expectedSize);
  }

  void test_visitor_purges_parameter_map_basic_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/,
                                           V3D(11, 0, 0) /*detector position*/);
    auto pmap = std::make_shared<ParameterMap>();
    auto detector = visitee->getDetector(visitee->getDetectorIDs()[0]);
    pmap->addV3D(detector->getComponentID(), "pos", Mantid::Kernel::V3D{12, 0, 0});
    pmap->addV3D(visitee->getComponentID(), "pos", Mantid::Kernel::V3D{13, 0, 0});

    TS_ASSERT_EQUALS(pmap->size(), 2);

    // Create the visitor.
    InstrumentVisitor visitor(std::make_shared<const Instrument>(visitee, pmap));

    // Visit everything. Purging should happen.
    visitor.walkInstrument();

    TSM_ASSERT_EQUALS("Detectors positions are purged by visitor at present", pmap->size(), 0);
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
    const V3D sourcePos(0, 0, 0);
    const V3D samplePos(10, 0, 0);
    const V3D detectorPos(11, 0, 0);
    // Create a very basic instrument to visit
    auto baseInstrument = ComponentCreationHelper::createMinimalInstrument(sourcePos, samplePos, detectorPos);
    auto paramMap = std::make_shared<Mantid::Geometry::ParameterMap>();

    TSM_ASSERT_EQUALS("Expect 0 items in the parameter map to start with", paramMap->size(), 0);
    auto source = baseInstrument->getComponentByName("source");
    const V3D newInstrumentPos(-10, 0, 0);
    paramMap->addV3D(baseInstrument.get(), "pos", newInstrumentPos);
    const V3D newSourcePos(-1, 0, 0);
    paramMap->addV3D(source.get(), "pos", newSourcePos - newInstrumentPos);

    // Test the moved things are where we expect them to be an that the
    // parameter map is populated.
    TSM_ASSERT_EQUALS("Expect 2 items in the parameter map", paramMap->size(), 2);

    paramMap->setInstrument(baseInstrument.get());

    TSM_ASSERT_EQUALS("Expect 0 items in the purged parameter map", paramMap->size(), 0);

    auto parInstrument = std::make_shared<Mantid::Geometry::Instrument>(baseInstrument, paramMap);
    TS_ASSERT_EQUALS(newSourcePos, parInstrument->getComponentByName("source")->getPos());
    TS_ASSERT_EQUALS(newInstrumentPos, parInstrument->getPos());

    auto &compInfo = paramMap->componentInfo();
    TSM_ASSERT("Check source position",
               Mantid::Kernel::toVector3d(compInfo.position(1)).isApprox(Mantid::Kernel::toVector3d(newSourcePos)));
    TSM_ASSERT("Check instrument position",
               Mantid::Kernel::toVector3d(compInfo.position(3)).isApprox(Mantid::Kernel::toVector3d(newInstrumentPos)));
  }

  void test_visitor_detector_sanity_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    // Create the visitor.
    const size_t detectorIndex = 0; // Internally we expect detector index to start at 0

    const size_t instrumentIndex = 3; // Instrument is always hightest index.

    InstrumentVisitor visitor(visitee);
    // Visit everything
    visitor.walkInstrument();

    auto compInfo = visitor.componentInfo();
    auto detInfo = visitor.detectorInfo();
    compInfo->setDetectorInfo(detInfo.get());
    detInfo->setComponentInfo(compInfo.get());

    TSM_ASSERT_EQUALS("Detector has parent of instrument", compInfo->parent(detectorIndex), instrumentIndex);
    TSM_ASSERT_EQUALS("Instrument has single detector", compInfo->detectorsInSubtree(instrumentIndex),
                      std::vector<size_t>{detectorIndex});
  }

  void test_visitor_component_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    InstrumentVisitor visitor(visitee);

    // Visit everything
    visitor.walkInstrument();

    std::set<Mantid::Geometry::ComponentID> componentIds(visitor.componentIds()->begin(),
                                                         visitor.componentIds()->end());

    auto componentIdToIndexMap = visitor.componentIdToIndexMap();

    TSM_ASSERT_EQUALS("Expect 4 component Ids", componentIds.size(), 4);
    TSM_ASSERT_EQUALS("Expect 4 component Ids in map", componentIdToIndexMap->size(), 4);

    TSM_ASSERT_EQUALS("Should contain the instrument id", 1, componentIds.count(visitee->getComponentID()));
    TSM_ASSERT_EQUALS("Should contain the sample id", 1,
                      componentIds.count(visitee->getComponentByName("some-surface-holder")->getComponentID()));
    TSM_ASSERT_EQUALS("Should contain the source id", 1,
                      componentIds.count(visitee->getComponentByName("source")->getComponentID()));

    auto detectorComponentId = visitee->getComponentByName("point-detector")->getComponentID();
    TSM_ASSERT_EQUALS("Should contain the detector id", 1, componentIds.count(detectorComponentId));
    TSM_ASSERT_EQUALS("Detectors are guaranteed to occupy the lowest component range",
                      componentIdToIndexMap->at(detectorComponentId), 0);

    std::set<size_t> uniqueIndices;
    for (auto id : componentIds) {
      uniqueIndices.insert(componentIdToIndexMap->at(id));
    }
    TSM_ASSERT_EQUALS("We should have unique index values in our map", uniqueIndices.size(), componentIds.size());
    TSM_ASSERT_EQUALS("Indices are out of range", *std::max_element(uniqueIndices.begin(), uniqueIndices.end()),
                      componentIds.size() - 1);
  }

  void test_visitor_detector_ranges_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    // Create the visitor.
    InstrumentVisitor visitor(visitee);

    // Visit everything
    visitor.walkInstrument();

    auto compInfo = visitor.componentInfo();
    auto detInfo = visitor.detectorInfo();
    compInfo->setDetectorInfo(detInfo.get());
    detInfo->setComponentInfo(compInfo.get());

    TS_ASSERT_EQUALS(compInfo->detectorsInSubtree(3), std::vector<size_t>{0});
  }

  void test_visitor_component_ranges_check() {
    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/,
                                           V3D(11, 0, 0) /*detector position*/);

    InstrumentVisitor visitor(makeParameterized(visitee));
    // Visit everything
    visitor.walkInstrument();

    auto compInfo = visitor.componentInfo();
    auto detInfo = visitor.detectorInfo();
    compInfo->setDetectorInfo(detInfo.get());
    detInfo->setComponentInfo(compInfo.get());

    TS_ASSERT_EQUALS(compInfo->size(), 4); // 4 components in total
    TS_ASSERT_EQUALS(detInfo->size(), 1);  // 1 component is a detector

    auto subTreeOfRoot = compInfo->componentsInSubtree(3);
    TS_ASSERT_EQUALS(std::set<size_t>(subTreeOfRoot.begin(), subTreeOfRoot.end()), (std::set<size_t>({0, 1, 2, 3})));

    auto subTreeOfNonRoot = compInfo->componentsInSubtree(1);
    TS_ASSERT_EQUALS(std::set<size_t>(subTreeOfNonRoot.begin(), subTreeOfNonRoot.end()), (std::set<size_t>({1})));
  }
  void test_visitor_collects_detector_id_to_index_mappings() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/
                                           ,
                                           V3D(11, 0, 0) /*detector position*/);

    InstrumentVisitor visitor(visitee);
    // Visit everything
    visitor.walkInstrument();

    TS_ASSERT_EQUALS(visitor.detectorIdToIndexMap()->size(), 1);
    TS_ASSERT_EQUALS(visitor.detectorIdToIndexMap()->at(1),
                     0); // ID 1 to index 0

    TS_ASSERT_EQUALS(visitor.detectorIds()->size(), 1);
    TS_ASSERT_EQUALS(visitor.detectorIds()->at(0), 1); // index 0 is ID 1
  }

  void test_visitation_of_rectangular_detector() {
    using Mantid::Beamline::ComponentType;
    // Need confidence that this works properly for RectangularDetectors
    const int nPixelsWide = 10; // Gives 10*10 detectors in total
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(1 /*n banks*/, nPixelsWide,
                                                                               1 /*sample-bank distance*/);
    InstrumentVisitor visitor(instrument);
    visitor.walkInstrument();

    auto wrappers = InstrumentVisitor::makeWrappers(*instrument, nullptr /*parameter map*/);
    auto compInfo = std::move(std::get<0>(wrappers));
    auto detInfo = std::move(std::get<1>(wrappers));

    TSM_ASSERT_EQUALS("Wrong number of detectors registered", visitor.detectorIds()->size(), nPixelsWide * nPixelsWide);

    using Mantid::Beamline::ComponentType;

    const size_t bankIndex = compInfo->indexOfAny("bank1");
    TS_ASSERT_EQUALS(compInfo->componentType(bankIndex),
                     ComponentType::Rectangular); // Bank is rectangular
    TS_ASSERT_DIFFERS(compInfo->componentType(compInfo->source()),
                      ComponentType::Rectangular); // Source is not a rectangular bank
    TS_ASSERT_DIFFERS(compInfo->componentType(0),
                      ComponentType::Rectangular); //  A detector is never a
                                                   //  bank, let alone a
                                                   //  detector
  }

  void test_visitation_of_non_rectangular_detectors() {
    using Mantid::Beamline::ComponentType;

    auto instrument = ComponentCreationHelper::createTestInstrumentCylindrical(1 /*n banks*/);
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument, nullptr /*parameter map*/);
    auto compInfo = std::move(std::get<0>(wrappers));
    auto detInfo = std::move(std::get<1>(wrappers));

    // Nothing should be marked as a rectangular bank
    for (size_t index = 0; index < compInfo->size(); ++index) {
      TS_ASSERT(compInfo->componentType(index) != ComponentType::Rectangular);
    }
  }

  void test_parent_indices() {

    const int nPixelsWide = 10; // Gives 10*10 detectors in total
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(1 /*n banks*/, nPixelsWide,
                                                                               1 /*sample-bank distance*/);

    InstrumentVisitor visitor(instrument);

    // Visit everything
    visitor.walkInstrument();

    auto compInfo = visitor.componentInfo();
    auto detInfo = visitor.detectorInfo();
    compInfo->setDetectorInfo(detInfo.get());
    detInfo->setComponentInfo(compInfo.get());

    TS_ASSERT_EQUALS(compInfo->parent(compInfo->source()), compInfo->root());
    TS_ASSERT_EQUALS(compInfo->parent(compInfo->sample()), compInfo->root());
    TS_ASSERT_EQUALS(compInfo->parent(compInfo->root()), compInfo->root());
  }

  void test_shapes() {

    const int nPixelsWide = 10; // Gives 10*10 detectors in total
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(1 /*n banks*/, nPixelsWide,
                                                                               1 /*sample-bank distance*/);

    // Visit everything
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument, nullptr /*parameter map*/);
    auto componentInfo = std::move(std::get<0>(wrappers));

    // Instrument
    const auto &instrumentShape = componentInfo->shape(componentInfo->root());
    TSM_ASSERT("CompAssemblies should have no shape", !instrumentShape.hasValidShape());
    // Bank 1
    const auto &subAssemblyShape = componentInfo->shape(componentInfo->root() - 3);
    TSM_ASSERT("CompAssemblies should have no shape", !subAssemblyShape.hasValidShape());
    const auto &detectorShape = componentInfo->shape(0 /*Is a detector index!*/);
    TSM_ASSERT("Detectors should have a shape", detectorShape.hasValidShape());

    // Check shapes are re-used as expected
    TSM_ASSERT_EQUALS("Shape object should be reused", &instrumentShape, &subAssemblyShape);
    TSM_ASSERT_EQUALS("Shape object should be reused", &detectorShape, &componentInfo->shape(1 /*another detector*/));
  }

  void test_names() {

    const int nPixelsWide = 10; // Gives 10*10 detectors in total
    auto instrument = ComponentCreationHelper::createTestInstrumentRectangular(1 /*n banks*/, nPixelsWide,
                                                                               1 /*sample-bank distance*/);

    // Visit everything
    auto wrappers = InstrumentVisitor::makeWrappers(*instrument, nullptr /*parameter map*/);
    auto componentInfo = std::move(std::get<0>(wrappers));

    // Check root name
    TS_ASSERT_EQUALS("basic_rect", componentInfo->name(componentInfo->root()));
    // Backward check that we get the right index
    TS_ASSERT_EQUALS(componentInfo->indexOfAny("basic_rect"), componentInfo->root());

    // Check all names are the same in old instrument and component info
    for (size_t index = 0; index < componentInfo->size(); ++index) {
      TS_ASSERT_EQUALS(componentInfo->componentID(index)->getName(), componentInfo->name(index));
    }
  }

  void test_purge_scale_factors() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/,
                                           V3D(11, 0, 0) /*detector position*/);
    auto pmap = std::make_shared<ParameterMap>();
    auto detector = visitee->getDetector(visitee->getDetectorIDs()[0]);
    // Add a scale factor for the detector

    Mantid::Kernel::V3D detScaling{2, 2, 2};
    pmap->addV3D(detector->getComponentID(), ParameterMap::scale(), detScaling);
    // Add as scale factor for the instrument
    Mantid::Kernel::V3D instrScaling{3, 3, 3};
    pmap->addV3D(visitee->getComponentID(), ParameterMap::scale(), instrScaling);
    // Sanity check inputs
    TS_ASSERT_EQUALS(pmap->size(), 2);

    auto wrappers = InstrumentVisitor::makeWrappers(*visitee, pmap.get());

    TSM_ASSERT_EQUALS("Detectors positions are purged by visitor at present", pmap->size(), 0);

    auto compInfo = std::move(std::get<0>(wrappers));
    TS_ASSERT_EQUALS(detScaling, compInfo->scaleFactor(0));
    TS_ASSERT_EQUALS(instrScaling, compInfo->scaleFactor(compInfo->root()));
  }

  void test_instrumentTreeWithMinimalInstrument() {
    /** This should produce the following instrument tree
     *   3
     * / | \
     *0  1  2
     */
    auto instrument = createMinimalInstrument(V3D(0, 0, 0), V3D(0, 0, 1), V3D(0, 0, 10));
    auto visitor = InstrumentVisitor(instrument);

    visitor.walkInstrument();

    auto componentInfo = visitor.componentInfo();
    auto root = componentInfo->root();
    TS_ASSERT_EQUALS(componentInfo->children(0).size(), 0);
    TS_ASSERT_EQUALS(componentInfo->children(1).size(), 0);
    TS_ASSERT_EQUALS(componentInfo->children(2).size(), 0);
    TS_ASSERT_EQUALS(componentInfo->children(root).size(), 3);
  }

  void test_instrumentTreeWithComplexInstrument() {
    /** This should produce the following instrument tree
     *               16
     *   /      /      \                \
     * 14      15       10              13
     *                /    \          /   \
     *               8      9      11       12
     *             /  \   /  \    /  \    /   \
     *            0    1  2   3  4    5   6    7
     */
    auto instrument = createTestInstrumentRectangular2(2, 2);
    auto visitor = InstrumentVisitor(instrument);

    visitor.walkInstrument();

    auto componentInfo = visitor.componentInfo();
    auto root = componentInfo->root();
    for (int i = 0; i < 8; i++)
      TS_ASSERT_EQUALS(componentInfo->children(i).size(), 0);

    TS_ASSERT_EQUALS(componentInfo->children(root).size(), 4);
    TS_ASSERT_EQUALS(componentInfo->children(8).size(), 2);
    TS_ASSERT_EQUALS(componentInfo->children(9).size(), 2);
    TS_ASSERT_EQUALS(componentInfo->children(11).size(), 2);
    TS_ASSERT_EQUALS(componentInfo->children(12).size(), 2);
    TS_ASSERT_EQUALS(componentInfo->children(10).size(), 2);
    TS_ASSERT_EQUALS(componentInfo->children(13).size(), 2);
    TS_ASSERT_EQUALS(componentInfo->children(14).size(), 0);
    TS_ASSERT_EQUALS(componentInfo->children(15).size(), 0);
  }
};

class InstrumentVisitorTestPerformance : public CxxTest::TestSuite {
private:
  const int m_nPixels = 1000;
  std::shared_ptr<const Mantid::Geometry::Instrument> m_instrument;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentVisitorTestPerformance *createSuite() { return new InstrumentVisitorTestPerformance(); }
  static void destroySuite(InstrumentVisitorTestPerformance *suite) { delete suite; }

  InstrumentVisitorTestPerformance() {
    m_instrument = makeParameterized(
        ComponentCreationHelper::createTestInstrumentRectangular(1 /*n banks*/, m_nPixels, 1 /*sample-bank distance*/));
  }

  void test_process_rectangular_instrument() {
    InstrumentVisitor visitor(m_instrument);
    visitor.walkInstrument();
    TS_ASSERT(visitor.size() >= size_t(m_nPixels * m_nPixels));
  }
};
