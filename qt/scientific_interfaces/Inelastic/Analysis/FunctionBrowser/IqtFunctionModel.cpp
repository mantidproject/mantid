// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtFunctionModel.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

#include <map>
#include <tuple>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

std::map<IqtFunctionModel::ParamID, std::string> g_paramName{
    {IqtFunctionModel::ParamID::EXP1_HEIGHT, "Height"},
    {IqtFunctionModel::ParamID::EXP1_LIFETIME, "Lifetime"},
    {IqtFunctionModel::ParamID::EXP2_HEIGHT, "Height"},
    {IqtFunctionModel::ParamID::EXP2_LIFETIME, "Lifetime"},
    {IqtFunctionModel::ParamID::STRETCH_HEIGHT, "Height"},
    {IqtFunctionModel::ParamID::STRETCH_LIFETIME, "Lifetime"},
    {IqtFunctionModel::ParamID::STRETCH_STRETCHING, "Stretching"},
    {IqtFunctionModel::ParamID::BG_A0, "A0"}};

std::tuple<double, double> calculateLifetimeAndHeight(Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  auto lifeTime = (x[1] - x[0]) / (log(y[0]) - log(y[1]));
  if (lifeTime <= 0)
    lifeTime = 1.0;
  auto const height = y[0] * exp(x[0] / lifeTime);
  return {lifeTime, height};
}

auto const expDecay = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  auto const [lifetime, height] = calculateLifetimeAndHeight(x, y);
  return std::unordered_map<std::string, double>{{"Height", height}, {"Lifetime", lifetime}};
};

auto const expDecayN = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  auto const [lifetime, height] = calculateLifetimeAndHeight(x, y);

  // Initialise small additional exp with 10% of amplitude and double the lifetime (if the lifetime is
  // too short it will correlate with any constant background)
  return std::unordered_map<std::string, double>{{"Height", 0.1 * height}, {"Lifetime", 2.0 * lifetime}};
};

auto const estimators = std::unordered_map<std::string, IDAFunctionParameterEstimation::ParameterEstimator>{
    {"ExpDecay", expDecay}, {"ExpDecayN", expDecayN}, {"StretchExp", expDecay}};

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

using namespace MantidWidgets;
using namespace Mantid::API;

IqtFunctionModel::IqtFunctionModel()
    : m_parameterEstimation(std::make_unique<IDAFunctionParameterEstimation>(estimators)) {}

void IqtFunctionModel::clearData() {
  m_numberOfExponentials = 0;
  m_hasStretchExponential = false;
  m_background.clear();
  m_model.clear();
}

void IqtFunctionModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    auto const name = fun->name();
    if (name == "ExpDecay") {
      m_numberOfExponentials = 1;
    } else if (name == "StretchExp") {
      m_hasStretchExponential = true;
    } else if (name == "FlatBackground") {
      m_background = name;
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
    m_model.setFunction(fun);
    return;
  }
  bool areExponentialsSet = false;
  bool isStretchSet = false;
  bool isBackgroundSet = false;
  for (size_t i = 0; i < fun->nFunctions(); ++i) {
    auto f = fun->getFunction(i);
    auto const name = f->name();
    if (name == "ExpDecay") {
      if (areExponentialsSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      if (m_numberOfExponentials == 0) {
        m_numberOfExponentials = 1;
      } else {
        m_numberOfExponentials = 2;
        areExponentialsSet = true;
      }
    } else if (name == "StretchExp") {
      if (isStretchSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_hasStretchExponential = true;
      areExponentialsSet = true;
      isStretchSet = true;
    } else if (name == "FlatBackground") {
      if (isBackgroundSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_background = name;
      areExponentialsSet = true;
      isStretchSet = true;
      isBackgroundSet = true;
    } else {
      clear();
      throw std::runtime_error("Function has wrong structure.");
    }
  }
  m_model.setFunction(fun);
}

IFunction_sptr IqtFunctionModel::getFullFunction() const { return m_model.getFullFunction(); }

IFunction_sptr IqtFunctionModel::getFitFunction() const { return m_model.getFitFunction(); }

bool IqtFunctionModel::hasFunction() const { return m_model.hasFunction(); }

void IqtFunctionModel::addFunction(std::string const &prefix, std::string const &funStr) {
  if (!prefix.empty())
    throw std::runtime_error("Function doesn't have member function with prefix " + prefix);
  auto fun = FunctionFactory::Instance().createInitialized(funStr);
  auto const name = fun->name();
  std::string newPrefix;
  if (name == "ExpDecay") {
    auto const ne = getNumberOfExponentials();
    if (ne > 1)
      throw std::runtime_error("Cannot add more exponentials.");
    setNumberOfExponentials(ne + 1);
    if (auto const exp2Prefix = getExp2Prefix()) {
      newPrefix = *exp2Prefix;
    } else {
      newPrefix = *getExp1Prefix();
    }
  } else if (name == "StretchExp") {
    if (hasStretchExponential())
      throw std::runtime_error("Cannot add more stretched exponentials.");
    setStretchExponential(true);
    newPrefix = *getStretchPrefix();
  } else if (name == "FlatBackground") {
    if (hasBackground())
      throw std::runtime_error("Cannot add more backgrounds.");
    setBackground(name);
    newPrefix = *getBackgroundPrefix();
  } else {
    throw std::runtime_error("Cannot add function " + name);
  }
  auto newFun = getFunctionWithPrefix(newPrefix, getSingleFunction(0));
  copyParametersAndErrors(*fun, *newFun);
  if (getNumberLocalFunctions() > 1) {
    copyParametersAndErrorsToAllLocalFunctions(*getSingleFunction(0));
  }
}

void IqtFunctionModel::removeFunction(std::string const &prefix) {
  if (prefix.empty()) {
    clear();
    return;
  }
  auto prefix1 = getExp1Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setNumberOfExponentials(0);
    return;
  }
  prefix1 = getExp2Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setNumberOfExponentials(1);
    return;
  }
  prefix1 = getStretchPrefix();
  if (prefix1 && *prefix1 == prefix) {
    setStretchExponential(false);
    return;
  }
  prefix1 = getBackgroundPrefix();
  if (prefix1 && *prefix1 == prefix) {
    removeBackground();
    return;
  }
  throw std::runtime_error("Function doesn't have member function with prefix " + prefix);
}

void IqtFunctionModel::setNumberOfExponentials(int n) {
  auto oldValues = getCurrentValues();
  m_numberOfExponentials = n;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
  estimateFunctionParameters();
}

int IqtFunctionModel::getNumberOfExponentials() const { return m_numberOfExponentials; }

void IqtFunctionModel::setStretchExponential(bool on) {
  auto oldValues = getCurrentValues();
  m_hasStretchExponential = on;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
  estimateFunctionParameters();
}

bool IqtFunctionModel::hasStretchExponential() const { return m_hasStretchExponential; }

void IqtFunctionModel::setBackground(std::string const &name) {
  auto oldValues = getCurrentValues();
  m_background = name;
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

void IqtFunctionModel::removeBackground() {
  auto oldValues = getCurrentValues();
  m_background.clear();
  m_model.setFunctionString(buildFunctionString());
  m_model.setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

bool IqtFunctionModel::hasBackground() const { return !m_background.empty(); }

void IqtFunctionModel::tieIntensities(bool on) {
  auto heightName = getParameterName(ParamID::STRETCH_HEIGHT);
  if (!heightName)
    heightName = getParameterName(ParamID::EXP1_HEIGHT);
  auto const a0Name = getParameterName(ParamID::BG_A0);
  if (!heightName || !a0Name)
    return;
  auto const tie = on ? "1-" + *a0Name : "";
  for (auto i = 0; i < getNumberDomains(); ++i) {
    setLocalParameterTie(*heightName, i, tie);
  }
}

EstimationDataSelector IqtFunctionModel::getEstimationDataSelector() const {
  return [](const Mantid::MantidVec &x, const Mantid::MantidVec &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    (void)range;
    size_t const n = 4;
    if (y.size() < n + 1)
      return DataForParameterEstimation{{}, {}};
    return DataForParameterEstimation{{x[0], x[n]}, {y[0], y[n]}};
  };
}

void IqtFunctionModel::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void IqtFunctionModel::estimateFunctionParameters() {
  m_parameterEstimation->estimateFunctionParameters(getFullFunction(), m_estimationData);
}

std::string IqtFunctionModel::setBackgroundA0(double value) {
  if (hasBackground()) {
    setParameter(ParamID::BG_A0, value);
    return *getParameterName(ParamID::BG_A0);
  }
  return "";
}

void IqtFunctionModel::setNumberDomains(int n) { m_model.setNumberDomains(n); }

int IqtFunctionModel::getNumberDomains() const { return m_model.getNumberDomains(); }

void IqtFunctionModel::setParameter(std::string const &parameterName, double value) {
  m_model.setParameter(parameterName, value);
}

void IqtFunctionModel::setParameterError(std::string const &parameterName, double value) {
  m_model.setParameterError(parameterName, value);
}

double IqtFunctionModel::getParameter(std::string const &parameterName) const {
  return m_model.getParameter(parameterName);
}

double IqtFunctionModel::getParameterError(std::string const &parameterName) const {
  return m_model.getParameterError(parameterName);
}

std::string IqtFunctionModel::getParameterDescription(std::string const &parameterName) const {
  return m_model.getParameterDescription(parameterName);
}

std::vector<std::string> IqtFunctionModel::getParameterNames() const { return m_model.getParameterNames(); }

IFunction_sptr IqtFunctionModel::getSingleFunction(int index) const { return m_model.getSingleFunction(index); }

IFunction_sptr IqtFunctionModel::getCurrentFunction() const { return m_model.getCurrentFunction(); }

std::vector<std::string> IqtFunctionModel::getGlobalParameters() const { return m_model.getGlobalParameters(); }

std::vector<std::string> IqtFunctionModel::getLocalParameters() const { return m_model.getLocalParameters(); }

void IqtFunctionModel::setGlobalParameters(std::vector<std::string> const &globals) {
  m_globals.clear();
  for (auto const &name : globals) {
    addGlobal(name);
  }
  auto newGlobals = makeGlobalList();
  m_model.setGlobalParameters(newGlobals);
}

bool IqtFunctionModel::isGlobal(std::string const &parameterName) const { return m_model.isGlobal(parameterName); }

void IqtFunctionModel::setGlobal(std::string const &parameterName, bool on) {
  if (parameterName.empty())
    return;
  if (on)
    addGlobal(parameterName);
  else
    removeGlobal(parameterName);
  auto globals = makeGlobalList();
  m_model.setGlobalParameters(globals);
}

void IqtFunctionModel::addGlobal(std::string const &parameterName) {
  auto const pid = getParameterId(parameterName);
  if (pid && !m_globals.contains(*pid)) {
    m_globals.push_back(*pid);
  }
}

void IqtFunctionModel::removeGlobal(std::string const &parameterName) {
  auto const pid = getParameterId(parameterName);
  if (pid && m_globals.contains(*pid)) {
    m_globals.removeOne(*pid);
  }
}

std::vector<std::string> IqtFunctionModel::makeGlobalList() const {
  std::vector<std::string> globals;
  for (auto const id : m_globals) {
    auto const name = getParameterName(id);
    if (name)
      globals.emplace_back(*name);
  }
  return globals;
}

void IqtFunctionModel::updateMultiDatasetParameters(const IFunction &fun) { m_model.updateMultiDatasetParameters(fun); }

void IqtFunctionModel::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  auto const nRows = paramTable.rowCount();
  if (nRows == 0)
    return;

  auto const globalParameterNames = getGlobalParameters();
  for (auto &&name : globalParameterNames) {
    auto valueColumn = paramTable.getColumn(name);
    auto errorColumn = paramTable.getColumn(name + "_Err");
    m_model.setParameter(name, valueColumn->toDouble(0));
    m_model.setParameterError(name, errorColumn->toDouble(0));
  }

  auto const localParameterNames = getLocalParameters();
  for (auto &&name : localParameterNames) {
    auto valueColumn = paramTable.getColumn(name);
    auto errorColumn = paramTable.getColumn(name + "_Err");
    if (nRows > 1) {
      for (size_t i = 0; i < nRows; ++i) {
        m_model.setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(i), errorColumn->toDouble(i));
      }
    } else {
      auto const i = m_model.currentDomainIndex();
      m_model.setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(0), errorColumn->toDouble(0));
    }
  }
}

void IqtFunctionModel::updateParameters(const IFunction &fun) { m_model.updateParameters(fun); }

void IqtFunctionModel::setCurrentDomainIndex(int i) { m_model.setCurrentDomainIndex(i); }

int IqtFunctionModel::currentDomainIndex() const { return m_model.currentDomainIndex(); }

void IqtFunctionModel::changeTie(std::string const &parameterName, std::string const &tie) {
  m_model.changeTie(parameterName, tie);
}

void IqtFunctionModel::addConstraint(std::string const &functionIndex, std::string const &constraint) {
  m_model.addConstraint(functionIndex, constraint);
}

void IqtFunctionModel::removeConstraint(std::string const &parameterName) { m_model.removeConstraint(parameterName); }

void IqtFunctionModel::setDatasets(const QList<FunctionModelDataset> &datasets) { m_model.setDatasets(datasets); }

QStringList IqtFunctionModel::getDatasetNames() const { return m_model.getDatasetNames(); }

QStringList IqtFunctionModel::getDatasetDomainNames() const { return m_model.getDatasetDomainNames(); }

double IqtFunctionModel::getLocalParameterValue(std::string const &parameterName, int i) const {
  return m_model.getLocalParameterValue(parameterName, i);
}

bool IqtFunctionModel::isLocalParameterFixed(std::string const &parameterName, int i) const {
  return m_model.isLocalParameterFixed(parameterName, i);
}

std::string IqtFunctionModel::getLocalParameterTie(std::string const &parameterName, int i) const {
  return m_model.getLocalParameterTie(parameterName, i);
}

std::string IqtFunctionModel::getLocalParameterConstraint(std::string const &parameterName, int i) const {
  return m_model.getLocalParameterConstraint(parameterName, i);
}

void IqtFunctionModel::setLocalParameterValue(std::string const &parameterName, int i, double value) {
  m_model.setLocalParameterValue(parameterName, i, value);
}

void IqtFunctionModel::setLocalParameterValue(std::string const &parameterName, int i, double value, double error) {
  m_model.setLocalParameterValue(parameterName, i, value, error);
}

void IqtFunctionModel::setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) {
  m_model.setLocalParameterTie(parameterName, i, tie);
}

void IqtFunctionModel::setLocalParameterConstraint(std::string const &parameterName, int i,
                                                   std::string const &constraint) {
  m_model.setLocalParameterConstraint(parameterName, i, constraint);
}

void IqtFunctionModel::setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) {
  m_model.setLocalParameterFixed(parameterName, i, fixed);
}

void IqtFunctionModel::setGlobalParameterValue(std::string const &parameterName, double value) {
  m_model.setGlobalParameterValue(parameterName, value);
}

void IqtFunctionModel::setParameter(ParamID name, double value) {
  auto const prefix = getPrefix(name);
  if (prefix) {
    m_model.setParameter(*prefix + g_paramName.at(name), value);
  }
}

boost::optional<double> IqtFunctionModel::getParameter(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameter(*paramName) : boost::optional<double>();
}

boost::optional<double> IqtFunctionModel::getParameterError(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterError(*paramName) : boost::optional<double>();
}

boost::optional<std::string> IqtFunctionModel::getParameterName(ParamID name) const {
  auto const prefix = getPrefix(name);
  return prefix ? *prefix + g_paramName.at(name) : boost::optional<std::string>();
}

boost::optional<std::string> IqtFunctionModel::getParameterDescription(ParamID name) const {
  auto const paramName = getParameterName(name);
  return paramName ? m_model.getParameterDescription(*paramName) : boost::optional<std::string>();
}

boost::optional<std::string> IqtFunctionModel::getPrefix(ParamID name) const {
  if (name <= ParamID::EXP1_LIFETIME) {
    return getExp1Prefix();
  } else if (name <= ParamID::EXP2_LIFETIME) {
    return getExp2Prefix();
  } else if (name <= ParamID::STRETCH_STRETCHING) {
    return getStretchPrefix();
  } else {
    return getBackgroundPrefix();
  }
}

QMap<IqtFunctionModel::ParamID, double> IqtFunctionModel::getCurrentValues() const {
  QMap<ParamID, double> values;
  auto store = [&values, this](ParamID name) { values[name] = *getParameter(name); };
  applyParameterFunction(store);
  return values;
}

QMap<IqtFunctionModel::ParamID, double> IqtFunctionModel::getCurrentErrors() const {
  QMap<ParamID, double> errors;
  auto store = [&errors, this](ParamID name) { errors[name] = *getParameterError(name); };
  applyParameterFunction(store);
  return errors;
}

QMap<int, std::string> IqtFunctionModel::getParameterNameMap() const {
  QMap<int, std::string> out;
  auto addToMap = [&out, this](ParamID name) { out[static_cast<int>(name)] = *getParameterName(name); };
  applyParameterFunction(addToMap);
  return out;
}

QMap<int, std::string> IqtFunctionModel::getParameterDescriptionMap() const {
  QMap<int, std::string> out;
  auto expDecay = FunctionFactory::Instance().createInitialized(buildExpDecayFunctionString());
  out[static_cast<int>(ParamID::EXP1_HEIGHT)] = expDecay->parameterDescription(0);
  out[static_cast<int>(ParamID::EXP1_LIFETIME)] = expDecay->parameterDescription(1);
  out[static_cast<int>(ParamID::EXP2_HEIGHT)] = expDecay->parameterDescription(0);
  out[static_cast<int>(ParamID::EXP2_LIFETIME)] = expDecay->parameterDescription(1);
  auto stretchExp = FunctionFactory::Instance().createInitialized(buildStretchExpFunctionString());
  out[static_cast<int>(ParamID::STRETCH_HEIGHT)] = stretchExp->parameterDescription(0);
  out[static_cast<int>(ParamID::STRETCH_LIFETIME)] = stretchExp->parameterDescription(1);
  out[static_cast<int>(ParamID::STRETCH_STRETCHING)] = stretchExp->parameterDescription(2);
  auto background = FunctionFactory::Instance().createInitialized(buildBackgroundFunctionString());
  out[static_cast<int>(ParamID::BG_A0)] = background->parameterDescription(0);
  return out;
}

void IqtFunctionModel::setCurrentValues(const QMap<ParamID, double> &values) {
  for (auto const name : values.keys()) {
    setParameter(name, values[name]);
  }
}

void IqtFunctionModel::applyParameterFunction(const std::function<void(ParamID)> &paramFun) const {
  if (m_numberOfExponentials > 0) {
    paramFun(ParamID::EXP1_HEIGHT);
    paramFun(ParamID::EXP1_LIFETIME);
  }
  if (m_numberOfExponentials > 1) {
    paramFun(ParamID::EXP2_HEIGHT);
    paramFun(ParamID::EXP2_LIFETIME);
  }
  if (m_hasStretchExponential) {
    paramFun(ParamID::STRETCH_HEIGHT);
    paramFun(ParamID::STRETCH_LIFETIME);
    paramFun(ParamID::STRETCH_STRETCHING);
  }
  if (!m_background.empty()) {
    paramFun(ParamID::BG_A0);
  }
}

boost::optional<IqtFunctionModel::ParamID> IqtFunctionModel::getParameterId(std::string const &parameterName) {
  boost::optional<ParamID> result;
  auto getter = [&result, parameterName, this](ParamID pid) {
    if (parameterName == *getParameterName(pid))
      result = pid;
  };
  applyParameterFunction(getter);
  return result;
}

std::string IqtFunctionModel::buildExpDecayFunctionString() const {
  return "name=ExpDecay,Height=1,Lifetime=1,constraints=(Height>0,Lifetime>0)";
}

std::string IqtFunctionModel::buildStretchExpFunctionString() const {
  return "name=StretchExp,Height=1,Lifetime=1,Stretching=1,constraints=(Height>"
         "0,Lifetime>0,0<Stretching<1.001)";
}

std::string IqtFunctionModel::buildBackgroundFunctionString() const {
  return "name=FlatBackground,A0=0,constraints=(A0>0)";
}

std::string IqtFunctionModel::buildFunctionString() const {
  QStringList functions;
  if (m_numberOfExponentials > 0) {
    functions << QString::fromStdString(buildExpDecayFunctionString());
  }
  if (m_numberOfExponentials > 1) {
    functions << QString::fromStdString(buildExpDecayFunctionString());
  }
  if (m_hasStretchExponential) {
    functions << QString::fromStdString(buildStretchExpFunctionString());
  }
  if (!m_background.empty()) {
    functions << QString::fromStdString(buildBackgroundFunctionString());
  }
  return functions.join(";").toStdString();
}

boost::optional<std::string> IqtFunctionModel::getExp1Prefix() const {
  if (m_numberOfExponentials == 0)
    return boost::optional<std::string>();
  if (m_numberOfExponentials == 1 && !m_hasStretchExponential && m_background.empty())
    return std::string("");
  return std::string("f0.");
}

boost::optional<std::string> IqtFunctionModel::getExp2Prefix() const {
  if (m_numberOfExponentials < 2)
    return boost::optional<std::string>();
  return std::string("f1.");
}

boost::optional<std::string> IqtFunctionModel::getStretchPrefix() const {
  if (!m_hasStretchExponential)
    return boost::optional<std::string>();
  if (m_numberOfExponentials == 0 && m_background.empty())
    return std::string("");
  return "f" + std::to_string(m_numberOfExponentials) + ".";
}

boost::optional<std::string> IqtFunctionModel::getBackgroundPrefix() const {
  if (m_background.empty())
    return boost::optional<std::string>();
  if (m_numberOfExponentials == 0 && !m_hasStretchExponential)
    return std::string("");
  return "f" + std::to_string(m_numberOfExponentials + (m_hasStretchExponential ? 1 : 0)) + ".";
}

} // namespace MantidQt::CustomInterfaces::IDA
