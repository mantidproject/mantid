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
  void dataLoaded();
  void previewPlot();
  void transAlgDone(bool error);
  void instrumentSet();
  void plotClicked();
  void saveClicked();

private:
  Ui::IndirectTransmission m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTTRANSMISSION_H_ */
