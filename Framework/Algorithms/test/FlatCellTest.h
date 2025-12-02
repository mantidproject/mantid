// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FlatCell.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

/// Upda

class FlatCellTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(masker.name(), "FlatCell"); }

  void testVersion() { TS_ASSERT_EQUALS(masker.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(masker.initialize());
    TS_ASSERT(masker.isInitialized());
  }

private:
  Mantid::Algorithms::FlatCell masker;
};
