// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmHasProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class AlgorithmHasPropertyTest : public CxxTest::TestSuite {
private:
  class AlgorithmWithWorkspace : public Algorithm {
  public:
    const std::string name() const override { return "AlgorithmWithWorkspace"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat"; }
    const std::string summary() const override { return "Test summary"; }

    void init() override { declareProperty("OutputWorkspace", ""); }
    void exec() override {}
  };

  class AlgorithmWithNoWorkspace : public Algorithm {
  public:
    const std::string name() const override { return "AlgorithmWithNoWorkspace"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat"; }
    const std::string summary() const override { return "Test summary"; }

    void init() override { declareProperty("NotOutputWorkspace", ""); }
    void exec() override {}
  };

  class AlgorithmWithInvalidProperty : public Algorithm {
  public:
    const std::string name() const override { return "AlgorithmWithInvalidProperty"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat"; }
    const std::string summary() const override { return "Test summary"; }

    void init() override {
      auto lower = std::make_shared<Mantid::Kernel::BoundedValidator<int>>();
      lower->setLower(0);
      declareProperty("OutputValue", -1, lower);
    }
    void exec() override {}
  };

public:
  void test_Algorithm_With_Correct_Property_Is_Valid() {
    AlgorithmHasProperty check("OutputWorkspace");
    IAlgorithm_sptr tester(new AlgorithmWithWorkspace);
    tester->initialize();
    tester->execute();

    TS_ASSERT_EQUALS(check.isValid(tester), "");
  }

  void test_Algorithm_Without_Property_Is_Invalid() {
    AlgorithmHasProperty check("OutputWorkspace");
    IAlgorithm_sptr tester(new AlgorithmWithNoWorkspace);
    tester->initialize();
    tester->execute();

    TS_ASSERT_EQUALS(check.isValid(tester), "Algorithm object does not have "
                                            "the required property "
                                            "\"OutputWorkspace\"");
  }

  void test_Algorithm_With_Invalid_Property_Is_Invalid() {
    AlgorithmHasProperty check("OutputValue");
    IAlgorithm_sptr tester(new AlgorithmWithInvalidProperty);
    tester->initialize();

    TS_ASSERT_EQUALS(check.isValid(tester), "Algorithm object contains the required property \"OutputValue\" but "
                                            "it has an invalid value: -1");
  }
};
