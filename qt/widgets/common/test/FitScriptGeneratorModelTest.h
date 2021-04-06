// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FitDomain.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorMockObjects.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <memory>
#include <vector>

using namespace MantidQt::MantidWidgets;
using namespace WorkspaceCreationHelper;

namespace {

Mantid::API::IFunction_sptr createIFunction(std::string const &functionString) {
  return Mantid::API::FunctionFactory::Instance().createInitialized(functionString);
}

Mantid::API::CompositeFunction_sptr toComposite(Mantid::API::IFunction_sptr function) {
  return std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(function);
}

Mantid::API::CompositeFunction_sptr createEmptyComposite() {
  return toComposite(createIFunction("name=CompositeFunction"));
}

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

MATCHER_P(VectorSize, expectedSize, "") { return arg.size() == expectedSize; }

GNU_DIAG_ON_SUGGEST_OVERRIDE

class FitScriptGeneratorModelTest : public CxxTest::TestSuite {

public:
  FitScriptGeneratorModelTest()
      : m_wsName("Name"), m_wsIndex(MantidQt::MantidWidgets::WorkspaceIndex(0)), m_workspace(create2DWorkspace(3, 3)),
        m_startX(m_workspace->x(m_wsIndex.value).front()), m_endX(m_workspace->x(m_wsIndex.value).back()) {
    Mantid::API::FrameworkManager::Instance();
  }

  static FitScriptGeneratorModelTest *createSuite() { return new FitScriptGeneratorModelTest; }

  static void destroySuite(FitScriptGeneratorModelTest *suite) { delete suite; }

  void setUp() override {
    m_flatBackground = createIFunction("name=FlatBackground");
    m_expDecay = createIFunction("name=ExpDecay");

    auto composite = createEmptyComposite();
    composite->addFunction(createIFunction("name=FlatBackground"));
    composite->addFunction(createIFunction("name=ExpDecay"));
    m_composite = composite;

    Mantid::API::AnalysisDataService::Instance().addOrReplace(m_wsName, m_workspace);

    m_model = std::make_unique<FitScriptGeneratorModel>();
    m_presenter = std::make_unique<MockFitScriptGeneratorPresenter>(m_model.get());
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void test_that_the_model_has_been_instantiated_with_the_expected_member_variables() {
    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 0);
    TS_ASSERT_EQUALS(m_model->getGlobalParameters().size(), 0);
    TS_ASSERT_EQUALS(m_model->getFittingMode(), FittingMode::SEQUENTIAL);
  }

  void test_that_addWorkspaceDomain_throws_nothing_when_a_domain_is_added_successfully() {
    TS_ASSERT_THROWS_NOTHING(m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX));
    TS_ASSERT(m_model->hasWorkspaceDomain(m_wsName, m_wsIndex));
    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex), nullptr);
  }

  void test_that_addWorkspaceDomain_throws_when_a_domain_already_exists() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    TS_ASSERT_THROWS(m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX), std::invalid_argument const &);
  }

  void test_that_removeWorkspaceDomain_will_not_throw_if_it_does_not_have_the_specified_domain() {
    TS_ASSERT_THROWS_NOTHING(m_model->removeWorkspaceDomain(m_wsName, m_wsIndex));
  }

  void test_that_removeWorkspaceDomain_will_remove_the_specified_domain() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addWorkspaceDomain("Name2", m_wsIndex, m_startX, m_endX);
    m_model->removeWorkspaceDomain(m_wsName, m_wsIndex);

    TS_ASSERT(!m_model->hasWorkspaceDomain(m_wsName, m_wsIndex));
    TS_ASSERT(m_model->hasWorkspaceDomain("Name2", m_wsIndex));
  }

  void test_that_removeWorkspaceDomain_will_clear_the_global_ties_that_have_expired() {
    setup_simultaneous_fit_with_global_tie();

    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 1);
    m_model->removeWorkspaceDomain(m_wsName, m_wsIndex);
    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 0);
  }

  void test_that_hasWorkspaceDomain_returns_false_if_a_workspace_domain_does_not_exist() {
    TS_ASSERT(!m_model->hasWorkspaceDomain(m_wsName, m_wsIndex));
  }

  void test_that_hasWorkspaceDomain_returns_true_if_a_workspace_domain_exists() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    TS_ASSERT(m_model->hasWorkspaceDomain(m_wsName, m_wsIndex));
  }

  void test_that_updateStartX_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->updateStartX(m_wsName, m_wsIndex, 1.0), std::invalid_argument const &);
  }

  void test_that_updateStartX_will_return_false_if_the_value_provided_is_out_of_range() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    TS_ASSERT(!m_model->updateStartX(m_wsName, m_wsIndex, -1.0));
  }

  void test_that_updateStartX_will_return_false_if_the_value_provided_is_larger_than_the_endX() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);

    TS_ASSERT(m_model->updateEndX(m_wsName, m_wsIndex, 2.0));
    TS_ASSERT(!m_model->updateStartX(m_wsName, m_wsIndex, 2.5));
  }

  void test_that_updateStartX_will_return_true_if_the_value_provided_is_valid() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    TS_ASSERT(m_model->updateStartX(m_wsName, m_wsIndex, 1.0));
  }

  void test_that_updateEndX_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->updateEndX(m_wsName, m_wsIndex, 1.0), std::invalid_argument const &);
  }

  void test_that_updateEndX_will_return_false_if_the_value_provided_is_out_of_range() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    TS_ASSERT(!m_model->updateEndX(m_wsName, m_wsIndex, 5.0));
  }

  void test_that_updateEndX_will_return_false_if_the_value_provided_is_smaller_than_the_startX() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);

    TS_ASSERT(m_model->updateStartX(m_wsName, m_wsIndex, 2.0));
    TS_ASSERT(!m_model->updateEndX(m_wsName, m_wsIndex, 1.0));
  }

  void test_that_updateEndX_will_return_true_if_the_value_provided_is_valid() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    TS_ASSERT(m_model->updateEndX(m_wsName, m_wsIndex, 2.0));
  }

  void test_that_addFunction_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->addFunction(m_wsName, m_wsIndex, m_flatBackground->asString()),
                     std::invalid_argument const &);
  }

  void test_that_addFunction_will_add_the_function_to_the_correct_domain() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addFunction(m_wsName, m_wsIndex, m_flatBackground->asString());

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->asString(), m_flatBackground->asString());
  }

  void test_that_addFunction_will_dynamically_adjust_the_global_ties_that_have_changed_function_index() {
    setup_simultaneous_fit_with_global_tie();

    auto const globalTiesBefore = m_model->getGlobalTies();
    TS_ASSERT_EQUALS(globalTiesBefore.size(), 1);
    TS_ASSERT_EQUALS(globalTiesBefore[0].m_parameter, "f0.A0");
    TS_ASSERT_EQUALS(globalTiesBefore[0].m_tie, "f1.A0");

    // Add a function (thereby creating a composite)
    m_model->addFunction(m_wsName, m_wsIndex, m_expDecay->asString());
    m_model->addFunction("Name2", m_wsIndex, m_expDecay->asString());

    // The global tie has shifted up one index because it is now a composite.
    auto const globalTiesAfter = m_model->getGlobalTies();
    TS_ASSERT_EQUALS(globalTiesAfter.size(), 1);
    TS_ASSERT_EQUALS(globalTiesAfter[0].m_parameter, "f0.f0.A0");
    TS_ASSERT_EQUALS(globalTiesAfter[0].m_tie, "f1.f0.A0");
  }

  void test_that_addFunction_will_throw_if_provided_a_composite_function() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addFunction(m_wsName, m_wsIndex, m_flatBackground->asString());

    m_model->addFunction(m_wsName, m_wsIndex, m_composite->asString());

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->asString(), m_flatBackground->asString());
  }

  void test_that_setFunction_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->setFunction(m_wsName, m_wsIndex, m_flatBackground->asString()),
                     std::invalid_argument const &);
  }

  void test_that_setFunction_will_set_the_function_in_the_correct_domain() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addWorkspaceDomain("Name2", m_wsIndex, m_startX, m_endX);
    m_model->addFunction(m_wsName, m_wsIndex, m_expDecay->asString());

    m_model->setFunction(m_wsName, m_wsIndex, m_flatBackground->asString());

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->asString(), m_flatBackground->asString());
    TS_ASSERT_EQUALS(m_model->getFunction("Name2", m_wsIndex), nullptr);
  }

  void test_that_setFunction_will_clear_the_global_ties_that_have_expired() {
    setup_simultaneous_fit_with_global_tie();

    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 1);
    m_model->setFunction(m_wsName, m_wsIndex, m_expDecay->asString());
    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 0);
  }

  void test_that_removeFunction_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->removeFunction(m_wsName, m_wsIndex, m_flatBackground->asString()),
                     std::invalid_argument const &);
  }

  void test_that_removeFunction_will_remove_the_function_in_the_correct_domain() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addFunction(m_wsName, m_wsIndex, m_flatBackground->asString());
    m_model->addFunction(m_wsName, m_wsIndex, m_expDecay->asString());

    m_model->removeFunction(m_wsName, m_wsIndex, m_flatBackground->asString());

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->asString(), m_expDecay->asString());
  }

  void test_that_removeFunction_will_clear_the_global_ties_that_have_expired() {
    setup_simultaneous_fit_with_global_tie();

    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 1);
    m_model->removeFunction(m_wsName, m_wsIndex, m_flatBackground->asString());
    m_model->removeFunction("Name2", m_wsIndex, m_flatBackground->asString());
    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 0);
  }

  void test_that_removeFunction_will_dynamically_adjust_the_global_ties_that_have_changed_function_index() {
    setup_simultaneous_fit_with_no_ties();

    // Add a function to create a composite
    m_model->addFunction(m_wsName, m_wsIndex, m_expDecay->asString());
    m_model->addFunction("Name2", m_wsIndex, m_expDecay->asString());
    m_model->updateParameterTie("Name2", m_wsIndex, "f1.f1.Height", "f0.f1.Height");

    auto const globalTiesBefore = m_model->getGlobalTies();
    TS_ASSERT_EQUALS(globalTiesBefore.size(), 1);
    TS_ASSERT_EQUALS(globalTiesBefore[0].m_parameter, "f1.f1.Height");
    TS_ASSERT_EQUALS(globalTiesBefore[0].m_tie, "f0.f1.Height");

    // Remove the flat background (thereby elimating the need for a composite)
    m_model->removeFunction(m_wsName, m_wsIndex, m_flatBackground->asString());
    m_model->removeFunction("Name2", m_wsIndex, m_flatBackground->asString());

    // The global tie has shifted down one index because the composite is gone.
    auto const globalTiesAfter = m_model->getGlobalTies();
    TS_ASSERT_EQUALS(globalTiesAfter.size(), 1);
    TS_ASSERT_EQUALS(globalTiesAfter[0].m_parameter, "f1.Height");
    TS_ASSERT_EQUALS(globalTiesAfter[0].m_tie, "f0.Height");
  }

  void test_that_getFunction_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->getFunction(m_wsName, m_wsIndex), std::invalid_argument const &);
  }

  void test_that_getEquivalentFunctionIndexForDomain_will_throw_if_the_domain_specified_does_not_exist() {
    setup_simultaneous_fit_with_global_tie();
    TS_ASSERT_THROWS(m_model->getEquivalentFunctionIndexForDomain("BadName", m_wsIndex, "f0.f0."),
                     std::invalid_argument const &);
  }

  void test_that_getEquivalentFunctionIndexForDomain_will_return_the_correct_function_index_for_simultaneous_mode() {
    setup_simultaneous_fit_with_global_tie();

    auto const equivalentIndex = m_model->getEquivalentFunctionIndexForDomain("Name2", m_wsIndex, "f0.f0.");
    TS_ASSERT_EQUALS(equivalentIndex, "f1.f0.");
  }

  void test_that_getEquivalentFunctionIndexForDomain_just_returns_the_index_if_it_is_an_empty_string() {
    setup_simultaneous_fit_with_global_tie();

    auto const equivalentIndex = m_model->getEquivalentFunctionIndexForDomain("Name2", m_wsIndex, "");
    TS_ASSERT_EQUALS(equivalentIndex, "");
  }

  void test_that_getEquivalentFunctionIndexForDomain_just_returns_the_index_if_it_is_in_sequential_mode() {
    auto const equivalentIndex = m_model->getEquivalentFunctionIndexForDomain(m_wsName, m_wsIndex, "f0.");
    TS_ASSERT_EQUALS(equivalentIndex, "f0.");
  }

  void test_that_getEquivalentParameterTieForDomain_will_throw_if_the_domain_specified_does_not_exist() {
    setup_simultaneous_fit_with_global_tie();
    TS_ASSERT_THROWS(m_model->getEquivalentParameterTieForDomain("BadName", m_wsIndex, "f0.f0.A0", "f0.f1.Height"),
                     std::invalid_argument const &);
  }

  void test_that_getEquivalentParameterTieForDomain_will_just_return_the_string_if_its_a_number_or_empty() {
    m_model->setFittingMode(FittingMode::SIMULTANEOUS);

    TS_ASSERT_EQUALS(m_model->getEquivalentParameterTieForDomain(m_wsName, m_wsIndex, "f0.f0.A0", "0"), "0");
    TS_ASSERT_EQUALS(m_model->getEquivalentParameterTieForDomain(m_wsName, m_wsIndex, "f0.f0.A0", "-1.0"), "-1.0");
    TS_ASSERT_EQUALS(m_model->getEquivalentParameterTieForDomain(m_wsName, m_wsIndex, "f0.f0.A0", ""), "");
    TS_ASSERT_EQUALS(m_model->getEquivalentParameterTieForDomain(m_wsName, m_wsIndex, "f0.f0.A0", "bad.parameter"),
                     "bad.parameter");
  }

  void test_that_getEquivalentParameterTieForDomain_will_just_return_the_original_tie_if_its_sequential_mode() {
    setup_simultaneous_fit_with_no_ties();
    TS_ASSERT_EQUALS(m_model->getEquivalentParameterTieForDomain(m_wsName, m_wsIndex, "f0.A0", "f1.Height"),
                     "f1.Height");
  }

  void
  test_that_getEquivalentParameterTieForDomain_will_return_a_tie_in_the_same_domain_if_the_parameter_domain_is_equal_to_the_tie_domain() {
    setup_simultaneous_fit_with_no_ties();
    TS_ASSERT_EQUALS(m_model->getEquivalentParameterTieForDomain(m_wsName, m_wsIndex, "f0.f0.A0", "f0.f1.Height"),
                     "f0.f1.Height");
  }

  void
  test_that_getEquivalentParameterTieForDomain_will_return_the_correct_tie_if_the_parameter_domain_and_tie_domain_are_different() {
    setup_simultaneous_fit_with_no_ties();
    TS_ASSERT_EQUALS(m_model->getEquivalentParameterTieForDomain(m_wsName, m_wsIndex, "f1.f0.A0", "f0.f1.Height"),
                     "f0.f1.Height");
  }

  void test_that_updateParameterValue_will_not_update_a_parameter_value_if_it_has_a_global_tie() {
    setup_simultaneous_fit_with_global_tie();

    m_model->updateParameterValue(m_wsName, m_wsIndex, "f0.A0", 2.0);

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getParameter("A0"), 0.0);
  }

  void test_that_updateParameterValue_will_update_a_parameter_value_if_it_does_not_have_a_global_tie() {
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterValue(m_wsName, m_wsIndex, "f0.A0", 2.0);

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getParameter("A0"), 2.0);
    TS_ASSERT_EQUALS(m_model->getFunction("Name2", m_wsIndex)->getParameter("A0"), 0.0);
  }

  void test_that_updateParameterValue_will_update_all_parameter_values_globally_tied_to_the_specified_parameter() {
    setup_simultaneous_fit_with_global_tie();

    m_model->updateParameterValue("Name2", m_wsIndex, "f1.A0", 2.0);

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getParameter("A0"), 2.0);
    TS_ASSERT_EQUALS(m_model->getFunction("Name2", m_wsIndex)->getParameter("A0"), 2.0);
  }

  void test_that_updateAttributeValue_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->updateAttributeValue(m_wsName, m_wsIndex, "A0", IFunction::Attribute(true)),
                     std::invalid_argument const &);
  }

  void test_that_updateAttributeValue_will_not_throw_if_the_attribute_specified_does_not_exist() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addFunction(m_wsName, m_wsIndex, m_flatBackground->asString());

    TS_ASSERT_THROWS_NOTHING(
        m_model->updateAttributeValue(m_wsName, m_wsIndex, "FakeAttribute", IFunction::Attribute(true)));
  }

  void test_that_updateAttributeValue_will_update_an_attribute_as_expected_when_it_exists() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);

    m_model->addFunction(m_wsName, m_wsIndex, m_flatBackground->asString());
    m_model->addFunction(m_wsName, m_wsIndex, m_expDecay->asString());

    TS_ASSERT(!m_model->getFunction(m_wsName, m_wsIndex)->getAttribute("NumDeriv").asBool());
    m_model->updateAttributeValue(m_wsName, m_wsIndex, "NumDeriv", IFunction::Attribute(true));
    TS_ASSERT(m_model->getFunction(m_wsName, m_wsIndex)->getAttribute("NumDeriv").asBool());
  }

  void test_that_updateParameterTie_will_not_throw_if_the_tie_is_invalid() {
    setup_sequential_fit_with_no_ties();
    TS_ASSERT_THROWS_NOTHING(m_model->updateParameterTie(m_wsName, m_wsIndex, "A0", "BadParameter"));
  }

  void test_that_updateParameterTie_will_throw_if_the_parameter_is_global() {
    setup_simultaneous_fit_with_global_parameter();
    TS_ASSERT_THROWS(m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "0"), std::invalid_argument const &);
  }

  void test_that_updateParameterTie_will_add_a_local_tie_when_in_sequential_mode() {
    setup_sequential_fit_with_no_ties();

    m_model->updateParameterTie(m_wsName, m_wsIndex, "A0", "0");

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getParameterStatus(0),
                     IFunction::ParameterStatus::Fixed);
  }

  void
  test_that_updateParameterTie_will_add_a_local_tie_when_in_simultaneous_mode_but_the_tie_has_the_same_domain_as_the_parameter() {
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "0");

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getParameterStatus(0),
                     IFunction::ParameterStatus::Fixed);
  }

  void
  test_that_updateParameterTie_will_add_a_global_tie_when_in_simultaneous_mode_and_the_tie_has_a_different_domain_to_the_parameter() {
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "f1.A0");

    auto const globalTie = m_model->getGlobalTies()[0];
    TS_ASSERT_EQUALS(globalTie.m_parameter, "f0.A0");
    TS_ASSERT_EQUALS(globalTie.m_tie, "f1.A0");
  }

  void test_that_updateParameterTie_will_remove_a_local_tie_when_the_tie_is_empty() {
    setup_sequential_fit_with_no_ties();

    m_model->updateParameterTie(m_wsName, m_wsIndex, "A0", "0");
    m_model->updateParameterTie(m_wsName, m_wsIndex, "A0", "");

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getParameterStatus(0),
                     IFunction::ParameterStatus::Active);
  }

  void test_that_updateParameterTie_will_remove_a_global_tie_when_in_simultaneous_mode_and_the_tie_is_empty() {
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "f1.A0");
    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "");

    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 0);
  }

  void test_that_updateParameterConstraint_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->updateParameterConstraint(m_wsName, m_wsIndex, "f0.", "0<A0<1"),
                     std::invalid_argument const &);
  }

  void test_that_updateParameterConstraint_will_add_a_constraint_as_expected_for_sequential_mode() {
    std::string const constraint("0<A0<1");
    setup_sequential_fit_with_no_ties();

    m_model->updateParameterConstraint(m_wsName, m_wsIndex, "", constraint);

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getConstraint(0)->asString(), constraint);
  }

  void test_that_updateParameterConstraint_will_add_a_constraint_as_expected_for_simultaneous_mode() {
    std::string const constraint("0<A0<1");
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterConstraint(m_wsName, m_wsIndex, "f0.", constraint);

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getConstraint(0)->asString(), constraint);
  }

  void test_that_removeParameterConstraint_will_throw_if_the_domain_specified_does_not_exist() {
    TS_ASSERT_THROWS(m_model->removeParameterConstraint(m_wsName, m_wsIndex, "f0.A0"), std::invalid_argument const &);
  }

  void test_that_removeParameterConstraint_will_add_a_constraint_as_expected_for_sequential_mode() {
    std::string const constraint("0<A0<1");
    setup_sequential_fit_with_no_ties();

    m_model->updateParameterConstraint(m_wsName, m_wsIndex, "", constraint);
    m_model->removeParameterConstraint(m_wsName, m_wsIndex, "A0");

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getConstraint(0), nullptr);
  }

  void test_that_removeParameterConstraint_will_add_a_constraint_as_expected_for_simultaneous_mode() {
    std::string const constraint("0<A0<1");
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterConstraint(m_wsName, m_wsIndex, "f0.", constraint);
    m_model->removeParameterConstraint(m_wsName, m_wsIndex, "f0.A0");

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getConstraint(0), nullptr);
  }

  void test_that_setGlobalParameters_will_throw_if_the_global_parameter_provided_is_not_in_all_domains() {
    m_model->setFittingMode(FittingMode::SIMULTANEOUS);

    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addWorkspaceDomain("Name2", m_wsIndex, m_startX, m_endX);

    m_model->setFunction(m_wsName, m_wsIndex, m_flatBackground->asString());

    TS_ASSERT_THROWS(m_model->setGlobalParameters(std::vector<std::string>{"A0"}), std::invalid_argument const &);
  }

  void test_that_setGlobalParameters_will_throw_if_the_global_parameter_provided_has_a_local_tie() {
    m_model->setFittingMode(FittingMode::SIMULTANEOUS);

    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addWorkspaceDomain("Name2", m_wsIndex, m_startX, m_endX);

    m_model->setFunction(m_wsName, m_wsIndex, m_expDecay->asString());
    m_model->setFunction("Name2", m_wsIndex, m_expDecay->asString());

    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.Height", "f0.Lifetime");

    TS_ASSERT_THROWS(m_model->setGlobalParameters(std::vector<std::string>{"Height"}), std::invalid_argument const &);
  }

  void test_that_setGlobalParameters_will_throw_if_the_global_parameter_provided_has_a_global_tie() {
    setup_simultaneous_fit_with_global_tie();
    TS_ASSERT_THROWS(m_model->setGlobalParameters(std::vector<std::string>{"A0"}), std::invalid_argument const &);
  }

  void test_that_setGlobalParameters_will_set_the_global_parameters_as_expected_when_they_are_valid() {
    setup_simultaneous_fit_with_no_ties();

    m_model->setGlobalParameters(std::vector<std::string>{"A0"});

    auto const globalParameter = m_model->getGlobalParameters()[0];
    TS_ASSERT_EQUALS(globalParameter.m_parameter, "A0");
  }

  void test_that_setFittingMode_will_throw_if_given_an_invalid_fitting_mode() {
    TS_ASSERT_THROWS(m_model->setFittingMode(FittingMode::SEQUENTIAL_AND_SIMULTANEOUS), std::invalid_argument const &);
  }

  void test_that_setFittingMode_will_clear_the_global_ties_and_tell_the_presenter() {
    setup_simultaneous_fit_with_global_tie();

    EXPECT_CALL(*m_presenter, setGlobalTies(VectorSize(0u))).Times(1);
    m_model->setFittingMode(FittingMode::SEQUENTIAL);

    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "f1.A0");

    EXPECT_CALL(*m_presenter, setGlobalTies(VectorSize(0u))).Times(1);
    m_model->setFittingMode(FittingMode::SIMULTANEOUS);
  }

  void test_that_setFittingMode_will_clear_the_global_parameters_and_tell_the_presenter() {
    setup_simultaneous_fit_with_global_parameter();

    EXPECT_CALL(*m_presenter, setGlobalParameters(VectorSize(0u))).Times(1);
    m_model->setFittingMode(FittingMode::SEQUENTIAL);

    m_model->setGlobalParameters(std::vector<std::string>{"A0"});

    EXPECT_CALL(*m_presenter, setGlobalParameters(VectorSize(0u))).Times(1);
    m_model->setFittingMode(FittingMode::SIMULTANEOUS);
  }

  void
  test_that_setting_the_value_of_a_parameter_to_a_value_outside_of_the_constraints_of_another_parameter_globally_tied_to_it_will_remove_the_tie() {
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterConstraint(m_wsName, m_wsIndex, "f0.", "0.0<A0<1.0");
    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "f1.A0");

    m_model->updateParameterValue("Name2", m_wsIndex, "f1.A0", 2.0);

    auto const function = m_model->getFunction(m_wsName, m_wsIndex);
    TS_ASSERT_EQUALS(function->getParameter("A0"), 0.0);
    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 0);
  }

  void
  test_that_attempting_to_globally_tie_a_parameter_to_another_parameter_with_a_value_outside_the_allowed_constraints_will_not_perform_the_tie() {
    setup_simultaneous_fit_with_no_ties();

    m_model->updateParameterConstraint(m_wsName, m_wsIndex, "f0.", "0.0<A0<1.0");
    m_model->updateParameterValue("Name2", m_wsIndex, "f1.A0", 2.0);

    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "f1.A0");

    auto const function = m_model->getFunction(m_wsName, m_wsIndex);
    TS_ASSERT_EQUALS(function->getParameter("A0"), 0.0);
    TS_ASSERT_EQUALS(m_model->getGlobalTies().size(), 0);
  }

  void test_that_all_previously_tied_parameters_have_the_same_value_when_a_global_tie_is_removed() {
    m_model->setFittingMode(FittingMode::SIMULTANEOUS);

    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addWorkspaceDomain("Name2", m_wsIndex, m_startX, m_endX);

    m_model->setFunction(m_wsName, m_wsIndex, m_expDecay->asString());
    m_model->setFunction("Name2", m_wsIndex, m_expDecay->asString());

    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.Height", "f1.Lifetime");
    m_model->updateParameterTie("Name2", m_wsIndex, "f1.Height", "f1.Lifetime");

    m_model->updateParameterValue("Name2", m_wsIndex, "f1.Lifetime", 2.0);

    // Remove the ties
    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.Height", "");
    m_model->updateParameterTie("Name2", m_wsIndex, "f1.Height", "");

    TS_ASSERT_EQUALS(m_model->getFunction(m_wsName, m_wsIndex)->getParameter("Height"), 2.0);
    TS_ASSERT_EQUALS(m_model->getFunction("Name2", m_wsIndex)->getParameter("Height"), 2.0);
  }

private:
  void setup_model_data() {
    m_model->addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX);
    m_model->addWorkspaceDomain("Name2", m_wsIndex, m_startX, m_endX);

    m_model->setFunction(m_wsName, m_wsIndex, m_flatBackground->asString());
    m_model->setFunction("Name2", m_wsIndex, m_flatBackground->asString());
  }

  void setup_sequential_fit_with_no_ties() {
    m_model->setFittingMode(FittingMode::SEQUENTIAL);
    setup_model_data();
  }

  void setup_simultaneous_fit_with_no_ties() {
    m_model->setFittingMode(FittingMode::SIMULTANEOUS);
    setup_model_data();
  }

  void setup_simultaneous_fit_with_global_tie() {
    setup_simultaneous_fit_with_no_ties();
    m_model->updateParameterTie(m_wsName, m_wsIndex, "f0.A0", "f1.A0");
  }

  void setup_simultaneous_fit_with_global_parameter() {
    setup_simultaneous_fit_with_no_ties();
    m_model->setGlobalParameters(std::vector<std::string>{"A0"});
  }

  std::string m_wsName;
  MantidQt::MantidWidgets::WorkspaceIndex m_wsIndex;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  double m_startX;
  double m_endX;
  Mantid::API::IFunction_sptr m_flatBackground;
  Mantid::API::IFunction_sptr m_expDecay;
  Mantid::API::IFunction_sptr m_composite;

  std::unique_ptr<FitScriptGeneratorModel> m_model;
  std::unique_ptr<MockFitScriptGeneratorPresenter> m_presenter;
};
