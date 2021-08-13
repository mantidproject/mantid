// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/FitScriptOptionsBrowser.h"

#include <cxxtest/TestSuite.h>

#include <memory>

using namespace MantidQt::MantidWidgets;

class FitScriptOptionsBrowserTest : public CxxTest::TestSuite {

public:
  FitScriptOptionsBrowserTest() { Mantid::API::FrameworkManager::Instance(); }

  static FitScriptOptionsBrowserTest *createSuite() { return new FitScriptOptionsBrowserTest; }
  static void destroySuite(FitScriptOptionsBrowserTest *suite) { delete suite; }

  void setUp() override { m_fitOptionsBrowser = std::make_unique<FitScriptOptionsBrowser>(nullptr); }

  void tearDown() override { m_fitOptionsBrowser.reset(); }

  void test_that_the_FitOptionsBrowser_can_be_instantiated_many_times_without_instability() {
    /*
     * This test was created in response to fixing an unreliable Read Access
     * Violation when creating the BasicFitOptionsBrowser. This failure would
     * happen once every 100-200 attempts to instantiate this class.
     */
    for (auto i = 0u; i < 100u; ++i)
      m_fitOptionsBrowser = std::make_unique<FitScriptOptionsBrowser>(nullptr);
  }

  void test_that_the_BasicFitOptionsBrowser_is_instantiated_with_the_expected_default_properties() {
    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getFittingMode(), FittingMode::SEQUENTIAL);
    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Max Iterations"), "500");
    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Minimizer"), "Levenberg-Marquardt");
    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Evaluation Type"), "CentrePoint");
    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Cost Function"), "Least squares");
  }

  void test_that_setting_the_fitting_mode_to_sequential_will_then_return_the_sequential_fitting_mode() {
    m_fitOptionsBrowser->setFittingMode(FittingMode::SIMULTANEOUS);

    m_fitOptionsBrowser->setFittingMode(FittingMode::SEQUENTIAL);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getFittingMode(), FittingMode::SEQUENTIAL);
  }

  void test_that_setting_the_fitting_mode_to_simultaneous_will_then_return_the_simultaneous_fitting_mode() {
    m_fitOptionsBrowser->setFittingMode(FittingMode::SIMULTANEOUS);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getFittingMode(), FittingMode::SIMULTANEOUS);
  }

  void test_that_getProperty_will_throw_if_the_property_provided_does_not_exist() {
    TS_ASSERT_THROWS(m_fitOptionsBrowser->getProperty("Bad Property"), std::runtime_error const &);
  }

  void test_that_setProperty_will_set_the_max_iterations_as_expected() {
    auto const maxIterations = "300";

    m_fitOptionsBrowser->setProperty("Max Iterations", maxIterations);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Max Iterations"), maxIterations);
  }

  void test_that_setProperty_will_set_the_minimizer_as_expected() {
    auto const minimizer = "FABADA";

    m_fitOptionsBrowser->setProperty("Minimizer", minimizer);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Minimizer"), minimizer);
  }

  void test_that_setProperty_will_throw_if_attempting_to_set_a_minimizer_that_does_not_exist() {
    auto const minimizer = "Bad Minimizer";
    TS_ASSERT_THROWS(m_fitOptionsBrowser->setProperty("Minimizer", minimizer), std::invalid_argument const &);
  }

  void test_that_setProperty_will_set_the_cost_function_as_expected() {
    auto const costFunction = "Poisson";

    m_fitOptionsBrowser->setProperty("Cost Function", costFunction);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Cost Function"), costFunction);
  }

  void test_that_setProperty_will_throw_if_attempting_to_set_a_cost_function_that_does_not_exist() {
    auto const costFunction = "Bad Cost Function";
    TS_ASSERT_THROWS(m_fitOptionsBrowser->setProperty("Cost Function", costFunction), std::invalid_argument const &);
  }

  void test_that_setProperty_will_set_the_evaluation_type_as_expected() {
    auto const evaluationType = "Histogram";

    m_fitOptionsBrowser->setProperty("Evaluation Type", evaluationType);

    TS_ASSERT_EQUALS(m_fitOptionsBrowser->getProperty("Evaluation Type"), evaluationType);
  }

  void test_that_setProperty_will_throw_if_attempting_to_set_a_evaluation_type_that_does_not_exist() {
    auto const evaluationType = "Bad Evaluation Type";
    TS_ASSERT_THROWS(m_fitOptionsBrowser->setProperty("Evaluation Type", evaluationType),
                     std::invalid_argument const &);
  }

private:
  std::unique_ptr<FitScriptOptionsBrowser> m_fitOptionsBrowser;
};
