// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvolutionModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidGeometry/Instrument.h"

#include <boost/algorithm/string/join.hpp>

#include <stdexcept>
#include <utility>

using namespace Mantid::API;

namespace {

IAlgorithm_sptr loadParameterFileAlgorithm(std::string const &workspaceName, std::string const &filename) {
  auto loadParamFile = AlgorithmManager::Instance().create("LoadParameterFile");
  loadParamFile->initialize();
  loadParamFile->setProperty("Workspace", workspaceName);
  loadParamFile->setProperty("Filename", filename);
  return loadParamFile;
}

void readAnalyserFromFile(const std::string &analyser, const MatrixWorkspace_sptr &workspace) {
  auto const instrument = workspace->getInstrument();
  auto const idfDirectory = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
  auto const reflection = instrument->getStringParameter("reflection")[0];
  auto const parameterFile =
      idfDirectory + instrument->getName() + "_" + analyser + "_" + reflection + "_Parameters.xml";

  auto loadParamFile = loadParameterFileAlgorithm(workspace->getName(), parameterFile);
  loadParamFile->execute();

  if (!loadParamFile->isExecuted())
    throw std::invalid_argument("Could not load parameter file, ensure instrument "
                                "directory is in data search paths.");
}

Mantid::Geometry::IComponent_const_sptr getAnalyser(const MatrixWorkspace_sptr &workspace) {
  auto const instrument = workspace->getInstrument();
  auto const analysers = instrument->getStringParameter("analyser");

  if (analysers.empty())
    throw std::invalid_argument("Could not load instrument resolution from parameter file");

  auto const component = instrument->getComponentByName(analysers[0]);
  if (component) {
    if (component->hasParameter("resolution")) {
      auto const resolutionParameters = component->getNumberParameter("resolution");
      if (resolutionParameters.empty())
        readAnalyserFromFile(analysers[0], workspace);
    }
  } else {
    readAnalyserFromFile(analysers[0], workspace);
  }
  return instrument->getComponentByName(analysers[0]);
}

boost::optional<double> instrumentResolution(const MatrixWorkspace_sptr &workspace) {
  try {
    auto const analyser = getAnalyser(workspace);
    if (analyser && analyser->hasParameter("resolution"))
      return analyser->getNumberParameter("resolution")[0];

    auto const instrument = workspace->getInstrument();
    if (instrument && instrument->hasParameter("resolution"))
      return instrument->getNumberParameter("resolution")[0];
    else if (instrument && instrument->hasParameter("EFixed"))
      return instrument->getNumberParameter("EFixed")[0] * 0.01;

    return boost::none;
  } catch (Mantid::Kernel::Exception::NotFoundError const &) {
    return boost::none;
  } catch (std::invalid_argument const &) {
    return boost::none;
  } catch (std::exception const &) {
    return boost::none;
  }
}

void getParameterNameChanges(const IFunction &model, const std::string &oldPrefix, const std::string &newPrefix,
                             std::unordered_map<std::string, std::string> &changes) {
  for (const auto &parameterName : model.getParameterNames())
    changes[newPrefix + parameterName] = oldPrefix + parameterName;
}

void getParameterNameChanges(const CompositeFunction &model, const std::string &prefixPrefix,
                             const std::string &prefixSuffix, std::size_t from, std::size_t to,
                             std::unordered_map<std::string, std::string> &changes) {
  size_t di = from > 0 ? 1 : 0;
  for (auto i = from; i < to; ++i) {
    const auto oldPrefix = "f" + std::to_string(i) + ".";
    const auto functionPrefix = "f" + std::to_string(i - di) + ".";
    const auto function = model.getFunction(i);
    auto newPrefix = prefixPrefix + functionPrefix;

    if (function->name() != "Delta Function")
      newPrefix += prefixSuffix;

    getParameterNameChanges(*function, oldPrefix, newPrefix, changes);
  }
}

std::unordered_map<std::string, std::string> parameterNameChanges(const CompositeFunction &model,
                                                                  const std::string &prefixPrefix,
                                                                  const std::string &prefixSuffix,
                                                                  std::size_t backgroundIndex) {
  std::unordered_map<std::string, std::string> changes;
  auto const nFunctions = model.nFunctions();
  if (nFunctions > 2) {
    getParameterNameChanges(model, prefixPrefix, prefixSuffix, 0, backgroundIndex, changes);

    const auto backgroundPrefix = "f" + std::to_string(backgroundIndex) + ".";
    getParameterNameChanges(*model.getFunction(backgroundIndex), backgroundPrefix, "f0.", changes);

    getParameterNameChanges(model, prefixPrefix, prefixSuffix, backgroundIndex + 1, model.nFunctions(), changes);
  } else if (nFunctions == 2) {
    const auto backgroundPrefix = "f" + std::to_string(backgroundIndex) + ".";
    getParameterNameChanges(*model.getFunction(backgroundIndex), backgroundPrefix, "f0.", changes);
    size_t otherIndex = backgroundIndex == 0 ? 1 : 0;
    const auto otherPrefix = "f" + std::to_string(otherIndex) + ".";
    getParameterNameChanges(*model.getFunction(otherIndex), otherPrefix, prefixPrefix, changes);
  } else {
    throw std::runtime_error("Composite function is expected to have more than 1 member.");
  }
  return changes;
}

std::unordered_map<std::string, std::string>
parameterNameChanges(const CompositeFunction &model, const std::string &prefixPrefix, const std::string &prefixSuffix) {
  std::unordered_map<std::string, std::string> changes;
  getParameterNameChanges(model, prefixPrefix, prefixSuffix, 0, model.nFunctions(), changes);
  return changes;
}

std::unordered_map<std::string, std::string>
parameterNameChanges(const IFunction &model, const std::string &prefixPrefix, const std::string &prefixSuffix) {
  std::unordered_map<std::string, std::string> changes;
  getParameterNameChanges(model, "", prefixPrefix + prefixSuffix, changes);
  return changes;
}

std::unordered_map<std::string, std::string> constructParameterNameChanges(const IFunction &model,
                                                                           boost::optional<std::size_t> backgroundIndex,
                                                                           bool temperatureUsed) {
  std::string prefixPrefix = backgroundIndex ? "f1.f1." : "f1.";
  std::string prefixSuffix = temperatureUsed ? "f1." : "";

  try {
    const auto &compositeModel = dynamic_cast<const CompositeFunction &>(model);
    if (backgroundIndex)
      return parameterNameChanges(compositeModel, prefixPrefix, prefixSuffix, *backgroundIndex);
    return parameterNameChanges(compositeModel, prefixPrefix, prefixSuffix);
  } catch (const std::bad_cast &) {
    return parameterNameChanges(model, prefixPrefix, prefixSuffix);
  }
}

IAlgorithm_sptr addSampleLogAlgorithm(const Workspace_sptr &workspace, const std::string &name, const std::string &text,
                                      const std::string &type) {
  auto addSampleLog = AlgorithmManager::Instance().create("AddSampleLog");
  addSampleLog->setLogging(false);
  addSampleLog->setProperty("Workspace", workspace->getName());
  addSampleLog->setProperty("LogName", name);
  addSampleLog->setProperty("LogText", text);
  addSampleLog->setProperty("LogType", type);
  return addSampleLog;
}

struct AddSampleLogRunner {
  AddSampleLogRunner(Workspace_sptr resultWorkspace, WorkspaceGroup_sptr resultGroup)
      : m_resultWorkspace(std::move(resultWorkspace)), m_resultGroup(std::move(resultGroup)) {}

  void operator()(const std::string &name, const std::string &text, const std::string &type) {
    addSampleLogAlgorithm(m_resultWorkspace, name, text, type)->execute();
    addSampleLogAlgorithm(m_resultGroup, name, text, type)->execute();
  }

private:
  Workspace_sptr m_resultWorkspace;
  WorkspaceGroup_sptr m_resultGroup;
};

std::vector<std::string> getNames(const MantidQt::CustomInterfaces::Inelastic::ResolutionCollectionType &workspaces) {
  std::vector<std::string> names;
  names.reserve(workspaces.size().value);
  std::transform(
      workspaces.begin(), workspaces.end(), std::back_inserter(names),
      [](const std::weak_ptr<Mantid::API::MatrixWorkspace> &workspace) { return workspace.lock()->getName(); });
  return names;
}

void setResolutionAttribute(const CompositeFunction_sptr &convolutionModel, const IFunction::Attribute &attr) {
  if (convolutionModel->name() == "Convolution")
    convolutionModel->getFunction(0)->setAttribute("Workspace", attr);
  else {
    auto convolution = std::dynamic_pointer_cast<CompositeFunction>(convolutionModel->getFunction(1));
    convolution->getFunction(0)->setAttribute("Workspace", attr);
  }
}
} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

ConvolutionModel::ConvolutionModel() { m_fitType = CONVOLUTION_STRING; }

IAlgorithm_sptr ConvolutionModel::sequentialFitAlgorithm() const {
  return AlgorithmManager::Instance().create("ConvolutionFitSequential");
}

IAlgorithm_sptr ConvolutionModel::simultaneousFitAlgorithm() const {
  return AlgorithmManager::Instance().create("ConvolutionFitSimultaneous");
}

boost::optional<double> ConvolutionModel::getInstrumentResolution(WorkspaceID workspaceID) const {
  if (workspaceID < getNumberOfWorkspaces())
    return instrumentResolution(getWorkspace(workspaceID));
  return boost::none;
}

MultiDomainFunction_sptr ConvolutionModel::getMultiDomainFunction() const {
  auto function = FittingModel::getMultiDomainFunction();
  const std::string base = "__ConvolutionResolution";

  for (auto i = 0u; i < function->nFunctions(); ++i)
    setResolutionAttribute(function, IFunction::Attribute(base + std::to_string(i)));
  return function;
}

void ConvolutionModel::setTemperature(const boost::optional<double> &temperature) { m_temperature = temperature; }

std::unordered_map<std::string, ParameterValue>
ConvolutionModel::createDefaultParameters(WorkspaceID workspaceID) const {
  std::unordered_map<std::string, ParameterValue> defaultValues;
  defaultValues["PeakCentre"] = ParameterValue(0.0);
  defaultValues["Centre"] = ParameterValue(0.0);
  // Reset all parameters to default of 1
  defaultValues["Amplitude"] = ParameterValue(1.0);
  defaultValues["beta"] = ParameterValue(1.0);
  defaultValues["Decay"] = ParameterValue(1.0);
  defaultValues["Diffusion"] = ParameterValue(1.0);
  defaultValues["Height"] = ParameterValue(1.0);
  defaultValues["Intensity"] = ParameterValue(1.0);
  defaultValues["Radius"] = ParameterValue(1.0);
  defaultValues["Tau"] = ParameterValue(1.0);

  auto resolution = getInstrumentResolution(workspaceID);
  if (resolution)
    defaultValues["FWHM"] = ParameterValue(*resolution);
  return defaultValues;
}

std::unordered_map<std::string, std::string> ConvolutionModel::mapDefaultParameterNames() const {
  const auto initialMapping = FittingModel::mapDefaultParameterNames();
  std::unordered_map<std::string, std::string> mapping;
  for (const auto &map : initialMapping) {
    auto mapped = m_parameterNameChanges.find(map.second);
    if (mapped != m_parameterNameChanges.end())
      mapping[map.first] = mapped->second;
    else
      mapping.insert(map);
  }
  return mapping;
}

void ConvolutionModel::addSampleLogs() {
  auto const result = getResultWorkspace();
  auto const group = getResultGroup();
  if (!result || !group) {
    return;
  }
  AddSampleLogRunner addSampleLog(result, group);
  addSampleLog("resolution_filename", boost::algorithm::join(getNames(m_resolution), ","), "String");

  if (m_temperature && m_temperature.get() != 0.0) {
    addSampleLog("temperature_correction", "true", "String");
    addSampleLog("temperature_value", std::to_string(m_temperature.get()), "Number");
  }
}

void ConvolutionModel::addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  FittingModel::addOutput(std::move(fitAlgorithm));
  addSampleLogs();
}

void ConvolutionModel::setParameterNameChanges(const IFunction &model, boost::optional<std::size_t> backgroundIndex) {
  m_parameterNameChanges =
      constructParameterNameChanges(model, std::move(backgroundIndex), m_temperature.is_initialized());
}

} // namespace MantidQt::CustomInterfaces::Inelastic
