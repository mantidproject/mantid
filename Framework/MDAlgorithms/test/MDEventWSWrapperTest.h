// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

#include "MantidMDAlgorithms/MDEventWSWrapper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

class MDEventWSWrapperTest : public CxxTest::TestSuite {
  std::unique_ptr<MDEventWSWrapper> pWSWrap;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEventWSWrapperTest *createSuite() { return new MDEventWSWrapperTest(); }
  static void destroySuite(MDEventWSWrapperTest *suite) { delete suite; }

  void test_construct() { TS_ASSERT_THROWS_NOTHING(pWSWrap = std::make_unique<MDEventWSWrapper>()); }
  void test_buildNewWS() {
    IMDEventWorkspace_sptr pws;
    MDWSDescription TWS0;
    MDWSDescription TWS10(10);
    MDWSDescription TWS5(5);

    TSM_ASSERT_THROWS("too few dimensions", pws = pWSWrap->createEmptyMDWS(TWS0), const std::invalid_argument &);
    TSM_ASSERT_THROWS("too many dimensions", pws = pWSWrap->createEmptyMDWS(TWS10), const std::invalid_argument &);
    TSM_ASSERT_THROWS("dimensions have not been defined ", pWSWrap->nDimensions(), const std::invalid_argument &);

    TSM_ASSERT_THROWS_NOTHING("should be fine", pws = pWSWrap->createEmptyMDWS(TWS5));

    TSM_ASSERT_EQUALS("should have 5 dimensions", 5, pWSWrap->nDimensions());

    TS_ASSERT_THROWS_NOTHING(pWSWrap->releaseWorkspace());

    TSM_ASSERT("should be unique", pws.use_count() == 1);
  }
  void test_AddEventsData() {

    const size_t n_dims(5), n_MDev(2);
    Mantid::API::BoxController_sptr bc;
    MDWSDescription targetWSDescr(5);
    std::vector<double> minval(5, -10), maxval(5, 10);
    targetWSDescr.setMinMax(minval, maxval);

    TSM_ASSERT_THROWS_NOTHING("should be fine", pWSWrap->createEmptyMDWS(targetWSDescr));

    // Build up the box controller
    TSM_ASSERT_THROWS_NOTHING("should be fine", bc = pWSWrap->pWorkspace()->getBoxController());

    // set default BC values
    TSM_ASSERT_THROWS_NOTHING("should be fine", bc->setSplitThreshold(5));
    TSM_ASSERT_THROWS_NOTHING("should be fine", bc->setMaxDepth(20));
    TSM_ASSERT_THROWS_NOTHING("should be fine", bc->setSplitInto(10));

    TSM_ASSERT_THROWS_NOTHING("should be fine", pWSWrap->pWorkspace()->splitBox());

    // allocate temporary buffer for MD Events data
    std::vector<Mantid::coord_t> allCoord(n_dims * n_MDev, 0.5);
    allCoord[0] = -0.5;

    std::vector<float> sig_err(2 * n_MDev, 2);
    std::vector<uint16_t> expInfoIndex(n_MDev, 2);
    std::vector<uint16_t> goniometer_index(n_MDev, 42);
    std::vector<uint32_t> det_ids(n_MDev, 5);

    TSM_ASSERT_THROWS_NOTHING("should be fine",
                              pWSWrap->addMDData(sig_err, expInfoIndex, goniometer_index, det_ids, allCoord, n_MDev));

    TSM_ASSERT_THROWS_NOTHING("should be fine", pWSWrap->pWorkspace()->refreshCache());

    TSM_ASSERT_EQUALS("all points should be added successfully", n_MDev, pWSWrap->pWorkspace()->getNPoints());
  }
};
