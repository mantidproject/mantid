#ifndef ALGORITHMHISTORYTEST_H_
#define ALGORITHMHISTORYTEST_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <cxxtest/TestSuite.h>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

// 'Empty' algorithm class for tests
class testalg : public Algorithm {
public:
  testalg() : Algorithm() {}
  ~testalg() override {}
  const std::string name() const override {
    return "testalg";
  } ///< Algorithm's name for identification
  int version() const override {
    return 1;
  } ///< Algorithm's version for identification
  const std::string category() const override {
    return "Cat";
  } ///< Algorithm's category for identification
  const std::string summary() const override { return "Test summary"; }

  void init() override {
    declareProperty("arg1_param", "x", Direction::Input);
    declareProperty("arg2_param", 23);
  }
  void exec() override {}
};

class AlgorithmHistoryTest : public CxxTest::TestSuite {
public:
  AlgorithmHistoryTest() : m_correctOutput(), m_execCount(0) {}

  void testPopulate() {
    AlgorithmHistory AH = createTestHistory();
    // dump output to sting
    std::ostringstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    TS_ASSERT_THROWS_NOTHING(output << AH);
    TS_ASSERT_EQUALS(output.str(), m_correctOutput);
    // Does it equal itself
    TS_ASSERT_EQUALS(AH, AH);
  }

  void test_Less_Than_Returns_True_For_If_Execution_Order_Is_Lower() {
    AlgorithmHistory first = createTestHistory();
    AlgorithmHistory second = createTestHistory();
    TS_ASSERT_LESS_THAN(first, second);
  }

  void test_getPropertyValue() {
    AlgorithmHistory alg = createTestHistory();
    TS_ASSERT_EQUALS(alg.getPropertyValue("arg1_param"), "y");
    TS_ASSERT_EQUALS(alg.getPropertyValue("arg2_param"), "23");
    TS_ASSERT_THROWS_ANYTHING(alg.getPropertyValue("none_existant"));
  }

  void test_Created_Algorithm_Matches_History() {
    Mantid::API::AlgorithmFactory::Instance().subscribe<testalg>();
    Algorithm *testInput = new testalg;
    testInput->initialize();
    testInput->setPropertyValue("arg2_param", "5");
    AlgorithmHistory history(testInput);

    IAlgorithm_sptr compareAlg = history.createAlgorithm();
    TS_ASSERT_EQUALS(compareAlg->name(), testInput->name());
    TS_ASSERT_EQUALS(compareAlg->version(), testInput->version());
    TS_ASSERT_EQUALS(compareAlg->category(), testInput->category());

    TS_ASSERT_EQUALS(compareAlg->getPropertyValue("arg1_param"), "x");
    TS_ASSERT_EQUALS(compareAlg->getPropertyValue("arg2_param"), "5");

    Mantid::API::AlgorithmFactory::Instance().unsubscribe(testInput->name(),
                                                          testInput->version());
    delete testInput;
  }

  void test_Nested_History() {
    Mantid::API::AlgorithmFactory::Instance().subscribe<testalg>();
    Algorithm *testInput = new testalg;
    using namespace Mantid::API;
    AlgorithmHistory algHist = createTestHistory();

    // create some nested history records
    auto child1 = createFromTestAlg("child1");
    auto subChild11 = createFromTestAlg("subChild11");
    child1.addChildHistory(boost::make_shared<AlgorithmHistory>(subChild11));

    auto child2 = createFromTestAlg("child2");
    auto subChild21 = createFromTestAlg("subChild21");
    auto subChild22 = createFromTestAlg("subChild22");
    child2.addChildHistory(boost::make_shared<AlgorithmHistory>(subChild21));
    child2.addChildHistory(boost::make_shared<AlgorithmHistory>(subChild22));

    auto child3 = createFromTestAlg("child3");

    algHist.addChildHistory(boost::make_shared<AlgorithmHistory>(child1));
    algHist.addChildHistory(boost::make_shared<AlgorithmHistory>(child2));
    algHist.addChildHistory(boost::make_shared<AlgorithmHistory>(child3));

    // check parent algorithm matches
    std::ostringstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    TS_ASSERT_THROWS_NOTHING(output << algHist);
    TS_ASSERT_EQUALS(output.str(), m_correctOutput);

    auto children = algHist.getChildHistories();
    TS_ASSERT_EQUALS(children.size(), 3);

    // check children are valid
    int i = 1;
    AlgorithmHistories::iterator it;
    for (it = children.begin(); it != children.end(); ++it, ++i) {
      IAlgorithm_sptr childAlg = (*it)->createAlgorithm();
      std::string index = boost::lexical_cast<std::string>(i);
      TS_ASSERT_EQUALS(childAlg->getPropertyValue("arg1_param"),
                       "child" + index);

      // check sub children
      auto subchildren = (*it)->getChildHistories();
      int j = 1;
      AlgorithmHistories::iterator jt;
      for (jt = subchildren.begin(); jt != subchildren.end(); ++j, ++jt) {
        IAlgorithm_sptr subChildAlg = (*jt)->createAlgorithm();
        std::string subindex = boost::lexical_cast<std::string>(j);
        TS_ASSERT_EQUALS(subChildAlg->getPropertyValue("arg1_param"),
                         "subChild" + index + subindex);
      }
    }
    Mantid::API::AlgorithmFactory::Instance().unsubscribe(testInput->name(),
                                                          testInput->version());
    delete testInput;
  }

  void test_Create_Child_Algorithm() {
    AlgorithmFactory::Instance().subscribe<testalg>();
    Algorithm *testInput = new testalg;
    using namespace Mantid::API;
    AlgorithmHistory algHist = createTestHistory();

    // create some nested history records
    auto child1 = createFromTestAlg("child1");
    auto subChild11 = createFromTestAlg("subChild11");
    child1.addChildHistory(boost::make_shared<AlgorithmHistory>(subChild11));

    auto child2 = createFromTestAlg("child2");
    auto subChild21 = createFromTestAlg("subChild21");
    auto subChild22 = createFromTestAlg("subChild22");
    child2.addChildHistory(boost::make_shared<AlgorithmHistory>(subChild21));
    child2.addChildHistory(boost::make_shared<AlgorithmHistory>(subChild22));

    auto child3 = createFromTestAlg("child3");

    algHist.addChildHistory(boost::make_shared<AlgorithmHistory>(child1));
    algHist.addChildHistory(boost::make_shared<AlgorithmHistory>(child2));
    algHist.addChildHistory(boost::make_shared<AlgorithmHistory>(child3));

    IAlgorithm_sptr alg = algHist.getChildAlgorithm(0);
    TS_ASSERT_EQUALS(alg->name(), testInput->name());
    TS_ASSERT_EQUALS(alg->version(), testInput->version());
    TS_ASSERT_EQUALS(alg->category(), testInput->category());

    Mantid::API::AlgorithmFactory::Instance().unsubscribe(testInput->name(),
                                                          testInput->version());
    delete testInput;

    TS_ASSERT_EQUALS(alg->getPropertyValue("arg1_param"), "child1");
  }

private:
  AlgorithmHistory createTestHistory() {
    m_correctOutput = "Algorithm: testalg ";
    m_correctOutput += "v1\n";
    m_correctOutput += "Execution Date: 2008-Feb-29 09:54:49\n";
    m_correctOutput += "Execution Duration: 14 seconds\n";
    m_correctOutput += "Parameters:\n";
    m_correctOutput += "  Name: arg1_param, ";
    m_correctOutput += "Value: y, ";
    m_correctOutput += "Default?: No, ";
    m_correctOutput += "Direction: Input\n";
    m_correctOutput += "  Name: arg2_param, ";
    m_correctOutput += "Value: 23, ";
    m_correctOutput += "Default?: Yes, ";
    m_correctOutput += "Direction: Input\n";

    // set the time
    std::time_t rawtime;
    std::tm *timeinfo = new std::tm;
    timeinfo->tm_isdst = -1;

    /* The datetime must match that in the strng above */
    std::time(&rawtime);
    timeinfo->tm_year = 108;
    timeinfo->tm_mon = 1;
    timeinfo->tm_mday = 29;
    timeinfo->tm_hour = 9;
    timeinfo->tm_min = 54;
    timeinfo->tm_sec = 49;
    // Convert to time_t but assuming the tm is specified in UTC time.
    std::time_t execTime_t =
        Mantid::Types::Core::DateAndTime::utc_mktime(timeinfo);
    // Create a UTC datetime from it
    Mantid::Types::Core::DateAndTime execTime;
    execTime.set_from_time_t(execTime_t);

    // Not really much to test
    testalg alg;
    alg.initialize();
    alg.setPropertyValue("arg1_param", "y");
    alg.execute();

    delete timeinfo;

    return AlgorithmHistory(&alg, execTime, 14.0, m_execCount++);
  }

  AlgorithmHistory createFromTestAlg(std::string paramValue) {
    Algorithm *testInput = new testalg;
    testInput->initialize();
    testInput->setPropertyValue("arg1_param", paramValue);
    AlgorithmHistory history(testInput, 1, -1.0, m_execCount++);

    delete testInput;
    return history;
  }

  std::string m_correctOutput;
  size_t m_execCount;
};

#endif /* ALGORITHMHISTORYTEST_H_*/
