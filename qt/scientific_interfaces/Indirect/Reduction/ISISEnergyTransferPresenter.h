// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ISISEnergyTransferData.h"
#include "ISISEnergyTransferModel.h"
#include "ISISEnergyTransferView.h"

#include "IndirectDataReductionTab.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IIETPresenter {
public:
  virtual void notifySaveClicked() = 0;
  virtual void notifyRunClicked() = 0;
  virtual void notifyPlotRawClicked() = 0;
  virtual void notifySaveCustomGroupingClicked(std::string const &customGrouping) = 0;
  virtual void notifyRunFinished() = 0;
};

class MANTIDQT_INDIRECT_DLL IETPresenter : public IndirectDataReductionTab, public IIETPresenter {
  Q_OBJECT

public:
  IETPresenter(IIndirectDataReduction *idrUI, IIETView *view, std::unique_ptr<IIETModel> model);
  ~IETPresenter() override;

  void setup() override;
  void run() override;

  void notifySaveClicked() override;
  void notifyRunClicked() override;
  void notifyPlotRawClicked() override;
  void notifySaveCustomGroupingClicked(std::string const &customGrouping) override;
  void notifyRunFinished() override;

public slots:
  bool validate() override;

private slots:
  void algorithmComplete(bool error);
  void plotRawComplete(bool error);
  void setInstrumentDefault();

private:
  bool validateInstrumentDetails();

  InstrumentData getInstrumentData();

  void setFileExtensionsByName(bool filter) override;

  std::string m_outputGroupName;

  IIETView *m_view;
  std::unique_ptr<IIETModel> m_model;
};
} // namespace CustomInterfaces
} // namespace MantidQt