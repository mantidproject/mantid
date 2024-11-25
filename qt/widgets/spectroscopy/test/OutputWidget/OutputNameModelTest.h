// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameModel.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class OutputNameModelTest : public CxxTest::TestSuite {
public:
  static OutputNameModelTest *createSuite() { return new OutputNameModelTest(); }

  static void destroySuite(OutputNameModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<OutputNameModel>(); }

  void test_index_label_position_is_correct() {
    std::vector<std::string> test_suffices({"_red", "_sqw"});
    m_model->setSuffixes(test_suffices);
    auto pos = m_model->findIndexToInsertLabel("test_red");
    TS_ASSERT_EQUALS(pos, 4);
    pos = m_model->findIndexToInsertLabel("test");
    TS_ASSERT_EQUALS(pos, 4);
    pos = m_model->findIndexToInsertLabel("test_red_sqw");
    TS_ASSERT_EQUALS(pos, 8);
  }

private:
  std::unique_ptr<IOutputNameModel> m_model;
};
