// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataReductionTab.h"
#include "IndirectMomentsModel.h"
#include "IndirectMomentsView.h"

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
  ~IndirectMoments() = default;

  void setup() override;
  void run() override;
  bool validate() override;

protected slots:
  /// Slot to update the guides when the range properties change
  void updateProperties(QtProperty *prop, double val);
  /// Called when the algorithm completes to update preview plot
  void momentsAlgComplete(bool error);
  /// Slots for plot and save
  void runClicked();
  void saveClicked();

private slots:
  void handleDataReady(QString const &dataName) override;
  void handleScaleChanged(int state);
  void handleScaleValueChanged(double value);

private:
  void plotNewData(QString const &filename);
  void setFileExtensionsByName(bool filter) override;
  Ui::IndirectMoments m_uiForm;
  std::unique_ptr<IndirectMomentsModel> m_model;
  std::unique_ptr<IndirectMomentsView> m_view;
};
} // namespace CustomInterfaces
} // namespace MantidQt
