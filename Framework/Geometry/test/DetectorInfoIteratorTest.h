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

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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

  void test_iterator_increment() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->begin();

    // Check that we start at the beginning
    TS_ASSERT(iter == detectorInfo->begin());

    // Increment and check index
    for (int i = 0; i < 11; ++i) {
      TS_ASSERT_EQUALS(iter->getIndex(), i);
      ++iter;
    }

    // Check we've reached the end
    TS_ASSERT(iter == detectorInfo->end());
  }

  void test_iterator_decrement() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->end();

    // Check that we start at the end
    TS_ASSERT(iter == detectorInfo->end());

    // Decrement and check index
    for (int i = 11; i > 0; --i) {
      TS_ASSERT_EQUALS(iter->getIndex(), i);
      --iter;
    }

    // Check we've reached the beginning
    TS_ASSERT(iter == detectorInfo->begin());
  }

  void test_iterator_advance() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->begin();

    // Advance 6 places
    std::advance(iter, 6);
    TS_ASSERT_EQUALS(iter->getIndex(), 6);

    // Go past end of valid range
    std::advance(iter, 8);
    TS_ASSERT(iter == detectorInfo->end());

    // Go backwards
    std::advance(iter, -2);
    TS_ASSERT_EQUALS(iter->getIndex(), 9);

    // Go to the start
    std::advance(iter, -9);
    TS_ASSERT(iter == detectorInfo->begin());
  }

  void test_copy_iterator() {
    // Get the DetectorInfo object
    auto detectorInfo = create_detector_info_object();
    auto iter = detectorInfo->begin();

    // Create a copy
    auto iterCopy = DetectorInfoIterator(iter);

    // Check
    TS_ASSERT_EQUALS(iter->getIndex(), 0);
    TS_ASSERT_EQUALS(iterCopy->getIndex(), 0);

    // Increment
    ++iter;
    ++iterCopy;

    // Check again
    TS_ASSERT_EQUALS(iter->getIndex(), 1);
    TS_ASSERT_EQUALS(iterCopy->getIndex(), 1);
  }
};

#endif /* MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_ */
