// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFAlgorithmManager.h"
#include "ALFAnalysisModel.h"
#include "DllConfig.h"
#include "IALFAlgorithmManagerSubscriber.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QWidget>

#include <memory>
#include <optional>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class IALFAnalysisView;
class IALFInstrumentPresenter;

class MANTIDQT_DIRECT_DLL IALFAnalysisPresenter {

public:
  virtual ~IALFAnalysisPresenter() = default;

  virtual QWidget *getView() = 0;

  virtual void setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                                     std::vector<double> const &twoThetas) = 0;

  virtual void notifyPeakPickerChanged() = 0;
  virtual void notifyPeakCentreEditingFinished() = 0;
  virtual void notifyFitClicked() = 0;
  virtual void notifyExportWorkspaceToADSClicked() = 0;
  virtual void notifyExternalPlotClicked() = 0;
  virtual void notifyResetClicked() = 0;

  virtual std::size_t numberOfTubes() const = 0;

  virtual void clear() = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisPresenter final : public IALFAnalysisPresenter,
                                                       public IALFAlgorithmManagerSubscriber {

public:
  explicit ALFAnalysisPresenter(IALFAnalysisView *view, std::unique_ptr<IALFAnalysisModel> model,
                                std::unique_ptr<IALFAlgorithmManager> algorithmManager);
  QWidget *getView() override;

  void setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                             std::vector<double> const &twoThetas) override;

  void notifyPeakPickerChanged() override;
  void notifyPeakCentreEditingFinished() override;
  void notifyFitClicked() override;
  void notifyExportWorkspaceToADSClicked() override;
  void notifyExternalPlotClicked() override;
  void notifyResetClicked() override;

  void notifyAlgorithmError(std::string const &message) override;
  void notifyCropWorkspaceComplete(Mantid::API::MatrixWorkspace_sptr const &workspace) override;
  void notifyFitComplete(Mantid::API::MatrixWorkspace_sptr workspace, Mantid::API::IFunction_sptr function,
                         std::string fitStatus) override;

  std::size_t numberOfTubes() const override;

  void clear() override;

private:
  std::optional<std::string> validateFitValues() const;
  bool checkPeakCentreIsWithinFitRange() const;

  void calculateEstimate();

  void updateViewFromModel();
  void updatePlotInViewFromModel();
  void updateTwoThetaInViewFromModel();
  void updatePeakCentreInViewFromModel();
  void updateRotationAngleInViewFromModel();

  IALFAnalysisView *m_view;
  std::unique_ptr<IALFAnalysisModel> m_model;
  std::unique_ptr<IALFAlgorithmManager> m_algorithmManager;
};
} // namespace CustomInterfaces
} // namespace MantidQt
