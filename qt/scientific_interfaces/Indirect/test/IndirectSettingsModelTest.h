// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTSETTINGSMODELTEST_H_
#define MANTIDQT_INDIRECTSETTINGSMODELTEST_H_

#include <cxxtest/TestSuite.h>
#include <memory>

#include "IndirectSettingsModel.h"

using namespace MantidQt::CustomInterfaces;

class IndirectSettingsModelTest : public CxxTest::TestSuite {
public:
  static IndirectSettingsModelTest *createSuite() {
    return new IndirectSettingsModelTest();
  }

  static void destroySuite(IndirectSettingsModelTest *suite) { delete suite; }

  void setUp() override { m_model = std::make_unique<IndirectSettingsModel>(); }

  void tearDown() override { m_model.reset(); }

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
