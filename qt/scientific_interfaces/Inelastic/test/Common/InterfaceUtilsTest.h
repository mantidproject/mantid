// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/InterfaceUtils.h"
#include "Common/WorkspaceUtils.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

#include <QPair>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;
using namespace Mantid::IndirectFitDataCreationHelper;

class InterfaceUtilsTest : public CxxTest::TestSuite {
public:
  InterfaceUtilsTest() = default;

  void test_interface_property_empty_if_no_interface_found() {
    TS_ASSERT_EQUALS(getInterfaceProperty("Empty", "EXTENSIONS", "all"), "");
  }

  void test_get_FB_WS_suffixes_function_returns_proper_interface_suffixes() {
    // There are many similar functions in the interface, this test will try only one pair of such functions
    TS_ASSERT_EQUALS(getResolutionWSSuffixes("Iqt"), QStringList({"_res", "_red", "_sqw"}));
    TS_ASSERT_EQUALS(getResolutionFBSuffixes("Iqt"), QStringList({"_res.nxs", "_red.nxs", "_sqw.nxs"}));
  }

  void test_groupingStrInRange_returns_true_if_the_string_is_in_range() {
    TS_ASSERT(groupingStrInRange("3,4,5-8,9+10", 3, 10));
    TS_ASSERT(groupingStrInRange("11,6-9,3:5,10", 3, 11));
    TS_ASSERT(groupingStrInRange("14,9-6,5:3,10, 2", 2, 14));
  }

  void test_groupingStrInRange_returns_false_if_the_min_or_max_is_out_of_range() {
    TS_ASSERT(!groupingStrInRange("3,4,5-8,9+10, 22", 3, 10));
    TS_ASSERT(!groupingStrInRange("11,6-9,3:5,10", 3, 10));
    TS_ASSERT(!groupingStrInRange("14,9-6,5:3,10, 2", 3, 14));
  }

  void test_groupingStrInRange_returns_false_if_grouping_string_is_empty() {
    TS_ASSERT(!groupingStrInRange("", 3, 10));
  }
};
