#ifndef MANTID_API_ALGORITHMPROPERTYTEST_H_
#define MANTID_API_ALGORITHMPROPERTYTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmHasProperty.h"
#include "MantidAPI/AlgorithmProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class AlgorithmPropertyTest : public CxxTest::TestSuite {
private:
  /// Use a fake algorithm object instead of a dependency on a real one.
  class SimpleSum : public Algorithm {
  public:
    SimpleSum() : Algorithm() {}
    ~SimpleSum() override {}
    const std::string name() const override { return "SimpleSum"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Dummy"; }
    const std::string summary() const override { return "Test summary"; }

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

  class HasAlgProp : public Algorithm {
  public:
    const std::string name() const override { return "HasAlgProp"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Dummy"; }
    const std::string summary() const override { return "Test summary"; }
    void init() override {
      declareProperty(make_unique<AlgorithmProperty>("CalculateStep"));
    }
    void exec() override {}
  };

  class HasAlgPropAndValidator : public Algorithm {
  public:
    const std::string name() const override { return "HasAlgPropAndValidator"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Dummy"; }
    const std::string summary() const override { return "Test summary"; }
    void init() override {
      declareProperty(make_unique<AlgorithmProperty>(
          "CalculateStep",
          boost::make_shared<AlgorithmHasProperty>("Output1")));
    }
    void exec() override {}
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmPropertyTest *createSuite() {
    return new AlgorithmPropertyTest();
  }
  static void destroySuite(AlgorithmPropertyTest *suite) { delete suite; }

  AlgorithmPropertyTest() {
    Mantid::API::AlgorithmFactory::Instance().subscribe<SimpleSum>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<HasAlgProp>();
    Mantid::API::AlgorithmFactory::Instance()
        .subscribe<HasAlgPropAndValidator>();
  }

  ~AlgorithmPropertyTest() override {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SimpleSum", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("HasAlgProp1", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe(
        "HasAlgPropAndValidator1", 1);
  }

  void test_A_Valid_Alg_String_Is_Accepted() {
    SimpleSum adder;
    adder.initialize();
    adder.execute();

    TS_ASSERT_EQUALS(adder.getPropertyValue("Output1"), "3");
    AlgorithmProperty testProp("CalculateStep");
    TS_ASSERT_EQUALS(testProp.setValue(adder.toString()), "");
  }

  void test_An_Invalid_String_Returns_An_Appropriate_Error() {
    AlgorithmProperty testProp("CalculateStep");
    TS_ASSERT_EQUALS(testProp.setValue("{\"name\":\"ComplexSum\"}"),
                     "Algorithm not registered ComplexSum");
  }

  void test_Alg_With_An_AlgorithmProperty_Accepts_Another_Algorithm() {
    HasAlgProp testAlg;
    testAlg.initialize();

    IAlgorithm_sptr adder =
        Mantid::API::AlgorithmFactory::Instance().create("SimpleSum", 1);
    adder->initialize();
    adder->execute();

    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("CalculateStep", adder));
    // Can we retrieve it again
    TS_ASSERT_THROWS_NOTHING(IAlgorithm_sptr calcStep =
                                 testAlg.getProperty("CalculateStep"));
    // (And const) Can we retrieve it again
    TS_ASSERT_THROWS_NOTHING(IAlgorithm_const_sptr calcStep =
                                 testAlg.getProperty("CalculateStep"));

    // Is it correct?
    IAlgorithm_sptr calcStep = testAlg.getProperty("CalculateStep");
    TS_ASSERT_EQUALS(calcStep->getPropertyValue("Output1"), "3");
  }

  void
  test_Alg_With_AlgorithmProperty_And_Validator_Fails_If_Input_Is_Invalid() {
    HasAlgPropAndValidator testAlg;
    testAlg.initialize();

    // Without initialize it has no properties
    IAlgorithm_sptr adder =
        Mantid::API::AlgorithmFactory::Instance().create("SimpleSum", 1);
    TS_ASSERT_THROWS(testAlg.setProperty("CalculateStep", adder),
                     std::invalid_argument);
    // Add the required property so now it should pass
    adder->initialize();
    TS_ASSERT_THROWS_NOTHING(testAlg.setProperty("CalculateStep", adder));
  }
};

#endif /* MANTID_API_ALGORITHMPROPERTYTEST_H_ */
