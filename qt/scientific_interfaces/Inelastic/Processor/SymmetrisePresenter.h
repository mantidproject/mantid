// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataProcessor.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "SymmetriseModel.h"
#include "SymmetriseView.h"
#include "ui_SymmetriseTab.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtCheckBoxFactory"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {

class ISymmetrisePresenter {
public:
  virtual void handleReflectTypeChanged(int value) = 0;
  virtual void handleDoubleValueChanged(std::string const &propname, double value) = 0;
  virtual void handleDataReady(std::string const &dataName) = 0;
  virtual void handlePreviewClicked() = 0;
  virtual void handleSaveClicked() = 0;
  virtual void setIsPreview(bool preview) = 0;
};

/** SymmetrisePresenter

  @author Dan Nixon
  @date 23/07/2014
*/
class MANTIDQT_INELASTIC_DLL SymmetrisePresenter : public DataProcessor,
                                                   public ISymmetrisePresenter,
                                                   public IRunSubscriber {
public:
  SymmetrisePresenter(QWidget *parent, std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner,
                      ISymmetriseView *view, std::unique_ptr<ISymmetriseModel> model);
  ~SymmetrisePresenter() override;

  // run widget
  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;
  const std::string getSubscriberName() const override { return "Symmetrise"; }

  void handleReflectTypeChanged(int value) override;
  void handleDoubleValueChanged(std::string const &propname, double value) override;
  void handleDataReady(std::string const &dataName) override;
  void handlePreviewClicked() override;
  void handleSaveClicked() override;

  void setIsPreview(bool preview) override;

protected:
  void runComplete(bool error) override;

private:
  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;

  Mantid::API::AnalysisDataServiceImpl &m_adsInstance;
  ISymmetriseView *m_view;
  std::unique_ptr<ISymmetriseModel> m_model;
  // wether batch algorunner is running preview or run buttons
  bool m_isPreview;
};
} // namespace CustomInterfaces
} // namespace MantidQt
