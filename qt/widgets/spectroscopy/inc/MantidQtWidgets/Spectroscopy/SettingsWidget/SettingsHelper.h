// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../DllConfig.h"

#include <string>

#include <QStringList>

class QSettings;

namespace MantidQt {
namespace CustomInterfaces {
namespace SettingsHelper {

class MANTID_SPECTROSCOPY_DLL SpectroscopySettings {
public:
  explicit SpectroscopySettings(bool restrictInputByName = true, bool externalPlotErrorBars = false,
                                bool loadHistory = true, QStringList developerFeatureFlags = {});

  [[nodiscard]] bool restrictInputByName() const;
  [[nodiscard]] bool externalPlotErrorBars() const;
  [[nodiscard]] bool loadHistory() const;
  [[nodiscard]] const QStringList &developerFeatureFlags() const;

private:
  bool m_restrictInputByName;
  bool m_externalPlotErrorBars;
  bool m_loadHistory;
  QStringList m_developerFeatureFlags;
};

/// Query an already-positioned store without changing state or writing settings.
[[nodiscard]] MANTID_SPECTROSCOPY_DLL SpectroscopySettings readSettings(const QSettings &settings);
/// Query the normal Indirect Settings group without changing application state.
[[nodiscard]] MANTID_SPECTROSCOPY_DLL SpectroscopySettings readSettings();
/// Persist only the values from a snapshot to an already-positioned store.
MANTID_SPECTROSCOPY_DLL void saveSettings(QSettings &settings, const SpectroscopySettings &values);
/// Persist a snapshot to the normal Indirect Settings group.
MANTID_SPECTROSCOPY_DLL void saveSettings(const SpectroscopySettings &values);

MANTID_SPECTROSCOPY_DLL bool restrictInputDataByName();
MANTID_SPECTROSCOPY_DLL bool externalPlotErrorBars();
MANTID_SPECTROSCOPY_DLL bool loadHistory();
MANTID_SPECTROSCOPY_DLL QStringList developerFeatureFlags();
MANTID_SPECTROSCOPY_DLL bool hasDevelopmentFlag(std::string const &flag);
} // namespace SettingsHelper
} // namespace CustomInterfaces
} // namespace MantidQt
