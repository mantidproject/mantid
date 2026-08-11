// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QSettings>
#include <QString>
#include <QVariant>
#include <memory>

namespace MantidQt::MantidWidgets {

/** A QSettings facade that applies only effective changes.
 *
 * The default constructor owns a standard QSettings instance. A caller may
 * instead inject a QSettings instance, which remains owned by the caller.
 *
 * Values are compared after converting the stored value to the requested
 * value's type. This avoids unnecessary writes caused by type information
 * lost by INI storage. An absent key is still written, even when the requested
 * value resembles a caller-side default.
 */
class QSettingsChangeAware {
public:
  QSettingsChangeAware() : m_ownedSettings(std::make_unique<QSettings>()), m_settings(*m_ownedSettings) {}
  explicit QSettingsChangeAware(QSettings &settings) : m_settings(settings) {}

  QSettingsChangeAware(QSettingsChangeAware const &) = delete;
  QSettingsChangeAware &operator=(QSettingsChangeAware const &) = delete;

  /** Set a value only when the effective stored value differs.
   * @return true when QSettings::setValue was called.
   */
  bool setValue(QString const &key, QVariant const &value) {
    if (m_settings.contains(key) && valuesEqual(m_settings.value(key), value))
      return false;

    m_settings.setValue(key, value);
    m_changed = true;
    return true;
  }

  /** Remove a key or group only when it contains a value.
   * @return true when QSettings::remove was called.
   */
  bool remove(QString const &key) {
    if (!containsKeyOrChildren(key))
      return false;

    m_settings.remove(key);
    m_changed = true;
    return true;
  }

  [[nodiscard]] bool changed() const noexcept { return m_changed; }

private:
  static bool valuesEqual(QVariant current, QVariant const &requested) {
    if (current == requested)
      return true;
    if (!current.isValid() || !requested.isValid())
      return false;

    if (!current.convert(requested.metaType()))
      return false;
    return current == requested;
  }

  bool containsKeyOrChildren(QString const &key) {
    if (m_settings.contains(key))
      return true;
    if (key.isEmpty())
      return !m_settings.allKeys().isEmpty();

    m_settings.beginGroup(key);
    auto const hasChildren = !m_settings.allKeys().isEmpty();
    m_settings.endGroup();
    return hasChildren;
  }

  std::unique_ptr<QSettings> m_ownedSettings;
  QSettings &m_settings;
  bool m_changed{false};
};

} // namespace MantidQt::MantidWidgets
