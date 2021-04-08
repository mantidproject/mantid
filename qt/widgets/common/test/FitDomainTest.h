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
#include "MantidQtWidgets/Common/FitDomain.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

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

class FitDomainTest : public CxxTest::TestSuite {

public:
  FitDomainTest()
      : m_wsName("Name"), m_wsIndex(WorkspaceIndex(0)), m_workspace(create2DWorkspace(3, 3)),
        m_startX(m_workspace->x(m_wsIndex.value).front()), m_endX(m_workspace->x(m_wsIndex.value).back()) {
    Mantid::API::FrameworkManager::Instance();
  }

  static FitDomainTest *createSuite() { return new FitDomainTest; }

  static void destroySuite(FitDomainTest *suite) { delete suite; }

  void setUp() override {
    m_flatBackground = createIFunction("name=FlatBackground");
    m_expDecay = createIFunction("name=ExpDecay");

    auto composite = createEmptyComposite();
    composite->addFunction(m_flatBackground->clone());
    composite->addFunction(m_expDecay->clone());
    m_composite = composite;

    Mantid::API::AnalysisDataService::Instance().addOrReplace(m_wsName, m_workspace);
    m_fitDomain = std::make_unique<FitDomain>(m_wsName, m_wsIndex, m_startX, m_endX);
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void test_that_the_FitDomain_has_been_instantiated_with_the_correct_data() {
    TS_ASSERT_EQUALS(m_fitDomain->workspaceName(), m_wsName);
    TS_ASSERT_EQUALS(m_fitDomain->workspaceIndex(), m_wsIndex);
    TS_ASSERT_EQUALS(m_fitDomain->startX(), m_startX);
    TS_ASSERT_EQUALS(m_fitDomain->endX(), m_endX);
    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy(), nullptr);
  }

  void test_that_setStartX_will_not_set_the_startX_if_the_value_is_out_of_range() {
    TS_ASSERT(!m_fitDomain->setStartX(-1.0));
    TS_ASSERT_EQUALS(m_fitDomain->startX(), m_startX);
  }

  void test_that_setStartX_will_not_set_the_startX_if_the_value_is_larger_than_the_endX() {
    TS_ASSERT(m_fitDomain->setEndX(2.0));

    TS_ASSERT(!m_fitDomain->setStartX(2.5));
    TS_ASSERT_EQUALS(m_fitDomain->startX(), m_startX);
  }

  void test_that_setStartX_will_set_the_startX_if_the_value_is_valid() {
    double startX(2.0);

    TS_ASSERT(m_fitDomain->setStartX(startX));
    TS_ASSERT_EQUALS(m_fitDomain->startX(), startX);
  }

  void test_that_setEndX_will_not_set_the_endX_if_the_value_is_out_of_range() {
    TS_ASSERT(!m_fitDomain->setEndX(4.0));
    TS_ASSERT_EQUALS(m_fitDomain->endX(), m_endX);
  }

  void test_that_setEndX_will_not_set_the_endX_if_the_value_is_smaller_than_the_startX() {
    TS_ASSERT(m_fitDomain->setStartX(2.0));

    TS_ASSERT(!m_fitDomain->setEndX(1.0));
    TS_ASSERT_EQUALS(m_fitDomain->endX(), m_endX);
  }

  void test_that_setEndX_will_set_the_endX_if_the_value_is_valid() {
    double endX(2.0);

    TS_ASSERT(m_fitDomain->setEndX(endX));
    TS_ASSERT_EQUALS(m_fitDomain->endX(), endX);
  }

  void test_that_setFunction_will_set_the_function_as_expected() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->asString(), m_flatBackground->asString());
  }

  void test_that_getFunction_returns_a_clone_of_the_function() {
    m_fitDomain->setFunction(m_flatBackground);

    auto modifiedFunction = m_fitDomain->getFunctionCopy();
    modifiedFunction->setParameter("A0", 5.0);

    TS_ASSERT_DIFFERS(m_fitDomain->getFunctionCopy()->asString(), modifiedFunction->asString());
  }

  void test_that_removeFunction_will_remove_the_function_with_the_given_name_from_a_non_composite_function() {
    m_fitDomain->setFunction(m_flatBackground);

    m_fitDomain->removeFunction(m_flatBackground->asString());

    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy(), nullptr);
  }

  void test_that_removeFunction_will_remove_the_function_with_the_given_name_from_a_composite_function() {
    m_fitDomain->setFunction(m_composite);

    m_fitDomain->removeFunction(m_flatBackground->asString());

    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->asString(), m_expDecay->asString());
  }

  void test_that_removeFunction_will_not_throw_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->removeFunction(m_flatBackground->asString()));
  }

  void test_that_removeFunction_will_not_remove_a_function_if_the_function_specified_does_not_exist() {
    m_fitDomain->setFunction(m_flatBackground);

    TS_ASSERT_THROWS_NOTHING(m_fitDomain->removeFunction(m_expDecay->asString()));
    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->asString(), m_flatBackground->asString());
  }

  void test_that_addFunction_will_add_a_function_correctly_for_a_single_ifunction() {
    m_fitDomain->addFunction(m_flatBackground);
    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->asString(), m_flatBackground->asString());
  }

  void test_that_addFunction_will_add_a_second_function_correctly_to_create_a_composite() {
    m_fitDomain->addFunction(m_flatBackground);
    m_fitDomain->addFunction(m_expDecay);

    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->asString(), m_composite->asString());
  }

  void test_that_addFunction_will_not_add_a_function_if_attempting_to_create_a_nested_composite_function() {
    m_fitDomain->addFunction(m_flatBackground);

    m_fitDomain->addFunction(m_composite);

    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->asString(), m_flatBackground->asString());
  }

  void test_that_getParameterValue_will_get_the_parameter_value_if_it_exists() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_EQUALS(m_fitDomain->getParameterValue("A0"), 0.0);
  }

  void test_that_getParameterValue_will_throw_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT_THROWS(m_fitDomain->getParameterValue("A0"), std::runtime_error const &);
  }

  void test_that_getParameterValue_will_throw_if_the_parameter_does_not_exist() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_THROWS(m_fitDomain->getParameterValue("Height"), std::runtime_error const &);
  }

  void test_that_setParameterValue_will_not_throw_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->setParameterValue("A0", 2.0));
  }

  void test_that_setParameterValue_will_not_throw_if_the_stored_function_does_not_have_the_specified_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->setParameterValue("Height", 2.0));
  }

  void test_that_setParameterValue_will_not_set_the_parameters_value_if_the_new_value_is_outside_the_constraints() {
    m_fitDomain->setFunction(m_flatBackground);
    m_fitDomain->updateParameterConstraint("", "A0", "0<A0<2");

    m_fitDomain->setParameterValue("A0", 3.0);

    TS_ASSERT_EQUALS(m_fitDomain->getParameterValue("A0"), 0.0);
  }

  void test_that_setParameterValue_will_set_the_parameter_value_ok_if_it_is_valid() {
    m_fitDomain->setFunction(m_flatBackground);

    m_fitDomain->setParameterValue("A0", 3.0);

    TS_ASSERT_EQUALS(m_fitDomain->getParameterValue("A0"), 3.0);
  }

  void test_that_getAttributeValue_will_get_the_attribute_value_if_it_exists() {
    m_fitDomain->setFunction(m_composite);
    TS_ASSERT(!m_fitDomain->getAttributeValue("NumDeriv").asBool());
  }

  void test_that_getAttributeValue_will_throw_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT_THROWS(m_fitDomain->getAttributeValue("A0"), std::runtime_error const &);
  }

  void test_that_getAttributeValue_will_throw_if_the_attribute_does_not_exist() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_THROWS(m_fitDomain->getAttributeValue("Height"), std::runtime_error const &);
  }

  void test_that_setAttributeValue_will_not_throw_if_the_stored_function_is_a_nullptr() {
    Mantid::API::IFunction::Attribute value(true);
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->setAttributeValue("NumDeriv", value));
  }

  void test_that_setAttributeValue_will_not_throw_if_the_stored_function_does_not_have_the_specified_attribute() {
    Mantid::API::IFunction::Attribute value(true);
    m_fitDomain->setFunction(m_flatBackground);

    TS_ASSERT_THROWS_NOTHING(m_fitDomain->setAttributeValue("NumDeriv", value));
  }

  void test_that_setAttributeValue_will_set_the_attribute_value_ok_if_it_is_valid() {
    Mantid::API::IFunction::Attribute value(true);
    m_fitDomain->setFunction(m_composite);

    m_fitDomain->setAttributeValue("NumDeriv", value);

    TS_ASSERT(m_fitDomain->getAttributeValue("NumDeriv").asBool());
  }

  void test_that_hasParameter_returns_false_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT(!m_fitDomain->hasParameter("A0"));
  }

  void test_that_hasParameter_returns_false_if_the_function_does_not_have_a_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT(!m_fitDomain->hasParameter("Height"));
  }

  void test_that_hasParameter_returns_false_if_the_function_does_have_a_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT(m_fitDomain->hasParameter("A0"));
  }

  void test_that_isParameterActive_returns_false_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT(!m_fitDomain->isParameterActive("A0"));
  }

  void test_that_isParameterActive_returns_false_if_the_function_does_not_have_the_specified_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT(!m_fitDomain->isParameterActive("Height"));
  }

  void test_that_isParameterActive_returns_false_if_a_parameter_is_tied() {
    m_fitDomain->setFunction(m_composite);
    TS_ASSERT(m_fitDomain->updateParameterTie("f0.A0", "f1.Height"));

    TS_ASSERT(!m_fitDomain->isParameterActive("f0.A0"));
    TS_ASSERT(m_fitDomain->isParameterActive("f1.Height"));
  }

  void test_that_isParameterActive_returns_true_if_a_parameter_is_constrained() {
    m_fitDomain->setFunction(m_flatBackground);
    m_fitDomain->updateParameterConstraint("", "A0", "0<A0<2");

    TS_ASSERT(m_fitDomain->isParameterActive("A0"));
  }

  void test_that_isParameterActive_returns_true_if_a_parameter_is_active() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT(m_fitDomain->isParameterActive("A0"));
  }

  void test_that_updateParameterTie_returns_true_by_default_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT(m_fitDomain->updateParameterTie("f0.A0", "f1.Height"));
  }

  void test_that_updateParameterTie_returns_true_by_default_if_the_stored_function_does_not_have_a_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT(m_fitDomain->updateParameterTie("f0.A0", "f1.Height"));
  }

  void test_that_updateParameterTie_will_give_a_parameter_a_tie_if_both_are_valid() {
    m_fitDomain->setFunction(m_composite);

    TS_ASSERT(m_fitDomain->updateParameterTie("f0.A0", "f1.Height"));
    TS_ASSERT(!m_fitDomain->isParameterActive("f0.A0"));
  }

  void test_that_updateParameterTie_will_not_throw_and_return_false_if_a_tie_is_invalid() {
    m_fitDomain->setFunction(m_composite);

    TS_ASSERT(!m_fitDomain->updateParameterTie("f0.A0", "f1.f0.BadData"));
    TS_ASSERT(m_fitDomain->isParameterActive("f0.A0"));
  }

  void test_that_updateParameterTie_will_clear_all_ties_if_the_provided_tie_is_a_blank_string() {
    m_fitDomain->setFunction(m_composite);

    TS_ASSERT(m_fitDomain->updateParameterTie("f0.A0", "f1.Height"));
    TS_ASSERT(!m_fitDomain->isParameterActive("f0.A0"));

    TS_ASSERT(m_fitDomain->updateParameterTie("f0.A0", ""));
    TS_ASSERT(m_fitDomain->isParameterActive("f0.A0"));
  }

  void test_that_clearParameterTie_does_not_throw_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->clearParameterTie("f0.A0"));
  }

  void test_that_clearParameterTie_does_not_throw_if_the_stored_function_does_not_have_a_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->clearParameterTie("f0.A0"));
  }

  void test_that_clearParameterTie_will_clear_the_tie_on_a_parameter() {
    m_fitDomain->setFunction(m_composite);

    TS_ASSERT(m_fitDomain->updateParameterTie("f0.A0", "f1.Height"));
    TS_ASSERT(!m_fitDomain->isParameterActive("f0.A0"));

    m_fitDomain->clearParameterTie("f0.A0");
    TS_ASSERT(m_fitDomain->isParameterActive("f0.A0"));
  }

  void test_that_updateParameterConstraint_will_not_throw_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->updateParameterConstraint("", "A0", "0<A0<2"));
  }

  void test_that_updateParameterConstraint_will_not_throw_if_the_stored_function_does_not_have_a_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->updateParameterConstraint("", "Height", "0<Height<2"));
  }

  void test_that_updateParameterConstraint_will_add_a_constraint_as_expected_to_a_non_composite_function() {
    std::string const constraint("0<A0<2");
    m_fitDomain->setFunction(m_flatBackground);

    m_fitDomain->updateParameterConstraint("", "A0", constraint);

    TS_ASSERT_EQUALS(m_flatBackground->getConstraint(0)->asString(), constraint);
  }

  void test_that_updateParameterConstraint_will_add_a_constraint_as_expected_to_a_composite_function() {
    std::string const constraint("0<Height<2");
    m_fitDomain->setFunction(m_composite);

    m_fitDomain->updateParameterConstraint("f1.", "Height", constraint);

    TS_ASSERT_EQUALS(m_composite->getConstraint(1)->asString(), constraint);
  }

  void test_that_removeParameterConstraint_will_not_throw_if_the_stored_function_is_a_nullptr() {
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->removeParameterConstraint("A0"));
  }

  void test_that_removeParameterConstraint_will_not_throw_if_the_stored_function_does_not_have_a_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->removeParameterConstraint("Height"));
  }

  void test_that_removeParameterConstraint_will_not_throw_if_the_parameter_does_not_have_a_constraint_to_remove() {
    m_fitDomain->setFunction(m_flatBackground);
    TS_ASSERT_THROWS_NOTHING(m_fitDomain->removeParameterConstraint("A0"));
  }

  void test_that_removeParameterConstraint_will_remove_the_constraint_on_a_parameter() {
    m_fitDomain->setFunction(m_flatBackground);
    m_fitDomain->updateParameterConstraint("", "A0", "0<A0<2");

    m_fitDomain->removeParameterConstraint("A0");

    TS_ASSERT_EQUALS(m_flatBackground->getConstraint(0), nullptr);
  }

  void
  test_that_setting_the_value_of_a_parameter_to_a_value_outside_of_the_constraints_of_another_parameter_tied_to_it_will_remove_the_tie() {
    m_fitDomain->setFunction(m_expDecay);
    m_fitDomain->updateParameterConstraint("", "Height", "0.5<Height<1.5");
    TS_ASSERT(m_fitDomain->updateParameterTie("Height", "Lifetime"));

    m_fitDomain->setParameterValue("Lifetime", 2.0);

    TS_ASSERT(m_fitDomain->isParameterActive("Height"));
    TS_ASSERT_EQUALS(m_fitDomain->getParameterValue("Height"), 1.0);
  }

  void
  test_that_attempting_to_tie_a_parameter_to_another_parameter_with_a_value_outside_the_allowed_constraints_will_not_perform_the_tie() {
    m_fitDomain->setFunction(m_expDecay);
    m_fitDomain->updateParameterConstraint("", "Height", "0.5<Height<1.5");
    m_fitDomain->setParameterValue("Lifetime", 2.0);

    TS_ASSERT(m_fitDomain->updateParameterTie("Height", "Lifetime"));

    TS_ASSERT(m_fitDomain->isParameterActive("Height"));
    TS_ASSERT_EQUALS(m_fitDomain->getParameterValue("Height"), 1.0);
  }

  void
  test_that_updateParameterConstraint_will_not_update_the_constraint_if_the_lower_bound_does_not_encompass_the_value_of_the_parameter() {
    m_fitDomain->setFunction(m_expDecay);

    // The value of Height is automatically 1.0
    m_fitDomain->updateParameterConstraint("", "Height", "1.1<Height<1.5");

    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->getConstraint(0), nullptr);
  }

  void
  test_that_updateParameterConstraint_will_not_update_the_constraint_if_the_upper_bound_does_not_encompass_the_value_of_the_parameter() {
    m_fitDomain->setFunction(m_expDecay);

    // The value of Height is automatically 1.0
    m_fitDomain->updateParameterConstraint("", "Height", "0.5<Height<0.9");

    TS_ASSERT_EQUALS(m_fitDomain->getFunctionCopy()->getConstraint(0), nullptr);
  }

  void test_that_isParameterValueWithinConstraints_returns_true_if_the_value_is_within_the_parameters_constraints() {
    m_fitDomain->setFunction(m_expDecay);

    m_fitDomain->updateParameterConstraint("", "Height", "0<Height<2");

    TS_ASSERT(m_fitDomain->isParameterValueWithinConstraints("Height", 0.0));
    TS_ASSERT(m_fitDomain->isParameterValueWithinConstraints("Height", 1.0));
    TS_ASSERT(m_fitDomain->isParameterValueWithinConstraints("Height", 2.0));
  }

  void
  test_that_isParameterValueWithinConstraints_returns_false_if_the_value_is_not_within_the_parameters_constraints() {
    m_fitDomain->setFunction(m_expDecay);

    m_fitDomain->updateParameterConstraint("", "Height", "0<Height<2");

    TS_ASSERT(!m_fitDomain->isParameterValueWithinConstraints("Height", -0.1));
    TS_ASSERT(!m_fitDomain->isParameterValueWithinConstraints("Height", 2.1));
  }

  void test_that_getParametersTiedTo_will_return_the_names_of_parameters_tied_to_the_given_parameter() {
    m_fitDomain->setFunction(m_expDecay);
    TS_ASSERT(m_fitDomain->updateParameterTie("Height", "Lifetime"));

    auto const tiedParameters = std::vector<std::string>{"Height"};
    TS_ASSERT_EQUALS(m_fitDomain->getParametersTiedTo("Height").size(), 0);
    TS_ASSERT_EQUALS(m_fitDomain->getParametersTiedTo("Lifetime"), tiedParameters);
  }

private:
  std::string m_wsName;
  WorkspaceIndex m_wsIndex;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  double m_startX;
  double m_endX;
  Mantid::API::IFunction_sptr m_flatBackground;
  Mantid::API::IFunction_sptr m_expDecay;
  Mantid::API::IFunction_sptr m_composite;

  std::unique_ptr<FitDomain> m_fitDomain;
};
