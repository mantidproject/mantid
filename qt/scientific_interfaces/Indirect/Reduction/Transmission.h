// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "IndirectDataReductionTab.h"
#include "MantidKernel/System.h"
#include "ui_Transmission.h"

namespace MantidQt {
namespace CustomInterfaces {
class IIndirectDataReduction;

class MANTIDQT_INDIRECT_DLL Transmission : public IndirectDataReductionTab {
  Q_OBJECT

public:
  Transmission(IIndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~Transmission() override;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void transAlgDone(bool error);
  void setInstrument();

  void runClicked();
  void saveClicked();

  void setRunEnabled(bool enabled);
  void setSaveEnabled(bool enabled);
  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString const &message = "Run", QString const &tooltip = "");

private:
  void setInstrument(QString const &instrumentName);

  Ui::Transmission m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt
