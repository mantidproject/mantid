#ifndef MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_
#define MANTID_GEOMETRY_DETECTORINFOITERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include <set>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
using Mantid::detid_t;


class DetectorInfoIteratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoIteratorTest *createSuite() { return new DetectorInfoIteratorTest(); }
  static void destroySuite(DetectorInfoIteratorTest *suite) { delete suite; }

  DetectorInfoIteratorTest() {};



  void test_visitor_detector_sanity_check() {

    // Create a very basic instrument to visit
    auto visitee = createMinimalInstrument(V3D(0, 0, 0) /*source pos*/,
      V3D(10, 0, 0) /*sample pos*/
      ,
      V3D(11, 0, 0) /*detector position*/);

    // Create the visitor.
    const size_t detectorIndex =
      0; // Internally we expect detector index to start at 0

    const size_t instrumentIndex = 3; // Instrument is always hightest index.

    InstrumentVisitor visitor(visitee);
    // Visit everything
    visitor.walkInstrument();

    auto compInfo = visitor.componentInfo();
    auto detInfo = visitor.detectorInfo();
    compInfo->setDetectorInfo(detInfo.get());
    detInfo->setComponentInfo(compInfo.get());

    TSM_ASSERT_EQUALS("Detector has parent of instrument",
      compInfo->parent(detectorIndex), instrumentIndex);
    TSM_ASSERT_EQUALS("Instrument has single detector",
      compInfo->detectorsInSubtree(instrumentIndex),
      std::vector<size_t>{detectorIndex});

    //auto iter = detInfo.begin();
    //TS_ASSERT(iter != hist.end());
  }


};
#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_ */
