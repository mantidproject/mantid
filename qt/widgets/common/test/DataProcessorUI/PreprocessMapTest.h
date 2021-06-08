// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PreprocessMap.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class PreprocessMapTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PreprocessMapTest *createSuite() { return new PreprocessMapTest(); }
  static void destroySuite(PreprocessMapTest *suite) { delete suite; }
  PreprocessMapTest() { FrameworkManager::Instance(); };

  void test_add_element() {
    PreprocessMap preprocessMap;
    preprocessMap.addElement("Runs", "Plus", "", "+");
    preprocessMap.addElement("Transmission Runs", "CreateTransmissionWorkspaceAuto", "TRANS_", "_",
                             "FirstTransmissionRun,SecondTransmissionRun");

    auto preprocessingInstructions = preprocessMap.asMap();

    PreprocessingAlgorithm algPlus = preprocessingInstructions["Runs"];
    TS_ASSERT_EQUALS(algPlus.name(), "Plus");
    TS_ASSERT_EQUALS(algPlus.prefix(), "");
    TS_ASSERT_EQUALS(algPlus.separator(), "+");
    TS_ASSERT_EQUALS(algPlus.blacklist(), std::set<QString>());

    PreprocessingAlgorithm algTrans = preprocessingInstructions["Transmission Runs"];
    TS_ASSERT_EQUALS(algTrans.name(), "CreateTransmissionWorkspaceAuto");
    TS_ASSERT_EQUALS(algTrans.prefix(), "TRANS_");
    TS_ASSERT_EQUALS(algTrans.separator(), "_");
    std::set<QString> blacklist = {"FirstTransmissionRun", "SecondTransmissionRun"};
    TS_ASSERT_EQUALS(algTrans.blacklist(), blacklist);
  }
};