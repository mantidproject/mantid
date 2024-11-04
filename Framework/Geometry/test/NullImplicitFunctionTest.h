// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/MDGeometry/NullImplicitFunction.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::Geometry;

class NullImplicitFunctionTest : public CxxTest::TestSuite {

public:
  void testGetName() {
    NullImplicitFunction function;

    TSM_ASSERT_EQUALS("The static and dynamic names do not align", NullImplicitFunction::functionName(),
                      function.getName());
  }

  void testEvaluateReturnsTrue() {
    NullImplicitFunction function;
    Mantid::coord_t coord[3] = {0, 0, 0};
    TS_ASSERT(function.isPointContained(coord));
  }

  void testToXMLEmpty() {
    NullImplicitFunction function;

    TSM_ASSERT_EQUALS("The xml string should be empty for any instance of this type", std::string(),
                      function.toXMLString());
  }
};
