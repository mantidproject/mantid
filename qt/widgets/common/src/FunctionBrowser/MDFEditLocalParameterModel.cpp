#include "MDFEditLocalParameterModel.h"
#include "MDFLogValueFinder.h"
#include "MultiDomainFunctionModel.h"

#include "MantidKernel/Statistics.h"

#include <algorithm>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/lexical_cast.hpp>

namespace {

using MantidQt::MantidWidgets::MultiDomainFunctionModel;
using MantidQt::MantidWidgets::ParameterValue;
using Mantid::Kernel::Math::StatisticType;

std::unordered_map<std::string, StatisticType> const STRING_TO_FUNCTION = {
    {"Mean", StatisticType::Mean},
    {"Min", StatisticType::Minimum},
    {"Max", StatisticType::Maximum},
    {"First", StatisticType::FirstValue},
    {"Last", StatisticType::LastValue}};

template <typename BeginIt, typename EndIt>
std::string join(BeginIt beginIt, EndIt endIt, std::string const &delimiter) {
  std::string joined = *beginIt;
  for (auto it = beginIt + 1; it < endIt; ++it)
    joined += delimiter + *it;
  return joined;
}

template <typename BeginIt, typename EndIt, typename F>
std::string join(BeginIt beginIt, EndIt endIt, std::string const &delimiter,
                 F const &toString) {
  return join(boost::make_transform_iterator(beginIt, toString),
              boost::make_transform_iterator(endIt, toString), delimiter);
}

template <typename F>
void forEachDomainInModel(MultiDomainFunctionModel &model, F const &functor) {
  auto const startDomain = model.getActiveDomain();
  for (auto i = 0u; i < model.numberOfDomains(); ++i) {
    model.setActiveDomain(i);
    functor();
  }
  model.setActiveDomain(startDomain);
}

void extractParameterProperties(MultiDomainFunctionModel &model,
                                std::string const &parameter,
                                std::vector<double> &values,
                                std::vector<std::string> &ties,
                                std::vector<bool> &fixes) {
  forEachDomainInModel(model, [&]() {
    values.emplace_back(model.parameterValue(parameter));
    ties.emplace_back(model.parameterTie(parameter).value_or(""));
    fixes.emplace_back(model.isParameterFixed(parameter));
  });
}

std::vector<std::string>
extractWorkspaceNames(MultiDomainFunctionModel &model) {
  std::vector<std::string> workspaceNames;
  workspaceNames.reserve(model.numberOfDomains());
  forEachDomainInModel(
      model, [&]() { workspaceNames.emplace_back(model.getWorkspaceName()); });
  return workspaceNames;
}

std::vector<std::size_t>
extractWorkspaceIndices(MultiDomainFunctionModel &model) {
  std::vector<std::size_t> workspaceIndices;
  workspaceIndices.reserve(model.numberOfDomains());
  forEachDomainInModel(model, [&]() {
    workspaceIndices.emplace_back(*model.getWorkspaceIndex());
  });
  return workspaceIndices;
}

void setParameterValues(MultiDomainFunctionModel &model,
                        std::string const &parameter,
                        std::vector<double>::const_iterator valueIterator) {
  forEachDomainInModel(model, [&]() {
    model.setLocalParameterValue(parameter, *valueIterator);
    ++valueIterator;
  });
}

void setParameterValues(MultiDomainFunctionModel &model,
                        std::string const &parameter,
                        std::vector<double> const &values) {
  setParameterValues(model, parameter, values.begin());
}

void setParameterTie(MultiDomainFunctionModel &model,
                     std::string const &parameter, std::string const &tie,
                     bool fixed) {
  if (fixed)
    model.fixLocalParameter(parameter);
  else if (!tie.empty())
    model.setLocalTie(parameter, tie);
  else if (model.isParameterTied(parameter))
    model.removeLocalTie(parameter);
}

void setParameterTies(MultiDomainFunctionModel &model,
                      std::string const &parameter,
                      std::vector<std::string>::const_iterator tieIterator,
                      std::vector<bool>::const_iterator fixedIterator) {
  forEachDomainInModel(model, [&]() {
    setParameterTie(model, parameter, *tieIterator, *fixedIterator);
    ++tieIterator;
    ++fixedIterator;
  });
}

void setParameterTies(MultiDomainFunctionModel &model,
                      std::string const &parameter,
                      std::vector<std::string> const &ties,
                      std::vector<bool> const &fixed) {
  setParameterTies(model, parameter, ties.begin(), fixed.begin());
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

EditLocalParameterModel::EditLocalParameterModel(
    MantidWidgets::MultiDomainFunctionModel &model,
    std::string const &parameter)
    : m_parameter(parameter), m_workspaceNames(extractWorkspaceNames(model)),
      m_workspaceIndices(extractWorkspaceIndices(model)),
      m_logFinder(m_workspaceNames) {
  m_values.reserve(model.numberOfDomains());
  m_ties.reserve(model.numberOfDomains());
  m_fixed.reserve(model.numberOfDomains());
  extractParameterProperties(model, parameter, m_values, m_ties, m_fixed);
}

EditLocalParameterModel::EditLocalParameterModel(
    std::string const &parameter, std::vector<double> const &values,
    std::vector<std::string> const &ties, std::vector<bool> const &fixes,
    std::vector<std::string> const &workspaceNames)
    : m_parameter(parameter), m_values(values), m_ties(ties), m_fixed(fixes),
      m_logFinder(workspaceNames) {}

std::string const &EditLocalParameterModel::getParameterName() const {
  return m_parameter;
}

std::vector<std::string> const &
EditLocalParameterModel::getWorkspaceNames() const {
  return m_workspaceNames;
}

std::vector<std::size_t> const &
EditLocalParameterModel::getWorkspaceIndices() const {
  return m_workspaceIndices;
}

std::size_t EditLocalParameterModel::numberOfParameters() const {
  return m_values.size();
}

double EditLocalParameterModel::getParameterValue(std::size_t index) const {
  return m_values[index];
}

std::string EditLocalParameterModel::getTie(std::size_t index) const {
  return m_ties[index];
}

std::string EditLocalParameterModel::getDelimitedParameters(
    std::string const &delimiter) const {
  return join(m_values.begin(), m_values.end(), delimiter, [](double value) {
    return boost::lexical_cast<std::string>(value);
  });
}

std::vector<std::string> EditLocalParameterModel::getLogNames() const {
  return m_logFinder.getLogNames();
}

bool EditLocalParameterModel::isFixed(std::size_t index) const {
  return m_fixed[index];
}

bool EditLocalParameterModel::isTied(std::size_t index) const {
  return !m_ties[index].empty();
}

void EditLocalParameterModel::setParameters(double value) {
  std::fill(m_values.begin(), m_values.end(), value);
}

void EditLocalParameterModel::setFixed(bool fixed) {
  std::fill(m_fixed.begin(), m_fixed.end(), fixed);
}

void EditLocalParameterModel::setTies(std::string const &tie) {
  std::fill(m_ties.begin(), m_ties.end(), tie);
}

void EditLocalParameterModel::setParameter(double value, std::size_t index) {
  m_values[index] = value;
}

void EditLocalParameterModel::fixParameter(bool fixed, std::size_t index) {
  m_fixed[index] = fixed;
}

void EditLocalParameterModel::setTie(std::string const &tie,
                                     std::size_t index) {
  m_ties[index] = tie;
}

void EditLocalParameterModel::setValuesToLog(std::string const &logName,
                                             std::string const &function) {
  for (auto i = 0u; i < m_values.size(); ++i)
    setValueToLog(logName, function, i);
}

void EditLocalParameterModel::setValueToLog(std::string const &logName,
                                            std::string const &function,
                                            std::size_t index) {
  try {
    m_values[index] = m_logFinder.getLogValue(
        logName, STRING_TO_FUNCTION.at(function), index);
  } catch (std::invalid_argument const &err) {
    m_values[index] = std::numeric_limits<double>::quiet_NaN();
    throw std::runtime_error(std::string("Failed to get log value:\n\n") +
                             err.what());
  }
}

void EditLocalParameterModel::updateFunctionModel(
    MantidWidgets::MultiDomainFunctionModel &functionModel) {
  setParameterValues(functionModel, m_parameter, m_values);
  setParameterTies(functionModel, m_parameter, m_ties, m_fixed);
}

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt
