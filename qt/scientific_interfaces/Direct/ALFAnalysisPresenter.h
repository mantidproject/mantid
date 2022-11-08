// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ALFAnalysisModel.h"
#include "ALFAnalysisView.h"
#include "DllConfig.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"

#include <memory>
#include <optional>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_DIRECT_DLL IALFAnalysisPresenter {

public:
  virtual ~IALFAnalysisPresenter() = default;
  virtual void destructor() = 0;
  virtual IALFAnalysisView *getView() = 0;
  virtual std::string getCurrentWS() = 0;
  virtual void clearCurrentWS() = 0;
  virtual void peakCentreEditingFinished() = 0;
  virtual void fitClicked() = 0;
  virtual void updateEstimateClicked() = 0;
  virtual void addSpectrum(const std::string &wsName) = 0;
};

class MANTIDQT_DIRECT_DLL ALFAnalysisPresenter : public QObject, public IALFAnalysisPresenter {
  Q_OBJECT

public:
  explicit ALFAnalysisPresenter(IALFAnalysisView *m_view, std::unique_ptr<IALFAnalysisModel> m_model);
  ~ALFAnalysisPresenter() { delete m_fitObserver; };
  void destructor() override { this->~ALFAnalysisPresenter(); };
  IALFAnalysisView *getView() override { return m_view; };
  std::string getCurrentWS() override { return m_currentName; };
  void clearCurrentWS() override { m_currentName = ""; };
  void peakCentreEditingFinished() override;
  void fitClicked() override;
  void updateEstimateClicked() override;
  void addSpectrum(const std::string &wsName) override;

private:
  std::optional<std::string> validateFitValues() const;
  bool checkDataIsExtracted() const;
  bool checkPeakCentreIsWithinFitRange() const;
  void updatePeakCentreInViewFromModel();

  VoidObserver *m_peakCentreObserver;
  VoidObserver *m_fitObserver;
  VoidObserver *m_updateEstimateObserver;

  IALFAnalysisView *m_view;
  std::unique_ptr<IALFAnalysisModel> m_model;
  std::string m_currentName;
};
} // namespace CustomInterfaces
} // namespace MantidQt
