// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <map>
#include <string>

class QSettings;

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL OptionsDialogSettings {
public:
  OptionsDialogSettings(std::map<std::string, bool> boolOptions = {}, std::map<std::string, int> intOptions = {});

  [[nodiscard]] std::map<std::string, bool> const &boolOptions() const;
  [[nodiscard]] std::map<std::string, int> const &intOptions() const;

  /// Query the supplied store without mutating or synchronizing it.
  [[nodiscard]] static OptionsDialogSettings readSettings(QSettings const &settings);
  /// Persist only the values in the supplied snapshot.
  static void saveSettings(QSettings &settings, OptionsDialogSettings const &values);

private:
  std::map<std::string, bool> m_boolOptions;
  std::map<std::string, int> m_intOptions;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL IOptionsDialogModel {
public:
  virtual ~IOptionsDialogModel() = default;
  [[nodiscard]] virtual OptionsDialogSettings defaultSettings() const = 0;
  /// Query persistent settings without changing model or presenter state.
  [[nodiscard]] virtual OptionsDialogSettings readSettings() const = 0;
  /// Persist the supplied immutable snapshot.
  virtual void saveSettings(OptionsDialogSettings const &settings) = 0;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
