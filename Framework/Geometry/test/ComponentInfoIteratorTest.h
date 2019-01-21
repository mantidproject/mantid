// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_COMPONENTINFOITERATORTEST_H_
#define MANTID_GEOMETRY_COMPONENTINFOITERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoItem.h"
#include "MantidGeometry/Instrument/ComponentInfoIterator.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <iterator>

using namespace ComponentCreationHelper;
using namespace Mantid::Geometry;

class ComponentInfoIteratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoIteratorTest *createSuite() {
    return new ComponentInfoIteratorTest();
  }
  static void destroySuite(ComponentInfoIteratorTest *suite) { delete suite; }

  std::unique_ptr<Mantid::Geometry::ComponentInfo>
  create_component_info_object() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0),   // Source position
                                           V3D(10, 0, 0),  // Sample position
                                           V3D(11, 0, 0)); // Detector position

    // Create the instrument visitor
    InstrumentVisitor visitor(visitee);

    // Return the DetectorInfo object
    return InstrumentVisitor::makeWrappers(*visitee, nullptr).first;
  }

  void test_iterator_cbegin() {
    auto componentInfo = create_component_info_object();
    auto iter = componentInfo->cbegin();
    // Check we start at the correct place
    TS_ASSERT(iter != componentInfo->cend());
    TS_ASSERT_EQUALS(iter->m_index, 0);
  }
  void test_iterator_cend() {
    auto componentInfo = create_component_info_object();
    auto iter = componentInfo->cend();
    // Check we end at the correct place
    TS_ASSERT(iter != componentInfo->cbegin());
    TS_ASSERT_EQUALS(iter->m_index, componentInfo->size());
  }
  void test_increment_upwards() {
    // Iterator starts at component index 0 (detectors usually) and finishes at
    // root.
    auto componentInfo = create_component_info_object();
    ComponentInfoConstIt it(*componentInfo, 0, componentInfo->size());
    TS_ASSERT(it->isDetector());
    std::advance(it, componentInfo->size() - 1);
    TS_ASSERT(!it->isDetector()); // Root is not a detector
  }

  void test_detector_components_behave_as_expected() {

    auto componentInfo = create_component_info_object();
    size_t detectorCount = 0;
    for (auto item : *componentInfo) {
      if (item.isDetector()) {
        ++detectorCount;
        TS_ASSERT_EQUALS(item.detectorsInSubtree().size(), 1);  // Self
        TS_ASSERT_EQUALS(item.componentsInSubtree().size(), 1); // Self
        TS_ASSERT_EQUALS(item.children().size(),
                         0); // Detectors have no children
      }
    }
    TS_ASSERT_EQUALS(detectorCount, 1); // See instrument description above
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTINFOITERATORTEST_H_ */
