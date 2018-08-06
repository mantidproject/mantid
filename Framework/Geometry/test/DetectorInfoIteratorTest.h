#ifndef MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_
#define MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_

#include <algorithm>
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <set>

#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
using Mantid::detid_t;

class DetectorInfoIteratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoIteratorTest *createSuite() {
    return new DetectorInfoIteratorTest();
  }
  static void destroySuite(DetectorInfoIteratorTest *suite) { delete suite; }

  DetectorInfoIteratorTest(){};

  Mantid::Geometry::DetectorInfo create_detector_info_object() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0),   // Source position
                                           V3D(10, 0, 0),  // Sample position
                                           V3D(11, 0, 0)); // Detector position

    // Add 10 more detectors to the instrument
    for (int i = 2; i < 12; i++) {
      Detector *det =
          new Detector("point-detector", i /*detector id*/, nullptr);
      det->setPos(V3D(10 + i, 0, 0));
      std::string num = std::to_string(i);
      det->setShape(createSphere(0.01 /*1cm*/, V3D(0, 0, 0), num));
      visitee->add(det);
      visitee->markAsDetector(det);
    }

    // Create the instrument visitor
    InstrumentVisitor visitor(visitee);

    // Visit everything
    visitor.walkInstrument();

    // Create the Beamline DetectorInfo
    auto detInfo = visitor.detectorInfo();

    // Get details from DetectorInfo
    auto detIds = visitor.detectorIds();
    auto detMap = visitor.detectorIdToIndexMap();

    // Create the Geometry DetectorInfo
    Mantid::Geometry::DetectorInfo detectorInfo{visitor.detectorInfo(), visitee,
                                                detIds, detMap};

    return detectorInfo;
  }

  void test_iterator_begin() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo.begin();

    // Check we start at the correct place
    TS_ASSERT(iter != detectorInfo.end());
  }

  void test_iterator_end() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo.end();

    // Check we start at the correct place
    TS_ASSERT(iter != detectorInfo.begin());
  }

  void test_iterator_increment() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo.begin();

    // Check that we start at the beginning
    TS_ASSERT(iter == detectorInfo.begin());

    // Increment and check index
    for (int i = 0; i < 11; ++i) {
      TS_ASSERT_EQUALS(iter->getIndex(), i);
      ++iter;
    }

    // Check we've reached the end
    TS_ASSERT(iter == detectorInfo.end());
  }

  void test_iterator_decrement() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo.end();

    // Check that we start at the end
    TS_ASSERT(iter == detectorInfo.end());

    // Decrement and check index
    for (int i = 11; i > 0; --i) {
      TS_ASSERT_EQUALS(iter->getIndex(), i);
      --iter;
    }

    // Check we've reached the beginning
    TS_ASSERT(iter == detectorInfo.begin());
  }

  void test_iterator_advance() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo.begin();

    // Advance 6 places
    std::advance(iter, 6);
    TS_ASSERT_EQUALS(iter->getIndex(), 6);

    // Go past end of valid range
    std::advance(iter, 8);
    TS_ASSERT(iter == detectorInfo.end());

    // Go backwards
    std::advance(iter, -2);
    TS_ASSERT_EQUALS(iter->getIndex(), 9);

    // Go past the start
    std::advance(iter, -100);
    TS_ASSERT(iter == detectorInfo.begin());
  }

  void test_iterator_position() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo.begin();

    for (int i = 0; i < 11; i++) {
      TS_ASSERT_EQUALS(iter->position(), i);
      ++iter;
    }
  }

  void test_copy_iterator() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo.begin();

    auto iterCopy = DetectorInfoIterator(iter);
    
    TS_ASSERT_EQUALS(iter->getIndex(), 0);
    TS_ASSERT_EQUALS(iterCopy->getIndex(), 0);

    TS_ASSERT_EQUALS(iter->position(), 0);
    TS_ASSERT_EQUALS(iterCopy->position(), 0);

    ++iter;
    ++iterCopy;

    TS_ASSERT_EQUALS(iter->getIndex(), 1);
    TS_ASSERT_EQUALS(iterCopy->getIndex(), 1);

    TS_ASSERT_EQUALS(iter->position(), 1);
    TS_ASSERT_EQUALS(iterCopy->position(),1);
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_ */
