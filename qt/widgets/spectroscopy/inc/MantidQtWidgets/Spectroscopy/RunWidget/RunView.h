// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"

#include "ui_RunView.h"

#include <string>

#include <QObject>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {

class IRunPresenter;

class MANTID_SPECTROSCOPY_DLL IRunView {
public:
  virtual ~IRunView() = default;

  virtual void subscribePresenter(IRunPresenter *presenter) = 0;

  virtual void setRunText(std::string const &text) = 0;

  virtual void displayWarning(std::string const &message) = 0;
};

class MANTID_SPECTROSCOPY_DLL RunView final : public QWidget, public IRunView {
  Q_OBJECT

public:
  RunView(QWidget *parent = nullptr);
  ~RunView() override = default;

  void subscribePresenter(IRunPresenter *presenter) override;

  void setRunText(std::string const &text) override;

  void displayWarning(std::string const &message) override;

private slots:
  void notifyRunClicked();

private:
  IRunPresenter *m_presenter;
  Ui::RunWidget m_uiForm;
};

} // namespace CustomInterfaces
} // namespace MantidQt
