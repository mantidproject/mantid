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
  ~OptionsDialogModel() = default;
  void applyDefaultOptions(std::map<std::string, bool> &boolOptions, std::map<std::string, int> &intOptions) override;
  void loadSettings(std::map<std::string, bool> &boolOptions, std::map<std::string, int> &intOptions) override;
  void saveSettings(const std::map<std::string, bool> &boolOptions,
                    const std::map<std::string, int> &intOptions) override;

private:
  const std::string REFLECTOMETRY_SETTINGS_GROUP = "ISISReflectometryUI";
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
