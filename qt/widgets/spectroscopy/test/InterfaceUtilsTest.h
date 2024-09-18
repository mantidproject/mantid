// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"

#include <QPair>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

class InterfaceUtilsTest : public CxxTest::TestSuite {
public:
  static InterfaceUtilsTest *createSuite() { return new InterfaceUtilsTest(); }

  static void destroySuite(InterfaceUtilsTest *suite) { delete suite; }

  void test_interface_property_empty_if_no_interface_found() {
    TS_ASSERT_EQUALS(getInterfaceProperty("Empty", "EXTENSIONS", "all"), "");
  }

  void test_get_FB_WS_suffixes_when_restrict_data_is_off() {
    auto mockRestrictInputDataByName = []() { return false; };
    InterfaceUtils::restrictInputDataByName = mockRestrictInputDataByName;

    // There are many similar functions in the interface, this test will try only one pair of such functions
    TS_ASSERT_EQUALS(getResolutionWSSuffixes("Iqt"), QStringList());
    TS_ASSERT_EQUALS(getResolutionFBSuffixes("Iqt"), QStringList({".nxs"}));
  }

  void test_get_FB_WS_suffixes_when_restrict_data_is_on() {
    auto mockRestrictInputDataByName = []() { return true; };
    InterfaceUtils::restrictInputDataByName = mockRestrictInputDataByName;

    // There are many similar functions in the interface, this test will try only one pair of such functions
    TS_ASSERT_EQUALS(getResolutionWSSuffixes("Iqt"), QStringList({"_res", "_red", "_sqw"}));
    TS_ASSERT_EQUALS(getResolutionFBSuffixes("Iqt"), QStringList({"_res.nxs", "_red.nxs", "_sqw.nxs"}));
  }
};
