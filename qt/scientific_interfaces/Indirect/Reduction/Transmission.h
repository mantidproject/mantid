// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "DataReductionTab.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "ui_Transmission.h"

namespace MantidQt {
namespace CustomInterfaces {
class IDataReduction;

class MANTIDQT_INDIRECT_DLL Transmission : public DataReductionTab, public IRunSubscriber {
  Q_OBJECT

public:
  Transmission(IDataReduction *idrUI, QWidget *parent = nullptr);
  ~Transmission() override;

  void handleRun() override;
  void handleValidation(IUserInputValidator *validator) const override;
  const std::string getSubscriberName() const override { return "Transmission"; }

private slots:
  void transAlgDone(bool error);

  void saveClicked();

  void setSaveEnabled(bool enabled);

private:
  void setLoadHistory(bool doLoadHistory) override;
  void setInstrument(QString const &instrumentName);
  void updateInstrumentConfiguration() override;

  Ui::Transmission m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt
