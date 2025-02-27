// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace MantidQt::CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IQuasiModel {
public:
  virtual ~IQuasiModel() = default;

  virtual void setSample(std::string const &workspaceName) = 0;
  virtual Mantid::API::MatrixWorkspace_sptr sample() const = 0;

  virtual void setResolution(std::string const &workspaceName) = 0;
  virtual Mantid::API::MatrixWorkspace_sptr resolution() const = 0;

  virtual void setOutputResult(std::string const &workspaceName) = 0;
  virtual void setOutputProbability(std::string const &workspaceName) = 0;
  virtual void setOutputFitGroup(std::string const &workspaceName) = 0;

  virtual Mantid::API::MatrixWorkspace_sptr outputFit(std::size_t const index) const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr outputResult() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr outputProbability() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr outputFitGroup() const = 0;

  virtual bool isResolution(std::string const &workspaceName) const = 0;

  virtual std::optional<std::string> curveColour(std::string const &label) const = 0;

  virtual MantidQt::API::IConfiguredAlgorithm_sptr
  setupBayesQuasiAlgorithm(std::string const &resNormName, std::string const &fixWidthName, std::string const &program,
                           std::string const &baseName, std::string const &background, double const eMin,
                           double const eMax, int const sampleBinning, int const resolutionBinning,
                           bool const elasticPeak, bool const fixWidth, bool const useResNorm,
                           bool const sequentialFit) const = 0;
  virtual MantidQt::API::IConfiguredAlgorithm_sptr
  setupBayesQuasi2Algorithm(std::string const &program, std::string const &baseName, std::string const &background,
                            double const eMin, double const eMax, bool const elasticPeak) const = 0;
  virtual MantidQt::API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(Mantid::API::Workspace_sptr workspace) const = 0;
};

class MANTIDQT_INELASTIC_DLL QuasiModel final : public IQuasiModel {

public:
  QuasiModel();
  ~QuasiModel() override = default;

  void setSample(std::string const &workspaceName) override;
  inline Mantid::API::MatrixWorkspace_sptr sample() const noexcept override { return m_sampleWorkspace; }

  void setResolution(std::string const &workspaceName) override;
  inline Mantid::API::MatrixWorkspace_sptr resolution() const noexcept override { return m_resolutionWorkspace; }

  void setOutputResult(std::string const &workspaceName) override;
  void setOutputProbability(std::string const &workspaceName) override;
  void setOutputFitGroup(std::string const &workspaceName) override;

  Mantid::API::MatrixWorkspace_sptr outputFit(std::size_t const index) const override;
  inline Mantid::API::MatrixWorkspace_sptr outputResult() const noexcept override { return m_outputResult; }
  inline Mantid::API::MatrixWorkspace_sptr outputProbability() const noexcept override { return m_outputProbability; }
  inline Mantid::API::WorkspaceGroup_sptr outputFitGroup() const noexcept override { return m_outputFitGroup; }

  bool isResolution(std::string const &workspaceName) const override;

  std::optional<std::string> curveColour(std::string const &label) const override;

  MantidQt::API::IConfiguredAlgorithm_sptr
  setupBayesQuasiAlgorithm(std::string const &resNormName, std::string const &fixWidthName, std::string const &program,
                           std::string const &baseName, std::string const &background, double const eMin,
                           double const eMax, int const sampleBinning, int const resolutionBinning,
                           bool const elasticPeak, bool const fixWidth, bool const useResNorm,
                           bool const sequentialFit) const override;
  MantidQt::API::IConfiguredAlgorithm_sptr
  setupBayesQuasi2Algorithm(std::string const &program, std::string const &baseName, std::string const &background,
                            double const eMin, double const eMax, bool const elasticPeak) const override;
  MantidQt::API::IConfiguredAlgorithm_sptr setupSaveAlgorithm(Mantid::API::Workspace_sptr workspace) const override;

private:
  Mantid::API::MatrixWorkspace_sptr m_sampleWorkspace;
  Mantid::API::MatrixWorkspace_sptr m_resolutionWorkspace;
  Mantid::API::MatrixWorkspace_sptr m_outputResult;
  Mantid::API::MatrixWorkspace_sptr m_outputProbability;
  Mantid::API::WorkspaceGroup_sptr m_outputFitGroup;
};

} // namespace MantidQt::CustomInterfaces
