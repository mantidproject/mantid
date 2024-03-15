// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtFunctionTemplateModel.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

#include <map>
#include <tuple>

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

double constexpr EPSILON = std::numeric_limits<double>::epsilon();

std::tuple<double, double> calculateLifetimeAndHeight(Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  auto const logY0 = log(y[0]);
  auto const logY1 = log(y[1]);
  auto lifeTime = std::abs(logY0 - logY1) > EPSILON ? (x[1] - x[0]) / (logY0 - logY1) : 1.0;
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

IqtFunctionTemplateModel::IqtFunctionTemplateModel()
    : MultiFunctionTemplateModel(std::make_unique<FunctionModel>(),
                                 std::make_unique<IDAFunctionParameterEstimation>(estimators)) {}

void IqtFunctionTemplateModel::clearData() {
  m_exponentialType = ExponentialType::None;
  m_stretchExpType = StretchExpType::None;
  m_backgroundType = BackgroundType::None;
  m_tieIntensitiesType = TieIntensitiesType::False;

  m_model->clear();
}

void IqtFunctionTemplateModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    auto const name = fun->name();
    if (name == "ExpDecay") {
      m_exponentialType = ExponentialType::OneExponential;
    } else if (name == "StretchExp") {
      m_stretchExpType = StretchExpType::StretchExponential;
    } else if (name == "FlatBackground") {
      m_backgroundType = BackgroundType::Flat;
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
    m_model->setFunction(fun);
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
      if (m_exponentialType == ExponentialType::None) {
        m_exponentialType = ExponentialType::OneExponential;
      } else {
        m_exponentialType = ExponentialType::TwoExponentials;
        areExponentialsSet = true;
      }
    } else if (name == "StretchExp") {
      if (isStretchSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_stretchExpType = StretchExpType::StretchExponential;
      areExponentialsSet = true;
      isStretchSet = true;
    } else if (name == "FlatBackground") {
      if (isBackgroundSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_backgroundType = BackgroundType::Flat;
      areExponentialsSet = true;
      isStretchSet = true;
      isBackgroundSet = true;
    } else {
      clear();
      throw std::runtime_error("Function has wrong structure.");
    }
  }
  m_model->setFunction(fun);
}

void IqtFunctionTemplateModel::addFunction(std::string const &prefix, std::string const &funStr) {
  if (!prefix.empty())
    throw std::runtime_error("Function doesn't have member function with prefix " + prefix);
  auto fun = FunctionFactory::Instance().createInitialized(funStr);
  auto const name = fun->name();
  std::string newPrefix;
  if (name == "ExpDecay") {
    if (numberOfExponentials() > 1)
      throw std::runtime_error("Cannot add more exponentials.");
    m_exponentialType =
        m_exponentialType == ExponentialType::None ? ExponentialType::OneExponential : ExponentialType::TwoExponentials;
    if (auto const exp2Prefix = getExp2Prefix()) {
      newPrefix = *exp2Prefix;
    } else {
      newPrefix = *getExp1Prefix();
    }
  } else if (name == "StretchExp") {
    if (hasStretchExponential())
      throw std::runtime_error("Cannot add more stretched exponentials.");
    m_stretchExpType = StretchExpType::StretchExponential;
    newPrefix = *getStretchPrefix();
  } else if (name == "FlatBackground") {
    if (hasBackground())
      throw std::runtime_error("Cannot add more backgrounds.");
    m_backgroundType = BackgroundType::Flat;
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

void IqtFunctionTemplateModel::setSubType(std::size_t subTypeIndex, int typeIndex) {
  auto oldValues = getCurrentValues();

  switch (IqtTypes::SubTypeIndex(subTypeIndex)) {
  case IqtTypes::SubTypeIndex::Exponential:
    m_exponentialType = static_cast<IqtTypes::ExponentialType>(typeIndex);
    break;
  case IqtTypes::SubTypeIndex::StretchExponential:
    m_stretchExpType = static_cast<IqtTypes::StretchExpType>(typeIndex);
    break;
  case IqtTypes::SubTypeIndex::Background:
    m_backgroundType = static_cast<IqtTypes::BackgroundType>(typeIndex);
    break;
  case IqtTypes::SubTypeIndex::TieIntensities:
    m_tieIntensitiesType = static_cast<IqtTypes::TieIntensitiesType>(typeIndex);
    break;
  default:
    throw std::logic_error("A matching IqtTypes::SubTypeIndex could not be found.");
  }

  m_model->setFunctionString(buildFunctionString());
  m_model->setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

void IqtFunctionTemplateModel::removeFunction(std::string const &prefix) {
  if (prefix.empty()) {
    clear();
    return;
  }
  auto prefix1 = getExp1Prefix();
  if (prefix1 && *prefix1 == prefix) {
    m_exponentialType = ExponentialType::None;
    return;
  }
  prefix1 = getExp2Prefix();
  if (prefix1 && *prefix1 == prefix) {
    m_exponentialType = ExponentialType::OneExponential;
    return;
  }
  prefix1 = getStretchPrefix();
  if (prefix1 && *prefix1 == prefix) {
    m_stretchExpType = StretchExpType::None;
    return;
  }
  prefix1 = getBackgroundPrefix();
  if (prefix1 && *prefix1 == prefix) {
    removeBackground();
    return;
  }
  throw std::runtime_error("Function doesn't have member function with prefix " + prefix);
}

int IqtFunctionTemplateModel::numberOfExponentials() const { return static_cast<int>(m_exponentialType); }

bool IqtFunctionTemplateModel::hasExponential() const { return m_exponentialType != ExponentialType::None; }

bool IqtFunctionTemplateModel::hasStretchExponential() const { return m_stretchExpType != StretchExpType::None; }

void IqtFunctionTemplateModel::removeBackground() {
  auto oldValues = getCurrentValues();
  m_backgroundType = BackgroundType::None;
  m_model->setFunctionString(buildFunctionString());
  m_model->setGlobalParameters(makeGlobalList());
  setCurrentValues(oldValues);
}

bool IqtFunctionTemplateModel::hasBackground() const { return m_backgroundType != BackgroundType::None; }

void IqtFunctionTemplateModel::tieIntensities(bool on) {
  auto heightName = getParameterName(ParamID::STRETCH_HEIGHT);
  if (!heightName)
    heightName = getParameterName(ParamID::EXP1_HEIGHT);
  auto const a0Name = getParameterName(ParamID::FLAT_BG_A0);
  if (!heightName || !a0Name)
    return;
  auto const tie = on ? "1-" + *a0Name : "";
  for (auto i = 0; i < getNumberDomains(); ++i) {
    setLocalParameterTie(*heightName, i, tie);
  }
}

EstimationDataSelector IqtFunctionTemplateModel::getEstimationDataSelector() const {
  return [](const Mantid::MantidVec &x, const Mantid::MantidVec &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    (void)range;
    size_t const n = 4;
    if (y.size() < n + 1)
      return DataForParameterEstimation{{}, {}};
    return DataForParameterEstimation{{x[0], x[n]}, {y[0], y[n]}};
  };
}

std::string IqtFunctionTemplateModel::setBackgroundA0(double value) {
  if (hasBackground()) {
    setParameter(ParamID::FLAT_BG_A0, value);
    return *getParameterName(ParamID::FLAT_BG_A0);
  }
  return "";
}

void IqtFunctionTemplateModel::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  (void)fitResolutions;
}

void IqtFunctionTemplateModel::setQValues(const std::vector<double> &qValues) { (void)qValues; }

std::optional<std::string> IqtFunctionTemplateModel::getPrefix(ParamID name) const {
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

void IqtFunctionTemplateModel::applyParameterFunction(const std::function<void(ParamID)> &paramFun) const {
  if (hasExponential()) {
    paramFun(ParamID::EXP1_HEIGHT);
    paramFun(ParamID::EXP1_LIFETIME);
  }
  if (numberOfExponentials() > 1) {
    paramFun(ParamID::EXP2_HEIGHT);
    paramFun(ParamID::EXP2_LIFETIME);
  }
  if (hasStretchExponential()) {
    paramFun(ParamID::STRETCH_HEIGHT);
    paramFun(ParamID::STRETCH_LIFETIME);
    paramFun(ParamID::STRETCH_STRETCHING);
  }
  if (hasBackground()) {
    paramFun(ParamID::FLAT_BG_A0);
  }
}

std::string IqtFunctionTemplateModel::buildExpDecayFunctionString() const {
  return "name=ExpDecay,Height=1,Lifetime=1,constraints=(Height>0,Lifetime>0)";
}

std::string IqtFunctionTemplateModel::buildStretchExpFunctionString() const {
  return "name=StretchExp,Height=1,Lifetime=1,Stretching=1,constraints=(Height>"
         "0,Lifetime>0,0<Stretching<1.001)";
}

std::string IqtFunctionTemplateModel::buildBackgroundFunctionString() const {
  return "name=FlatBackground,A0=0,constraints=(A0>0)";
}

std::string IqtFunctionTemplateModel::buildFunctionString() const {
  QStringList functions;
  if (hasExponential()) {
    functions << QString::fromStdString(buildExpDecayFunctionString());
  }
  if (numberOfExponentials() > 1) {
    functions << QString::fromStdString(buildExpDecayFunctionString());
  }
  if (hasStretchExponential()) {
    functions << QString::fromStdString(buildStretchExpFunctionString());
  }
  if (hasBackground()) {
    functions << QString::fromStdString(buildBackgroundFunctionString());
  }
  return functions.join(";").toStdString();
}

std::optional<std::string> IqtFunctionTemplateModel::getExp1Prefix() const {
  if (!hasExponential())
    return std::optional<std::string>();
  if (numberOfExponentials() == 1 && !hasStretchExponential() && !hasBackground())
    return std::string("");
  return std::string("f0.");
}

std::optional<std::string> IqtFunctionTemplateModel::getExp2Prefix() const {
  if (numberOfExponentials() < 2)
    return std::optional<std::string>();
  return std::string("f1.");
}

std::optional<std::string> IqtFunctionTemplateModel::getStretchPrefix() const {
  if (!hasStretchExponential())
    return std::optional<std::string>();
  if (!hasExponential() && !hasBackground())
    return std::string("");
  return "f" + std::to_string(numberOfExponentials()) + ".";
}

std::optional<std::string> IqtFunctionTemplateModel::getBackgroundPrefix() const {
  if (!hasBackground())
    return std::optional<std::string>();
  if (!hasExponential() && !hasStretchExponential())
    return std::string("");
  return "f" + std::to_string(numberOfExponentials() + (hasStretchExponential() ? 1 : 0)) + ".";
}

} // namespace MantidQt::CustomInterfaces::IDA
