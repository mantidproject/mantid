// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DataReductionTab.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"

#include "MantidKernel/System.h"
#include "ui_ILLEnergyTransfer.h"

namespace MantidQt {
namespace CustomInterfaces {
class IDataReduction;

class MANTIDQT_INDIRECT_DLL ILLEnergyTransfer : public DataReductionTab, public IRunSubscriber {
  Q_OBJECT

public:
  ILLEnergyTransfer(IDataReduction *idrUI, QWidget *parent = nullptr);
  ~ILLEnergyTransfer() override;

  void handleValidation(IUserInputValidator *validator) const override;
  void handleRun() override;
  const std::string getSubscriberName() const override { return "ILLEnergyTransfer"; }

private slots:
  void algorithmComplete(bool error);

private:
  void updateInstrumentConfiguration() override;

  Ui::ILLEnergyTransfer m_uiForm;
  double m_backScaling = 1.;
  double m_backCalibScaling = 1.;
  double m_peakRange[2];
  int m_pixelRange[2];
  std::string m_suffix;
  void save();
  void plot();
};
} // namespace CustomInterfaces
} // namespace MantidQt
