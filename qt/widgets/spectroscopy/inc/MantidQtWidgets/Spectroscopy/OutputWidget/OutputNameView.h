// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "MantidKernel/System.h"
#include "ui_OutputName.h"
#include <QRegExpValidator>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {

class IOutputNamePresenter;

class MANTID_SPECTROSCOPY_DLL IOutputNameView {
public:
  ~IOutputNameView() = default;
  virtual void subscribePresenter(IOutputNamePresenter *presenter) = 0;

  virtual void enableLabelEditor() const = 0;
  virtual void setWarningLabel(std::string const &text, std::string const &textColor) const = 0;
  virtual void setOutputNameLabel(std::string const &text) const = 0;

  virtual std::string getCurrentLabel() const = 0;
  virtual std::string getCurrentOutputName() const = 0;
};

class MANTID_SPECTROSCOPY_DLL OutputNameView final : public QWidget, public IOutputNameView {
  Q_OBJECT
public:
  OutputNameView(QWidget *parent = nullptr);
  ~OutputNameView() override = default;

  void subscribePresenter(IOutputNamePresenter *presenter) override;

  void enableLabelEditor() const override;
  void setWarningLabel(std::string const &text, std::string const &textColor) const override;
  void setOutputNameLabel(std::string const &text) const override;

  std::string getCurrentLabel() const override;
  std::string getCurrentOutputName() const override;

private slots:
  void notifyUpdateOutputLabel();

private:
  Ui::OutputName m_uiForm;
  IOutputNamePresenter *m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt
