// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/UnaryOperation.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class UnaryOpHelper : public Mantid::Algorithms::UnaryOperation {
public:
  /// Default constructor
  UnaryOpHelper() : UnaryOperation() {};
  /// Destructor
  ~UnaryOpHelper() override {};

  const std::string name() const override { return "None"; }
  int version() const override { return 0; }
  const std::string summary() const override { return "Test summary"; }

  // Pass-throughs to UnaryOperation methods
  const std::string inputPropName() const override { return UnaryOperation::inputPropName(); }
  const std::string outputPropName() const override { return UnaryOperation::outputPropName(); }

private:
  void performUnaryOperation(const double, const double, const double, double &, double &) override {}
};

class UnaryOperationTest : public CxxTest::TestSuite {
public:
  void testCategory() { TS_ASSERT_EQUALS(helper.category(), "Arithmetic") }

  void testInputPropName() { TS_ASSERT_EQUALS(helper.inputPropName(), "InputWorkspace") }

  void testOutputPropName() { TS_ASSERT_EQUALS(helper.outputPropName(), "OutputWorkspace") }

  void testInit() {
    UnaryOpHelper helper2;
    TS_ASSERT_THROWS_NOTHING(helper2.initialize())
    TS_ASSERT(helper2.isInitialized())

    const std::vector<Property *> props = helper2.getProperties();
    TS_ASSERT_EQUALS(props.size(), 2)

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace")
    TS_ASSERT(props[0]->isDefault())
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]))

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace")
    TS_ASSERT(props[1]->isDefault())
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[1]))
  }

  void testExec() {
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add("InputWS", inputWS);

    UnaryOpHelper helper3;
    helper3.initialize();
    TS_ASSERT_THROWS_NOTHING(helper3.setPropertyValue("InputWorkspace", "InputWS"))
    TS_ASSERT_THROWS_NOTHING(helper3.setPropertyValue("OutputWorkspace", "InputWS"))

    TS_ASSERT_THROWS_NOTHING(helper3.execute())
    TS_ASSERT(helper3.isExecuted())

    AnalysisDataService::Instance().remove("InputWS");
  }

private:
  UnaryOpHelper helper;
};
