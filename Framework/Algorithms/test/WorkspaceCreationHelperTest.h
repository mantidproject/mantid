// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::API;

/** Test class for the helpers in MantidTestHelpers/WorkspaceCreationHelper.h */
class WorkspaceCreationHelperTest : public CxxTest::TestSuite {
public:
  void test_create2DWorkspaceWithRectangularInstrument() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20);
    TS_ASSERT(ws);
    TS_ASSERT(ws->getInstrument());
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2 * 100);
    TS_ASSERT_EQUALS(ws->blocksize(), 20);
    TS_ASSERT(ws->getSpectrum(0).hasDetectorID(100));
    TS_ASSERT(ws->getSpectrum(1).hasDetectorID(101));
    TS_ASSERT_DELTA(ws->dataY(5)[0], 2.0, 1e-5);
  }

  void test_createEventWorkspaceWithFullInstrument() {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 10);
    TS_ASSERT(ws);
    TS_ASSERT(ws->getInstrument());
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2 * 100);
    TS_ASSERT(ws->getSpectrum(0).hasDetectorID(100));
    TS_ASSERT(ws->getSpectrum(1).hasDetectorID(101));
  }

  void test_create2DWorkspaceWithValues() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace123(1, 2, false, std::set<int64_t>(), true);
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(ws->size(), 2);
    TS_ASSERT(ws->hasDx(0));
    TS_ASSERT_EQUALS(ws->dx(0).rawData()[0], 2.);
    Workspace2D_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace123(1, 2, false, std::set<int64_t>(), false);
    TS_ASSERT(!ws2->hasDx(0));
  }

  void test_create2DWorkspace123WithMaskedBin() {
    int numHist = 3;
    int numBins = 4;
    // mask bin at workspace index 0, bin 1
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace123WithMaskedBin(numHist, numBins, 0, 1);
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), numHist);
    TS_ASSERT_EQUALS(ws->blocksize(), numBins);
    TS_ASSERT_EQUALS(ws->hasMaskedBins(0), true);
    TS_ASSERT_EQUALS(ws->hasMaskedBins(1), false);
  }

  void test_create2DWorkspaceRagged() {
    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceRagged();
    TS_ASSERT(ws->isRaggedWorkspace());
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(ws->x(0).size(), 4);
    TS_ASSERT_EQUALS(ws->x(1).size(), 3);
    TS_ASSERT_EQUALS(ws->y(0).size(), 3);
    TS_ASSERT_EQUALS(ws->y(1).size(), 2);
  }
};
