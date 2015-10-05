#ifndef WORKSPACEHISTORYTEST_H_
#define WORKSPACEHISTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/Property.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "Poco/File.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class WorkspaceHistoryTest : public CxxTest::TestSuite {
private:
  /// Use a fake algorithm object instead of a dependency on a real one.
  class SimpleSum : public Algorithm {
  public:
    SimpleSum() : Algorithm() {}
    virtual ~SimpleSum() {}
    const std::string name() const { return "SimpleSum"; }
    int version() const { return 1; }
    const std::string category() const { return "Dummy"; }
    const std::string summary() const { return "Dummy summary"; }

    void init() {
      declareProperty("Input1", 2);
      declareProperty("Input2", 1);
      declareProperty("Output1", -1, Direction::Output);
    }
    void exec() {
      const int lhs = getProperty("Input1");
      const int rhs = getProperty("Input2");
      const int sum = lhs + rhs;

      setProperty("Output1", sum);
    }
  };

  class SimpleSum2 : public SimpleSum {
  public:
    const std::string name() const { return "SimpleSum2"; }
    int version() const { return 1; }
    const std::string category() const { return "Dummy"; }

    void init() {
      SimpleSum::init();
      declareProperty("Input3", 4);
      declareProperty("Output2", -1, Direction::Output);
    }
    void exec() {
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

    AlgorithmHistory alg1("FirstAlgorithm", 2);
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
    AlgorithmHistory alg1(&simplesum,
                          Mantid::Kernel::DateAndTime::defaultTime(), 1.0, 0);
    AlgorithmHistory alg2(&simplesum2,
                          Mantid::Kernel::DateAndTime::defaultTime(), 1.0, 1);

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
    TS_ASSERT_THROWS(emptyHistory.lastAlgorithm(), std::out_of_range);
    TS_ASSERT_THROWS(emptyHistory.getAlgorithm(1), std::out_of_range);
  }
};

class WorkspaceHistoryTestPerformance : public CxxTest::TestSuite {
public:
  void test_Wide_History() {
    int depth = 3;
    int width = 50;

    AlgorithmHistory algHist("AnAlgorithm", 1);
    build_Algorithm_History(algHist, width, depth);
    m_wsHist.addHistory(boost::make_shared<AlgorithmHistory>(algHist));
  }

  void test_Deep_History() {
    int depth = 10;
    int width = 3;

    AlgorithmHistory algHist("AnAlgorithm", 1);
    build_Algorithm_History(algHist, width, depth);
    m_wsHist.addHistory(boost::make_shared<AlgorithmHistory>(algHist));
  }

  void build_Algorithm_History(AlgorithmHistory &parent, int width,
                               int depth = 0) {
    if (depth > 0) {

      for (int i = 0; i < width; ++i) {
        AlgorithmHistory algHist("AnAlgorithm", 1);
        build_Algorithm_History(algHist, width, depth - 1);
        parent.addChildHistory(boost::make_shared<AlgorithmHistory>(algHist));
      }
    }
  }

private:
  Mantid::API::WorkspaceHistory m_wsHist;
};

#endif
