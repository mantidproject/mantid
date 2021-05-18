// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ApplyInstrumentToPeaks.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using Mantid::Algorithms::ApplyInstrumentToPeaks;

class ApplyInstrumentToPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyInstrumentToPeaksTest *createSuite() { return new ApplyInstrumentToPeaksTest(); }
  static void destroySuite(ApplyInstrumentToPeaksTest *suite) { delete suite; }

  void test_Something() {
    auto ws = prepareWorkspace();

    TS_FAIL("You forgot to write a test!");
  }

private:
  // Create workspace
  MatrixWorkspace_sptr prepareWorkspace() {
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    return inputWS;
  }
};
