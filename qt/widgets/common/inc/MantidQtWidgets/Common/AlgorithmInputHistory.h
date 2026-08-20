// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidQtWidgets/Common/Configurable.h"
#include <QHash>
#include <QString>

namespace MantidQt {
namespace API {

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmInputHistorySettings {
public:
  using InputHistory = QHash<QString, QHash<QString, QString>>;

  AlgorithmInputHistorySettings(InputHistory lastInput = {}, QString previousDirectory = {});

  [[nodiscard]] InputHistory const &lastInput() const;
  [[nodiscard]] QString const &previousDirectory() const;

private:
  InputHistory m_lastInput;
  QString m_previousDirectory;
};

/** This abstract class deals with the loading and saving of previous algorithm
    property values to/from MantidPlot's QSettings.
*/
class EXPORT_OPT_MANTIDQT_COMMON AbstractAlgorithmInputHistory
    : public MantidWidgets::Configurable<AlgorithmInputHistorySettings> {
public:
  AbstractAlgorithmInputHistory(const AbstractAlgorithmInputHistory &) = delete;
  AbstractAlgorithmInputHistory &operator=(const AbstractAlgorithmInputHistory &) = delete;
  /// Abstract destructor
  virtual ~AbstractAlgorithmInputHistory() override = 0;

  /// Update the old values that are stored here. Only valid
  /// values are stored here
  void storeNewValue(const QString &algName, const std::pair<QString, QString> &property);

  /// Clear values for a particular algorithm
  void clearAlgorithmInput(const QString &algName);

  /// Retrieve an old parameter value
  QString previousInput(const QString &algName, const QString &propName) const;

  /// Set the directory that was accessed when the previous open file dialog was
  /// used
  void setPreviousDirectory(const QString &lastdir);

  /// Get the directory that was accessed when the previous open file dialog was
  /// used
  const QString &getPreviousDirectory() const;

  /// Save the values stored here to persistent storage
  void save() const;

  /// Query persistent storage without changing the history.
  [[nodiscard]] AlgorithmInputHistorySettings readSettings(const QSettings &storage) const;
  /// Apply an already-read snapshot without persistent I/O.
  void restoreSettings(const AlgorithmInputHistorySettings &settings) override;
  /// Capture current history without persistent I/O.
  [[nodiscard]] AlgorithmInputHistorySettings captureSettings() const override;
  /// Persist an explicit snapshot.
  void saveSettings(QSettings &storage, const AlgorithmInputHistorySettings &settings) const;

protected:
  /// Constructor
  AbstractAlgorithmInputHistory(const QString &settingsGroup);

private:
  /// Load any values that are available from persistent storage
  void initializeSettings();

  /// A map indexing the algorithm name and a list of property name:value pairs
  QHash<QString, QHash<QString, QString>> m_lastInput;

  /// The directory that last used by an open file dialog
  QString m_previousDirectory;

  /// The string denoting the group (in the QSettings) where the algorithm
  /// properties are stored
  QString m_algorithmsGroup;

  /// The string denoting the key for the previous dir storage
  QString m_dirKey;
};

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmInputHistoryImpl : public AbstractAlgorithmInputHistory {
private:
  AlgorithmInputHistoryImpl() : AbstractAlgorithmInputHistory("Mantid/Algorithms") {}
  ~AlgorithmInputHistoryImpl() override = default;

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmInputHistoryImpl>;
};

using AlgorithmInputHistory = Mantid::Kernel::SingletonHolder<AlgorithmInputHistoryImpl>;
} // namespace API
} // namespace MantidQt

namespace Mantid {
namespace Kernel {
EXTERN_MANTIDQT_COMMON template class EXPORT_OPT_MANTIDQT_COMMON
    Mantid::Kernel::SingletonHolder<MantidQt::API::AlgorithmInputHistoryImpl>;
}
} // namespace Mantid
