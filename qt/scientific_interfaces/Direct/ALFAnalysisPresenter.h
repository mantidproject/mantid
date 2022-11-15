// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFAnalysisModel.h"
#include "DllConfig.h"

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

  virtual void subscribeInstrumentPresenter(IALFInstrumentPresenter *presenter) = 0;

  virtual void notifyPeakCentreEditingFinished() = 0;
  virtual void notifyFitClicked() = 0;
  virtual void notifyUpdateEstimateClicked() = 0;

  virtual void notifyTubeExtracted() = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisPresenter final : public IALFAnalysisPresenter {

public:
  explicit ALFAnalysisPresenter(IALFAnalysisView *m_view, std::unique_ptr<IALFAnalysisModel> m_model);
  QWidget *getView() override;

  void subscribeInstrumentPresenter(IALFInstrumentPresenter *presenter) override;

  void notifyPeakCentreEditingFinished() override;
  void notifyFitClicked() override;
  void notifyUpdateEstimateClicked() override;

  void notifyTubeExtracted() override;

private:
  std::optional<std::string> validateFitValues() const;
  bool checkPeakCentreIsWithinFitRange() const;
  void updatePeakCentreInViewFromModel();

  IALFInstrumentPresenter *m_instrumentPresenter;

  IALFAnalysisView *m_view;
  std::unique_ptr<IALFAnalysisModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt
