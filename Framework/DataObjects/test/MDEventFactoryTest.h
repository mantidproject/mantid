// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDEventFactory.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class MDEventFactoryTest : public CxxTest::TestSuite {
public:
  /** Create MDEW's with various number of dimensions */
  void test_factory() {
    IMDEventWorkspace_sptr ew;
    ew = MDEventFactory::CreateMDWorkspace(4, "MDLeanEvent");
    TS_ASSERT_EQUALS(ew->getNumDims(), 4);
    TSM_ASSERT_EQUALS("Should have volume normalization as a default", ew->displayNormalization(),
                      Mantid::API::VolumeNormalization);
    TSM_ASSERT_EQUALS("Should have volume normalization as a default", ew->displayNormalizationHisto(),
                      Mantid::API::VolumeNormalization);

    size_t n = 9;
    auto eventNormalization = Mantid::API::NoNormalization;
    auto histoNormalization = Mantid::API::NumEventsNormalization;
    ew = MDEventFactory::CreateMDWorkspace(n, "MDLeanEvent", eventNormalization, histoNormalization);
    TS_ASSERT_EQUALS(ew->getNumDims(), n);
    TSM_ASSERT_EQUALS("Should have no normalization set.", ew->displayNormalization(), eventNormalization);
    TSM_ASSERT_EQUALS("Should have number of events set.", ew->displayNormalizationHisto(), histoNormalization);

    TS_ASSERT_THROWS(ew = MDEventFactory::CreateMDWorkspace(0), const std::invalid_argument &);
  }

  void test_box_factory() {
    BoxController_sptr bc = std::make_shared<BoxController>(4);

    IMDNode *Box = MDEventFactory::createBox(4, MDEventFactory::BoxType::MDBoxWithLean, bc);
    TS_ASSERT_EQUALS(Box->getNumDims(), 4);
    MDBox<MDLeanEvent<4>, 4> *leanBox = dynamic_cast<MDBox<MDLeanEvent<4>, 4> *>(Box);
    TS_ASSERT(leanBox != nullptr);
    delete Box;

    bc.reset(new BoxController(9));
    Box = MDEventFactory::createBox(9, MDEventFactory::BoxType::MDBoxWithFat, bc);
    TS_ASSERT_EQUALS(Box->getNumDims(), 9);
    MDBox<MDEvent<9>, 9> *fatBox = dynamic_cast<MDBox<MDEvent<9>, 9> *>(Box);
    TS_ASSERT(fatBox != nullptr);
    delete Box;

    bc.reset(new BoxController(3));
    Box = MDEventFactory::createBox(3, MDEventFactory::BoxType::MDGridBoxWithLean, bc);
    TS_ASSERT_EQUALS(Box->getNumDims(), 3);
    MDGridBox<MDLeanEvent<3>, 3> *leanGridBox = dynamic_cast<MDGridBox<MDLeanEvent<3>, 3> *>(Box);
    TS_ASSERT(leanGridBox != nullptr);
    delete Box;

    bc.reset(new BoxController(1));
    Box = MDEventFactory::createBox(1, MDEventFactory::BoxType::MDGridBoxWithFat, bc);
    TS_ASSERT_EQUALS(Box->getNumDims(), 1);
    MDGridBox<MDEvent<1>, 1> *fatGridBox = dynamic_cast<MDGridBox<MDEvent<1>, 1> *>(Box);
    TS_ASSERT(fatGridBox != nullptr);
    delete Box;

    TS_ASSERT_THROWS(MDEventFactory::createBox(0, MDEventFactory::BoxType::MDBoxWithLean, bc),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(MDEventFactory::createBox(10, MDEventFactory::BoxType::MDGridBoxWithFat, bc),
                     const std::invalid_argument &);
  }

  // Templated function that will be called for a specific MDEW
  template <typename MDE, size_t nd> void functionTest(typename MDEventWorkspace<MDE, nd>::sptr ws) {
    test_value = ws->getNumDims();
  }

  void test_CALL_MDEVENT_FUNCTION_macro() {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDLeanEvent<1>, 1>());
    TS_ASSERT_EQUALS(ew->getNumDims(), 1);
    TS_ASSERT_EQUALS(ew->getNPoints(), 0);
    test_value = 0;
    CALL_MDEVENT_FUNCTION(functionTest, ew);
    TS_ASSERT_EQUALS(test_value, 1);
  }

  void test_CALL_MDEVENT_FUNCTION_macro_2() {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDLeanEvent<8>, 8>());
    TS_ASSERT_EQUALS(ew->getNumDims(), 8);
    TS_ASSERT_EQUALS(ew->getNPoints(), 0);
    test_value = 0;
    CALL_MDEVENT_FUNCTION(functionTest, ew);
    TS_ASSERT_EQUALS(test_value, 8);
  }

  size_t test_value;
};
