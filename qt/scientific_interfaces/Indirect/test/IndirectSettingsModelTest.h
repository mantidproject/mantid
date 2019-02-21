// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTSETTINGSMODELTEST_H_
#define MANTIDQT_INDIRECTSETTINGSMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectSettingsModel.h"

#include "MantidKernel/make_unique.h"

using namespace MantidQt::CustomInterfaces::IDA;

class IndirectSettingsModelTest : public CxxTest::TestSuite {
public:
  static IndirectSettingsModelTest *createSuite() {
    return new IndirectSettingsModelTest();
  }

  static void destroySuite(IndirectSettingsModelTest *suite) { delete suite; }

  void setUp() override {
    m_model = Mantid::Kernel::make_unique<IndirectSettingsModel>(
        "Data Analysis", "restrict-input-by-name,plot-error-bars");
  }

  void tearDown() override { m_model.reset(); }

  void
  test_that_the_model_has_been_instantiated_with_the_correct_settings_group() {
    TS_ASSERT_EQUALS(m_model->getSettingsGroup(), "Data Analysis");
  }

  void
  test_that_hasInterfaceSettings_returns_true_when_the_model_stores_interface_specific_settings() {
    TS_ASSERT(m_model->hasInterfaceSettings());
  }

  void
  test_that_hasInterfaceSettings_returns_false_when_the_model_does_not_store_interface_specific_settings() {
    m_model = std::make_unique<IndirectSettingsModel>("Data Analysis", "");
    TS_ASSERT(!m_model->hasInterfaceSettings());
  }

  void
  test_that_isSettingAvailable_returns_true_if_the_setting_is_stored_by_the_model() {
    TS_ASSERT(m_model->isSettingAvailable("restrict-input-by-name"));
    TS_ASSERT(m_model->isSettingAvailable("plot-error-bars"));
  }

  void
  test_that_isSettingAvailable_returns_false_if_the_setting_is_not_stored_by_the_model() {
    TS_ASSERT(!m_model->isSettingAvailable("false-setting"));
  }

  void test_that_setFacility_will_set_the_saved_facility() {
    m_model->setFacility("ISIS");
    TS_ASSERT_EQUALS(m_model->getFacility(), "ISIS");
    m_model->setFacility("ILL");
    TS_ASSERT_EQUALS(m_model->getFacility(), "ILL");
  }

private:
  std::unique_ptr<IndirectSettingsModel> m_model;
};

#endif /* MANTIDQT_INDIRECTSETTINGSMODELTEST_H_ */
