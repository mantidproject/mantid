// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include <MantidAPI/AlgorithmManager.h>
#include <MantidAPI/AlgorithmRuntimeProps.h>
#include <MantidQtWidgets/Common/IConfiguredAlgorithm.h>

namespace MantidQt::CustomInterfaces {
using namespace Mantid::API;

typedef std::vector<std::string> StringVec;

struct SampleLogs {
  void setLogNames(StringVec const &logNames) { m_logNames = logNames; }
  void setLogValues(StringVec const &logValues) { m_logValues = logValues; }
  void setLogTypes(StringVec const &logTypes) { m_logTypes = logTypes; }

  StringVec logNames() const { return m_logNames; }
  StringVec logValues() const { return m_logValues; }
  StringVec logTypes() const { return m_logTypes; }

private:
  StringVec m_logNames;
  StringVec m_logValues;
  StringVec m_logTypes;
};

class MANTIDQT_INELASTIC_DLL IResNormModel {
public:
  virtual ~IResNormModel() = default;
  virtual double eMin() const = 0;
  virtual double eMax() const = 0;
  virtual void setEMin(double value) = 0;
  virtual void setEMax(double value) = 0;

  virtual API::IConfiguredAlgorithm_sptr setupResNormAlgorithm(std::string const &outputWsName,
                                                               std::string const &vanWorkspace,
                                                               std::string const &resWorkspace) const = 0;
  virtual API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(const std::string &wsName,
                                                            const std::string &filename = "") const = 0;

  virtual void copyLogs(const MatrixWorkspace_sptr &resultWorkspace, const Workspace_sptr &workspace) const = 0;
  virtual void addAdditionalLogs(const Workspace_sptr &resultWorkspace) const = 0;
  virtual void processLogs(std::string const &vanWsName, std::string const &resWsName,
                           std::string const &outputWsName) = 0;
  virtual void updateLogs(std::string const &vanadiumSampName, std::string const &resSampName) = 0;
};

class MANTIDQT_INELASTIC_DLL ResNormModel final : public IResNormModel {
public:
  ResNormModel();
  ~ResNormModel() override = default;

  double eMin() const override;
  double eMax() const override;
  void setEMin(double value) override;
  void setEMax(double value) override;

  API::IConfiguredAlgorithm_sptr setupResNormAlgorithm(std::string const &outputWsName, std::string const &vanWorkspace,
                                                       std::string const &resWorkspace) const override;
  API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(const std::string &wsName,
                                                    const std::string &filename = "") const override;

  void copyLogs(const MatrixWorkspace_sptr &resultWorkspace, const Workspace_sptr &workspace) const override;
  void processLogs(std::string const &vanWsName, std::string const &resWsName,
                   std::string const &outputWsName) override;
  void addAdditionalLogs(const Workspace_sptr &resultWorkspace) const override;
  void updateLogs(std::string const &vanadiumSampName, std::string const &resSampName) override;

private:
  double m_eMin;
  double m_eMax;
  SampleLogs m_logs;
};
} // namespace MantidQt::CustomInterfaces
