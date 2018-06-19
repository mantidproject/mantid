#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBSMERGETEST_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBSMERGETEST_H_
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include "../ISISReflectometry/Reduction/ReductionJobs.h"
#include "../ISISReflectometry/Reduction/WorkspaceNamesFactory.h"

using testing::_;
using testing::NiceMock;
using testing::Mock;

using namespace MantidQt::CustomInterfaces;

template <typename Group> class MockModificationListener {
public:
  using Row = typename Group::RowType;

  MOCK_METHOD2_T(groupAppended, void(int, Group const &));
  MOCK_METHOD3_T(rowAppended, void(int, int, Row const &));
  MOCK_METHOD3_T(rowModified, void(int, int, Row const &));
};

class ReductionJobsMergeTest : public CxxTest::TestSuite {
public:
  ReductionJobsMergeTest()
      : m_thetaTolerance(0.001), m_slicing(), m_nameFactory(m_slicing) {}
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReductionJobsMergeTest *createSuite() {
    return new ReductionJobsMergeTest();
  }

  static void destroySuite(ReductionJobsMergeTest *suite) { delete suite; }

  UnslicedRow rowWithAngle(double angle) {
    return rowWithNameAndAngle("1012", angle);
  }

  UnslicedRow rowWithNameAndAngle(std::string const &name, double angle) {
    auto wsNames =
        ReductionWorkspaces({"TOF_" + name}, {"", ""}, "", "IvsLam_" + name,
                            "IvsQ_" + name, "IvsQ_binned_" + name);
    return UnslicedRow({name}, angle, {"", ""}, boost::none, boost::none, {},
                       wsNames);
  }

  UnslicedRow rowWithNamesAndAngle(std::vector<std::string> const &names,
                                   double angle) {
    auto wsNames = ReductionWorkspaces(names, {"", ""}, "", "", "", "");
    return UnslicedRow(names, angle, {"", ""}, boost::none, boost::none, {},
                       wsNames);
  }

  void testMergeEmptyModels() {
    auto target = UnslicedReductionJobs();
    auto addition = UnslicedReductionJobs();
    NiceMock<MockModificationListener<UnslicedGroup>> listener;
    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT(target.groups().empty());
  }

  void testMergeJobsIntoEmpty() {
    auto target = UnslicedReductionJobs();
    auto addition = UnslicedReductionJobs();
    NiceMock<MockModificationListener<UnslicedGroup>> listener;
    addition.appendGroup(UnslicedGroup("A"));

    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExisting() {
    NiceMock<MockModificationListener<UnslicedGroup>> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup("A"));
    auto addition = UnslicedReductionJobs();
    addition.appendGroup(UnslicedGroup("B"));

    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT_EQUALS(2u, target.groups().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testCallsAppendWhenAddingGroup() {
    NiceMock<MockModificationListener<UnslicedGroup>> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup("A"));
    auto addition = UnslicedReductionJobs();
    addition.appendGroup(UnslicedGroup("B"));

    EXPECT_CALL(listener, groupAppended(1, _));

    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExistingWhenNameClashButNoRows() {
    MockModificationListener<UnslicedGroup> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup("A"));
    auto addition = UnslicedReductionJobs();
    addition.appendGroup(UnslicedGroup("A"));

    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExistingWhenNameClashButRowsWithDifferentAngles() {
    MockModificationListener<UnslicedGroup> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup("A", {rowWithAngle(0.1)}, ""));
    auto addition = UnslicedReductionJobs();
    addition.appendGroup(UnslicedGroup("A", {rowWithAngle(0.2)}, ""));

    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(2u, target.groups()[0].rows().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testCallsAppendWhenAddingRow() {
    MockModificationListener<UnslicedGroup> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup("A", {rowWithAngle(0.1)}, ""));
    auto addition = UnslicedReductionJobs();
    addition.appendGroup(UnslicedGroup("A", {rowWithAngle(0.2)}, ""));

    EXPECT_CALL(listener, rowAppended(0, 1, _)).Times(1);

    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(2u, target.groups()[0].rows().size());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testMergeJobsIntoExistingWhenNameClashAndRowsHaveSameAngles() {
    MockModificationListener<UnslicedGroup> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup("A", {rowWithNameAndAngle("C", 0.1)}, ""));
    auto addition = UnslicedReductionJobs();
    addition.appendGroup(
        UnslicedGroup("A", {rowWithNameAndAngle("D", 0.1)}, ""));

    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(1u, target.groups()[0].rows().size());
    TS_ASSERT_EQUALS(std::vector<std::string>({"C", "D"}),
                     target.groups()[0].rows()[0].get().runNumbers());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  void testCallsModifiedWhenMergingRow() {
    MockModificationListener<UnslicedGroup> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup("A", {rowWithNameAndAngle("C", 0.1)}, ""));
    auto addition = UnslicedReductionJobs();
    addition.appendGroup(
        UnslicedGroup("A", {rowWithNameAndAngle("D", 0.1)}, ""));

    EXPECT_CALL(listener, rowModified(0, 0, _));
    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT_EQUALS(1u, target.groups().size());
    TS_ASSERT_EQUALS(1u, target.groups()[0].rows().size());
    TS_ASSERT_EQUALS(std::vector<std::string>({"C", "D"}),
                     target.groups()[0].rows()[0].get().runNumbers());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

  template <typename T>
  bool equalRunNumbers(ReductionJobs<T> const &lhs,
                       ReductionJobs<T> const &rhs) {
    if (lhs.groups().size() == rhs.groups().size()) {
      auto lhsGroupIt = lhs.groups().cbegin();
      auto rhsGroupIt = rhs.groups().cbegin();
      for (; lhsGroupIt != lhs.groups().cend(); ++lhsGroupIt, ++rhsGroupIt) {
        auto const &lhsGroup = (*lhsGroupIt);
        auto const &rhsGroup = (*rhsGroupIt);
        if (lhsGroup.rows().size() == rhsGroup.rows().size()) {
          auto lhsRowIt = lhsGroup.rows().cbegin();
          auto rhsRowIt = rhsGroup.rows().cbegin();
          for (; lhsRowIt != lhsGroup.rows().cend(); ++lhsRowIt, ++rhsRowIt) {
            auto const &lhsRow = (*lhsRowIt);
            auto const &rhsRow = (*rhsRowIt);
            if (lhsRow.is_initialized() != rhsRow.is_initialized())
              return false;
            else if (lhsRow.is_initialized())
              if (lhsRow.get().runNumbers() != rhsRow.get().runNumbers())
                return false;
          }
        } else {
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
                         rowWithNamesAndAngle({"B", "C"}, 0.0), m_nameFactory);

    TS_ASSERT_EQUALS(std::vector<std::string>({"A", "B", "C"}),
                     row.runNumbers());
  }

  void testMergeIntoSelfResultsInNoChange() {
    MockModificationListener<UnslicedGroup> listener;
    auto target = UnslicedReductionJobs();
    target.appendGroup(UnslicedGroup(
        "S1 SI/ D20 ",
        {rowWithNameAndAngle("47450", 0.7), rowWithNameAndAngle("47451", 2.3)},
        ""));

    target.appendGroup(UnslicedGroup(
        "S2 SI/ D20 ", {rowWithNamesAndAngle({"47450", "47453"}, 0.7)}, ""));

    auto addition = target;
    mergeJobsInto(target, addition, m_thetaTolerance, m_nameFactory, listener);

    TS_ASSERT(equalRunNumbers(target, addition))
    TS_ASSERT(Mock::VerifyAndClearExpectations(&listener));
  }

private:
  double m_thetaTolerance;
  Slicing m_slicing;
  WorkspaceNamesFactory m_nameFactory;
};
#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBSMERGETEST_H_
