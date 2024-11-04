// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataReductionTab.h"
#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "ISISEnergyTransferModel.h"
#include "ISISEnergyTransferView.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"

#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IIETPresenter {
public:
  virtual void notifyFindingRun() = 0;
  virtual void notifySaveClicked() = 0;
  virtual void notifyPlotRawClicked() = 0;
  virtual void notifySaveCustomGroupingClicked(std::string const &customGrouping) = 0;
  virtual void notifyRunFinished() = 0;
};

class MANTIDQT_INDIRECT_DLL IETPresenter : public DataReductionTab,
                                           public IIETPresenter,
                                           public IRunSubscriber,
                                           public API::IAlgorithmRunnerSubscriber {

public:
  IETPresenter(IDataReduction *idrUI, IIETView *view, std::unique_ptr<IIETModel> model,
               std::unique_ptr<API::IAlgorithmRunner> algorithmRunner);

  void notifyFindingRun() override;
  void notifySaveClicked() override;
  void notifyPlotRawClicked() override;
  void notifySaveCustomGroupingClicked(std::string const &customGrouping) override;
  void notifyRunFinished() override;

  void notifyBatchComplete(API::IConfiguredAlgorithm_sptr &lastAlgorithm, bool error) override;

  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;
  const std::string getSubscriberName() const override { return "ISISEnergyTransfer"; }

private:
  void validateInstrumentDetails(IUserInputValidator *validator) const;
  void updateInstrumentConfiguration() override;

  void handleReductionComplete();
  void handlePlotRawPreProcessComplete();

  InstrumentData getInstrumentData() const;

  void setFileExtensionsByName(bool filter) override;
  void setLoadHistory(bool doLoadHistory) override;

  IIETView *m_view;
  std::unique_ptr<IIETModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt
