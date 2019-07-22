// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_INDIRECTMOMENTS_H_
#define MANTID_CUSTOMINTERFACES_INDIRECTMOMENTS_H_

#include "IndirectDataReductionTab.h"

#include "MantidKernel/System.h"
#include "ui_IndirectMoments.h"

#include <QFont>

namespace MantidQt {
namespace CustomInterfaces {
/** IndirectMoments : Calculates the S(Q,w) Moments of the provided data with
  the user specified range and scale factor


  @author Samuel Jackson
  @date 13/08/2013
*/
class DLLExport IndirectMoments : public IndirectDataReductionTab {
  Q_OBJECT

public:
  IndirectMoments(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~IndirectMoments() override;

  void setup() override;
  void run() override;
  bool validate() override;

protected slots:
  /// Slot for when the range selector changes
  void rangeChanged(double min, double max);
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val);
  /// Called when the algorithm completes to update preview plot
  void momentsAlgComplete(bool error);
  /// Slots for plot and save
  void runClicked();
  void saveClicked();

  void setRunEnabled(bool enabled);
  void setSaveEnabled(bool enabled);
  void setOutputButtonsEnabled(std::string const &enableOutputButtons);
  void updateRunButton(bool enabled = true,
                       std::string const &enableOutputButtons = "unchanged",
                       QString const message = "Run",
                       QString const tooltip = "");

private slots:
  void handleDataReady(QString const &dataName) override;

private:
  void plotNewData(QString const &filename);
  void setFileExtensionsByName(bool filter) override;

  Ui::IndirectMoments m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_CUSTOMINTERFACES_INDIRECTMOMENTS_H_ */
