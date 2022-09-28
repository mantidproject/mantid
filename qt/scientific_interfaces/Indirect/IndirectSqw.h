// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataReductionTab.h"
#include "IndirectSqwModel.h"
#include "IndirectSqwView.h"

#include "MantidKernel/System.h"
#include "ui_IndirectSqw.h"

namespace MantidQt {
namespace CustomInterfaces {
/** IndirectSqw

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport IndirectSqw : public IndirectDataReductionTab {
  Q_OBJECT

public:
  IndirectSqw(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~IndirectSqw() = default;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void handleDataReady(QString const &dataName) override;
  void sqwAlgDone(bool error);

  void runClicked();
  void saveClicked();

  void qLowChanged(double value);
  void qWidthChanged(double value);
  void qHighChanged(double value);
  void eLowChanged(double value);
  void eWidthChanged(double value);
  void eHighChanged(double value);
  void rebinEChanged(int value);

private:
  void connectSignals();
  void plotRqwContour();
  void setFileExtensionsByName(bool filter) override;

  std::unique_ptr<IndirectSqwModel> m_model;
  std::unique_ptr<IndirectSqwView> m_view;
};
} // namespace CustomInterfaces
} // namespace MantidQt
