// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IOptionsDialogModel.h"
#include <map>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialogModel : public IOptionsDialogModel {
public:
  OptionsDialogModel();
  ~OptionsDialogModel() override = default;
  [[nodiscard]] OptionsDialogSettings defaultSettings() const override;
  [[nodiscard]] OptionsDialogSettings readSettings() const override;
  void saveSettings(OptionsDialogSettings const &settings) override;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
