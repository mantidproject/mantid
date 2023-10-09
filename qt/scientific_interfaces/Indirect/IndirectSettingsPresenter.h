// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectSettingsModel.h"
#include "IndirectSettingsView.h"

#include <memory>

namespace MantidQt {
namespace CustomInterfaces {
class IIndirectSettings;

class MANTIDQT_INDIRECT_DLL IndirectSettingsPresenter {

public:
  explicit IndirectSettingsPresenter(std::unique_ptr<IndirectSettingsModel> model, IIndirectSettingsView *view);

  QWidget *getView();
  void subscribeParent(IIndirectSettings *parent);

  void loadSettings();

  void notifyOkClicked();
  void notifyApplyClicked();
  void notifyCancelClicked();

private:
  void saveSettings();

  void setApplyingChanges(bool applyingChanges);

  std::unique_ptr<IndirectSettingsModel> m_model;
  IIndirectSettingsView *m_view;
  IIndirectSettings *m_parent;
};

} // namespace CustomInterfaces
} // namespace MantidQt
