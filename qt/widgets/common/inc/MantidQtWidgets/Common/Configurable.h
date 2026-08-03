// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"

class QSettings;

namespace MantidQt {
namespace MantidWidgets {

/**
 * @page settings_lifecycle Persistent settings lifecycle
 *
 * Settings APIs should separate persistent I/O from changes to widgets or
 * other in-memory state. New and migrated APIs use the following operations:
 *
 * - `readSettings(const QSettings &)` queries persistent storage and returns a
 *   `[[nodiscard]]` typed snapshot. It does not change application state, call
 *   a QSettings mutator, synchronize the store, or create a lock file.
 * - `restoreSettings(const Settings &)` applies a snapshot to application
 *   state. It receives no QSettings object and performs no persistent I/O.
 * - `captureSettings()` returns a typed snapshot of current application state
 *   without performing persistent I/O.
 * - `saveSettings(QSettings &, const Settings &)` is the persistent write
 *   operation. It accepts mutable storage and writes only its documented keys.
 *
 * A private initialization helper may compose reading and restoration, but it
 * should not expose that composition as `loadSettings`: the name does not say
 * whether persistent I/O occurs or whether application state is changed.
 * Functions that load scientific data are outside this terminology.
 *
 * Existing APIs that accept QSettings and immediately update a widget combine
 * reading and restoration. Their documentation must identify them as legacy
 * combined operations until their signatures are split.
 */

/**
 * Defines the in-memory half of the persistent settings lifecycle.
 * Persistent read and save operations belong to the concrete Settings type.
 */
template <typename Settings> class Configurable {
public:
  virtual ~Configurable() = default;
  /// Apply an already-read snapshot without persistent I/O.
  virtual void restoreSettings(const Settings &) = 0;
  /// Capture current in-memory state without persistent I/O.
  [[nodiscard]] virtual Settings captureSettings() const = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
