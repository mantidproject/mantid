// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTTRANSMISSION_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTTRANSMISSION_H_

#include "IndirectDataReductionTab.h"
#include "MantidKernel/System.h"
#include "ui_IndirectTransmission.h"

namespace MantidQt {
namespace CustomInterfaces {
/** IndirectTransmission

  Provides the UI interface to the IndirectTransmissionMonitor algorithm to
  calculate
  sample transmission using a sample and container raw run file.


  @author Samuel Jackson
  @date 13/08/2013
*/
class DLLExport IndirectTransmission : public IndirectDataReductionTab {
  Q_OBJECT

public:
  IndirectTransmission(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~IndirectTransmission() override;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void transAlgDone(bool error);
  void setInstrument();

  void runClicked();
  void plotClicked();
  void saveClicked();

  void setRunEnabled(bool enabled);
  void setPlotEnabled(bool enabled);
  void setSaveEnabled(bool enabled);
  void setOutputButtonsEnabled(std::string const &enableOutputButtons);
  void updateRunButton(bool enabled = true,
                       std::string const &enableOutputButtons = "unchanged",
                       QString const message = "Run",
                       QString const tooltip = "");
  void setPlotIsPlotting(bool plotting);

private:
  void setInstrument(QString const &instrumentName);

  Ui::IndirectTransmission m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTTRANSMISSION_H_ */
