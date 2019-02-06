// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ALGORITHMPROPERTYTEST_H_
#define MANTID_API_ALGORITHMPROPERTYTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmHasProperty.h"
#include "MantidAPI/AlgorithmProperty.h"
#include <json/value.h>

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
    using Mantid::API::AlgorithmFactory;
    auto &algmFactory = AlgorithmFactory::Instance();
    algmFactory.subscribe<SimpleSum>();
    algmFactory.subscribe<HasAlgProp>();
    algmFactory.subscribe<HasAlgPropAndValidator>();
  }

  ~AlgorithmPropertyTest() override {
    using Mantid::API::AlgorithmFactory;
    auto &algmFactory = AlgorithmFactory::Instance();
    algmFactory.unsubscribe("SimpleSum", 1);
    algmFactory.unsubscribe("HasAlgProp1", 1);
    algmFactory.unsubscribe("HasAlgPropAndValidator1", 1);
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

  void test_ValueAsJson() {
    AlgorithmProperty prop("name");
    SimpleSum adder;
    adder.initialize();
    adder.execute();
    prop.setValue(adder.toString());
    const auto jsonValue = prop.valueAsJson();
    TS_ASSERT_EQUALS(Json::objectValue, jsonValue.type());
    TS_ASSERT_EQUALS(adder.name(), jsonValue["name"].asString());
    TS_ASSERT_EQUALS(adder.version(), jsonValue["version"].asInt());
    TS_ASSERT_EQUALS(3, jsonValue["properties"]["Output1"].asInt());
  }

  void test_SetValueFromJson_With_Valid_Json() {
    AlgorithmProperty prop("PropName");
    const std::string helpMessage{prop.setValueFromJson(createAlgorithmJson())};

    TS_ASSERT(helpMessage.empty());
    auto algorithm = prop();
    TS_ASSERT_EQUALS(algorithm->toString(), prop.value());
    auto getIntProperty = [&algorithm](const std::string &name) {
      return static_cast<int>(algorithm->getProperty(name));
    };
    TS_ASSERT_EQUALS(5, getIntProperty("Input1"));
    TS_ASSERT_EQUALS(10, getIntProperty("Input2"));
    TS_ASSERT_EQUALS(15, getIntProperty("Output1"));
  }

  void test_SetValueFromJson_With_Invalid_Json() {
    Json::Value algm{1};
    AlgorithmProperty prop("PropName");
    const std::string helpMessage{prop.setValueFromJson(algm)};
    TS_ASSERT(!helpMessage.empty());
  }

  void test_Copy_Constructor() {
    AlgorithmProperty src("PropName");
    src.setValueFromJson(createAlgorithmJson());
    AlgorithmProperty dest{src};

    TS_ASSERT_EQUALS(src.value(), dest.value());
    TS_ASSERT_EQUALS(src.valueAsJson(), dest.valueAsJson());
  }

  void test_Move_Constructor() {
    AlgorithmProperty src("PropName");
    const auto algJson = createAlgorithmJson();
    src.setValueFromJson(algJson);
    AlgorithmProperty dest{std::move(src)};

    TS_ASSERT_EQUALS(Algorithm::fromJson(algJson)->toString(), dest.value());
    TS_ASSERT_EQUALS(algJson, dest.valueAsJson());
  }

private:
  Json::Value createAlgorithmJson() {
    Json::Value algm;
    algm["name"] = "SimpleSum";
    algm["version"] = 1;
    Json::Value properties;
    properties["Input1"] = 5;
    properties["Input2"] = 10;
    properties["Output1"] = 15;
    algm["properties"] = properties;
    return algm;
  }
};

#endif /* MANTID_API_ALGORITHMPROPERTYTEST_H_ */
