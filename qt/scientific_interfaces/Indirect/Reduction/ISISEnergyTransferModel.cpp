// Mantid Repository : https://github.ervicecom/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISEnergyTransferModel.h"
#include "ISISEnergyTransferModelUtils.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "ReductionAlgorithmUtils.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;

namespace MantidQt::CustomInterfaces {
IETModel::IETModel() : m_outputGroupName(), m_outputWorkspaces() {}

std::vector<std::string> IETModel::validateRunData(IETRunData const &runData) {
  std::vector<std::string> errors;
  IETDataValidator validator;

  auto inputFiles = runData.getInputData().getInputFiles();
  bool isRunFileValid = !inputFiles.empty();
  std::string firstFileName = inputFiles.substr(0, inputFiles.find(','));

  std::string analysisError = validator.validateAnalysisData(runData.getAnalysisData());
  if (!analysisError.empty()) {
    errors.push_back(analysisError);
  }

  std::string conversionError = validator.validateConversionData(runData.getConversionData());
  if (!conversionError.empty()) {
    errors.push_back(conversionError);
  }

  std::vector<std::string> backgroundErrors = validator.validateBackgroundData(
      runData.getBackgroundData(), runData.getConversionData(), firstFileName, isRunFileValid);
  errors.insert(errors.end(), backgroundErrors.begin(), backgroundErrors.end());

  errors.erase(std::remove(errors.begin(), errors.end(), ""), errors.end());

  return errors;
}

void IETModel::setInstrumentProperties(IAlgorithmRuntimeProps &properties, InstrumentData const &instData) {
  Mantid::API::AlgorithmProperties::update("Instrument", instData.getInstrument(), properties);
  Mantid::API::AlgorithmProperties::update("Analyser", instData.getAnalyser(), properties);
  Mantid::API::AlgorithmProperties::update("Reflection", instData.getReflection(), properties);
}

void IETModel::setInputProperties(IAlgorithmRuntimeProps &properties, IETInputData const &inputData) {
  Mantid::API::AlgorithmProperties::update("InputFiles", inputData.getInputFiles(), properties);
  Mantid::API::AlgorithmProperties::update("SumFiles", inputData.getSumFiles(), properties);
  Mantid::API::AlgorithmProperties::update("LoadLogFiles", inputData.getLoadLogFiles(), properties);
  if (inputData.getUseCalibration()) {
    Mantid::API::AlgorithmProperties::update("CalibrationWorkspace", inputData.getCalibrationWorkspace(), properties);
  }
}

void IETModel::setConversionProperties(IAlgorithmRuntimeProps &properties, IETConversionData const &conversionData,
                                       std::string const &instrument) {
  std::vector<int> detectorRange;

  if (instrument == "IRIS" || instrument == "OSIRIS") {
    Mantid::API::AlgorithmProperties::update("Efixed", conversionData.getEfixed(), properties);
  }
  detectorRange.emplace_back(conversionData.getSpectraMin());
  detectorRange.emplace_back(conversionData.getSpectraMax());
  Mantid::API::AlgorithmProperties::update("SpectraRange", detectorRange, properties);
}

void IETModel::setBackgroundProperties(IAlgorithmRuntimeProps &properties, IETBackgroundData const &backgroundData) {
  if (backgroundData.getRemoveBackground()) {
    std::vector<double> backgroundRange;
    backgroundRange.emplace_back(backgroundData.getBackgroundStart());
    backgroundRange.emplace_back(backgroundData.getBackgroundEnd());
    Mantid::API::AlgorithmProperties::update("BackgroundRange", backgroundRange, properties);
  }
}

void IETModel::setRebinProperties(IAlgorithmRuntimeProps &properties, IETRebinData const &rebinData) {
  if (rebinData.getShouldRebin()) {
    std::string rebin;

    if (rebinData.getRebinType() == IETRebinType::SINGLE) {
      rebin = std::to_string(rebinData.getRebinLow()) + "," + std::to_string(rebinData.getRebinWidth()) + "," +
              std::to_string(rebinData.getRebinHigh());
    } else {
      rebin = rebinData.getRebinString();
    }
    Mantid::API::AlgorithmProperties::update("RebinString", rebin, properties);
  }
}

void IETModel::setAnalysisProperties(IAlgorithmRuntimeProps &properties, IETAnalysisData const &analysisData) {
  if (analysisData.getUseDetailedBalance()) {
    Mantid::API::AlgorithmProperties::update("DetailedBalance", analysisData.getDetailedBalance(), properties);
  }
}

void IETModel::setOutputProperties(IAlgorithmRuntimeProps &properties, IETOutputData const &outputData,
                                   std::string const &outputGroupName) {
  if (outputData.getUseDeltaEInWavenumber()) {
    Mantid::API::AlgorithmProperties::update("UnitX", std::string("DeltaE_inWavenumber"), properties);
  }
  Mantid::API::AlgorithmProperties::update("FoldMultipleFrames", outputData.getFoldMultipleFrames(), properties);
  Mantid::API::AlgorithmProperties::update("OutputWorkspace", outputGroupName, properties);
}

std::string IETModel::getOutputGroupName(InstrumentData const &instData, std::string const &inputText) {
  std::string instrument = instData.getInstrument();
  std::string analyser = instData.getAnalyser();
  std::string reflection = instData.getReflection();

  return instrument + inputText + "_" + analyser + "_" + reflection + "_Reduced";
}

MantidQt::API::IConfiguredAlgorithm_sptr IETModel::energyTransferAlgorithm(InstrumentData const &instData,
                                                                           IETRunData &runData) {
  auto properties = runData.groupingProperties();

  setInstrumentProperties(*properties, instData);
  setInputProperties(*properties, runData.getInputData());
  setConversionProperties(*properties, runData.getConversionData(), instData.getInstrument());
  setBackgroundProperties(*properties, runData.getBackgroundData());
  setRebinProperties(*properties, runData.getRebinData());
  setAnalysisProperties(*properties, runData.getAnalysisData());

  m_outputGroupName = getOutputGroupName(instData, runData.getInputData().getInputText());
  setOutputProperties(*properties, runData.getOutputData(), m_outputGroupName);

  auto reductionAlg = AlgorithmManager::Instance().create("ISISIndirectEnergyTransfer");
  reductionAlg->initialize();
  return std::make_shared<API::ConfiguredAlgorithm>(std::move(reductionAlg), std::move(properties));
}

std::vector<std::string> IETModel::validatePlotData(IETPlotData const &plotParams) {
  std::vector<std::string> errors;

  const std::string inputFiles = plotParams.getInputData().getInputFiles();
  if (inputFiles.empty()) {
    errors.push_back("You must select a run file.");
  }

  IETDataValidator validator;

  bool isRunFileValid = !inputFiles.empty();
  std::string firstFileName = inputFiles.substr(0, inputFiles.find(','));

  std::string conversionError = validator.validateConversionData(plotParams.getConversionData());
  if (!conversionError.empty()) {
    errors.push_back(conversionError);
  }

  std::vector<std::string> backgroundErrors = validator.validateBackgroundData(
      plotParams.getBackgroundData(), plotParams.getConversionData(), firstFileName, isRunFileValid);
  errors.insert(errors.end(), backgroundErrors.begin(), backgroundErrors.end());

  errors.erase(std::remove(errors.begin(), errors.end(), ""), errors.end());

  return errors;
}

std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
IETModel::plotRawAlgorithmQueue(InstrumentData const &instData, IETPlotData const &plotParams) const {
  auto const [rawFile, basename] = parseInputFiles(plotParams.getInputData().getInputFiles());

  auto const data = plotParams.getConversionData();
  auto const detectorList = createDetectorList(data.getSpectraMin(), data.getSpectraMax());

  return plotRawAlgorithmQueue(rawFile, basename, instData.getInstrument(), detectorList,
                               plotParams.getBackgroundData());
}

std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
IETModel::plotRawAlgorithmQueue(std::string const &rawFile, std::string const &basename,
                                std::string const &instrumentName, std::vector<int> const &detectorList,
                                IETBackgroundData const &backgroundData) const {
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithmDeque;
  algorithmDeque.emplace_back(loadConfiguredAlg(rawFile, instrumentName, detectorList, basename));

  if (backgroundData.getRemoveBackground()) {
    auto const bgStart = backgroundData.getBackgroundStart();
    auto const bgEnd = backgroundData.getBackgroundEnd();

    algorithmDeque.emplace_back(calculateFlatBackgroundConfiguredAlg(basename, bgStart, bgEnd, basename + "_bg"));
    algorithmDeque.emplace_back(groupDetectorsConfiguredAlg(basename + "_bg", detectorList, basename + "_grp"));
    algorithmDeque.emplace_back(groupDetectorsConfiguredAlg(basename, detectorList, basename + "_grp_raw"));
  } else {
    algorithmDeque.emplace_back(groupDetectorsConfiguredAlg(basename, detectorList, basename + "_grp"));
  }
  return algorithmDeque;
}

void IETModel::saveWorkspace(std::string const &workspaceName, IETSaveData const &saveTypes) {
  if (saveTypes.getNexus())
    save("SaveNexusProcessed", workspaceName, workspaceName + ".nxs");
  if (saveTypes.getSPE())
    save("SaveSPE", workspaceName, workspaceName + ".spe");
  if (saveTypes.getASCII())
    save("SaveAscii", workspaceName, workspaceName + ".dat", 2);
  if (saveTypes.getAclimax())
    saveAclimax(workspaceName, workspaceName + "_aclimax.dat");
  if (saveTypes.getDaveGrp())
    saveDaveGroup(workspaceName, workspaceName + ".grp");
}

void IETModel::save(std::string const &algorithmName, std::string const &workspaceName, std::string const &outputName,
                    int const version, std::string const &separator) {
  auto saver = AlgorithmManager::Instance().create(algorithmName, version);
  saver->initialize();
  saver->setProperty("InputWorkspace", workspaceName);
  saver->setProperty("Filename", outputName);
  if (!separator.empty())
    saver->setProperty("Separator", separator);
  saver->execute();
}

void IETModel::saveAclimax(std::string const &workspaceName, std::string const &outputName, std::string const &xUnits) {
  auto const bins = xUnits == "DeltaE_inWavenumber" ? "24, -0.005, 4000" : "3, -0.005, 500"; // cm-1 or meV
  auto const temporaryName = workspaceName + "_aclimax_save_temp";

  auto rebin = AlgorithmManager::Instance().create("Rebin");
  rebin->initialize();
  rebin->setProperty("InputWorkspace", workspaceName);
  rebin->setProperty("OutputWorkspace", temporaryName);
  rebin->setProperty("Params", bins);
  rebin->execute();

  save("SaveAscii", temporaryName, outputName, -1, "Tab");
  deleteWorkspace(temporaryName);
}

void IETModel::saveDaveGroup(std::string const &workspaceName, std::string const &outputName) {
  auto const temporaryName = workspaceName + "_davegrp_save_temp";

  auto converter = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  converter->initialize();
  converter->setProperty("InputWorkspace", workspaceName);
  converter->setProperty("OutputWorkspace", temporaryName);
  converter->setProperty("Target", "ElasticQ");
  converter->setProperty("EMode", "Indirect");
  converter->execute();

  save("SaveDaveGrp", temporaryName, outputName);
  deleteWorkspace(temporaryName);
}

void IETModel::createGroupingWorkspace(std::string const &instrumentName, std::string const &analyser,
                                       std::string const &customGrouping, std::string const &outputName) {
  auto creator = AlgorithmManager::Instance().create("CreateGroupingWorkspace");
  creator->initialize();
  creator->setProperty("InstrumentName", instrumentName);
  creator->setProperty("ComponentName", analyser);
  creator->setProperty("CustomGroupingString", customGrouping);
  creator->setProperty("OutputWorkspace", outputName);

  creator->execute();
}

double IETModel::loadDetailedBalance(std::string const &filename) {
  std::vector<std::string> const logNames{"sample", "sample_top", "sample_bottom"};
  auto const detailedBalance = loadSampleLog(filename, logNames, 300.0);
  return detailedBalance;
}

std::vector<std::string> IETModel::groupWorkspaces(std::string const &groupName, std::string const &instrument,
                                                   std::string const &groupOption, bool const shouldGroup) {
  m_outputWorkspaces.clear();

  if (doesExistInADS(groupName)) {
    if (auto const outputGroup = getADSWorkspace<WorkspaceGroup>(groupName)) {
      m_outputWorkspaces = outputGroup->getNames();

      if (instrument == "OSIRIS") {
        if (!shouldGroup) {
          ungroupWorkspace(outputGroup->getName());
        }
      } else {
        if (groupOption == IETGroupOption::UNGROUPED) {
          ungroupWorkspace(outputGroup->getName());
        } else if (groupOption == IETGroupOption::SAMPLECHANGERGROUPED) {
          groupWorkspaceBySampleChanger(outputGroup->getName());
          // If we are grouping by sample we want to ungroup the reduced group leaving only the sample grouped
          ungroupWorkspace(outputGroup->getName());
        }
      }
    }
  }

  return m_outputWorkspaces;
}

void IETModel::ungroupWorkspace(std::string const &workspaceName) {
  auto ungroup = AlgorithmManager::Instance().create("UnGroupWorkspace");
  ungroup->initialize();
  ungroup->setProperty("InputWorkspace", workspaceName);
  ungroup->execute();
}

void IETModel::groupWorkspaceBySampleChanger(std::string const &workspaceName) {
  auto group = AlgorithmManager::Instance().create("GroupBySampleChangerPosition");
  group->initialize();
  group->setProperty("InputWorkspace", workspaceName);
  std::string prefix = workspaceName;
  prefix.erase(workspaceName.find("_Reduced"), 8);
  group->setProperty("OutputGroupPrefix", prefix);
  group->setProperty("OutputGroupSuffix", "Reduced");
  group->execute();
}

} // namespace MantidQt::CustomInterfaces
