#ifndef MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H
#define MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidQtMantidWidgets/HintStrategy.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"

#include <boost/scoped_ptr.hpp>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

//=====================================================================================
// Functional tests
//=====================================================================================
class AlgorithmHintStrategyTest : public CxxTest::TestSuite {

  // Inner class :: Fake Algorithm
  class FakeAlgorithm : public Algorithm {

  public:
    FakeAlgorithm() {}
    ~FakeAlgorithm() override {}
    const std::string name() const override { return "Fake Algorithm"; };
    int version() const override { return 1; };
    const std::string category() const override { return ""; };
    const std::string summary() const override { return "A Fake Algorithm"; };

  private:
    void init() override {
      declareProperty("IntValue", 0);
      declareProperty("DoubleValue", 0.01);
      declareProperty("BoolValue", false);
      declareProperty("StringValue", "Empty");
      auto mustBePositive =
          boost::make_shared<Mantid::Kernel::BoundedValidator<int>>();
      mustBePositive->setLower(0);
      declareProperty("PositiveIntValue", 0, mustBePositive);
      declareProperty("PositiveIntValue1", 0, mustBePositive);
      declareProperty(
          Mantid::Kernel::make_unique<Mantid::Kernel::ArrayProperty<int>>(
              "IntArray"));
      declareProperty(
          Mantid::Kernel::make_unique<Mantid::Kernel::ArrayProperty<double>>(
              "DoubleArray"));
      declareProperty(Mantid::Kernel::make_unique<
          Mantid::Kernel::ArrayProperty<std::string>>("StringArray"));
    };
    void exec() override { return; };
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmHintStrategyTest *createSuite() {
    return new AlgorithmHintStrategyTest();
  }
  static void destroySuite(AlgorithmHintStrategyTest *suite) { delete suite; }

  AlgorithmHintStrategyTest() {
    m_propAlg = static_cast<IAlgorithm_sptr>(new FakeAlgorithm());
    m_propAlg->initialize();
    // Expected hints for PropertyAlgorithm
    m_propMap["IntValue"] = "";
    m_propMap["DoubleValue"] = "";
    m_propMap["BoolValue"] = "";
    m_propMap["StringValue"] = "";
    m_propMap["PositiveIntValue"] = "";
    m_propMap["PositiveIntValue1"] = "";
    m_propMap["IntArray"] = "";
    m_propMap["DoubleArray"] = "";
    m_propMap["StringArray"] = "";
  }

  void testCreateHints() {
    boost::scoped_ptr<HintStrategy> strategy(
        new AlgorithmHintStrategy(m_propAlg, std::set<std::string>()));
    TS_ASSERT_EQUALS(m_propMap, strategy->createHints());
  }

  void testBlacklist() {
    std::set<std::string> blacklist;
    blacklist.insert("DoubleValue");
    blacklist.insert("IntArray");

    boost::scoped_ptr<HintStrategy> strategy(
        new AlgorithmHintStrategy(m_propAlg, blacklist));
    auto expected = m_propMap;
    expected.erase("DoubleValue");
    expected.erase("IntArray");
    TS_ASSERT_EQUALS(expected, strategy->createHints());
  }

protected:
  IAlgorithm_sptr m_propAlg;
  std::map<std::string, std::string> m_propMap;
};

#endif /*MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H */
