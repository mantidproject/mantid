// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_
#define MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace ComponentCreationHelper;
using namespace Mantid::Geometry;

/** DetectorInfoIteratorTest

Test class for testing the iterator behaviour for DetectorInfoIterator.

@author Bhuvan Bezawada, STFC
@date 2018
*/

class DetectorInfoIteratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoIteratorTest *createSuite() {
    return new DetectorInfoIteratorTest();
  }
  static void destroySuite(DetectorInfoIteratorTest *suite) { delete suite; }

  DetectorInfoIteratorTest(){};

  std::unique_ptr<Mantid::Geometry::DetectorInfo>
  create_detector_info_object() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0),   // Source position
                                           V3D(10, 0, 0),  // Sample position
                                           V3D(11, 0, 0)); // Detector position

    // Total number of detectors will be 11
    // Need to loop until 12
    int numDetectors = 12;

    // Add 10 more detectors to the instrument
    for (int i = 2; i < numDetectors; i++) {
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

    // Return the DetectorInfo object
    return InstrumentVisitor::makeWrappers(*visitee, nullptr).second;
  }

  void test_iterator_begin() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->begin();

    // Check we start at the correct place
    TS_ASSERT(iter != detectorInfo->end());
  }

  void test_iterator_end() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->end();

    // Check we start at the correct place
    TS_ASSERT(iter != detectorInfo->begin());
  }

  void test_iterator_increment_and_positions() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->begin();

    // Check that we start at the beginning
    TS_ASSERT(iter == detectorInfo->begin());

    // Doubles for values in the V3D position object
    double xValue = 0.0;
    double zero = 0.0;

    // Increment the iterator and check positions
    for (int i = 0; i < 11; ++i) {
      // Store expected X as double
      xValue = 11.0 + (double)i;

      // Assertions
      TS_ASSERT_EQUALS(iter->position().X(), xValue);
      TS_ASSERT(iter->position().Y() == zero);
      TS_ASSERT(iter->position().Z() == zero);

      // Increment the iterator
      ++iter;
    }

    // Check we've reached the end
    TS_ASSERT(iter == detectorInfo->end());
  }

  void test_iterator_decrement_and_positions() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->end();

    // Check that we start at the end
    TS_ASSERT(iter == detectorInfo->end());

    // Doubles for values in the V3D position object
    double xValue = 0.0;
    double zero = 0.0;

    // Increment the iterator and check positions
    for (int i = 11; i > 0; --i) {
      // Decrement the iterator
      --iter;

      // Store expected X as double
      xValue = 10.0 + (double)i;

      // Assertions
      TS_ASSERT_EQUALS(iter->position().X(), xValue);
      TS_ASSERT(iter->position().Y() == zero);
      TS_ASSERT(iter->position().Z() == zero);
    }

    // Check we've reached the beginning
    TS_ASSERT(iter == detectorInfo->begin());
  }

  void test_iterator_advance_and_positions() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->begin();

    // Store the expected X value
    double xValue = 0.0;

    // Advance 6 places
    xValue = 17.0;
    std::advance(iter, 6);
    TS_ASSERT_EQUALS(iter->position().X(), xValue)

    // Go backwards 2 places
    xValue = 15.0;
    std::advance(iter, -2);
    TS_ASSERT_EQUALS(iter->position().X(), xValue)

    // Go to the start
    std::advance(iter, -4);
    TS_ASSERT(iter == detectorInfo->begin());
  }

  void test_copy_iterator_and_positions() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->begin();

    // Create a copy
    auto iterCopy = DetectorInfoIterator(iter);

    // Check
    TS_ASSERT_EQUALS(iter->position().X(), 11.0);
    TS_ASSERT_EQUALS(iterCopy->position().X(), 11.0);

    // Increment
    ++iter;
    ++iterCopy;

    // Check again
    TS_ASSERT_EQUALS(iter->position().X(), 12.0);
    TS_ASSERT_EQUALS(iterCopy->position().X(), 12.0);
  }
};

#endif /* MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_ */
