// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../DllConfig.h"
#include "SettingsModel.h"
#include "SettingsView.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
class ISettings;

class MANTID_SPECTROSCOPY_DLL SettingsPresenter {

public:
  explicit SettingsPresenter(std::unique_ptr<SettingsModel> model, ISettingsView *view);

  QWidget *getView();
  void subscribeParent(ISettings *parent);

  void loadSettings();

  void notifyOkClicked();
  void notifyApplyClicked();
  void notifyCancelClicked();

private:
  void saveSettings();

  void setApplyingChanges(bool applyingChanges);

  std::unique_ptr<SettingsModel> m_model;
  ISettingsView *m_view;
  ISettings *m_parent;
};

} // namespace CustomInterfaces
} // namespace MantidQt
