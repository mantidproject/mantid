#include "MantidQtWidgets/Common/FunctionBrowser/MultiDomainFunctionModel.h"

#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"

#include "MantidKernel/make_unique.h"

using namespace Mantid::API;

namespace {
using namespace MantidQt::MantidWidgets;

Expression createExpression(std::string const &str) {
  Expression expression;
  expression.parse(str);
  return expression;
}

bool expressionContainsParameter(Expression const &expression,
                                 std::string const &parameter) {
  for (auto const &term : expression) {
    if (term.str() == parameter)
      return true;
  }
  return false;
}

bool expressionContainsParameter(std::string const &expression,
                                 std::string const &parameter) {
  return expressionContainsParameter(createExpression(expression), parameter);
}

void removeTiesWhichContainParameter(FunctionProperties &properties,
                                     std::string const &parameter) {
  properties.removeTieIf(
      [&](std::string const &tieParameter, std::string const &expression) {
        return tieParameter == parameter ||
               expressionContainsParameter(expression, parameter);
      });
}

void removeConstraintsWhichContainParameter(FunctionProperties &properties,
                                            std::string const &parameter) {
  properties.removeConstraintIf(
      [&](const std::pair<std::string, BoundaryConstraint> &constraint) {
        return constraint.first == parameter;
      });
}

std::string functionIndex(std::size_t index) {
  return "f" + std::to_string(index) + ".";
}

template <typename T> IFunction::Attribute createAttribute(T const &value) {
  return IFunction::Attribute(value);
}

IFunction_sptr createFunction(std::string const &name) {
  return FunctionFactory::Instance().createFunction(name);
}

CompositeFunction_sptr getCompositeAt(CompositeFunction const &composite,
                                      std::size_t index) {
  return boost::dynamic_pointer_cast<CompositeFunction>(
      composite.getFunction(index));
}

template <typename BeginIterator, typename EndIterator>
IFunction_sptr getFunctionAt(CompositeFunction_sptr composite,
                             BeginIterator startIt, EndIterator endIt) {
  if (startIt > endIt)
    return nullptr;
  else if (startIt == endIt)
    return composite;

  for (auto i = startIt; i < endIt - 1; ++i) {
    if (!(composite = getCompositeAt(*composite, *i)))
      return nullptr;
  }
  return composite->getFunction(*(endIt - 1));
}

IFunction_sptr getFunctionAt(CompositeFunction_sptr function,
                             std::vector<std::size_t> const &position) {
  return getFunctionAt(function, position.begin(), position.end());
}

std::size_t addFunctionAt(CompositeFunction_sptr composite,
                          IFunction const &functionToAdd,
                          std::vector<std::size_t> const &position) {
  auto const function = getFunctionAt(composite, position);
  if (composite = boost::dynamic_pointer_cast<CompositeFunction>(function)) {
    composite->addFunction(functionToAdd.clone());
    return composite->nFunctions() - 1;
  }
  throw std::runtime_error("Unable to add function to non-composite function.");
}

IFunction_sptr removeFunctionAt(CompositeFunction_sptr composite,
                                std::vector<std::size_t> const &position) {
  auto const function =
      getFunctionAt(composite, position.begin(), position.end() - 1);
  composite = boost::dynamic_pointer_cast<CompositeFunction>(function);

  if (composite && !position.empty()) {
    auto const removed = composite->getFunction(position.back());
    composite->removeFunction(position.back());
    return removed;
  }
  return nullptr;
}

std::unique_ptr<IConstraint> createConstraint(IFunction &function,
                                              std::string const &expression) {
  auto constraint =
      ConstraintFactory::Instance().createInitialized(&function, expression);
  return std::unique_ptr<IConstraint>(constraint);
}

void addTiesToFunction(IFunction &function,
                       FunctionProperties const &properties) {
  properties.forEachTie(
      [&function](std::string const &parameter, std::string const &expression) {
        function.tie(parameter, expression);
      });
}

void addConstraintsToFunction(IFunction &function,
                              FunctionProperties const &properties) {
  properties.forEachConstraint(
      [&function](std::string const &parameter,
                  BoundaryConstraint const &constraint) {
        function.addConstraint(
            createConstraint(function, constraint.str(parameter)));
      });
}

void setParameterValueInFunction(IFunction &function,
                                 std::string const &parameter,
                                 ParameterValue const &value) {
  auto const index = function.parameterIndex(parameter);
  function.setParameter(index, value.value);
  if (value.error)
    function.setError(index, *value.error);
}

void setParameterValuesInFunction(IFunction &function,
                                  LocalFunctionProperties const &properties) {
  properties.forEachParameter(
      [&function](std::string const &parameter, ParameterValue const &value) {
        setParameterValueInFunction(function, parameter, value);
      });
}

void setAttributeValuesInFunction(IFunction &function,
                                  LocalFunctionProperties const &properties) {
  properties.forEachAttribute([&function](std::string const &attribute,
                                          const IFunction::Attribute &value) {
    function.setAttribute(attribute, value);
  });
}

void addPropertiesToFunction(IFunction &function,
                             FunctionProperties const &properties) {
  addTiesToFunction(function, properties);
  addConstraintsToFunction(function, properties);
}

void addWorkspacePropertiesToFunction(
    IFunction &function, LocalFunctionProperties const &properties) {
  if (auto const workspace = properties.getWorkspace()) {
    function.setMatrixWorkspace(workspace, *properties.getWorkspaceIndex(),
                                workspace->x(0).front(),
                                workspace->x(0).back());
  }
}

void addPropertiesToLocalFunction(IFunction &function,
                                  LocalFunctionProperties const &properties) {
  addPropertiesToFunction(function, properties);
  setParameterValuesInFunction(function, properties);
  setAttributeValuesInFunction(function, properties);
  addWorkspacePropertiesToFunction(function, properties);
}

IFunction_sptr createLocalFunction(IFunction const &function,
                                   LocalFunctionProperties const &properties) {
  auto newFunction = function.clone();
  addPropertiesToLocalFunction(*newFunction, properties);
  return newFunction;
}

boost::shared_ptr<MultiDomainFunction> createMultiDomainFunction(
    IFunction const &localFunction,
    std::vector<LocalFunctionProperties> const &localFunctionProperties,
    FunctionProperties const &globalFunctionProperties) {
  boost::shared_ptr<MultiDomainFunction> function(new MultiDomainFunction);
  for (auto i = 0u; i < localFunctionProperties.size(); ++i) {
    function->addFunction(
        createLocalFunction(localFunction, localFunctionProperties[i]));
    function->setDomainIndex(i, i);
  }
  addPropertiesToFunction(*function, globalFunctionProperties);
  return function;
}

IFunction_sptr getMinimalFormOfComposite(CompositeFunction_sptr composite) {
  if (composite->nFunctions() == 1)
    return composite->getFunction(0);
  return composite;
}

CompositeFunction_sptr createComposite(IFunction_sptr function) {
  if (auto const composite =
          boost::dynamic_pointer_cast<CompositeFunction>(function))
    return composite;

  CompositeFunction_sptr composite(new CompositeFunction);
  composite->addFunction(function);
  return composite;
}

CompositeFunction_sptr createComposite(std::string const &functionString) {
  auto const function =
      FunctionFactory::Instance().createInitialized(functionString);
  return createComposite(function);
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {

MultiDomainFunctionModel::MultiDomainFunctionModel()
    : m_localFunctionProperties{LocalFunctionProperties()}, m_activeDomain(0),
      m_function(new CompositeFunction) {}

IFunction_sptr MultiDomainFunctionModel::getFitFunction() const {
  if (m_localFunctionProperties.size() == 1)
    return getLocalFunction(0);
  return createMultiDomainFunction(getFunction(), m_localFunctionProperties,
                                   m_globalFunctionProperties);
}

bool MultiDomainFunctionModel::hasZeroDomains() const {
  return m_localFunctionProperties.size() == 1 &&
         !m_localFunctionProperties.front().hasDataset();
}

std::size_t MultiDomainFunctionModel::numberOfParameters() const {
  return m_function->nParams();
}

std::size_t MultiDomainFunctionModel::numberOfDomains() const {
  return m_localFunctionProperties.size();
}

std::string
MultiDomainFunctionModel::getParameterName(std::size_t index) const {
  return m_function->parameterName(index);
}

double MultiDomainFunctionModel::getParameterValue(
    std::string const &parameter) const {
  auto const value = getActiveProperties().getParameterValue(parameter);
  return value ? *value : m_function->getParameter(parameter);
}

boost::optional<double> MultiDomainFunctionModel::getParameterError(
    std::string const &parameter) const {
  return getActiveProperties().getParameterError(parameter);
}

boost::optional<std::string>
MultiDomainFunctionModel::getParameterTie(std::string const &parameter) const {
  auto const tie = getActiveProperties().getTieOrNone(parameter);
  return tie ? tie : getGlobalTie(parameter);
}

boost::optional<std::string>
MultiDomainFunctionModel::getGlobalTie(std::string const &parameter) const {
  return m_globalFunctionProperties.getTieOrNone(functionIndex(m_activeDomain) +
                                                 parameter);
}

boost::optional<double> MultiDomainFunctionModel::getParameterLowerBound(
    std::string const &name) const {
  return getActiveProperties().getParameterLowerBound(name);
}

boost::optional<double> MultiDomainFunctionModel::getParameterUpperBound(
    std::string const &name) const {
  return getActiveProperties().getParameterUpperBound(name);
}

std::vector<std::string> MultiDomainFunctionModel::getAttributeNames() const {
  return m_function->getAttributeNames();
}

IFunction::Attribute const &
MultiDomainFunctionModel::getAttribute(std::string const &name) const {
  return getActiveProperties().getAttribute(name);
}

MatrixWorkspace_const_sptr MultiDomainFunctionModel::getWorkspace() const {
  return getActiveProperties().getWorkspace();
}

std::string MultiDomainFunctionModel::getWorkspaceName() const {
  if (auto const workspace = getWorkspace())
    return workspace->getName();
  return "";
}

boost::optional<std::size_t>
MultiDomainFunctionModel::getWorkspaceIndex() const {
  return getActiveProperties().getWorkspaceIndex();
}

bool MultiDomainFunctionModel::isComposite(
    std::vector<std::size_t> const &position) const {
  return nullptr != boost::dynamic_pointer_cast<CompositeFunction>(
                        getFunctionAt(m_function, position));
}

std::size_t MultiDomainFunctionModel::numberOfFunctionsAt(
    std::vector<std::size_t> const &position) const {
  auto const function = getFunctionAt(m_function, position);
  if (auto const composite =
          boost::dynamic_pointer_cast<CompositeFunction>(function))
    return composite->nFunctions();
  return 0;
}

bool MultiDomainFunctionModel::isParameterTied(std::string const &name) const {
  return getActiveProperties().isTied(name);
}

bool MultiDomainFunctionModel::isParameterFixed(std::string const &name) const {
  return getActiveProperties().isFixed(name);
}

bool MultiDomainFunctionModel::isParameterConstrained(
    std::string const &name) const {
  return getActiveProperties().isConstrained(name);
}

std::string MultiDomainFunctionModel::getLocalFunctionString() const {
  return getLocalFunction(m_activeDomain)->asString();
}

IFunction_sptr
MultiDomainFunctionModel::getLocalFunction(std::size_t domain) const {
  return createLocalFunction(*getMinimalFormOfComposite(m_function),
                             m_localFunctionProperties[domain]);
}

std::size_t MultiDomainFunctionModel::numberOfLocalParameters() const {
  return m_function->nParams();
}

IFunction &MultiDomainFunctionModel::getFunction() const {
  return *getMinimalFormOfComposite(m_function);
}

LocalFunctionProperties &MultiDomainFunctionModel::getActiveProperties() {
  return m_localFunctionProperties.at(m_activeDomain);
}

LocalFunctionProperties const &
MultiDomainFunctionModel::getActiveProperties() const {
  return m_localFunctionProperties.at(m_activeDomain);
}

std::size_t MultiDomainFunctionModel::getActiveDomain() const {
  return m_activeDomain;
}

void MultiDomainFunctionModel::setActiveDomain(std::size_t domain) {
  if (domain >= m_localFunctionProperties.size())
    throw std::out_of_range("Domain " + std::to_string(domain) +
                            " is out of range.");
  m_activeDomain = domain;
}

void MultiDomainFunctionModel::setFunction(std::string const &functionString) {
  clear();
  m_function = createComposite(functionString);
}

void MultiDomainFunctionModel::setFunction(IFunction_sptr function) {
  clear();
  m_function = createComposite(function);
}

std::size_t MultiDomainFunctionModel::addFunction(
    std::string const &name, std::vector<std::size_t> const &position) {
  return addFunctionAt(m_function, *createFunction(name), position);
}

void MultiDomainFunctionModel::removeFunction(
    std::vector<std::size_t> const &position) {
  auto const function = removeFunctionAt(m_function, position);
  for (auto i = 0u; i < function->nParams(); ++i)
    removeParameterProperties(function->parameterName(i));
}

void MultiDomainFunctionModel::removeTiesContainingParameter(
    std::string const &parameter) {
  for (auto &localProperties : m_localFunctionProperties)
    removeTiesWhichContainParameter(localProperties, parameter);
  removeTiesWhichContainParameter(m_globalFunctionProperties,
                                  functionIndex(m_activeDomain) + parameter);
}

void MultiDomainFunctionModel::removeConstraintsContainingParameter(
    std::string const &parameter) {
  for (auto &localProperties : m_localFunctionProperties)
    removeConstraintsWhichContainParameter(localProperties, parameter);
  removeConstraintsWhichContainParameter(
      m_globalFunctionProperties, functionIndex(m_activeDomain) + parameter);
}

void MultiDomainFunctionModel::removeParameterProperties(
    std::string const &parameterName) {
  removeTiesContainingParameter(parameterName);
  removeConstraintsContainingParameter(parameterName);
}

void MultiDomainFunctionModel::setStringAttribute(std::string const &name,
                                                  std::string const &value) {
  getActiveProperties().setAttribute(name, createAttribute(value));
}

void MultiDomainFunctionModel::setDoubleAttribute(std::string const &name,
                                                  double value) {
  getActiveProperties().setAttribute(name, createAttribute(value));
}

void MultiDomainFunctionModel::setIntAttribute(std::string const &name,
                                               int value) {
  getActiveProperties().setAttribute(name, createAttribute(value));
}

void MultiDomainFunctionModel::setBoolAttribute(std::string const &name,
                                                bool value) {
  getActiveProperties().setAttribute(name, createAttribute(value));
}

void MultiDomainFunctionModel::setVectorAttribute(
    std::string const &name, const std::vector<double> &value) {
  getActiveProperties().setAttribute(name, createAttribute(value));
}

void MultiDomainFunctionModel::setVectorAttributeSize(std::string const &name,
                                                      std::size_t size) {
  getActiveProperties().resizeVectorAttribute(name, size);
}

void MultiDomainFunctionModel::addEmptyDomain() {
  m_localFunctionProperties.emplace_back(LocalFunctionProperties());
}

void MultiDomainFunctionModel::addDomains(MatrixWorkspace_sptr workspace) {
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i)
    addDomain(workspace, i);
}

void MultiDomainFunctionModel::addDomains(MatrixWorkspace_sptr workspace,
                                          std::size_t from, std::size_t to) {
  for (auto i = from; i <= to; ++i)
    addDomain(workspace, i);
}

void MultiDomainFunctionModel::addDomain(MatrixWorkspace_sptr workspace,
                                         std::size_t workspaceIndex) {
  if (hasZeroDomains())
    m_localFunctionProperties[0] =
        LocalFunctionProperties(workspace, workspaceIndex);
  else
    m_localFunctionProperties.emplace_back(
        LocalFunctionProperties(workspace, workspaceIndex));
}

void MultiDomainFunctionModel::removeDomain(std::size_t domain) {
  m_localFunctionProperties.erase(m_localFunctionProperties.begin() + domain);

  if (m_activeDomain >= m_localFunctionProperties.size())
    m_activeDomain = m_localFunctionProperties.size();
  if (m_localFunctionProperties.empty())
    addEmptyDomain();
}

void MultiDomainFunctionModel::clearDomains() {
  m_localFunctionProperties = {LocalFunctionProperties()};
  m_globalFunctionProperties = FunctionProperties();
}

void MultiDomainFunctionModel::clear() {
  m_function = CompositeFunction_sptr(new CompositeFunction);
  clearDomains();
}

void MultiDomainFunctionModel::setParameterValue(
    std::string const &parameterName, double value) {
  getActiveProperties().setParameterValue(parameterName, value);
}

void MultiDomainFunctionModel::setParameterError(
    std::string const &parameterName, double value) {
  getActiveProperties().setParameterValue(parameterName, value);
}

void MultiDomainFunctionModel::removeParameterError(
    std::string const &parameterName) {
  getActiveProperties().removeParameterError(parameterName);
}

void MultiDomainFunctionModel::removeParameterErrors() {
  getActiveProperties().removeParameterErrors();
}

void MultiDomainFunctionModel::fixParameter(std::string const &parameterName) {
  fixParameterInDomain(parameterName, m_activeDomain);
}

void MultiDomainFunctionModel::unfixParameter(
    std::string const &parameterName) {
  unfixParameterInDomain(parameterName, m_activeDomain);
}

void MultiDomainFunctionModel::setParameterTie(std::string const &parameterName,
                                               std::string const &expression) {
  addLocalTieToDomain(parameterName, expression, m_activeDomain);
}

void MultiDomainFunctionModel::removeTie(std::string const &parameterName) {
  removeLocalTieFromDomain(parameterName, m_activeDomain);
}

void MultiDomainFunctionModel::removeTies() {
  removeLocalTiesFromDomain(m_activeDomain);
}

void MultiDomainFunctionModel::setParameterUpperBound(
    std::string const &parameterName, double bound) {
  addUpperBoundToDomain(parameterName, bound, m_activeDomain);
}

void MultiDomainFunctionModel::setParameterLowerBound(
    std::string const &parameterName, double bound) {
  addLowerBoundToDomain(parameterName, bound, m_activeDomain);
}

void MultiDomainFunctionModel::setParameterBounds(
    std::string const &parameterName, double lowerBound, double upperBound) {
  addBoundsToDomain(parameterName, lowerBound, upperBound, m_activeDomain);
}

void MultiDomainFunctionModel::setParameterBoundsWithinPercentile(
    std::string const &parameterName, double percentile) {
  addBoundsToDomainWithinPercentile(parameterName, percentile, m_activeDomain);
}

void MultiDomainFunctionModel::removeConstraint(
    std::string const &parameterName, std::string const &type) {
  if (type == "LowerBound")
    getActiveProperties().removeLowerBound(parameterName);
  else
    getActiveProperties().removeUpperBound(parameterName);
}

void MultiDomainFunctionModel::removeConstraints(
    std::string const &parameterName) {
  removeLocalConstraintsFromDomain(parameterName, m_activeDomain);
}

void MultiDomainFunctionModel::fixParameterInDomain(
    std::string const &parameterName, std::size_t domain) {
  m_localFunctionProperties[domain].fixParameter(parameterName);
}

void MultiDomainFunctionModel::unfixParameterInDomain(
    std::string const &parameterName, std::size_t domain) {
  m_localFunctionProperties[domain].removeTie(parameterName);
}

void MultiDomainFunctionModel::addLocalTieToDomain(
    std::string const &parameterName, std::string const &expression,
    std::size_t domain) {
  if (m_globalFunctionProperties.isTied(parameterName))
    throw std::runtime_error("Invalid attempt to add local tie to parameter '" +
                             parameterName + "'; already has a global tie.");
  m_localFunctionProperties[domain].tie(parameterName, expression);
}

void MultiDomainFunctionModel::removeLocalTieFromDomain(
    std::string const &parameterName, std::size_t domain) {
  m_localFunctionProperties[domain].removeTie(parameterName);
  m_globalFunctionProperties.removeTie(functionIndex(domain) + parameterName);
}

void MultiDomainFunctionModel::removeLocalTiesFromDomain(std::size_t domain) {
  m_localFunctionProperties[domain].clearTies();
}

void MultiDomainFunctionModel::addEqualityGlobalTie(
    std::string const &parameterName) {
  auto const firstParameter = functionIndex(0) + parameterName;
  addGlobalTie(parameterName, firstParameter, 1, numberOfDomains());
}

void MultiDomainFunctionModel::addGlobalTie(std::string const &parameterName,
                                            std::string const &expression) {
  addGlobalTie(parameterName, expression, 0, m_function->getNumberDomains());
}

void MultiDomainFunctionModel::addGlobalTie(std::string const &parameterName,
                                            std::string const &expression,
                                            std::size_t domain) {
  if (m_localFunctionProperties[domain].isTied(parameterName))
    throw std::runtime_error(
        "Invalid attempt to add global tie to parameter '" + parameterName +
        "'; already has a local tie.");
  m_globalFunctionProperties.tie(functionIndex(domain) + parameterName,
                                 expression);
}

void MultiDomainFunctionModel::addGlobalTie(std::string const &parameterName,
                                            std::string const &expression,
                                            std::size_t fromDomain,
                                            std::size_t toDomain) {
  for (auto i = fromDomain; i < toDomain; ++i)
    addGlobalTie(parameterName, expression, i);
}

void MultiDomainFunctionModel::removeGlobalTies(
    std::string const &parameterName) {
  m_globalFunctionProperties.removeTie(parameterName);
}

void MultiDomainFunctionModel::removeLocalTies(
    std::string const &parameterName) {
  for (auto &properties : m_localFunctionProperties)
    properties.removeTie(parameterName);
}

void MultiDomainFunctionModel::clearTies() {
  m_globalFunctionProperties.clearTies();
  for (auto &localProperties : m_localFunctionProperties)
    localProperties.clearTies();
}

void MultiDomainFunctionModel::addUpperBoundToDomain(
    std::string const &parameterName, double bound, std::size_t domain) {
  m_localFunctionProperties[domain].setUpperBound(parameterName, bound);
}

void MultiDomainFunctionModel::addLowerBoundToDomain(
    std::string const &parameterName, double bound, std::size_t domain) {
  m_localFunctionProperties[domain].setLowerBound(parameterName, bound);
}

void MultiDomainFunctionModel::addBoundsToDomain(
    std::string const &parameterName, double lowerBound, double upperBound,
    std::size_t domain) {
  m_localFunctionProperties[domain].setConstraint(parameterName, lowerBound,
                                                  upperBound);
}

void MultiDomainFunctionModel::addBoundsToDomainWithinPercentile(
    std::string const &parameterName, double percentile, std::size_t domain) {
  auto const value = m_function->getParameter(parameterName);
  auto const lower = value * (1.0 - percentile);
  auto const upper = value * (1.0 + percentile);
  addBoundsToDomain(parameterName, lower, upper, domain);
}

void MultiDomainFunctionModel::removeLocalConstraintsFromDomain(
    std::string const &parameterName, std::size_t domain) {
  m_localFunctionProperties[domain].removeConstraints(parameterName);
}

void MultiDomainFunctionModel::clearLocalConstraintsFromDomain(
    std::size_t domain) {
  m_localFunctionProperties[domain].clearConstraints();
}

} // namespace MantidWidgets
} // namespace MantidQt
