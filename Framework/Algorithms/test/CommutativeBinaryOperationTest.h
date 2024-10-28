// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CommutativeBinaryOperation.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;

class CommutativeBinaryOpHelper : public Mantid::Algorithms::CommutativeBinaryOperation {
public:
  /// Default constructor
  CommutativeBinaryOpHelper() : CommutativeBinaryOperation() {};
  /// Destructor
  ~CommutativeBinaryOpHelper() override {};
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CommutativeBinaryOperationHelper"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's summary for identification overriding a virtual method
  const std::string summary() const override { return "Commutative binary operation helper."; }

  std::string checkSizeCompatibility(const MatrixWorkspace_const_sptr &ws1, const MatrixWorkspace_const_sptr &ws2) {
    m_lhs = ws1;
    m_rhs = ws2;
    m_lhsBlocksize = ws1->blocksize();
    m_rhsBlocksize = ws2->blocksize();
    BinaryOperation::checkRequirements();
    return CommutativeBinaryOperation::checkSizeCompatibility(ws1, ws2);
  }

private:
  // Unhide base class method to avoid Intel compiler warning
  using BinaryOperation::checkSizeCompatibility;
  // Overridden BinaryOperation methods
  void performBinaryOperation(const HistogramData::Histogram &lhs, const HistogramData::Histogram &rhs,
                              HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) override {
    UNUSED_ARG(lhs);
    UNUSED_ARG(rhs);
    UNUSED_ARG(YOut);
    UNUSED_ARG(EOut);
  }
  void performBinaryOperation(const HistogramData::Histogram &lhs, const double rhsY, const double rhsE,
                              HistogramData::HistogramY &YOut, HistogramData::HistogramE &EOut) override {
    UNUSED_ARG(lhs);
    UNUSED_ARG(rhsY);
    UNUSED_ARG(rhsE);
    UNUSED_ARG(YOut);
    UNUSED_ARG(EOut);
  }
};

class CommutativeBinaryOperationTest : public CxxTest::TestSuite {
public:
  void testcheckSizeCompatibility1D1D() {
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create1DWorkspaceFib(10, true);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create1DWorkspaceFib(20, true);
    MatrixWorkspace_sptr work_in3 = WorkspaceCreationHelper::create1DWorkspaceFib(10, true);
    MatrixWorkspace_sptr work_in4 = WorkspaceCreationHelper::create1DWorkspaceFib(5, true);
    MatrixWorkspace_sptr work_in5 = WorkspaceCreationHelper::create1DWorkspaceFib(3, true);
    MatrixWorkspace_sptr work_in6 = WorkspaceCreationHelper::create1DWorkspaceFib(1, true);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in6).empty());
  }

  void testcheckSizeCompatibility2D1D() {
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace123(10, 10, true);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create1DWorkspaceFib(20, true);
    MatrixWorkspace_sptr work_in3 = WorkspaceCreationHelper::create1DWorkspaceFib(10, true);
    MatrixWorkspace_sptr work_in4 = WorkspaceCreationHelper::create1DWorkspaceFib(5, true);
    MatrixWorkspace_sptr work_in5 = WorkspaceCreationHelper::create1DWorkspaceFib(3, true);
    MatrixWorkspace_sptr work_in6 = WorkspaceCreationHelper::create1DWorkspaceFib(1, true);
    MatrixWorkspace_sptr work_event1 = WorkspaceCreationHelper::createEventWorkspace(10, 1);
    MatrixWorkspace_sptr work_event2 = WorkspaceCreationHelper::createEventWorkspace(1, 10);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in6).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_event1).empty());
    // bin boundaries will not match
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_event2).empty());
  }

  void testcheckSizeCompatibility2D2D() {
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::create2DWorkspace(10, 20);
    MatrixWorkspace_sptr work_in3 = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    MatrixWorkspace_sptr work_in4 = WorkspaceCreationHelper::create2DWorkspace(5, 5);
    MatrixWorkspace_sptr work_in5 = WorkspaceCreationHelper::create2DWorkspace(3, 3);
    MatrixWorkspace_sptr work_in6 = WorkspaceCreationHelper::create2DWorkspace(100, 1);
    MatrixWorkspace_sptr work_in7 = WorkspaceCreationHelper::createWorkspaceSingleValue(10.0);
    MatrixWorkspace_sptr work_event1 = WorkspaceCreationHelper::createEventWorkspace(10, 1);
    MatrixWorkspace_sptr work_event2 = WorkspaceCreationHelper::createEventWorkspace(10, 10);
    CommutativeBinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in6).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in7).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in7, work_in1).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_event1).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_event2).empty());
  }
};
