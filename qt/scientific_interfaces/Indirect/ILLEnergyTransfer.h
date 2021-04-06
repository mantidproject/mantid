// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataReductionTab.h"

#include "MantidKernel/System.h"
#include "ui_ILLEnergyTransfer.h"

namespace MantidQt {
namespace CustomInterfaces {
/** ILLEnergyTransfer

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport ILLEnergyTransfer : public IndirectDataReductionTab {
  Q_OBJECT

public:
  ILLEnergyTransfer(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~ILLEnergyTransfer() override;

  void setup() override;
  void run() override;

public slots:
  bool validate() override;

private slots:
  void algorithmComplete(bool error);
  void setInstrumentDefault();

  void runClicked();
  void setRunEnabled(bool enabled);
  void updateRunButton(bool enabled = true, std::string const &enableOutputButtons = "unchanged",
                       QString const &message = "Run", QString const &tooltip = "");

private:
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
