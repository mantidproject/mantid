// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBSMERGETEST_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBSMERGETEST_H_
#include "Common/ZipRange.h"
#include "Reduction/ReductionJobs.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using testing::Mock;
using testing::NiceMock;
using testing::_;

using namespace MantidQt::CustomInterfaces;

class MockModificationListener {
public:
  MOCK_METHOD2_T(groupAppended, void(int, Group const &));
  MOCK_METHOD1_T(groupRemoved, void(int));
  MOCK_METHOD3_T(rowInserted, void(int, int, Row const &));
  MOCK_METHOD3_T(rowModified, void(int, int, Row const &));
};

class ReductionJobsMergeTest : public CxxTest::TestSuite {
public:
  ReductionJobsMergeTest() : m_thetaTolerance(0.001) {}
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReductionJobsMergeTest *createSuite() {
    return new ReductionJobsMergeTest();
  }

  static void destroySuite(ReductionJobsMergeTest *suite) { delete suite; }

  Row rowWithAngle(double angle) { return rowWithNameAndAngle("1012", angle); }

  Row rowWithNameAndAngle(std::string const &name, double angle) {
    auto wsNames = ReductionWorkspaces({"TOF_" + name}, TransmissionRunPair{});
    return Row({name}, angle, {"", ""}, RangeInQ(), boost::none, {}, wsNames);
  }

  Row rowWithNamesAndAngle(std::vector<std::string> const &names,
                           double angle) {
    auto wsNames = ReductionWorkspaces(names, TransmissionRunPair{});
    return Row(names, angle, {"", ""}, RangeInQ(), boost::none, {}, wsNames);
  }

  void testMergeEmptyModels() {
    auto target = ReductionJobs();
    auto addition = ReductionJobs();
    NiceMock<MockModificationListener> listener;
    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT(target.groups().empty());
  }

  void testMergeJobsIntoEmpty() {
    auto target = ReductionJobs();
    auto addition = ReductionJobs();
    NiceMock<MockModificationListener> listener;
    addition.appendGroup(Group("A"));

    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExisting() {
    NiceMock<MockModificationListener> listener;
    auto target = ReductionJobs();
    target.appendGroup(Group("A"));
    auto addition = ReductionJobs();
    addition.appendGroup(Group("B"));

    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT_EQUALS(2u, target.groups().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testCallsAppendWhenAddingGroup() {
    NiceMock<MockModificationListener> listener;
    auto target = ReductionJobs();
    target.appendGroup(Group("A"));
    auto addition = ReductionJobs();
    addition.appendGroup(Group("B"));

    EXPECT_CALL(listener, groupAppended(1, _));

    mergeJobsInto(target, addition, m_thetaTolerance, listener);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExistingWhenNameClashButNoRows() {
    MockModificationListener listener;
    auto target = ReductionJobs();
    target.appendGroup(Group("A"));
    auto addition = ReductionJobs();
    addition.appendGroup(Group("A"));

    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExistingWhenNameClashButRowsWithDifferentAngles() {
    MockModificationListener listener;
    auto target = ReductionJobs();
    target.appendGroup(Group("A", {rowWithAngle(0.1)}));
    auto addition = ReductionJobs();
    addition.appendGroup(Group("A", {rowWithAngle(0.2)}));

    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(2u, target.groups()[0].rows().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testCallsInsertWhenAddingRow() {
    MockModificationListener listener;
    auto target = ReductionJobs();
    target.appendGroup(Group("A", {rowWithAngle(0.1)}));
    auto addition = ReductionJobs();
    addition.appendGroup(Group("A", {rowWithAngle(0.2)}));

    EXPECT_CALL(listener, rowInserted(0, 1, _)).Times(1);

    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(2u, target.groups()[0].rows().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExistingWhenNameClashAndRowsHaveSameAngles() {
    MockModificationListener listener;
    auto target = ReductionJobs();
    target.appendGroup(Group("A", {rowWithNameAndAngle("C", 0.1)}));
    auto addition = ReductionJobs();
    addition.appendGroup(Group("A", {rowWithNameAndAngle("D", 0.1)}));

    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(1u, target.groups()[0].rows().size());
    TS_ASSERT_EQUALS(std::vector<std::string>({"C", "D"}),
                     target.groups()[0].rows()[0].get().runNumbers());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testCallsModifiedWhenMergingRow() {
    MockModificationListener listener;
    auto target = ReductionJobs();
    target.appendGroup(Group("A", {rowWithNameAndAngle("C", 0.1)}));
    auto addition = ReductionJobs();
    addition.appendGroup(Group("A", {rowWithNameAndAngle("D", 0.1)}));

    EXPECT_CALL(listener, rowModified(0, 0, _));
    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(1u, target.groups()[0].rows().size());
    TS_ASSERT_EQUALS(std::vector<std::string>({"C", "D"}),
                     target.groups()[0].rows()[0].get().runNumbers());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  bool haveEqualRunNumbers(ReductionJobs const &lhs, ReductionJobs const &rhs) {
    if (lhs.groups().size() == rhs.groups().size()) {
      for (auto groupPair : zip_range(lhs.groups(), rhs.groups())) {
        for (auto rowPair : zip_range(boost::get<0>(groupPair).rows(),
                                      boost::get<1>(groupPair).rows())) {
          auto const &lhsRow = boost::get<0>(rowPair);
          auto const &rhsRow = boost::get<1>(rowPair);
          if (lhsRow.is_initialized() != rhsRow.is_initialized())
            return false;
          else if (lhsRow.is_initialized())
            if (lhsRow.get().runNumbers() != rhsRow.get().runNumbers())
              return false;
        }
      }
      return true;
    } else {
      return false;
    }
  }

  void testMergingRowsProducesUnionOfRunNumbers() {
    auto row = mergedRow(rowWithNamesAndAngle({"A", "B"}, 0.0),
                         rowWithNamesAndAngle({"B", "C"}, 0.0));

    TS_ASSERT_EQUALS(std::vector<std::string>({"A", "B", "C"}),
                     row.runNumbers());
  }

  void testMergeIntoSelfResultsInNoChange() {
    MockModificationListener listener;
    auto target = ReductionJobs();
    target.appendGroup(
        Group("S1 SI/ D20 ", {rowWithNameAndAngle("47450", 0.7),
                              rowWithNameAndAngle("47451", 2.3)}));

    target.appendGroup(
        Group("S2 SI/ D20 ", {rowWithNamesAndAngle({"47450", "47453"}, 0.7)}));

    auto addition = target;
    mergeJobsInto(target, addition, m_thetaTolerance, listener);

    TS_ASSERT(haveEqualRunNumbers(target, addition))
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

private:
  double m_thetaTolerance;
};
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBSMERGETEST_H_
