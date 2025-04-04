// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFunctionTemplateModel.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/ConvolutionFunctionModel.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

#include <map>

namespace {
using namespace MantidQt::CustomInterfaces::Inelastic;

double constexpr DEFAULT_TEMPERATURE_CORRECTION = 100.0;

auto const lorentzian = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  return std::unordered_map<std::string, double>{{"Amplitude", y[1]}, {"FWHM", 2.0 * std::abs(x[1] - x[0])}};
};

auto const sqeFunction = [](Mantid::MantidVec const &x, Mantid::MantidVec const &y) {
  (void)x;
  return std::unordered_map<std::string, double>{{"Height", y[1]}};
};

auto const estimators = std::unordered_map<std::string, FunctionParameterEstimation::ParameterEstimator>{
    {"Lorentzian", lorentzian},        {"LorentzianN", lorentzian},       {"TeixeiraWaterSQE", sqeFunction},
    {"FickDiffusionSQE", sqeFunction}, {"ChudleyElliotSQE", sqeFunction}, {"HallRossSQE", sqeFunction}};

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

using namespace Mantid::API;

ConvFunctionTemplateModel::ConvFunctionTemplateModel()
    : MultiFunctionTemplateModel(std::make_unique<ConvolutionFunctionModel>(),
                                 std::make_unique<FunctionParameterEstimation>(estimators)) {}

ConvolutionFunctionModel *ConvFunctionTemplateModel::model() const {
  return dynamic_cast<ConvolutionFunctionModel *>(m_model.get());
}

void ConvFunctionTemplateModel::clearData() {
  m_lorentzianType = LorentzianType::None;
  m_fitType = FitType::None;
  m_deltaType = DeltaType::None;
  m_tempCorrectionType = TempCorrectionType::None;
  m_backgroundType = BackgroundType::None;
  m_tiePeakCentresType = TiePeakCentresType::False;

  m_model->clear();
}

void ConvFunctionTemplateModel::setModel() {
  model()->setModel(buildBackgroundFunctionString(), m_fitResolutions, buildLorentzianPeaksString(),
                    buildFitTypeString(), hasDeltaFunction(), m_qValues, m_isQDependentFunction, hasTempCorrection(),
                    DEFAULT_TEMPERATURE_CORRECTION);
  if (hasTempCorrection() && !m_globals.contains(ParamID::TEMPERATURE)) {
    m_globals.push_back(ParamID::TEMPERATURE);
  }
  m_model->setGlobalParameters(makeGlobalList());
  tiePeakCentres();
  estimateFunctionParameters();
}

void ConvFunctionTemplateModel::setFunction(IFunction_sptr fun) {
  clearData();
  if (!fun)
    return;
  if (fun->name() == "Convolution") {
    checkConvolution(fun);
  } else if (fun->name() == "CompositeFunction") {
    bool isBackgroundSet = false;
    for (size_t i = 0; i < fun->nFunctions(); ++i) {
      auto innerFunction = fun->getFunction(i);
      auto const name = innerFunction->name();
      if (name == "FlatBackground") {
        if (isBackgroundSet) {
          throw std::runtime_error("Function has wrong structure.");
        }
        m_backgroundType = BackgroundType::Flat;
        isBackgroundSet = true;
      } else if (name == "LinearBackground") {
        if (isBackgroundSet) {
          throw std::runtime_error("Function has wrong structure.");
        }
        m_backgroundType = BackgroundType::Linear;
        isBackgroundSet = true;
      } else if (name == "Convolution") {
        checkConvolution(innerFunction);
      }
    }
  }
  m_model->setFunction(fun);
}

void ConvFunctionTemplateModel::checkConvolution(const IFunction_sptr &fun) {
  bool isFitTypeSet = false;
  bool isResolutionSet = false;
  bool isLorentzianTypeSet = false;
  for (size_t i = 0; i < fun->nFunctions(); ++i) {
    auto innerFunction = fun->getFunction(i);
    auto const name = innerFunction->name();
    if (name == "Resolution") {
      if (isResolutionSet) {
        throw std::runtime_error("Function has wrong structure.");
      }
      isResolutionSet = true;
    } else if (name == "ProductFunction") {
      if (innerFunction->getFunction(0)->name() != "ConvTempCorrection" ||
          innerFunction->getFunction(0)->nParams() != 1 ||
          !innerFunction->getFunction(0)->hasParameter("Temperature")) {
        throw std::runtime_error("Function has wrong structure.");
      }
      m_tempCorrectionType = ConvTypes::TempCorrectionType::Exponential;
      if (std::dynamic_pointer_cast<CompositeFunction>(innerFunction->getFunction(1)))
        checkConvolution(innerFunction->getFunction(1));
      else
        checkSingleFunction(innerFunction->getFunction(1), isLorentzianTypeSet, isFitTypeSet);
    }

    else if (name == "CompositeFunction") {
      checkConvolution(innerFunction);
    } else {
      checkSingleFunction(innerFunction, isLorentzianTypeSet, isFitTypeSet);
    }
  }
}

void ConvFunctionTemplateModel::checkSingleFunction(const IFunction_sptr &fun, bool &isLorentzianTypeSet,
                                                    bool &isFitTypeSet) {
  auto const name = fun->name();
  if (name == "Lorentzian") {
    if (isLorentzianTypeSet && m_lorentzianType != LorentzianType::OneLorentzian) {
      throw std::runtime_error("Function has wrong structure.");
    }
    if (isLorentzianTypeSet)
      m_lorentzianType = LorentzianType::TwoLorentzians;
    else
      m_lorentzianType = LorentzianType::OneLorentzian;
    isLorentzianTypeSet = true;
  }

  if (FitTypeStringToEnum.count(name) == 1) {
    if (isFitTypeSet) {
      throw std::runtime_error("Function has wrong structure. More than one fit type set");
    }
    m_fitType = FitTypeStringToEnum[name];
    m_isQDependentFunction = FitTypeQDepends[m_fitType];
    isFitTypeSet = true;
  } else if (name == "DeltaFunction") {
    m_deltaType = ConvTypes::DeltaType::Delta;
  } else if (!isFitTypeSet && !isLorentzianTypeSet) {
    clear();
    throw std::runtime_error("Function has wrong structure. Function not recognized");
  }
}

void ConvFunctionTemplateModel::addFunction(std::string const &prefix, std::string const &funStr) {
  if (!prefix.empty())
    throw std::runtime_error("Function doesn't have member function with prefix " + prefix);
  auto fun = FunctionFactory::Instance().createInitialized(funStr);
  auto const name = fun->name();
  std::string newPrefix;
  if (name == "Lorentzian") {
    if (m_lorentzianType == LorentzianType::TwoLorentzians) {
      throw std::runtime_error("Cannot add more Lorentzians.");
    } else if (m_lorentzianType == LorentzianType::OneLorentzian) {
      m_lorentzianType = LorentzianType::TwoLorentzians;
      newPrefix = *getLor2Prefix();
    } else if (m_lorentzianType == LorentzianType::None) {
      m_lorentzianType = LorentzianType::OneLorentzian;
      newPrefix = *getLor1Prefix();
    } else {
      throw std::runtime_error("Cannot add more Lorentzians.");
    }
  } else if (name == "DeltaFunction") {
    if (hasDeltaFunction())
      throw std::runtime_error("Cannot add a DeltaFunction.");
    setSubType(ConvTypes::SubTypeIndex::Delta, static_cast<int>(DeltaType::Delta));
    newPrefix = *getDeltaPrefix();
  } else if (name == "FlatBackground" || name == "LinearBackground") {
    if (hasBackground())
      throw std::runtime_error("Cannot add more backgrounds.");
    if (name == "FlatBackground") {
      setSubType(ConvTypes::SubTypeIndex::Background, static_cast<int>(BackgroundType::Flat));
    } else if (name == "LinearBackground") {
      setSubType(ConvTypes::SubTypeIndex::Background, static_cast<int>(BackgroundType::Linear));
    }
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

void ConvFunctionTemplateModel::removeFunction(std::string const &prefix) {
  if (prefix.empty()) {
    clear();
    return;
  }
  auto prefix1 = getLor1Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setSubType(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(LorentzianType::None));
    return;
  }
  prefix1 = getLor2Prefix();
  if (prefix1 && *prefix1 == prefix) {
    setSubType(ConvTypes::SubTypeIndex::Lorentzian, static_cast<int>(LorentzianType::OneLorentzian));
    return;
  }
  prefix1 = getDeltaPrefix();
  if (prefix1 && *prefix1 == prefix) {
    setSubType(ConvTypes::SubTypeIndex::Delta, static_cast<int>(DeltaType::None));
    return;
  }
  prefix1 = getBackgroundPrefix();
  if (prefix1 && *prefix1 == prefix) {
    removeBackground();
    return;
  }
  throw std::runtime_error("Function doesn't have member function with prefix " + prefix);
}

bool ConvFunctionTemplateModel::hasTempCorrection() const { return m_tempCorrectionType != TempCorrectionType::None; }

bool ConvFunctionTemplateModel::hasDeltaFunction() const { return m_deltaType != DeltaType::None; }

void ConvFunctionTemplateModel::removeBackground() {
  auto oldValues = getCurrentValues();
  m_backgroundType = BackgroundType::None;
  setModel();
  setCurrentValues(oldValues);
}

bool ConvFunctionTemplateModel::hasBackground() const { return m_backgroundType != BackgroundType::None; }

EstimationDataSelector ConvFunctionTemplateModel::getEstimationDataSelector() const {
  return [](const Mantid::MantidVec &x, const Mantid::MantidVec &y,
            const std::pair<double, double> &range) -> DataForParameterEstimation {
    (void)range;

    auto const maxElement = std::max_element(y.cbegin(), y.cend());
    auto const halfMaxElement =
        std::find_if(y.cbegin(), y.cend(), [&maxElement](double const val) { return val > *maxElement / 2.0; });
    if (maxElement == y.cend() || halfMaxElement == y.cend())
      return DataForParameterEstimation{{}, {}};

    auto const maxElementIndex = std::distance(y.cbegin(), maxElement);
    auto const halfMaxElementIndex = std::distance(y.cbegin(), halfMaxElement);

    return DataForParameterEstimation{{x[halfMaxElementIndex], x[maxElementIndex]}, {*halfMaxElement, *maxElement}};
  };
}

void ConvFunctionTemplateModel::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_fitResolutions = fitResolutions;
  setModel();
}

std::string ConvFunctionTemplateModel::setBackgroundA0(double value) {
  if (hasBackground()) {
    auto const paramID = (m_backgroundType == BackgroundType::Flat) ? ParamID::FLAT_BG_A0 : ParamID::LINEAR_BG_A0;
    setParameter(paramID, value);
    if (auto const paramName = getParameterName(paramID))
      return *paramName;
  }
  return "";
}

void ConvFunctionTemplateModel::setSubType(std::size_t subTypeIndex, int typeIndex) {
  auto oldValues = getCurrentValues();

  switch (ConvTypes::SubTypeIndex(subTypeIndex)) {
  case ConvTypes::SubTypeIndex::Fit:
    m_fitType = static_cast<ConvTypes::FitType>(typeIndex);
    m_isQDependentFunction = FitTypeQDepends[m_fitType];
    break;
  case ConvTypes::SubTypeIndex::Lorentzian:
    m_lorentzianType = static_cast<ConvTypes::LorentzianType>(typeIndex);
    break;
  case ConvTypes::SubTypeIndex::Delta:
    m_deltaType = static_cast<ConvTypes::DeltaType>(typeIndex);
    break;
  case ConvTypes::SubTypeIndex::TempCorrection:
    m_tempCorrectionType = static_cast<ConvTypes::TempCorrectionType>(typeIndex);
    break;
  case ConvTypes::SubTypeIndex::Background:
    m_backgroundType = static_cast<ConvTypes::BackgroundType>(typeIndex);
    break;
  case ConvTypes::SubTypeIndex::TiePeakCentres:
    m_tiePeakCentresType = static_cast<ConvTypes::TiePeakCentresType>(typeIndex);
    break;
  default:
    throw std::logic_error("A matching ConvTypes::SubTypeIndex could not be found.");
  }

  setModel();
  setCurrentValues(oldValues);
}

std::map<std::size_t, int> ConvFunctionTemplateModel::getSubTypes() const {
  std::map<std::size_t, int> subTypes;
  subTypes[ConvTypes::SubTypeIndex::Lorentzian] = static_cast<int>(m_lorentzianType);
  subTypes[ConvTypes::SubTypeIndex::Fit] = static_cast<int>(m_fitType);
  subTypes[ConvTypes::SubTypeIndex::Delta] = static_cast<int>(m_deltaType);
  subTypes[ConvTypes::SubTypeIndex::TempCorrection] = static_cast<int>(m_tempCorrectionType);
  subTypes[ConvTypes::SubTypeIndex::Background] = static_cast<int>(m_backgroundType);
  subTypes[ConvTypes::SubTypeIndex::TiePeakCentres] = static_cast<int>(m_tiePeakCentresType);
  return subTypes;
}

void ConvFunctionTemplateModel::tiePeakCentres() {
  auto const lor1PeakCentreName = getParameterName(ParamID::LOR1_PEAKCENTRE);
  auto const lor2PeakCentreName = getParameterName(ParamID::LOR2_PEAKCENTRE);
  if (!lor1PeakCentreName || !lor2PeakCentreName)
    return;
  auto const tie = m_tiePeakCentresType == TiePeakCentresType::True ? *lor1PeakCentreName : "";
  for (auto i = 0; i < getNumberDomains(); ++i) {
    setLocalParameterTie(*lor2PeakCentreName, i, tie);
  }
}

int ConvFunctionTemplateModel::getNumberOfPeaks() const {
  if (m_lorentzianType == LorentzianType::None)
    return 0;
  if (m_lorentzianType == LorentzianType::TwoLorentzians)
    return 2;
  return 1;
}

std::optional<std::string> ConvFunctionTemplateModel::getPrefix(ParamID name) const {
  if (name >= ParamID::FLAT_BG_A0) {
    return model()->backgroundPrefix();
  } else if (name == ParamID::DELTA_HEIGHT || name == ParamID::DELTA_CENTER) {
    return model()->deltaFunctionPrefix();
  } else if (name == ParamID::TEMPERATURE) {
    return model()->tempFunctionPrefix();
  } else if (name >= ParamID::TW_HEIGHT) {
    return model()->fitTypePrefix();
  } else {
    auto const prefixes = model()->peakPrefixes();
    auto const index = name > ParamID::LOR1_FWHM && name <= ParamID::LOR2_FWHM ? 1 : 0;
    if (!prefixes || index >= static_cast<int>(prefixes->size()))
      return std::optional<std::string>();
    return (*prefixes)[index];
  }
}

void ConvFunctionTemplateModel::applyParameterFunction(const std::function<void(ParamID)> &paramFun) const {
  applyToFitFunction<ConvTypes::LorentzianSubType>(m_lorentzianType, paramFun);
  applyToFitFunction<ConvTypes::FitSubType>(m_fitType, paramFun);
  applyToFitFunction<ConvTypes::DeltaSubType>(m_deltaType, paramFun);
  applyToFitFunction<ConvTypes::TempSubType>(m_tempCorrectionType, paramFun);
  applyToFitFunction<ConvTypes::BackgroundSubType>(m_backgroundType, paramFun);
}

std::string ConvFunctionTemplateModel::buildLorentzianFunctionString() const {
  return "name=Lorentzian,Amplitude=1,FWHM=1,constraints=(Amplitude>0,FWHM>"
         "0)";
}

std::string ConvFunctionTemplateModel::buildTeixeiraFunctionString() const {
  return "name=TeixeiraWaterSQE, Height=1, DiffCoeff=2.3, Tau=1.25, Centre=0, "
         "constraints=(Height>0, DiffCoeff>0, Tau>0)";
}

std::string ConvFunctionTemplateModel::buildTeixeiraIqtFTFunctionString() const {
  return "name=TeixeiraWaterIqtFT, Amp=1, Tau1=1.25, Gamma=1.2, "
         "constraints=(Amp>0, Gamma>0, Tau1>0)";
}

std::string ConvFunctionTemplateModel::buildFickFunctionString() const {
  return "name=FickDiffusionSQE, Height=1, DiffCoeff=2.3, Centre=0, "
         "constraints=(Height>0, DiffCoeff>0)";
}

std::string ConvFunctionTemplateModel::buildChudleyElliotString() const {
  return "name=ChudleyElliotSQE, Height=1, Tau=1.25, Centre=0, L=1.0, "
         "constraints=(Height>0, Tau>0, L>0)";
}

std::string ConvFunctionTemplateModel::buildHallRossString() const {
  return "name=HallRossSQE, Height=1, Tau=1.25, Centre=0, L=1.0, "
         "constraints=(Height>0, Tau>0, L>0)";
}

std::string ConvFunctionTemplateModel::buildStretchExpFTFunctionString() const {
  return "name=StretchedExpFT, Height=0.1, Tau=100, Beta=1, Centre=0, "
         "constraints=(Height>0, Tau>0)";
}

std::string ConvFunctionTemplateModel::buildDiffRotDiscreteCircleFunctionString() const {
  return "name=DiffRotDiscreteCircle, f1.Intensity=1, f1.Radius=1, f1.Decay=1, "
         "f1.Shift=0, constraints=(f1.Intensity>0, f1.Radius>0)";
}

std::string ConvFunctionTemplateModel::buildInelasticDiffRotDiscreteCircleFunctionString() const {
  return "name=InelasticDiffRotDiscreteCircle, Intensity=1, Radius=1, Decay=1, "
         "Shift=0, constraints=(Intensity>0, Radius>0)";
}

std::string ConvFunctionTemplateModel::buildElasticDiffRotDiscreteCircleFunctionString() const {
  return "name=ElasticDiffRotDiscreteCircle, Height=1, Centre=0, Radius=1, "
         "constraints=(Height>0, Radius>0)";
}

std::string ConvFunctionTemplateModel::buildDiffSphereFunctionString() const {
  return "name=DiffSphere, Q=1, f0.Q=1, "
         "f0.WorkspaceIndex=2147483647, f1.Q = 1, f1.WorkspaceIndex = "
         "2147483647, f0.Height = 1, f0.Centre = 0, f0.Radius = 2, "
         "f1.Intensity = 1, f1.Radius = 2, f1.Diffusion = 0.05, f1.Shift = 0";
}

std::string ConvFunctionTemplateModel::buildElasticDiffSphereFunctionString() const {
  return "name=ElasticDiffSphere, Height=1, Centre=0, Radius=2, "
         "constraints=(Height>0, Radius>0)";
}

std::string ConvFunctionTemplateModel::buildInelasticDiffSphereFunctionString() const {
  return "name=InelasticDiffSphere, Intensity=1, Radius=2, Diffusion=0.05, "
         "Shift=0, constraints=(Intensity>0, Radius>0, Diffusion>0)";
}

std::string ConvFunctionTemplateModel::buildIsoRotDiffFunctionString() const {
  return "name=IsoRotDiff, f1.Height=0.1, f1.Radius=2, f1.Tau=100, f1.Centre=0";
}

std::string ConvFunctionTemplateModel::buildElasticIsoRotDiffFunctionString() const {
  return "name=ElasticIsoRotDiff, Height=0.1, Radius=2";
}

std::string ConvFunctionTemplateModel::buildInelasticIsoRotDiffFunctionString() const {
  return "name=InelasticIsoRotDiff, Height=0.1, Radius=2, Tau=100, Centre=0";
}

std::string ConvFunctionTemplateModel::buildPeaksFunctionString() const {
  std::string functions;
  if (m_lorentzianType == LorentzianType::OneLorentzian) {
    functions.append(buildLorentzianFunctionString());
  } else if (m_lorentzianType == LorentzianType::TwoLorentzians) {
    auto const lorentzian = buildLorentzianFunctionString();
    functions.append(lorentzian);
    functions.append(";");
    functions.append(lorentzian);
  }
  if (m_fitType == FitType::TeixeiraWater) {
    functions.append(buildTeixeiraFunctionString());
  } else if (m_fitType == FitType::TeixeiraWaterIqtFT) {
    functions.append(buildTeixeiraIqtFTFunctionString());
  } else if (m_fitType == FitType::FickDiffusion) {
    functions.append(buildFickFunctionString());
  } else if (m_fitType == FitType::ChudleyElliot) {
    functions.append(buildChudleyElliotString());
  } else if (m_fitType == FitType::HallRoss) {
    functions.append(buildHallRossString());
  } else if (m_fitType == FitType::StretchedExpFT) {
    functions.append(buildStretchExpFTFunctionString());
  } else if (m_fitType == FitType::DiffSphere) {
    functions.append(buildDiffSphereFunctionString());
  } else if (m_fitType == FitType::ElasticDiffSphere) {
    functions.append(buildElasticDiffSphereFunctionString());
  } else if (m_fitType == FitType::InelasticDiffSphere) {
    functions.append(buildInelasticDiffSphereFunctionString());
  } else if (m_fitType == FitType::DiffRotDiscreteCircle) {
    functions.append(buildDiffRotDiscreteCircleFunctionString());
  } else if (m_fitType == FitType::InelasticDiffRotDiscreteCircle) {
    functions.append(buildInelasticDiffRotDiscreteCircleFunctionString());
  } else if (m_fitType == FitType::ElasticDiffRotDiscreteCircle) {
    functions.append(buildElasticDiffRotDiscreteCircleFunctionString());
  } else if (m_fitType == FitType::IsoRotDiff) {
    functions.append(buildIsoRotDiffFunctionString());
  } else if (m_fitType == FitType::ElasticIsoRotDiff) {
    functions.append(buildElasticIsoRotDiffFunctionString());
  } else if (m_fitType == FitType::InelasticIsoRotDiff) {
    functions.append(buildInelasticIsoRotDiffFunctionString());
  }
  return functions;
}

std::string ConvFunctionTemplateModel::buildLorentzianPeaksString() const {
  std::string functions;
  if (m_lorentzianType == LorentzianType::OneLorentzian) {
    functions.append(buildLorentzianFunctionString());
  } else if (m_lorentzianType == LorentzianType::TwoLorentzians) {
    auto const lorentzian = buildLorentzianFunctionString();
    functions.append(lorentzian);
    functions.append(";");
    functions.append(lorentzian);
  }
  return functions;
}

std::string ConvFunctionTemplateModel::buildFitTypeString() const {
  std::string functions;
  if (m_fitType == FitType::TeixeiraWater) {
    functions.append(buildTeixeiraFunctionString());
  } else if (m_fitType == FitType::TeixeiraWaterIqtFT) {
    functions.append(buildTeixeiraIqtFTFunctionString());
  } else if (m_fitType == FitType::FickDiffusion) {
    functions.append(buildFickFunctionString());
  } else if (m_fitType == FitType::ChudleyElliot) {
    functions.append(buildChudleyElliotString());
  } else if (m_fitType == FitType::HallRoss) {
    functions.append(buildHallRossString());
  } else if (m_fitType == FitType::StretchedExpFT) {
    functions.append(buildStretchExpFTFunctionString());
  } else if (m_fitType == FitType::DiffSphere) {
    functions.append(buildDiffSphereFunctionString());
  } else if (m_fitType == FitType::ElasticDiffSphere) {
    functions.append(buildElasticDiffSphereFunctionString());
  } else if (m_fitType == FitType::InelasticDiffSphere) {
    functions.append(buildInelasticDiffSphereFunctionString());
  } else if (m_fitType == FitType::DiffRotDiscreteCircle) {
    functions.append(buildDiffRotDiscreteCircleFunctionString());
  } else if (m_fitType == FitType::InelasticDiffRotDiscreteCircle) {
    functions.append(buildInelasticDiffRotDiscreteCircleFunctionString());
  } else if (m_fitType == FitType::ElasticDiffRotDiscreteCircle) {
    functions.append(buildElasticDiffRotDiscreteCircleFunctionString());
  } else if (m_fitType == FitType::IsoRotDiff) {
    functions.append(buildIsoRotDiffFunctionString());
  } else if (m_fitType == FitType::ElasticIsoRotDiff) {
    functions.append(buildElasticIsoRotDiffFunctionString());
  } else if (m_fitType == FitType::InelasticIsoRotDiff) {
    functions.append(buildInelasticIsoRotDiffFunctionString());
  }
  return functions;
}

std::string ConvFunctionTemplateModel::buildBackgroundFunctionString() const {
  if (m_backgroundType == BackgroundType::None)
    return "";
  return "name=" + m_backgroundSubtype.getFunctionName(m_backgroundType) + ",A0=0,constraints=(A0>0)";
}

std::optional<std::string> ConvFunctionTemplateModel::getLor1Prefix() const {
  auto const prefixes = model()->peakPrefixes();
  if (!prefixes || prefixes->size() < 1)
    return std::optional<std::string>();
  return (*prefixes)[0];
}

std::optional<std::string> ConvFunctionTemplateModel::getLor2Prefix() const {
  auto const prefixes = model()->peakPrefixes();
  if (!prefixes || prefixes->size() < 2)
    return std::optional<std::string>();
  return (*prefixes)[1];
}

std::optional<std::string> ConvFunctionTemplateModel::getFitTypePrefix() const { return model()->fitTypePrefix(); }

std::optional<std::string> ConvFunctionTemplateModel::getDeltaPrefix() const { return model()->deltaFunctionPrefix(); }

std::optional<std::string> ConvFunctionTemplateModel::getBackgroundPrefix() const {
  return model()->backgroundPrefix();
}

} // namespace MantidQt::CustomInterfaces::Inelastic
