// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"

#include <QString>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace CustomInterfaces {
class SettingsPresenter;

class MANTID_SPECTROSCOPY_DLL ISettingsView {

public:
  virtual QWidget *getView() = 0;
  virtual void subscribePresenter(SettingsPresenter *presenter) = 0;

  virtual void setSelectedFacility(QString const &text) = 0;
  virtual QString getSelectedFacility() const = 0;

  virtual void setRestrictInputByNameChecked(bool check) = 0;
  virtual bool isRestrictInputByNameChecked() const = 0;

  virtual void setPlotErrorBarsChecked(bool check) = 0;
  virtual bool isPlotErrorBarsChecked() const = 0;

  virtual void setDeveloperFeatureFlags(QStringList const &flags) = 0;
  virtual QStringList developerFeatureFlags() const = 0;

  virtual void setApplyText(QString const &text) = 0;
  virtual void setApplyEnabled(bool enable) = 0;
  virtual void setOkEnabled(bool enable) = 0;
  virtual void setCancelEnabled(bool enable) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt
