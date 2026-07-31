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
 * Defines the legacy interface for an object to restore and write settings
 * that persist between instances.
 *
 * `readSettings` is a combined read-and-restore operation rather than the
 * query-only operation described by @ref settings_lifecycle. Implementations
 * may query the supplied const storage and update their own in-memory state,
 * but must not mutate or synchronize the storage. `writeSettings` is the
 * persistent write operation.
 */
class EXPORT_OPT_MANTIDQT_COMMON Configurable {
public:
  virtual ~Configurable() = default;
  /// Query settings and immediately restore the state of this object; does not write.
  virtual void readSettings(const QSettings &) = 0;
  /// Persist the current state of this object to the supplied mutable storage.
  virtual void writeSettings(QSettings &) const = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt
