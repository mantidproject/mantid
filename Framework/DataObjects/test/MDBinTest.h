// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/MDBin.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataObjects;

class MDBinTest : public CxxTest::TestSuite {
public:
  void test_constructor() {
    using MDE = MDLeanEvent<3>;
    MDBin<MDE, 3> bin;
    for (size_t d = 0; d < 3; d++) {
      TS_ASSERT_EQUALS(bin.m_min[d], -std::numeric_limits<coord_t>::max());
      TS_ASSERT_EQUALS(bin.m_max[d], std::numeric_limits<coord_t>::max());
    }
  }
};
