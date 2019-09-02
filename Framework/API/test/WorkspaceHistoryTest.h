// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef WORKSPACEHISTORYTEST_H_
#define WORKSPACEHISTORYTEST_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/Property.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "Poco/File.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class WorkspaceHistoryTest : public CxxTest::TestSuite {
private:
  /// Use a fake algorithm object instead of a dependency on a real one.
  class SimpleSum : public Algorithm {
  public:
    SimpleSum() : Algorithm() {}
    ~SimpleSum() override {}
    const std::string name() const override { return "SimpleSum"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Dummy"; }
    const std::string summary() const override { return "Dummy summary"; }

    void init() override {
      declareProperty("Input1", 2);
      declareProperty("Input2", 1);
      declareProperty("Output1", -1, Direction::Output);
    }
    void exec() override {
      const int lhs = getProperty("Input1");
      const int rhs = getProperty("Input2");
      const int sum = lhs + rhs;

      setProperty("Output1", sum);
    }
  };

  class SimpleSum2 : public SimpleSum {
  public:
    const std::string name() const override { return "SimpleSum2"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Dummy"; }

    void init() override {
      SimpleSum::init();
      declareProperty("Input3", 4);
      declareProperty("Output2", -1, Direction::Output);
    }
    void exec() override {
      SimpleSum::exec();
      int sum = this->getProperty("Output1");
      setProperty("Output2", sum + 1);
    }
  };

public:
  void test_New_History_Is_Empty() {
    WorkspaceHistory history;
    TS_ASSERT_EQUALS(history.size(), 0);
    TS_ASSERT_EQUALS(history.empty(), true);
  }

  void test_Adding_History_Entry() {
    WorkspaceHistory history;
    TS_ASSERT_EQUALS(history.size(), 0);
    TS_ASSERT_EQUALS(history.empty(), true);

    AlgorithmHistory alg1("FirstAlgorithm", 2,
                          "207ca8f8-fee0-49ce-86c8-7842a7313c2e");
    alg1.addProperty("FirstAlgProperty", "1", false,
                     Mantid::Kernel::Direction::Input);

    boost::shared_ptr<AlgorithmHistory> algHistPtr =
        boost::make_shared<AlgorithmHistory>(alg1);
    history.addHistory(algHistPtr);
    TS_ASSERT_EQUALS(history.size(), 1);
    TS_ASSERT_EQUALS(history.empty(), false);

    Mantid::API::AlgorithmHistories algs = history.getAlgorithmHistories();
    TS_ASSERT_EQUALS(algs.size(), 1);
    TS_ASSERT_EQUALS(history.getAlgorithmHistory(0)->name(), "FirstAlgorithm");
    TS_ASSERT_EQUALS((*algs.begin())->name(), "FirstAlgorithm");
  }

  void test_Asking_For_A_Given_Algorithm_Returns_The_Correct_One() {
    Mantid::API::AlgorithmFactory::Instance().subscribe<SimpleSum>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<SimpleSum2>();

    SimpleSum simplesum;
    simplesum.initialize();
    simplesum.setPropertyValue("Input1", "5");
    simplesum.execute();
    SimpleSum2 simplesum2;
    simplesum2.initialize();
    simplesum2.setPropertyValue("Input3", "10");
    simplesum2.execute();

    WorkspaceHistory history;
    AlgorithmHistory alg1(
        &simplesum, Mantid::Types::Core::DateAndTime::defaultTime(), 1.0, 0);
    AlgorithmHistory alg2(
        &simplesum2, Mantid::Types::Core::DateAndTime::defaultTime(), 1.0, 1);

    history.addHistory(boost::make_shared<AlgorithmHistory>(alg1));
    history.addHistory(boost::make_shared<AlgorithmHistory>(alg2));

    auto second = history.getAlgorithmHistory(1);
    TS_ASSERT_EQUALS(second->name(), "SimpleSum2");

    IAlgorithm_sptr first = history.getAlgorithm(0);
    TS_ASSERT_EQUALS(first->name(), "SimpleSum");
    TS_ASSERT_EQUALS(first->getPropertyValue("Input1"), "5");
    TS_ASSERT_EQUALS(first->getPropertyValue("Output1"), "6");

    // Last algorithm
    IAlgorithm_sptr lastAlg = history.lastAlgorithm();
    TS_ASSERT_EQUALS(lastAlg->name(), "SimpleSum2");

    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SimpleSum", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SimpleSum2", 1);
  }

  void test_Empty_History_Throws_When_Retrieving_Attempting_To_Algorithms() {
    WorkspaceHistory emptyHistory;
    TS_ASSERT_THROWS(emptyHistory.lastAlgorithm(), const std::out_of_range &);
    TS_ASSERT_THROWS(emptyHistory.getAlgorithm(1), const std::out_of_range &);
  }
};

class WorkspaceHistoryTestPerformance : public CxxTest::TestSuite {
public:
  WorkspaceHistoryTestPerformance() {
    constructAlgHistories1();
    constructAlgHistories2();
  }

  void setUp() override { m_wsHist.clearHistory(); }

  void test_Wide_History() {
    int depth = 3;
    int width = 50;
    auto algHist = boost::make_shared<AlgorithmHistory>(
        "AnAlgorithm", 1, "207ca8f8-fee0-49ce-86c8-7842a7313c2e");
    build_Algorithm_History(*algHist, width, depth);
    m_wsHist.addHistory(std::move(algHist));
  }

  void test_Deep_History() {
    int depth = 10;
    int width = 3;

    auto algHist = boost::make_shared<AlgorithmHistory>(
        "AnAlgorithm", 1, "207ca8f8-fee0-49ce-86c8-7842a7313c2e");
    build_Algorithm_History(*algHist, width, depth);
    m_wsHist.addHistory(std::move(algHist));
  }

  void test_standard_insertion_500000_times() {
    for (auto i = 0u; i < 500000; ++i) {
      m_wsHist.addHistory(m_1000000Histories1[i]);
    }
  }

  void test_standard_insertion_1000000_times() {
    for (auto i = 0u; i < 1000000; ++i) {
      m_wsHist.addHistory(m_1000000Histories1[i]);
    }
  }

  void test_adding_1000000_to_500000_workspace_histories() {
    // It's hard to test this without doing this bit
    for (auto i = 0u; i < 500000; ++i) {
      m_wsHist.addHistory(m_1000000Histories1[i]);
    }
    // The actual test
    m_wsHist.addHistory(m_1000000Histories2);
  }

  void test_adding_1000000_to_1000000_workspace_histories() {
    // It's hard to test this without doing this bit
    for (auto i = 0u; i < 1000000; ++i) {
      m_wsHist.addHistory(m_1000000Histories1[i]);
    }
    // The actual test
    m_wsHist.addHistory(m_1000000Histories2);
  }

private:
  void build_Algorithm_History(AlgorithmHistory &parent, int width,
                               int depth = 0) {
    if (depth > 0) {
      for (int i = 0; i < width; ++i) {
        auto algHist = boost::make_shared<AlgorithmHistory>(
            "AnAlgorithm", 1, "207ca8f8-fee0-49ce-86c8-7842a7313c2e");
        build_Algorithm_History(*algHist, width, depth - 1);
        parent.addChildHistory(std::move(algHist));
      }
    }
  }

  void constructAlgHistories1() {
    for (auto i = 1u; i < 1000001; ++i) {
      auto algHist = boost::make_shared<AlgorithmHistory>(
          "AnAlgorithm", i, "207ca8f8-fee0-49ce-86c8-7842a7313c2e");
      m_1000000Histories1.emplace_back(std::move(algHist));
    }
  }
  void constructAlgHistories2() {
    for (auto i = 1000001u; i < 1000001; ++i) {
      auto algHist = boost::make_shared<AlgorithmHistory>(
          "AnAlgorithm", i, "207ca8f8-fee0-49ce-86c8-7842a7313c2e");
      m_1000000Histories2.addHistory(std::move(algHist));
    }
  }

  Mantid::API::WorkspaceHistory m_wsHist;
  std::vector<AlgorithmHistory_sptr> m_1000000Histories1;
  WorkspaceHistory m_1000000Histories2;
};

#endif
