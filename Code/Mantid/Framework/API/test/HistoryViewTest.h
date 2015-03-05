#ifndef HISTORYVIEWTEST_H_
#define HISTORYVIEWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/HistoryView.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Kernel::DateAndTime;


class HistoryViewTest : public CxxTest::TestSuite
{

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistoryViewTest *createSuite() { return new HistoryViewTest(); }
  static void destroySuite( HistoryViewTest *suite ) { delete suite; }

private:

  // 'Empty' algorithm class for tests
  class testalg : public Algorithm
  {
  public:
    testalg() : Algorithm() {}
    virtual ~testalg() {}
    const std::string summary() const { return "testalg"; } ///< Algorithm's documentation summary
    const std::string name() const { return "testalg";} ///< Algorithm's name for identification
    int version() const  { return 1;} ///< Algorithm's version for identification
    const std::string category() const { return "Cat";} ///< Algorithm's category for identification

    void init()
    {
      declareProperty("name", "", Direction::Input);
    }

    void exec() {}
  };

private:
  AlgorithmHistory_sptr createFromTestAlg(const std::string& name, DateAndTime execTime = DateAndTime::defaultTime())
  {
    testalg alg;
    alg.initialize();
    alg.setPropertyValue("name", name);
    alg.execute();

    AlgorithmHistory history(&alg, execTime, 14.0,  m_execCount++);
    return boost::make_shared<AlgorithmHistory>(history);
  }

public:
  HistoryViewTest() : m_wsHist(), m_execCount(0)
  {
    //create dummy history structure
    auto alg1 = createFromTestAlg("alg1", DateAndTime(100, 0));
    auto child1 = createFromTestAlg("child1", DateAndTime(110, 0));
    alg1->addChildHistory(child1);

    auto alg2 = createFromTestAlg("alg2", DateAndTime(200, 0));
    auto child2 = createFromTestAlg("child2", DateAndTime(210, 0));

    auto subChild21 = createFromTestAlg("subChild21", DateAndTime(211, 0));
    auto subChild22 = createFromTestAlg("subChild22", DateAndTime(212, 0));

    child2->addChildHistory(subChild21);
    child2->addChildHistory(subChild22);

    alg2->addChildHistory(child2);

    auto alg3 = createFromTestAlg("alg3", DateAndTime(300, 0));

    m_wsHist.addHistory(alg1);
    m_wsHist.addHistory(alg2);
    m_wsHist.addHistory(alg3);
  }

  void test_Empty()
  {
    WorkspaceHistory wsHist;
    HistoryView view(wsHist);
    TS_ASSERT_EQUALS ( view.size(), 0 );
    TS_ASSERT_THROWS_ANYTHING ( view.unroll(0) );
    TS_ASSERT_THROWS_ANYTHING ( view.roll(0) );
  }

  void test_Build()
  {
    HistoryView view(m_wsHist);
    TS_ASSERT_EQUALS( view.size(), 3);

    int i = 0;
    auto items = view.getAlgorithmsList();
    for (auto it = items.begin(); it != items.end(); ++it, ++i)
    {
      auto history = it->getAlgorithmHistory();
      auto props = history->getProperties();
      TS_ASSERT_EQUALS(props[0]->value(), "alg" + boost::lexical_cast<std::string>(i+1) );
    }
  }

  void test_Unroll_History()
  {
    HistoryView view(m_wsHist);
    //unroll alg 2
    TS_ASSERT_THROWS_NOTHING( view.unroll(0) );
    TS_ASSERT_EQUALS(view.size(), 4);

    auto items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 4);

    std::vector<std::string> propNames(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "child1")
    TS_ASSERT_EQUALS(propNames[2], "alg2")
    TS_ASSERT_EQUALS(propNames[3], "alg3")
  }

  void test_Simple_Roll_History()
  {
    //tests the case where we have a single layer of history unrolled
    HistoryView view(m_wsHist);

    //unroll alg 2
    TS_ASSERT_THROWS_NOTHING( view.unroll(0) );

    TS_ASSERT_EQUALS(view.size(), 4);
    auto items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 4);

    std::vector<std::string> propNames(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    //check it unrolled properly
    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "child1")
    TS_ASSERT_EQUALS(propNames[2], "alg2")
    TS_ASSERT_EQUALS(propNames[3], "alg3")

    //roll it back up
    TS_ASSERT_THROWS_NOTHING( view.roll(0) );

    TS_ASSERT_EQUALS(view.size(), 3);
    items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 3);

    propNames.clear();
    propNames.resize(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    //check it rolled back up properly
    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "alg2")
    TS_ASSERT_EQUALS(propNames[2], "alg3")
  }

  void test_Complex_Roll_History()
  {
    //tests the case where we have multiple layers of history unrolled
    HistoryView view(m_wsHist);

    //unroll alg2
    TS_ASSERT_THROWS_NOTHING( view.unroll(1) );

    TS_ASSERT_EQUALS(view.size(), 4);
    auto items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 4);

    std::vector<std::string> propNames(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "alg2")
    TS_ASSERT_EQUALS(propNames[2], "child2")
    TS_ASSERT_EQUALS(propNames[3], "alg3")

    //unroll another level of history
    TS_ASSERT_THROWS_NOTHING( view.unroll(2) )

    TS_ASSERT_EQUALS(view.size(), 6);
    items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 6);

    propNames.clear();
    propNames.resize(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "alg2")
    TS_ASSERT_EQUALS(propNames[2], "child2")
    TS_ASSERT_EQUALS(propNames[3], "subChild21")
    TS_ASSERT_EQUALS(propNames[4], "subChild22")
    TS_ASSERT_EQUALS(propNames[5], "alg3")

    //now roll everything back up to the top level
    TS_ASSERT_THROWS_NOTHING( view.roll(1) )

    TS_ASSERT_EQUALS(view.size(), 3);
    items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 3);

    propNames.clear();
    propNames.resize(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "alg2")
    TS_ASSERT_EQUALS(propNames[2], "alg3")
  }

  void test_Unroll_All()
  {
    HistoryView view(m_wsHist);

    TS_ASSERT_THROWS_NOTHING( view.unrollAll() );

    TS_ASSERT_EQUALS(view.size(), 7);
    auto items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 7);

    std::vector<std::string> propNames(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "child1")
    TS_ASSERT_EQUALS(propNames[2], "alg2")
    TS_ASSERT_EQUALS(propNames[3], "child2")
    TS_ASSERT_EQUALS(propNames[4], "subChild21")
    TS_ASSERT_EQUALS(propNames[5], "subChild22")
    TS_ASSERT_EQUALS(propNames[6], "alg3")
  }

  void test_Roll_All()
  {
    HistoryView view(m_wsHist);

    TS_ASSERT_THROWS_NOTHING( view.unrollAll() );

    TS_ASSERT_EQUALS(view.size(), 7);
    auto items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 7);

    std::vector<std::string> propNames(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg1")
    TS_ASSERT_EQUALS(propNames[1], "child1")
    TS_ASSERT_EQUALS(propNames[2], "alg2")
    TS_ASSERT_EQUALS(propNames[3], "child2")
    TS_ASSERT_EQUALS(propNames[4], "subChild21")
    TS_ASSERT_EQUALS(propNames[5], "subChild22")
    TS_ASSERT_EQUALS(propNames[6], "alg3")

    TS_ASSERT_THROWS_NOTHING( view.rollAll() );

    items = view.getAlgorithmsList();
    int i = 0;
    for (auto it = items.begin(); it != items.end(); ++it, ++i)
    {
      auto history = it->getAlgorithmHistory();
      auto props = history->getProperties();
      TS_ASSERT_EQUALS(props[0]->value(), "alg" + boost::lexical_cast<std::string>(i+1) );
    }
  }

  void test_Index_To_Large()
  {
    HistoryView view(m_wsHist);
    TS_ASSERT_THROWS_ANYTHING ( view.unroll(3) );
    TS_ASSERT_THROWS_ANYTHING ( view.roll(3) );
  }

  void test_Filter_By_Exec_Time_Full_Range()
  {
    HistoryView view(m_wsHist);

    // Unroll to get all algorithms
    TS_ASSERT_THROWS_NOTHING( view.unrollAll() );
    TS_ASSERT_EQUALS(view.size(), 7);

    // Filter by time with a start and end time
    TS_ASSERT_THROWS_NOTHING( view.filterBetweenExecDate(DateAndTime(200, 0), DateAndTime(211, 0)) );
    TS_ASSERT_EQUALS(view.size(), 3);

    // Get algorithm list and compare results
    auto items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 3);

    std::vector<std::string> propNames(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg2")
    TS_ASSERT_EQUALS(propNames[1], "child2")
    TS_ASSERT_EQUALS(propNames[2], "subChild21")
  }

  void test_Filter_By_Exec_Time_Start_Only()
  {
    HistoryView view(m_wsHist);

    // Unroll to get all algorithms
    TS_ASSERT_THROWS_NOTHING( view.unrollAll() );
    TS_ASSERT_EQUALS(view.size(), 7);

    // Filter by time with a start time only
    TS_ASSERT_THROWS_NOTHING( view.filterBetweenExecDate(DateAndTime(200, 0)) );
    TS_ASSERT_EQUALS(view.size(), 5);

    // Get algorithm list and compare results
    auto items = view.getAlgorithmsList();
    TS_ASSERT_EQUALS(items.size(), 5);

    std::vector<std::string> propNames(items.size());
    for (size_t i=0;i<items.size();++i)
    {
      propNames[i] = items[i].getAlgorithmHistory()->getProperties()[0]->value();
    }

    TS_ASSERT_EQUALS(propNames[0], "alg2")
    TS_ASSERT_EQUALS(propNames[1], "child2")
    TS_ASSERT_EQUALS(propNames[2], "subChild21")
    TS_ASSERT_EQUALS(propNames[3], "subChild22")
    TS_ASSERT_EQUALS(propNames[4], "alg3")
  }

  WorkspaceHistory m_wsHist;
  size_t m_execCount;

};

#endif
