#ifndef BINARYOPERATIONTEST_H_
#define BINARYOPERATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Timer.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Geometry::IDetector_const_sptr;

class BinaryOpHelper : public Mantid::Algorithms::BinaryOperation {
public:
  /// Default constructor
  BinaryOpHelper() : BinaryOperation(){};
  /// Destructor
  virtual ~BinaryOpHelper(){};

  /// function to return a name of the algorithm, must be overridden in all
  /// algorithms
  virtual const std::string name() const { return "BinaryOpHelper"; }
  /// function to return a version of the algorithm, must be overridden in all
  /// algorithms
  virtual int version() const { return 1; }
  /// function to return a category of the algorithm. A default implementation
  /// is provided
  virtual const std::string category() const { return "Helper"; }
  /// function to return the summary of the algorithm. A default implementation
  /// is provided.
  virtual const std::string summary() const { return "Summary of this test."; }

  std::string checkSizeCompatibility(const MatrixWorkspace_const_sptr ws1,
                                     const MatrixWorkspace_const_sptr ws2) {
    m_lhs = ws1;
    m_rhs = ws2;
    BinaryOperation::checkRequirements();
    return BinaryOperation::checkSizeCompatibility(ws1, ws2);
  }

private:
  // Unhide base class method to avoid Intel compiler warning
  using BinaryOperation::checkSizeCompatibility;
  // Overridden BinaryOperation methods
  void performBinaryOperation(const Mantid::MantidVec &,
                              const Mantid::MantidVec &,
                              const Mantid::MantidVec &,
                              const Mantid::MantidVec &,
                              const Mantid::MantidVec &, Mantid::MantidVec &,
                              Mantid::MantidVec &) {}
  void performBinaryOperation(const Mantid::MantidVec &,
                              const Mantid::MantidVec &,
                              const Mantid::MantidVec &, const double,
                              const double, Mantid::MantidVec &,
                              Mantid::MantidVec &) {}
};

class BinaryOperationTest : public CxxTest::TestSuite {
public:
  void testcheckSizeCompatibility1D1D() {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace2D_sptr work_in2 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace2D_sptr work_in3 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace2D_sptr work_in4 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace2D_sptr work_in5 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace2D_sptr work_in6 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in6).empty());
  }

  void testcheckSizeCompatibility2D1D() {
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 =
        WorkspaceCreationHelper::Create2DWorkspace123(10, 10);
    Workspace2D_sptr work_in2 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(20);
    Workspace2D_sptr work_in3 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(10);
    Workspace2D_sptr work_in4 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(5);
    Workspace2D_sptr work_in5 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(3);
    Workspace2D_sptr work_in6 =
        WorkspaceCreationHelper::Create1DWorkspaceFib(1);
    MatrixWorkspace_sptr work_inEvent1 =
        WorkspaceCreationHelper::CreateEventWorkspace(10, 1);
    // will not pass x array does not match
    MatrixWorkspace_sptr work_inEvent2 =
        WorkspaceCreationHelper::CreateEventWorkspace(1, 10);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in6).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_inEvent1).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_inEvent2).empty());
  }

  void testcheckSizeCompatibility2D2D() {

    // Register the workspace in the data service
    Workspace2D_sptr work_in1 =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    Workspace2D_sptr work_in2 =
        WorkspaceCreationHelper::Create2DWorkspace(10, 20);
    Workspace2D_sptr work_in3 =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    Workspace2D_sptr work_in4 =
        WorkspaceCreationHelper::Create2DWorkspace(5, 5);
    Workspace2D_sptr work_in5 =
        WorkspaceCreationHelper::Create2DWorkspace(3, 3);
    Workspace2D_sptr work_in6 =
        WorkspaceCreationHelper::Create2DWorkspace(100, 1);
    MatrixWorkspace_sptr work_inEvent1 =
        WorkspaceCreationHelper::CreateEventWorkspace(5, 5);
    MatrixWorkspace_sptr work_inEvent2 =
        WorkspaceCreationHelper::CreateEventWorkspace(10, 10);
    BinaryOpHelper helper;
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in2).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_in3).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in4).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in5).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_in6).empty());
    TS_ASSERT(!helper.checkSizeCompatibility(work_in1, work_inEvent1).empty());
    TS_ASSERT(helper.checkSizeCompatibility(work_in1, work_inEvent2).empty());
  }

  void testMaskedSpectraPropagation() {
    const int nHist = 5, nBins = 10;
    std::set<int64_t> masking;
    masking.insert(0);
    masking.insert(2);
    masking.insert(4);

    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::Create2DWorkspace123(nHist, nBins, 0, masking);
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::Create2DWorkspace154(nHist, nBins);

    BinaryOpHelper helper;
    helper.initialize();
    helper.setProperty("LHSWorkspace", work_in1);
    helper.setProperty("RHSWorkspace", work_in2);
    const std::string outputSpace("test");
    helper.setPropertyValue("OutputWorkspace", outputSpace);
    helper.setRethrows(true);
    helper.execute();

    TS_ASSERT(helper.isExecuted());

    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputSpace);
    TS_ASSERT(output);

    for (int i = 0; i < nHist; ++i) {
      IDetector_const_sptr det;
      try {
        det = output->getDetector(i);
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
      }

      TSM_ASSERT("Detector was found", det);
      if (det) {
        if (masking.count(i) == 0) {
          TS_ASSERT_EQUALS(det->isMasked(), false);
        } else {
          TS_ASSERT_EQUALS(det->isMasked(), true);
        }
      }
    }
  }

  BinaryOperation::BinaryOperationTable_sptr
  do_test_buildBinaryOperationTable(std::vector<std::vector<int>> lhs,
                                    std::vector<std::vector<int>> rhs,
                                    bool expect_throw = false) {
    EventWorkspace_sptr lhsWS =
        WorkspaceCreationHelper::CreateGroupedEventWorkspace(lhs, 50, 1.0);
    EventWorkspace_sptr rhsWS =
        WorkspaceCreationHelper::CreateGroupedEventWorkspace(rhs, 50, 1.0);
    BinaryOperation::BinaryOperationTable_sptr table;
    Mantid::Kernel::Timer timer1;
    if (expect_throw) {
      TS_ASSERT_THROWS(
          table = BinaryOperation::buildBinaryOperationTable(lhsWS, rhsWS),
          std::runtime_error);
    } else {
      TS_ASSERT_THROWS_NOTHING(
          table = BinaryOperation::buildBinaryOperationTable(lhsWS, rhsWS));
      // std::cout << timer1.elapsed() << " sec to run
      // buildBinaryOperationTable\n";
      TS_ASSERT(table);
      TS_ASSERT_EQUALS(table->size(), lhsWS->getNumberHistograms());
    }
    return table;
  }

  void test_buildBinaryOperationTable_simpleLHS_by_groupedRHS() {
    std::vector<std::vector<int>> lhs(6), rhs(2);
    for (int i = 0; i < 6; i++) {
      // one detector per pixel in lhs
      lhs[i].push_back(i);
      // 3 detectors in each on the rhs
      rhs[i / 3].push_back(i);
    }
    auto table = do_test_buildBinaryOperationTable(lhs, rhs);
    for (int i = 0; i < 6; i++) {
      TS_ASSERT_EQUALS((*table)[i], i / 3);
    }
  }

  void
  test_buildBinaryOperationTable_simpleLHS_by_groupedRHS_mismatched_throws() {
    std::vector<std::vector<int>> lhs(6), rhs(2);
    for (int i = 0; i < 6; i++) {
      // one detector per pixel in lhs, but they start at 3
      lhs[i].push_back(i + 3);
      // 3 detectors in each on the rhs
      rhs[i / 3].push_back(i);
    }
    auto table = do_test_buildBinaryOperationTable(lhs, rhs, false);
    TS_ASSERT_EQUALS((*table)[0], 1);
    TS_ASSERT_EQUALS((*table)[1], 1);
    TS_ASSERT_EQUALS((*table)[2], 1);
    TS_ASSERT_EQUALS((*table)[3], -1);
    TS_ASSERT_EQUALS((*table)[4], -1);
    TS_ASSERT_EQUALS((*table)[5], -1);
  }

  void test_buildBinaryOperationTable_groupedLHS_by_groupedRHS() {
    std::vector<std::vector<int>> lhs(8), rhs(4);
    for (int i = 0; i < 16; i++) {
      // two detectors per pixel in lhs
      lhs[i / 2].push_back(i);
      // 4 detectors in each on the rhs
      rhs[i / 4].push_back(i);
    }
    auto table = do_test_buildBinaryOperationTable(lhs, rhs);
    for (int i = 0; i < 8; i++) {
      TS_ASSERT_EQUALS((*table)[i], i / 2);
    }
  }

  void
  test_buildBinaryOperationTable_groupedLHS_by_groupedRHS_bad_overlap_throws() {
    std::vector<std::vector<int>> lhs(6), rhs(4);
    for (int i = 0; i < 24; i++) {
      // 4 detectors per pixel in lhs
      lhs[i / 4].push_back(i);
      // 6 detectors in each on the rhs
      rhs[i / 6].push_back(i);
    }
    auto table = do_test_buildBinaryOperationTable(lhs, rhs, false);
    TS_ASSERT_EQUALS((*table)[0], 0);  // 0-3 go into 0-5
    TS_ASSERT_EQUALS((*table)[1], -1); // 4-7 fails to go anywhere
    TS_ASSERT_EQUALS((*table)[2], 1);  // 8-11 goes into 6-11
  }

  void test_buildBinaryOperationTable_simpleLHS_by_groupedRHS_large() {
    std::vector<std::vector<int>> lhs(2000, std::vector<int>(1)),
        rhs(20, std::vector<int>(100));
    for (int i = 0; i < 2000; i++) {
      // 1 detector per pixel in lhs
      lhs[i][0] = i;
      // 1000 detectors in each on the rhs
      rhs[i / 100][i % 100] = i;
    }
    auto table = do_test_buildBinaryOperationTable(lhs, rhs);
    for (int i = 0; i < 2000; i++) {
      TS_ASSERT_EQUALS((*table)[i], i / 100);
    }
  }
};

#endif /*BINARYOPERATIONTEST_H_*/
