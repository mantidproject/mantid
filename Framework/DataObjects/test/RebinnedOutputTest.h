// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

class RebinnedOutputTest : public CxxTest::TestSuite {
private:
  RebinnedOutput_sptr ws;
  int nHist;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinnedOutputTest *createSuite() { return new RebinnedOutputTest(); }
  static void destroySuite(RebinnedOutputTest *suite) { delete suite; }

  RebinnedOutputTest() {
    nHist = 6;
    ws = WorkspaceCreationHelper::createRebinnedOutputWorkspace();
  }

  void testClone() {
    RebinnedOutput_sptr cloned(ws->clone());

    // Swap ws with cloned pointer, such that we can reuse existing tests.
    ws.swap(cloned);

    // Run all other tests on ws.
    testId();
    testRepresentation();
    testSetF();

    // Undo swap, to avoid possible interferences.
    ws.swap(cloned);
  }

  void testId() { TS_ASSERT_EQUALS(ws->id(), "RebinnedOutput"); }

  void testRepresentation() {
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(ws->blocksize(), nHist);
    TS_ASSERT_EQUALS(ws->dataX(0).size(), 7);
    TS_ASSERT_EQUALS(ws->dataX(0)[2], -1.);
    TS_ASSERT_EQUALS(ws->dataY(1)[3], 1.);
    // 1/sqrt(3)
    TS_ASSERT_DELTA(ws->dataE(1)[3], 0.57735026918963, 1.e-5);
    TS_ASSERT_EQUALS(ws->dataF(0).size(), nHist);
    TS_ASSERT_EQUALS(ws->dataF(1)[3], 3.);
  }

  void testSetF() {
    MantidVecPtr f;
    f.access().resize(nHist, 2.0);
    ws->setF(1, f);
    TS_ASSERT_EQUALS(ws->dataF(1)[3], 2.);
  }
};
