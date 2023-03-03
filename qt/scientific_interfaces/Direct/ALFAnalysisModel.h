// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFAnalysisModel {

public:
  virtual ~IALFAnalysisModel() = default;

  virtual void clear() = 0;

  virtual void setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                     std::vector<double> const &twoThetas) = 0;
  virtual Mantid::API::MatrixWorkspace_sptr extractedWorkspace() const = 0;
  virtual bool isDataExtracted() const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr doFit(std::pair<double, double> const &range) = 0;
  virtual void calculateEstimate(Mantid::API::MatrixWorkspace_sptr const &workspace) = 0;

  virtual void exportWorkspaceCopyToADS() const = 0;

  virtual Mantid::API::MatrixWorkspace_sptr plottedWorkspace() const = 0;
  virtual std::vector<int> plottedWorkspaceIndices() const = 0;

  virtual void setPeakParameters(Mantid::API::IPeakFunction_const_sptr const &peak) = 0;
  virtual void setPeakCentre(double const centre) = 0;
  virtual double peakCentre() const = 0;
  virtual double background() const = 0;
  virtual Mantid::API::IPeakFunction_const_sptr getPeakCopy() const = 0;

  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  cropWorkspaceProperties(std::pair<double, double> const &range) const = 0;
  virtual std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  fitProperties(std::pair<double, double> const &range) const = 0;

  virtual void setFitResult(Mantid::API::MatrixWorkspace_sptr workspace, Mantid::API::IFunction_sptr function,
                            std::string fitStatus) = 0;
  virtual Mantid::API::MatrixWorkspace_sptr fitWorkspace() const = 0;

  virtual std::string fitStatus() const = 0;

  virtual std::size_t numberOfTubes() const = 0;

  virtual std::optional<double> averageTwoTheta() const = 0;
  virtual std::vector<double> allTwoThetas() const = 0;

  virtual std::optional<double> rotationAngle() const = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisModel final : public IALFAnalysisModel {

public:
  ALFAnalysisModel();

  void clear() override;

  void setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                             std::vector<double> const &twoThetas) override;
  Mantid::API::MatrixWorkspace_sptr extractedWorkspace() const override;
  bool isDataExtracted() const override;

  Mantid::API::MatrixWorkspace_sptr doFit(std::pair<double, double> const &range) override;
  void calculateEstimate(Mantid::API::MatrixWorkspace_sptr const &workspace) override;

  void exportWorkspaceCopyToADS() const override;

  Mantid::API::MatrixWorkspace_sptr plottedWorkspace() const override;
  std::vector<int> plottedWorkspaceIndices() const override;

  void setPeakParameters(Mantid::API::IPeakFunction_const_sptr const &peak) override;
  void setPeakCentre(double const centre) override;
  double peakCentre() const override;
  double background() const override;
  Mantid::API::IPeakFunction_const_sptr getPeakCopy() const override;

  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  cropWorkspaceProperties(std::pair<double, double> const &range) const override;
  std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
  fitProperties(std::pair<double, double> const &range) const override;

  void setFitResult(Mantid::API::MatrixWorkspace_sptr workspace, Mantid::API::IFunction_sptr function,
                    std::string fitStatus) override;
  inline Mantid::API::MatrixWorkspace_sptr fitWorkspace() const noexcept override { return m_fitWorkspace; }

  std::string fitStatus() const override;

  std::size_t numberOfTubes() const override;

  std::optional<double> averageTwoTheta() const override;
  inline std::vector<double> allTwoThetas() const noexcept override { return m_twoThetas; };

  std::optional<double> rotationAngle() const override;

private:
  Mantid::API::IFunction_sptr calculateEstimateImpl(Mantid::API::MatrixWorkspace_sptr const &workspace);

  Mantid::API::IFunction_sptr m_function;
  std::string m_fitStatus;
  std::vector<double> m_twoThetas;
  Mantid::API::MatrixWorkspace_sptr m_extractedWorkspace;
  Mantid::API::MatrixWorkspace_sptr m_fitWorkspace;
};

} // namespace CustomInterfaces
} // namespace MantidQt
