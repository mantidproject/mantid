// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TimeSplitter.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/TimeROI.h"

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::TimeSplitter;
using Mantid::Kernel::TimeROI;
using Mantid::Types::Core::DateAndTime;

namespace {
const DateAndTime ONE("2023-01-01T11:00:00");
const DateAndTime TWO("2023-01-01T12:00:00");
const DateAndTime THREE("2023-01-01T13:00:00");
const DateAndTime FOUR("2023-01-01T14:00:00");
const DateAndTime FIVE("2023-01-01T15:00:00");
} // namespace

class TimeSplitterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TimeSplitterTest *createSuite() { return new TimeSplitterTest(); }
  static void destroySuite(TimeSplitterTest *suite) { delete suite; }

  void test_valueAtTime() {
    // to start everything is either in 0th output or masked
    TimeSplitter splitter(TWO, FOUR);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({0}));

    // add ROI for first half to go to 1st output
    splitter.addROI(TWO, THREE, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({0, 1}));

    // add ROI for second half to go to 2nd output
    splitter.addROI(THREE, FOUR, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 2}));

    // have whole thing go to 3rd output
    splitter.addROI(TWO, FOUR, 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({3}));

    // prepend a section that goes to 1st output
    splitter.addROI(ONE, TWO, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 3}));

    // append a section that goes to 2nd output
    splitter.addROI(FOUR, FIVE, 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({1, 2, 3}));

    // set before the beginning to mask
    splitter.addROI(ONE, TWO, -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), 2);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 3);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({2, 3}));

    // set after the end to mask
    splitter.addROI(FOUR, FIVE, -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 3);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({3}));
  }

  void test_emptySplitter() {
    TimeSplitter splitter;
    TS_ASSERT_EQUALS(splitter.valueAtTime(DateAndTime("2023-01-01T11:00:00")), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 0);
    TS_ASSERT_EQUALS(splitter.outputWorkspaceIndices(), std::vector<int>({}));
  }

  void test_gap() {
    TimeSplitter splitter;
    // create a splitter with a gap
    splitter.addROI(ONE, TWO, 0);
    splitter.addROI(THREE, FOUR, 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);

    // fill in the gap with a different value
    splitter.addROI(TWO, THREE, 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 4);

    // fill in the gap with the same value as before and after
    splitter.addROI(TWO, THREE, 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(ONE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(TWO), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(THREE), 0);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FOUR), -1);
    TS_ASSERT_EQUALS(splitter.valueAtTime(FIVE), -1);
    TS_ASSERT_EQUALS(splitter.numRawValues(), 2);
  }

  void test_getTimeROI() {

    // start with empty TimeSplitter
    TimeSplitter splitter;
    TS_ASSERT(splitter.getTimeROI(-1).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());

    // place to put the output
    TimeROI roi;

    // add a single TimeROI
    splitter.addROI(ONE, THREE, 1);
    TS_ASSERT(splitter.getTimeROI(-1).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.empty());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);

    // add the same output index, but with a gap from the previous
    splitter.addROI(FOUR, FIVE, 1);
    roi = splitter.getTimeROI(-2); // intentionally trying a "big" negative for ignore filter
    TS_ASSERT(!roi.empty());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 2);
    TS_ASSERT(splitter.getTimeROI(0).empty());
    TS_ASSERT(splitter.getTimeROI(0).empty());
    roi = splitter.getTimeROI(1);
    TS_ASSERT(!roi.empty());
    TS_ASSERT_EQUALS(roi.numBoundaries(), 4);
  }

  void test_timeSplitterFromMatrixWorkspace() {
    TimeSplitter splitter;
    splitter.addROI(DateAndTime(0, 0), DateAndTime(10, 0), 1);
    splitter.addROI(DateAndTime(10, 0), DateAndTime(15, 0), 3);
    splitter.addROI(DateAndTime(15, 0), DateAndTime(20, 0), 2);

    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 3);

    auto &X = ws->dataX(0);
    auto &Y = ws->dataY(0);
    // X[0] is 0 by default, unit is seconds.
    X[1] = 10.0;
    X[2] = 15.0;
    X[3] = 20.0;

    Y[0] = 1.0;
    Y[1] = 3.0;
    Y[2] = 2.0;
    auto convertedSplitter = new TimeSplitter(ws);

    TS_ASSERT(splitter.numRawValues() == convertedSplitter->numRawValues() && convertedSplitter->numRawValues() == 4);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(0, 0)) == convertedSplitter->valueAtTime(DateAndTime(0, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(0, 0)) == 1);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(12, 0)) == convertedSplitter->valueAtTime(DateAndTime(12, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(12, 0)) == 3);
    TS_ASSERT(splitter.valueAtTime(DateAndTime(20, 0)) == convertedSplitter->valueAtTime(DateAndTime(20, 0)) &&
              convertedSplitter->valueAtTime(DateAndTime(20, 0)) == -1);
  }

  void test_timeSplitterFromMatrixWorkspaceOffset() {
    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);

    auto &X = ws->dataX(0);
    auto &Y = ws->dataY(0);
    // X[0] is 0 by default, unit is seconds.
    X[1] = 10.0;
    Y[0] = 1.0;
    auto convertedSplitter = new TimeSplitter(ws, TWO);
    // New starting point of converted splitter is TWO
    TS_ASSERT(convertedSplitter->valueAtTime(DateAndTime(0)) == -1);
    TS_ASSERT(convertedSplitter->valueAtTime(TWO) == 1);
  }

  void test_timeSplitterFromMatrixWorkspaceError() {
    // Testing the case where an X value in the MatrixWorkspace is negative.

    Mantid::API::MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 3);

    auto &X = ws->dataX(0);
    auto &Y = ws->dataY(0);
    X[0] = -5.0;
    X[1] = 10.0;
    X[2] = 15.0;
    X[3] = 20.0;
    Y[0] = 1.0;
    Y[1] = 3.0;
    Y[2] = 2.0;
    TS_ASSERT_THROWS(new TimeSplitter(ws), std::runtime_error &);
  }
};
