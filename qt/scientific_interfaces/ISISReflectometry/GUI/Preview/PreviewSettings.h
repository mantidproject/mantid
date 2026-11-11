// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QSettings>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class PreviewSettings {
public:
  explicit PreviewSettings(bool useLegacyInstrumentView = false) : m_useLegacyInstrumentView(useLegacyInstrumentView) {}

  [[nodiscard]] bool useLegacyInstrumentView() const { return m_useLegacyInstrumentView; }

  /// Query persistent storage without changing application state or writing settings.
  [[nodiscard]] static PreviewSettings readSettings(const QSettings &settings) {
    return PreviewSettings(settings.value("InstrumentView/use_legacy_instrument_view", false).toBool());
  }

private:
  bool m_useLegacyInstrumentView;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
