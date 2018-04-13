#include "IndirectConvFitModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidGeometry/Instrument.h"

#include <boost/algorithm/string/replace.hpp>

#include <stdexcept>

using namespace Mantid::API;

namespace {
IFunction_sptr createResolutionFunction() {
  auto func = FunctionFactory::Instance().createFunction("Resolution");
  // add resolution file
  IFunction::Attribute attr("__ConvFit_Resolution");
  func->setAttribute("Workspace", attr);
  return func;
}

CompositeFunction_sptr applyTemperatureCorrection(IFunction_sptr function,
                                                  IFunction_sptr correction,
                                                  double value) {
  auto product = boost::dynamic_pointer_cast<CompositeFunction>(
      FunctionFactory::Instance().createFunction("ProductFunction"));
  product->addFunction(correction);
  product->addFunction(function);
  product->tie("f0.Temp", std::to_string(value));
  product->applyTies();
  return product;
}

IFunction_sptr createTemperatureCorrection(double correction) {
  // create temperature correction function to multiply with the lorentzians
  IFunction_sptr tempFunc;

  // create user function for the exponential correction
  // (x*temp) / 1-exp(-(x*temp))
  tempFunc = FunctionFactory::Instance().createFunction("UserFunction");
  // 11.606 is the conversion factor from meV to K
  std::string formula = "((x*11.606)/Temp) / (1 - exp(-((x*11.606)/Temp)))";
  IFunction::Attribute att(formula);
  tempFunc->setAttribute("Formula", att);
  tempFunc->setParameter("Temp", correction);
  return tempFunc;
}

CompositeFunction_sptr addTemperatureCorrection(CompositeFunction_sptr model,
                                                double value) {
  auto correction = createTemperatureCorrection(value);

  for (auto i = 0u; i < model->nFunctions(); ++i) {
    auto function = model->getFunction(i);

    if (function->name() != "DeltaFunction") {
      auto corrected = applyTemperatureCorrection(function, correction, value);
      model->replaceFunction(i, corrected);
    }
  }
  return model;
}

CompositeFunction_sptr addTemperatureCorrection(IFunction_sptr model,
                                                double value) {
  auto correction = createTemperatureCorrection(value);
  return applyTemperatureCorrection(model, correction, value);
}

IAlgorithm_sptr loadParameterFileAlgorithm(MatrixWorkspace_sptr workspace,
                                           const std::string &filename) {
  IAlgorithm_sptr loadParamFile =
      AlgorithmManager::Instance().create("LoadParameterFile");
  loadParamFile->setChild(true);
  loadParamFile->initialize();
  loadParamFile->setProperty("Workspace", workspace);
  loadParamFile->setProperty("Filename", filename);
  return loadParamFile;
}

void readAnalyserFromFile(const std::string &analyser,
                          MatrixWorkspace_sptr workspace) {
  auto instrument = workspace->getInstrument();
  auto idfDirectory = Mantid::Kernel::ConfigService::Instance().getString(
      "instrumentDefinition.directory");
  auto reflection = instrument->getStringParameter("reflection")[0];
  auto parameterFile = idfDirectory + instrument->getName() + "_" + analyser +
                       "_" + reflection + "_Parameters.xml";

  IAlgorithm_sptr loadParamFile =
      loadParameterFileAlgorithm(workspace, parameterFile);
  loadParamFile->execute();

  if (!loadParamFile->isExecuted())
    throw std::invalid_argument(
        "Could not load parameter file, ensure instrument "
        "directory is in data search paths.");
}

Mantid::Geometry::IComponent_const_sptr
getAnalyser(MatrixWorkspace_sptr workspace) {
  auto instrument = workspace->getInstrument();
  auto analysers = instrument->getStringParameter("analyser");

  if (analysers.empty())
    throw std::invalid_argument(
        "Could not load instrument resolution from parameter file");

  auto component = instrument->getComponentByName(analysers[0]);
  auto resolutionParameters = component->getNumberParameter("resolution");
  if (nullptr == component || resolutionParameters.empty())
    readAnalyserFromFile(analysers[0], workspace);
  return workspace->getInstrument()->getComponentByName(analysers[0]);
}

double instrumentResolution(MatrixWorkspace_sptr workspace) {
  try {
    auto analyser = getAnalyser(workspace);
    if (analyser != nullptr)
      return analyser->getNumberParameter("resolution")[0];
    else
      return workspace->getInstrument()->getNumberParameter("resolution")[0];
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    throw std::invalid_argument(
        "Could not load instrument resolution from parameter file");
  }
}

MatrixWorkspace_sptr cloneWorkspace(MatrixWorkspace_sptr inputWS,
                                    const std::string &outputWSName) {
  IAlgorithm_sptr cloneAlg =
      AlgorithmManager::Instance().create("CloneWorkspace");
  cloneAlg->setLogging(false);
  cloneAlg->initialize();
  cloneAlg->setProperty("InputWorkspace", inputWS);
  cloneAlg->setProperty("OutputWorkspace", outputWSName);
  cloneAlg->execute();
  return cloneAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr appendWorkspace(MatrixWorkspace_sptr leftWS,
                                     MatrixWorkspace_sptr rightWS,
                                     int numHistograms,
                                     const std::string &outputWSName) {
  IAlgorithm_sptr appendAlg =
      AlgorithmManager::Instance().create("AppendSpectra");
  appendAlg->setLogging(false);
  appendAlg->initialize();
  appendAlg->setProperty("InputWorkspace1", leftWS);
  appendAlg->setProperty("InputWorkspace2", rightWS);
  appendAlg->setProperty("Number", numHistograms);
  appendAlg->setProperty("OutputWorkspace", outputWSName);
  appendAlg->execute();
  return appendAlg->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr extendResolutionWorkspace(MatrixWorkspace_sptr resolution,
                                               std::size_t numberOfHistograms) {
  const auto resolutionNumHist = resolution->getNumberHistograms();
  if (resolutionNumHist != 1 && resolutionNumHist != numberOfHistograms) {
    std::string msg(
        "Resolution must have either one or as many spectra as the sample");
    throw std::runtime_error(msg);
  }

  const auto resolutionName = "__ConvFit_Resolution";
  auto resolutionWS = cloneWorkspace(resolution, resolutionName);

  // Append to cloned workspace if necessary
  if (resolutionNumHist == 1 && numberOfHistograms > 1)
    return appendWorkspace(resolutionWS, resolution,
                           static_cast<int>(numberOfHistograms - 1),
                           resolutionName);
  return resolutionWS;
}

void getParameterNameChanges(
    const IFunction &model, const std::string &oldPrefix,
    const std::string &newPrefix,
    std::unordered_map<std::string, std::string> &changes) {
  for (const auto &parameterName : model.getParameterNames())
    changes[oldPrefix + parameterName] = newPrefix + parameterName;
}

std::unordered_map<std::string, std::string>
parameterNameChanges(const CompositeFunction &model,
                     const std::string &prefixPrefix,
                     const std::string &prefixSuffix, std::size_t offset) {
  std::unordered_map<std::string, std::string> changes;

  for (auto i = 0u; i < model.nFunctions(); ++i) {
    const auto oldPrefix = "f" + std::to_string(i + offset) + ".";
    const auto functionPrefix = "f" + std::to_string(i) + ".";
    const auto function = model.getFunction(i);
    auto newPrefix = prefixPrefix + functionPrefix;

    if (function->name() != "Delta Function")
      newPrefix += prefixSuffix;

    getParameterNameChanges(*function, oldPrefix, newPrefix, changes);
  }
  return changes;
}

std::unordered_map<std::string, std::string>
parameterNameChanges(const IFunction &model, const std::string &prefixPrefix,
                     const std::string &prefixSuffix, std::size_t offset) {
  std::unordered_map<std::string, std::string> changes;
  getParameterNameChanges(model, "", prefixPrefix + prefixSuffix, changes);
  return changes;
}

std::unordered_map<std::string, std::string>
constructParameterNameChanges(const IFunction &model, bool temperatureUsed,
                              bool backgroundUsed) {
  std::string prefixPrefix = backgroundUsed ? "f1.f1." : "f1.";
  std::string prefixSuffix = temperatureUsed ? "f1." : "";
  std::size_t offset = backgroundUsed ? 1 : 0;

  try {
    const auto &compositeModel = dynamic_cast<const CompositeFunction &>(model);
    return parameterNameChanges(compositeModel, prefixPrefix, prefixSuffix,
                                offset);
  } catch (const std::bad_cast &) {
    return parameterNameChanges(model, prefixPrefix, prefixSuffix, offset);
  }
}

IAlgorithm_sptr addSampleLogAlgorithm(Workspace_sptr workspace,
                                      const std::string &name,
                                      const std::string &text,
                                      const std::string &type) {
  auto addSampleLog = AlgorithmManager::Instance().create("AddSampleLog");
  addSampleLog->setLogging(false);
  addSampleLog->setProperty("Workspace", workspace);
  addSampleLog->setProperty("LogName", name);
  addSampleLog->setProperty("LogText", text);
  addSampleLog->setProperty("LogType", type);
  return addSampleLog;
}

struct AddSampleLogRunner {
  AddSampleLogRunner(MatrixWorkspace_sptr resultWorkspace,
                     WorkspaceGroup_sptr resultGroup)
      : m_resultWorkspace(resultWorkspace), m_resultGroup(resultGroup) {}

  void operator()(const std::string &name, const std::string &text,
                  const std::string &type) const {
    addSampleLogAlgorithm(m_resultWorkspace, name, text, type)->execute();
    addSampleLogAlgorithm(m_resultGroup, name, text, type)->execute();
  }

private:
  MatrixWorkspace_sptr m_resultWorkspace;
  WorkspaceGroup_sptr m_resultGroup;
};

std::string backgroundString(IFunction_sptr function) {
  const auto functionName = function->name();

  if (functionName == "FlatBackgroud") {
    if (function->isFixed(0))
      return "FixF";
    return "FitF";
  } else if (functionName == "LinearBackground")
    return "FitL";
  return "";
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectConvFitModel::IndirectConvFitModel() {}

IndirectConvFitModel::~IndirectConvFitModel() {
  if (AnalysisDataService::Instance().doesExist("__ConvFit_Resolution"))
    AnalysisDataService::Instance().remove("__ConvFit_Resolution");
}

IAlgorithm_sptr IndirectConvFitModel::sequentialFitAlgorithm() const {
  return AlgorithmManager::Instance().create("ConvolutionFitSequential");
}

IAlgorithm_sptr IndirectConvFitModel::simultaneousFitAlgorithm() const {
  return AlgorithmManager::Instance().create("ConvolutionFitSimultaneous");
}

std::string IndirectConvFitModel::sequentialFitOutputName() const {
  if (isMultiFit())
    return "MultiConvFit_" + m_fitType + m_backgroundString;
  return inputDisplayName(
      "%1%_conv_" + m_fitType + m_backgroundString + "_s%2%", "_to_", 0);
}

std::string IndirectConvFitModel::simultaneousFitOutputName() const {
  return sequentialFitOutputName();
}

double
IndirectConvFitModel::getInstrumentResolution(std::size_t dataIndex) const {
  return instrumentResolution(getWorkspace(dataIndex));
}

std::size_t IndirectConvFitModel::maximumHistograms() const {
  std::size_t max = getWorkspace(0)->getNumberHistograms();

  for (auto i = 1u; i < numberOfWorkspaces(); ++i) {
    const auto histograms = getWorkspace(i)->getNumberHistograms();
    max = max >= histograms ? max : histograms;
  }
  return max;
}

void IndirectConvFitModel::setFitFunction(IFunction_sptr model,
                                          IFunction_sptr background) {
  setParameterNameChanges(*model, background != nullptr);
  CompositeFunction_sptr comp(new CompositeFunction);

  if (!(model &&
        AnalysisDataService::Instance().doesExist("__ConvFit_Resolution")))
    IndirectFittingModel::setFitFunction(
        CompositeFunction_sptr(new CompositeFunction));

  auto conv = boost::dynamic_pointer_cast<CompositeFunction>(
      FunctionFactory::Instance().createFunction("Convolution"));
  conv->addFunction(createResolutionFunction());

  if (m_temperature) {
    auto compositeModel = boost::dynamic_pointer_cast<CompositeFunction>(model);
    if (compositeModel)
      model = addTemperatureCorrection(compositeModel, m_temperature.get());
    else
      model = addTemperatureCorrection(model, m_temperature.get());
  }
  conv->addFunction(model);

  if (background) {
    comp->addFunction(background);
    comp->addFunction(conv);
  } else
    comp = conv;

  m_backgroundString = background ? backgroundString(background) : "";
  IndirectFittingModel::setFitFunction(comp);
}

void IndirectConvFitModel::setTemperature(
    const boost::optional<double> &temperature) {
  m_temperature = temperature;
}

void IndirectConvFitModel::addWorkspace(MatrixWorkspace_sptr workspace,
                                        const Spectra &spectra) {
  IndirectFittingModel::addWorkspace(workspace, spectra);
  extendResolution();
}

void IndirectConvFitModel::removeWorkspace(std::size_t index) {
  IndirectFittingModel::removeWorkspace(index);
  extendResolution();
}

void IndirectConvFitModel::setResolution(
    Mantid::API::MatrixWorkspace_sptr resolution) {
  m_resolutionWorkspace = resolution;
  extendResolutionWorkspace(resolution, maximumHistograms());
}

void IndirectConvFitModel::extendResolution() {
  const auto resolutionWorkspace = m_resolutionWorkspace.lock();
  if (resolutionWorkspace)
    extendResolutionWorkspace(resolutionWorkspace, maximumHistograms());
}

void IndirectConvFitModel::setFitTypeString(const std::string &fitType) {
  m_fitType = fitType;
}

std::unordered_map<std::string, ParameterValue>
IndirectConvFitModel::getDefaultParameters(std::size_t index) const {
  std::unordered_map<std::string, ParameterValue> defaultValues;
  defaultValues["PeakCentre"] = 0.0;
  defaultValues["Centre"] = 0.0;
  // Reset all parameters to default of 1
  defaultValues["Amplitude"] = 1.0;
  defaultValues["beta"] = 1.0;
  defaultValues["Decay"] = 1.0;
  defaultValues["Diffusion"] = 1.0;
  defaultValues["height"] = 1.0; // Lower case in StretchedExp - this can be
                                 // improved with a case insensitive check
  defaultValues["Height"] = 1.0;
  defaultValues["Intensity"] = 1.0;
  defaultValues["Radius"] = 1.0;
  defaultValues["tau"] = 1.0;

  defaultValues["FWHM"] = instrumentResolution(getWorkspace(index));
  return defaultValues;
}

void IndirectConvFitModel::addSampleLogs() {
  AddSampleLogRunner addSampleLog(getResultWorkspace(), getResultGroup());
  addSampleLog("resolution_filename", m_resolutionWorkspace.lock()->getName(),
               "String");

  if (m_temperature && m_temperature.get() != 0.0) {
    addSampleLog("temperature_correction", "true", "String");
    addSampleLog("temperature_value", std::to_string(m_temperature.get()),
                 "Number");
  }
}

IndirectFitOutput IndirectConvFitModel::createFitOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &m_fittingData) const {
  return IndirectFitOutput(resultGroup, parameterTable, resultWorkspace,
                           m_fittingData, m_parameterNameChanges);
}

void IndirectConvFitModel::addOutput(
    IndirectFitOutput *fitOutput, WorkspaceGroup_sptr resultGroup,
    ITableWorkspace_sptr parameterTable, MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &m_fittingData) const {
  fitOutput->addOutput(resultGroup, parameterTable, resultWorkspace,
                       m_fittingData, m_parameterNameChanges);
}

void IndirectConvFitModel::setParameterNameChanges(const IFunction &model,
                                                   bool backgroundUsed) {
  m_parameterNameChanges = constructParameterNameChanges(
      model, backgroundUsed, m_temperature.is_initialized());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
