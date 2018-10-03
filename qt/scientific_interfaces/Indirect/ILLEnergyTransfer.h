// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ILLENERGYTRANSFER_H_
#define MANTIDQTCUSTOMINTERFACES_ILLENERGYTRANSFER_H_

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

#endif // MANTIDQTCUSTOMINTERFACES_ILLENERGYTRANSFER_H_
