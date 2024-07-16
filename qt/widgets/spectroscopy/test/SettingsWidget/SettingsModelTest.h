// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <memory>

#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsModel.h"

using namespace MantidQt::CustomInterfaces;

class SettingsModelTest : public CxxTest::TestSuite {
public:
  static SettingsModelTest *createSuite() { return new SettingsModelTest(); }

  static void destroySuite(SettingsModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<SettingsModel>(); }

  void tearDown() override { m_model.reset(); }

  void test_that_setFacility_will_set_the_saved_facility() {
    m_model->setFacility("ISIS");
    TS_ASSERT_EQUALS(m_model->getFacility(), "ISIS");
    m_model->setFacility("ILL");
    TS_ASSERT_EQUALS(m_model->getFacility(), "ILL");
  }

private:
  std::unique_ptr<SettingsModel> m_model;
};
