// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "ISISEnergyTransferValidator.h"
#include "MantidAPI/Algorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IIETModel {
public:
  virtual ~IIETModel() = default;

  virtual void setInstrumentProperties(IAlgorithmRuntimeProps &properties, InstrumentData const &instData) = 0;

  virtual std::vector<std::string> validateRunData(IETRunData const &runData) = 0;
  virtual std::vector<std::string> validatePlotData(IETPlotData const &plotData) = 0;

  virtual MantidQt::API::IConfiguredAlgorithm_sptr energyTransferAlgorithm(InstrumentData const &instData,
                                                                           IETRunData &runParams) = 0;
  virtual std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
  plotRawAlgorithmQueue(InstrumentData const &instData, IETPlotData const &plotData) const = 0;

  virtual void saveWorkspace(std::string const &workspaceName, IETSaveData const &saveData) = 0;

  virtual void createGroupingWorkspace(std::string const &instrumentName, std::string const &analyser,
                                       std::string const &customGrouping, std::string const &outputName) = 0;
  virtual double loadDetailedBalance(std::string const &filename) = 0;

  virtual std::vector<std::string> groupWorkspaces(std::string const &groupName, std::string const &instrument,
                                                   std::string const &groupOption, bool const shouldGroup) = 0;

  virtual std::string outputGroupName() const = 0;
  virtual std::vector<std::string> outputWorkspaceNames() const = 0;
};

class MANTIDQT_INDIRECT_DLL IETModel : public IIETModel {
public:
  IETModel();
  ~IETModel() override = default;

  void setInstrumentProperties(IAlgorithmRuntimeProps &properties, InstrumentData const &instData) override;

  std::vector<std::string> validateRunData(IETRunData const &runData) override;
  std::vector<std::string> validatePlotData(IETPlotData const &plotData) override;

  MantidQt::API::IConfiguredAlgorithm_sptr energyTransferAlgorithm(InstrumentData const &instData,
                                                                   IETRunData &runParams) override;
  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
  plotRawAlgorithmQueue(InstrumentData const &instData, IETPlotData const &plotData) const override;

  void saveWorkspace(std::string const &workspaceName, IETSaveData const &saveData) override;

  void createGroupingWorkspace(std::string const &instrumentName, std::string const &analyser,
                               std::string const &customGrouping, std::string const &outputName) override;
  double loadDetailedBalance(std::string const &filename) override;

  std::vector<std::string> groupWorkspaces(std::string const &groupName, std::string const &instrument,
                                           std::string const &groupOption, bool const shouldGroup) override;

  // Public for testing purposes
  void setInputProperties(IAlgorithmRuntimeProps &properties, IETInputData const &inputData);
  void setConversionProperties(IAlgorithmRuntimeProps &properties, IETConversionData const &conversionData,
                               std::string const &instrument);
  void setBackgroundProperties(IAlgorithmRuntimeProps &properties, IETBackgroundData const &backgroundData);
  void setRebinProperties(IAlgorithmRuntimeProps &properties, IETRebinData const &rebinData);
  void setAnalysisProperties(IAlgorithmRuntimeProps &properties, IETAnalysisData const &analysisData);
  void setOutputProperties(IAlgorithmRuntimeProps &properties, IETOutputData const &outputData,
                           std::string const &outputGroupName);
  std::string getOutputGroupName(InstrumentData const &instData, std::string const &inputFiles);

  [[nodiscard]] inline std::string outputGroupName() const noexcept override { return m_outputGroupName; }

  [[nodiscard]] inline std::vector<std::string> outputWorkspaceNames() const noexcept override {
    return m_outputWorkspaces;
  }

private:
  void saveDaveGroup(std::string const &workspaceName, std::string const &outputName);
  void saveAclimax(std::string const &workspaceName, std::string const &outputName,
                   std::string const &xUnits = "DeltaE_inWavenumber");
  void save(std::string const &algorithmName, std::string const &workspaceName, std::string const &outputName,
            int const version = -1, std::string const &separator = "");

  void ungroupWorkspace(std::string const &workspaceName);
  void groupWorkspaceBySampleChanger(std::string const &workspaceName);

  std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>
  plotRawAlgorithmQueue(std::string const &rawFile, std::string const &basename, std::string const &instrumentName,
                        std::vector<int> const &detectorList, IETBackgroundData const &backgroundData) const;

  std::string m_outputGroupName;
  std::vector<std::string> m_outputWorkspaces;
};
} // namespace CustomInterfaces
} // namespace MantidQt
