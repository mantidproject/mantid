#include "MantidQtWidgets/Common/FunctionBrowser/FunctionProperties.h"

#include "MantidAPI/MatrixWorkspace.h"

namespace {
Mantid::API::IFunction::Attribute
getResizedVectorAttribute(Mantid::API::IFunction::Attribute const &attribute,
                          std::size_t size) {
  auto vec = attribute.asVector();
  vec.resize(size);
  return Mantid::API::IFunction::Attribute(vec);
}

Mantid::API::IFunction::Attribute createVectorAttribute(std::size_t size) {
  return Mantid::API::IFunction::Attribute(std::vector<double>(size));
}

std::string createLowerBoundString(boost::optional<double> const &lowerBound) {
  return lowerBound ? std::to_string(*lowerBound) + "<" : "";
}

std::string createUpperBoundString(boost::optional<double> const &upperBound) {
  return upperBound ? "<" + std::to_string(*upperBound) : "";
}
} // namespace

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

std::string BoundaryConstraint::str(std::string const &parameter) const {
  return createLowerBoundString(lowerBound) + parameter +
         createUpperBoundString(upperBound);
}

FunctionProperties::FunctionProperties() : m_ties(), m_constraints() {}

bool FunctionProperties::isTied(std::string const &parameterName) const {
  auto const tieIt = m_ties.find(parameterName);
  return m_ties.end() != tieIt && !tieIt->second.empty();
}

bool FunctionProperties::isFixed(std::string const &parameterName) const {
  return m_fixed.end() != m_fixed.find(parameterName);
}

bool FunctionProperties::isConstrained(std::string const &parameterName) const {
  return m_constraints.end() != findConstraintOf(parameterName);
}

std::string const &
FunctionProperties::getTie(std::string const &parameterName) const {
  auto const tieIt = m_ties.find(parameterName);
  if (m_ties.end() != tieIt)
    return tieIt->second;
  throw std::runtime_error("No tie present for " + parameterName);
}

boost::optional<std::string> const
FunctionProperties::getTieOrNone(std::string const &parameterName) const {
  auto const tieIt = m_ties.find(parameterName);
  if (m_ties.end() != tieIt)
    return tieIt->second;
  return boost::none;
}

boost::optional<double> FunctionProperties::getParameterLowerBound(
    std::string const &parameterName) const {
  auto constraintIt = findConstraintOf(parameterName);
  if (m_constraints.end() != constraintIt)
    return constraintIt->second.lowerBound;
  return boost::none;
}

boost::optional<double> FunctionProperties::getParameterUpperBound(
    std::string const &parameterName) const {
  auto constraintIt = findConstraintOf(parameterName);
  if (m_constraints.end() != constraintIt)
    return constraintIt->second.upperBound;
  return boost::none;
}

void FunctionProperties::tie(std::string const &parameterName,
                             std::string const &expression) {
  m_ties[parameterName] = expression;
}

void FunctionProperties::removeTie(std::string const &parameterName) {
  m_ties.erase(parameterName);
  m_fixed.erase(parameterName);
}

void FunctionProperties::clearTies() { m_ties.clear(); }

void FunctionProperties::fixParameterTo(std::string const &parameterName,
                                        double value) {
  m_ties[parameterName] = std::to_string(value);
  m_fixed.insert(parameterName);
}

void FunctionProperties::setConstraint(std::string const &parameterName,
                                       double lowerBound, double upperBound) {
  auto const constraintIt = findConstraintOf(parameterName);
  if (m_constraints.end() != constraintIt)
    constraintIt->second = BoundaryConstraint{lowerBound, upperBound};
  else
    m_constraints.emplace_back(parameterName,
                               BoundaryConstraint{lowerBound, upperBound});
}

void FunctionProperties::setLowerBound(std::string const &parameterName,
                                       double bound) {
  auto const constraintIt = findConstraintOf(parameterName);
  if (m_constraints.end() != constraintIt)
    constraintIt->second.lowerBound = bound;
  else
    m_constraints.emplace_back(parameterName,
                               BoundaryConstraint{bound, boost::none});
}

void FunctionProperties::setUpperBound(std::string const &parameterName,
                                       double bound) {
  auto const constraintIt = findConstraintOf(parameterName);
  if (m_constraints.end() != constraintIt)
    constraintIt->second.upperBound = bound;
  else
    m_constraints.emplace_back(parameterName,
                               BoundaryConstraint{boost::none, bound});
}

void FunctionProperties::removeLowerBound(std::string const &parameterName) {
  auto const constraintIt = findConstraintOf(parameterName);
  if (m_constraints.end() != constraintIt)
    constraintIt->second.lowerBound.reset();
}

void FunctionProperties::removeUpperBound(std::string const &parameterName) {
  auto const constraintIt = findConstraintOf(parameterName);
  if (m_constraints.end() != constraintIt)
    constraintIt->second.upperBound.reset();
}

void FunctionProperties::removeConstraints(std::string const &parameter) {
  auto const position = findConstraintIf(
      [&parameter](
          std::pair<std::string, BoundaryConstraint> const &constraint) {
        return constraint.first == parameter;
      });
  m_constraints.erase(position);
}

void FunctionProperties::clearConstraints() { m_constraints.clear(); }

typename std::vector<std::pair<std::string, BoundaryConstraint>>::const_iterator
FunctionProperties::findConstraintOf(std::string const &parameterName) const {
  return findConstraintIf(
      [&parameterName](
          std::pair<std::string, BoundaryConstraint> const &constraint) {
        return constraint.first == parameterName;
      });
}

typename std::vector<std::pair<std::string, BoundaryConstraint>>::iterator
FunctionProperties::findConstraintOf(std::string const &parameterName) {
  return findConstraintIf(
      [&parameterName](
          std::pair<std::string, BoundaryConstraint> const &constraint) {
        return constraint.first == parameterName;
      });
}

LocalFunctionProperties::LocalFunctionProperties() {}

LocalFunctionProperties::LocalFunctionProperties(MatrixWorkspace_sptr workspace,
                                                 std::size_t workspaceIndex)
    : m_dataset(Dataset{workspace, workspaceIndex}) {}

bool LocalFunctionProperties::hasDataset() const {
  return m_dataset.is_initialized();
}

MatrixWorkspace_sptr LocalFunctionProperties::getWorkspace() const {
  if (m_dataset)
    return m_dataset->workspace;
  return nullptr;
}

boost::optional<std::size_t>
LocalFunctionProperties::getWorkspaceIndex() const {
  if (m_dataset)
    return m_dataset->index;
  return boost::none;
}

Mantid::API::IFunction::Attribute const &
LocalFunctionProperties::getAttribute(std::string const &name) const {
  auto const attributeIt = m_attributes.find(name);
  if (m_attributes.end() != attributeIt)
    return attributeIt->second;
  throw std::runtime_error("No attribute exists with the name '" + name + "'.");
}

boost::optional<double> LocalFunctionProperties::getParameterValue(
    std::string const &parameterName) const {
  try {
    return getParameter(parameterName).value;
  } catch (std::runtime_error const &) {
    return boost::none;
  }
}

boost::optional<double> LocalFunctionProperties::getParameterError(
    std::string const &parameterName) const {
  return getParameter(parameterName).error;
}

void LocalFunctionProperties::removeParameter(
    std::string const &parameterName) {
  m_parameters.erase(parameterName);
}

void LocalFunctionProperties::setParameterValue(
    std::string const &parameterName, double value) {
  try {
    getParameter(parameterName).value = value;
  } catch (std::runtime_error const &) {
    m_parameters[parameterName] = ParameterValue(value);
  }
}

void LocalFunctionProperties::setParameterError(
    std::string const &parameterName, double error) {
  try {
    getParameter(parameterName).error = error;
  } catch (std::runtime_error const &) {
    m_parameters[parameterName] = ParameterValue(0, error);
  }
}

void LocalFunctionProperties::removeParameterError(
    std::string const &parameterName) {
  getParameter(parameterName).error.reset();
}

void LocalFunctionProperties::removeParameterErrors() {
  for (auto &parameter : m_parameters)
    parameter.second.error.reset();
}

void LocalFunctionProperties::setAttribute(
    std::string const &name,
    const Mantid::API::IFunction::Attribute &attribute) {
  m_attributes[name] = attribute;
}

void LocalFunctionProperties::resizeVectorAttribute(std::string const &name,
                                                    std::size_t size) {
  auto const attributeIt = m_attributes.find(name);
  if (attributeIt != m_attributes.end())
    m_attributes[name] = getResizedVectorAttribute(attributeIt->second, size);
  else
    m_attributes[name] = createVectorAttribute(size);
}

void LocalFunctionProperties::fixParameter(std::string const &parameterName) {
  FunctionProperties::fixParameterTo(parameterName,
                                     getParameter(parameterName).value);
}

ParameterValue const &
LocalFunctionProperties::getParameter(std::string const &parameterName) const {
  auto const parameterIt = m_parameters.find(parameterName);
  if (m_parameters.end() != parameterIt)
    return parameterIt->second;
  throw std::runtime_error("Parameter not found in function: " + parameterName);
}

ParameterValue &
LocalFunctionProperties::getParameter(std::string const &parameterName) {
  auto const parameterIt = m_parameters.find(parameterName);
  if (m_parameters.end() != parameterIt)
    return parameterIt->second;
  throw std::runtime_error("Parameter not found in function: " + parameterName);
}

} // namespace MantidWidgets
} // namespace MantidQt
