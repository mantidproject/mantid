// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFAnalysisModel.h"
#include "DllConfig.h"
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
  virtual void notifyResetClicked() = 0;

  virtual std::size_t numberOfTubes() const = 0;

  virtual void clear() = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisPresenter final : public IALFAnalysisPresenter {

public:
  explicit ALFAnalysisPresenter(IALFAnalysisView *m_view, std::unique_ptr<IALFAnalysisModel> m_model);
  QWidget *getView() override;

  void setExtractedWorkspace(Mantid::API::MatrixWorkspace_sptr const &workspace,
                             std::vector<double> const &twoThetas) override;

  void notifyPeakPickerChanged() override;
  void notifyPeakCentreEditingFinished() override;
  void notifyFitClicked() override;
  void notifyResetClicked() override;

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
};
} // namespace CustomInterfaces
} // namespace MantidQt
