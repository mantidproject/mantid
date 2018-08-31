#ifndef MANTIDWIDGETS_MULTIDOMAINFUNCTIONMODELTEST_H_
#define MANTIDWIDGETS_MULTIDOMAINFUNCTIONMODELTEST_H_

#include "MantidQtWidgets/Common/FunctionBrowser/MultiDomainFunctionModel.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

#include <sstream>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
Mantid::API::MatrixWorkspace_sptr createWorkspace(int numberOfHistograms) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, 10);
}

CompositeFunction_const_sptr
getComposite(MultiDomainFunctionModel const &model) {
  return boost::dynamic_pointer_cast<CompositeFunction const>(
      model.getFitFunction());
}

boost::shared_ptr<MultiDomainFunction const>
getMultiDomainFunction(MultiDomainFunctionModel const &model) {
  return boost::dynamic_pointer_cast<MultiDomainFunction const>(
      model.getFitFunction());
}

void addMultipleFunctionsToModel(MultiDomainFunctionModel &model,
                                 std::vector<std::size_t> const &position) {}

template <typename Name, typename... Names>
void addMultipleFunctionsToModel(MultiDomainFunctionModel &model,
                                 std::vector<std::size_t> const &position,
                                 Name const &functionName,
                                 Names const &... functionNames) {
  model.addFunction(functionName, position);
  addMultipleFunctionsToModel(model, position, functionNames...);
}

MultiDomainFunctionModel createNewSingleDomainModel() {
  MultiDomainFunctionModel model;
  model.addDomain(createWorkspace(1), 0);
  return model;
}

MultiDomainFunctionModel createNewMultipleDomainModel(int numberOfDomains) {
  MultiDomainFunctionModel model;
  model.addDomains(createWorkspace(numberOfDomains));
  return model;
}

template <typename F>
void forEachFunctionIn(CompositeFunction const &composite, F const &functor) {
  for (auto i = 0u; i < composite.nFunctions(); ++i)
    functor(*composite.getFunction(i));
}

std::string getBoundaryConstraintString(std::string const &parameter,
                                        double lower, double upper) {
  std::ostringstream ostr;
  ostr << lower << "<" << parameter << "<" << upper;
  return ostr.str();
}

void addFunctionsToComposite(CompositeFunction &composite) {}

template <typename Function, typename... Functions>
void addFunctionsToComposite(CompositeFunction &composite,
                             Function const &function,
                             Functions const &... functions) {
  composite.addFunction(function);
  addFunctionsToComposite(composite, functions...);
}

template <typename Function, typename... Functions>
CompositeFunction_sptr createComposite(Function const &function,
                                       Functions const &... functions) {
  auto composite = CompositeFunction_sptr(new CompositeFunction);
  addFunctionsToComposite(*composite, function, functions...);
  return composite;
}

IFunction_sptr createFunction(std::string const &name) {
  return FunctionFactory::Instance().createFunction(name);
}

CompositeFunction_sptr createCompositeFunction(std::string const &name) {
  return boost::dynamic_pointer_cast<CompositeFunction>(createFunction(name));
}

template <typename... Functions>
CompositeFunction_sptr
createConvolutionWithFlatBackground(Functions const &... functions) {
  auto convolution = createCompositeFunction("Convolution");
  convolution->addFunction(createFunction("FlatBackground"));
  convolution->addFunction(createComposite(functions...));
  return convolution;
}

} // namespace

class MultiDomainFunctionModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiDomainFunctionModelTest *createSuite() {
    return new MultiDomainFunctionModelTest();
  }
  static void destroySuite(MultiDomainFunctionModelTest *suite) {
    delete suite;
  }

  MultiDomainFunctionModelTest() { FrameworkManager::Instance(); }

  void test_default_fit_function_is_empty_composite() {
    auto const composite = getComposite(createSingleDomainModel());
    TS_ASSERT_DIFFERS(composite, nullptr);
    TS_ASSERT_EQUALS(composite->nFunctions(), 0);
  }

  void test_single_function_can_be_added_to_empty_model() {
    auto model = createSingleDomainModel("Lorentzian");
    TS_ASSERT_EQUALS(model.numberOfFunctionsAt({}), 1);
    TS_ASSERT_EQUALS(model.getFitFunction()->name(), "Lorentzian");
  }

  void test_multiple_functions_can_be_added_to_empty_model() {
    auto const model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                               "FlatBackground");
    TS_ASSERT_EQUALS(model.numberOfFunctionsAt({}), 3);
  }

  void test_fit_function_can_be_created_from_model() {
    auto const model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                               "FlatBackground");

    CompositeFunction_const_sptr function;
    TS_ASSERT_THROWS_NOTHING(function = getComposite(model));
    TS_ASSERT_EQUALS(function->nFunctions(), 3);
    TS_ASSERT_EQUALS(function->getFunction(0)->name(), "Lorentzian");
    TS_ASSERT_EQUALS(function->getFunction(1)->name(), "DeltaFunction");
    TS_ASSERT_EQUALS(function->getFunction(2)->name(), "FlatBackground");
  }

  void test_parameter_value_can_be_set_on_non_composite_model() {
    auto model = createSingleDomainModel("Lorentzian");

    std::string const parameter = "PeakCentre";
    double const value = 5.0;
    TS_ASSERT_THROWS_NOTHING(model.setParameterValue(parameter, value));
    TS_ASSERT_EQUALS(model.getParameterValue(parameter), value);
    TS_ASSERT_EQUALS(model.getFitFunction()->getParameter(parameter), value);
  }

  void test_parameter_value_can_be_set_on_composite_model() {
    auto model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                         "FlatBackground");

    std::string const parameter = "f1.Centre";
    double const value = 5.0;
    TS_ASSERT_THROWS_NOTHING(model.setParameterValue(parameter, value));
    TS_ASSERT_EQUALS(model.getFitFunction()->getParameter(parameter), value);
  }

  void test_parameter_can_be_tied_in_single_domain_model() {
    auto model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                         "FlatBackground");

    std::string const parameter = "f1.Centre";
    std::string const expression = "f0.PeakCentre";
    TS_ASSERT_THROWS_NOTHING(model.setParameterTie(parameter, expression));
    TS_ASSERT_EQUALS(model.getParameterTie(parameter), expression);
  }

  void test_function_with_tie_can_be_created_from_single_domain_model() {
    auto model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                         "FlatBackground");

    std::string const parameter = "f1.Centre";
    std::string const expression = "f0.PeakCentre";
    TS_ASSERT_THROWS_NOTHING(model.setParameterTie(parameter, expression));

    assertHasTie(*getComposite(model), parameter, expression);
  }

  void test_parameter_can_be_given_lower_bound_in_single_domain_model() {
    auto model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                         "FlatBackground");

    std::string const parameter = "f0.Amplitude";
    double const bound = 0.0;
    TS_ASSERT_THROWS_NOTHING(model.setParameterLowerBound(parameter, bound));
    TS_ASSERT_EQUALS(model.getParameterLowerBound(parameter), bound);
  }

  void test_parameter_can_be_given_upper_bound_in_single_domain_model() {
    auto model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                         "FlatBackground");

    std::string const parameter = "f2.A0";
    double const bound = 1.0;
    TS_ASSERT_THROWS_NOTHING(model.setParameterUpperBound(parameter, bound));
    TS_ASSERT_EQUALS(model.getParameterUpperBound(parameter), bound);
  }

  void test_function_with_bounds_can_be_created_from_single_domain_model() {
    auto model = createSingleDomainModel("Lorentzian", "DeltaFunction",
                                         "FlatBackground");

    std::string const parameter = "f2.A0";
    double const lower = 0.0;
    double const upper = 1.0;

    TS_ASSERT_THROWS_NOTHING(model.setParameterBounds(parameter, lower, upper));
    assertHasBounds(*getComposite(model), parameter, lower, upper);
  }

  void test_function_can_be_set_in_single_domain_model() {
    auto composite = createComposite(createFunction("Lorentzian"),
                                     createFunction("DeltaFunction"));
    std::string const parameter = "f0.PeakCentre";
    double const value = 1.0;
    composite->setParameter("f0.PeakCentre", 1.0);

    auto model = createSingleDomainModel();
    TS_ASSERT_THROWS_NOTHING(model.setFunction(composite));
    TS_ASSERT_EQUALS(model.numberOfFunctionsAt({}), 2);
    TS_ASSERT_EQUALS(model.getParameterValue(parameter), value);
  }

  void test_function_can_be_set_with_string_in_single_domain_model() {
    auto composite = createComposite(createFunction("Lorentzian"),
                                     createFunction("DeltaFunction"));
    std::string const parameter = "f0.PeakCentre";
    double const value = 1.0;
    composite->setParameter("f0.PeakCentre", 1.0);

    auto model = createSingleDomainModel();
    TS_ASSERT_THROWS_NOTHING(model.setFunction(composite->asString()));
    TS_ASSERT_EQUALS(model.numberOfFunctionsAt({}), 2);
    TS_ASSERT_EQUALS(model.getParameterValue(parameter), value);
  }

  void test_function_can_be_removed_from_single_domain_model() {
    auto convolution = createConvolutionWithFlatBackground(
        createFunction("Lorentzian"), createFunction("DeltaFunction"));

    auto model = createSingleDomainModel();
    TS_ASSERT_THROWS_NOTHING(model.setFunction(convolution));
    TS_ASSERT_THROWS_NOTHING(model.removeFunction({1, 0}));
    TS_ASSERT_EQUALS(model.numberOfFunctionsAt({1}), 1);
  }

  void test_function_can_be_created_after_function_is_removed_from_model() {}

  void test_multiple_domain_model_can_be_created() {
    MultiDomainFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model = createMultipleDomainModel(3));
    TS_ASSERT_EQUALS(model.numberOfDomains(), 3);
  }

  void test_function_can_be_added_to_multiple_domain_model() {
    MultiDomainFunctionModel model;
    TS_ASSERT_THROWS_NOTHING(model = createMultipleDomainModel(3));
    TS_ASSERT_THROWS_NOTHING(model.addFunction("Lorentzian", {}));
    TS_ASSERT_EQUALS(model.numberOfFunctionsAt({}), 1);
  }

  void test_multiple_domain_function_can_be_created_from_model() {
    auto const model = createMultipleDomainModel(3, "Lorentzian");

    boost::shared_ptr<MultiDomainFunction const> function;
    TS_ASSERT_THROWS_NOTHING(function = getMultiDomainFunction(model));
    TS_ASSERT_EQUALS(function->getNumberDomains(), 3)

    forEachFunctionIn(*function, [&](IFunction const &function) {
      TS_ASSERT_EQUALS(function.name(), "Lorentzian")
    });
  }

  void test_global_equality_tie_can_be_added_to_multiple_domain_model() {
    auto model = createMultipleDomainModel(3, "Lorentzian", "DeltaFunction",
                                           "FlatBackground");
    std::string const parameter = "f0.Amplitude";

    TS_ASSERT_THROWS_NOTHING(model.addEqualityGlobalTie(parameter));
    TS_ASSERT_THROWS_NOTHING(model.setActiveDomain(1));
    TS_ASSERT_EQUALS(model.getParameterTie(parameter), "f0." + parameter);
    TS_ASSERT_THROWS_NOTHING(model.setActiveDomain(2));
    TS_ASSERT_EQUALS(model.getParameterTie(parameter), "f0." + parameter);
  }

  void test_function_with_global_equality_tie_can_be_created_from_model() {
    auto model = createMultipleDomainModel(3, "Lorentzian", "DeltaFunction",
                                           "FlatBackground");

    std::string const parameter = "f0.Amplitude";
    TS_ASSERT_THROWS_NOTHING(model.addEqualityGlobalTie(parameter));

    auto const function = getMultiDomainFunction(model);
    assertHasTie(*function, "f1." + parameter, "f0." + parameter);
    assertHasTie(*function, "f2." + parameter, "f0." + parameter);
  }

private:
  template <typename... Names>
  MultiDomainFunctionModel
  createSingleDomainModel(Names const &... functionNames) {
    auto model = createNewSingleDomainModel();
    addFunctionsToModel(model, functionNames...);
    return model;
  }

  template <typename... Names>
  MultiDomainFunctionModel
  createMultipleDomainModel(int numberOfDomains,
                            Names const &... functionNames) {
    auto model = createNewMultipleDomainModel(numberOfDomains);
    addFunctionsToModel(model, functionNames...);
    return model;
  }

  template <typename... Names>
  void addFunctionsToModel(MultiDomainFunctionModel &model,
                           Names const &... functionNames) {
    addFunctionsToModel(model, {}, functionNames...);
  }

  template <typename... Names>
  void addFunctionsToModel(MultiDomainFunctionModel &model,
                           std::vector<std::size_t> const &position,
                           Names const &... functionNames) {
    TS_ASSERT_THROWS_NOTHING(
        addMultipleFunctionsToModel(model, position, functionNames...));
  }

  void assertHasTie(Mantid::API::IFunction const &function,
                    std::string const &parameter,
                    std::string const &expression) {
    auto const index = function.parameterIndex(parameter);

    Mantid::API::ParameterTie *tie;
    TS_ASSERT_THROWS_NOTHING(tie = function.getTie(index));
    TS_ASSERT_EQUALS(tie->asString(), parameter + "=" + expression);
  }

  void assertHasBounds(Mantid::API::IFunction const &function,
                       std::string const &parameter, double lowerBound,
                       double upperBound) {
    auto const index = function.parameterIndex(parameter);
    auto const expected =
        getBoundaryConstraintString(parameter, lowerBound, upperBound);

    Mantid::API::IConstraint *constraint;
    TS_ASSERT_THROWS_NOTHING(constraint = function.getConstraint(index));
    TS_ASSERT_EQUALS(constraint->asString(), expected);
  }
};

#endif
