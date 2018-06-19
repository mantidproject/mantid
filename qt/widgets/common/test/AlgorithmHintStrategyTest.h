#ifndef MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H
#define MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidQtWidgets/Common/HintStrategy.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"

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
  }

  void testCreateHints() {
    auto strategy = AlgorithmHintStrategy(m_propAlg, {});
    auto expected = std::vector<Hint>(
        {Hint("IntValue", ""), Hint("DoubleValue", ""), Hint("BoolValue", ""),
         Hint("StringValue", ""), Hint("PositiveIntValue", ""),
         Hint("PositiveIntValue1", ""), Hint("IntArray", ""),
         Hint("DoubleArray", ""), Hint("StringArray", "")});
    TS_ASSERT_EQUALS(expected, strategy.createHints());
  }

  void testBlacklist() {
    auto strategy =
        AlgorithmHintStrategy(m_propAlg, {"DoubleValue", "IntArray"});
    auto expected = std::vector<Hint>(
        {Hint("IntValue", ""), Hint("DoubleValue", ""), Hint("BoolValue", ""),
         Hint("StringValue", ""), Hint("PositiveIntValue", ""),
         Hint("PositiveIntValue1", ""), Hint("StringArray", "")});

    TS_ASSERT_EQUALS(expected, strategy.createHints());
  }

protected:
  IAlgorithm_sptr m_propAlg;
};

#endif /*MANTID_MANTIDWIDGETS_ALGORITHMHINTSTRATEGYTEST_H */
