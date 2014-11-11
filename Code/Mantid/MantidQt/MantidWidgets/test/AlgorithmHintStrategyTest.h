#ifndef MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H
#define MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtMantidWidgets/HintStrategy.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

//=====================================================================================
// Functional tests
//=====================================================================================
class AlgorithmHintStrategyTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmHintStrategyTest *createSuite() { return new AlgorithmHintStrategyTest(); }
  static void destroySuite( AlgorithmHintStrategyTest *suite ) { delete suite; }

  AlgorithmHintStrategyTest()
  {
    FrameworkManager::Instance();
    m_propAlg = AlgorithmManager::Instance().create("PropertyAlgorithm");
    //Expected hints for PropertyAlgorithm
    m_propMap["IntValue"]          = "";
    m_propMap["DoubleValue"]       = "";
    m_propMap["BoolValue"]         = "";
    m_propMap["StringValue"]       = "";
    m_propMap["PositiveIntValue"]  = "";
    m_propMap["PositiveIntValue1"] = "";
    m_propMap["IntArray"]          = "";
    m_propMap["DoubleArray"]       = "";
    m_propMap["StringArray"]       = "";
  }

  void testCreateHints()
  {
    boost::scoped_ptr<HintStrategy> strategy(new AlgorithmHintStrategy(m_propAlg, std::set<std::string>()));
    TS_ASSERT_EQUALS(m_propMap, strategy->createHints());
  }

  void testBlacklist()
  {
    std::set<std::string> blacklist;
    blacklist.insert("DoubleValue");
    blacklist.insert("IntArray");

    boost::scoped_ptr<HintStrategy> strategy(new AlgorithmHintStrategy(m_propAlg, blacklist));
    auto expected = m_propMap;
    expected.erase("DoubleValue");
    expected.erase("IntArray");
    TS_ASSERT_EQUALS(expected, strategy->createHints());
  }

protected:
  IAlgorithm_sptr m_propAlg;
  std::map<std::string,std::string> m_propMap;
};

#endif /*MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H */
