// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"

#include <cxxtest/TestSuite.h>

#include <memory>

#include <QtTest>

using namespace MantidQt::MantidWidgets;

class FitScriptGeneratorDataTableTest : public CxxTest::TestSuite {

public:
  static FitScriptGeneratorDataTableTest *createSuite() {
    return new FitScriptGeneratorDataTableTest;
  }
  static void destroySuite(FitScriptGeneratorDataTableTest *suite) {
    delete suite;
  }

  FitScriptGeneratorDataTableTest() {}

  void setUp() override {
    m_dataTable = std::make_unique<FitScriptGeneratorDataTableTest>();
  }

  void tearDown() override { m_dataTable.reset(); }

  void test_empty() {}

private:
  std::unique_ptr<FitScriptGeneratorDataTableTest> m_dataTable;
};
