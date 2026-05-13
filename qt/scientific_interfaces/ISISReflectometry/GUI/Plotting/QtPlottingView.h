// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlottingView.h"
#include "ui_PlottingWidget.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class QtPlottingView : public QWidget, public IPlottingView {
  Q_OBJECT
public:
  explicit QtPlottingView(QWidget *parent = nullptr);

  void subscribe(PlottingViewSubscriber *notifyee) override;
  void setOutputOptionsEnabled(bool enabled) override;

private:
  void initLayout();

  Ui::PlottingWidget m_ui;
  PlottingViewSubscriber *m_notifyee;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
