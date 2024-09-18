// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitOutput.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Spectroscopy/FitData.h"

#include <memory>
#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

class MANTIDQT_INELASTIC_DLL IFitPlotModel {
public:
  virtual ~IFitPlotModel() = default;

  virtual Mantid::API::MatrixWorkspace_sptr getWorkspace() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getResultWorkspace() const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getGuessWorkspace() const = 0;
  virtual MantidWidgets::FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const = 0;

  virtual WorkspaceID getActiveWorkspaceID() const = 0;
  virtual WorkspaceIndex getActiveWorkspaceIndex() const = 0;
  virtual FitDomainIndex getActiveDomainIndex() const = 0;
  virtual WorkspaceID numberOfWorkspaces() const = 0;
  virtual std::pair<double, double> getRange() const = 0;
  virtual std::pair<double, double> getWorkspaceRange() const = 0;
  virtual std::pair<double, double> getResultRange() const = 0;
  virtual std::optional<double> getFirstHWHM() const = 0;
  virtual std::optional<double> getFirstPeakCentre() const = 0;
  virtual std::optional<double> getFirstBackgroundLevel() const = 0;
  virtual double calculateHWHMMaximum(double minimum) const = 0;
  virtual double calculateHWHMMinimum(double maximum) const = 0;
  virtual bool canCalculateGuess() const = 0;

  virtual void setActiveIndex(WorkspaceID workspaceID) = 0;
  virtual void setActiveSpectrum(WorkspaceIndex spectrum) = 0;

  virtual void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) = 0;
};

class MANTIDQT_INELASTIC_DLL FitPlotModel final : public IFitPlotModel {
public:
  FitPlotModel(std::vector<FitData> *fittingData, IFitOutput *fitOutput);

  Mantid::API::MatrixWorkspace_sptr getWorkspace() const override;
  Mantid::API::MatrixWorkspace_sptr getResultWorkspace() const override;
  Mantid::API::MatrixWorkspace_sptr getGuessWorkspace() const override;
  MantidWidgets::FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const override;

  WorkspaceID getActiveWorkspaceID() const override;
  WorkspaceIndex getActiveWorkspaceIndex() const override;
  FitDomainIndex getActiveDomainIndex() const override;
  WorkspaceID numberOfWorkspaces() const override;
  std::pair<double, double> getRange() const override;
  std::pair<double, double> getWorkspaceRange() const override;
  std::pair<double, double> getResultRange() const override;
  std::optional<double> getFirstHWHM() const override;
  std::optional<double> getFirstPeakCentre() const override;
  std::optional<double> getFirstBackgroundLevel() const override;
  double calculateHWHMMaximum(double minimum) const override;
  double calculateHWHMMinimum(double maximum) const override;
  bool canCalculateGuess() const override;

  void setActiveIndex(WorkspaceID workspaceID) override;
  void setActiveSpectrum(WorkspaceIndex spectrum) override;

  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;

private:
  std::pair<double, double> getGuessRange() const;

  Mantid::API::IFunction_sptr getSingleFunction(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  FitDomainIndex getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  std::optional<ResultLocationNew> getResultLocation(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;
  size_t numberOfSpectra(WorkspaceID workspaceID) const;

  Mantid::API::MatrixWorkspace_sptr createGuessWorkspace(const Mantid::API::MatrixWorkspace_sptr &inputWorkspace,
                                                         const Mantid::API::IFunction_const_sptr &func, double startX,
                                                         double endX) const;

  std::vector<double> computeOutput(const Mantid::API::IFunction_const_sptr &func,
                                    const std::vector<double> &dataX) const;

  Mantid::API::IAlgorithm_sptr createWorkspaceAlgorithm(std::size_t numberOfSpectra, const std::vector<double> &dataX,
                                                        const std::vector<double> &dataY) const;

  Mantid::API::MatrixWorkspace_sptr extractSpectra(const Mantid::API::MatrixWorkspace_sptr &inputWS, int startIndex,
                                                   int endIndex, double startX, double endX) const;

  Mantid::API::MatrixWorkspace_sptr appendSpectra(const Mantid::API::MatrixWorkspace_sptr &inputWS,
                                                  const Mantid::API::MatrixWorkspace_sptr &spectraWS) const;

  Mantid::API::MatrixWorkspace_sptr cropWorkspace(const Mantid::API::MatrixWorkspace_sptr &inputWS, double startX,
                                                  double endX, int startIndex, int endIndex) const;

  void deleteWorkspace(const std::string &name) const;

  std::vector<FitData> *m_fittingData;
  IFitOutput *m_fitOutput;
  WorkspaceID m_activeWorkspaceID;
  WorkspaceIndex m_activeWorkspaceIndex;
  Mantid::API::MultiDomainFunction_sptr m_activeFunction;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
