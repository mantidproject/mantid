// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_RunView.h"

#include <QObject>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {

class IRunPresenter;

class IRunView {
public:
  virtual ~IRunView() = default;

  virtual void subscribePresenter(IRunPresenter *presenter) = 0;
};

class RunView : public QWidget, public IRunView {
  Q_OBJECT

public:
  RunView(QWidget *parent);

  void subscribePresenter(IRunPresenter *presenter) override;

private:
  IRunPresenter *m_presenter;
  Ui::RunWidget m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt