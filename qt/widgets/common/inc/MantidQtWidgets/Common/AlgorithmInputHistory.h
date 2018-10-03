// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_ALGORITHMINPUTHISTORY_H_
#define MANTIDQT_API_ALGORITHMINPUTHISTORY_H_

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

/** This abstract class deals with the loading and saving of previous algorithm
    property values to/from MantidPlot's QSettings.
*/
class EXPORT_OPT_MANTIDQT_COMMON AbstractAlgorithmInputHistory
    : public MantidWidgets::Configurable {
public:
  AbstractAlgorithmInputHistory(const AbstractAlgorithmInputHistory &) = delete;
  AbstractAlgorithmInputHistory &
  operator=(const AbstractAlgorithmInputHistory &) = delete;
  /// Abstract destructor
  virtual ~AbstractAlgorithmInputHistory() = 0;

  /// Update the old values that are stored here. Only valid
  /// values are stored here
  void storeNewValue(const QString &algName,
                     const QPair<QString, QString> &property);

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

  /// @copydoc MantidWidgets::Configurable::readSettings
  void readSettings(const QSettings &storage) override;

  /// @copydoc MantidWidgets::Configurable::writeSettings
  void writeSettings(QSettings &storage) const override;

protected:
  /// Constructor
  AbstractAlgorithmInputHistory(QString settingsGroup);

private:
  /// Load any values that are available from persistent storage
  void load();

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

class EXPORT_OPT_MANTIDQT_COMMON AlgorithmInputHistoryImpl
    : public AbstractAlgorithmInputHistory {
private:
  AlgorithmInputHistoryImpl()
      : AbstractAlgorithmInputHistory("Mantid/Algorithms") {}
  ~AlgorithmInputHistoryImpl() override {}

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmInputHistoryImpl>;
};

using AlgorithmInputHistory =
    Mantid::Kernel::SingletonHolder<AlgorithmInputHistoryImpl>;
} // namespace API
} // namespace MantidQt

namespace Mantid {
namespace Kernel {
EXTERN_MANTIDQT_COMMON template class EXPORT_OPT_MANTIDQT_COMMON
    Mantid::Kernel::SingletonHolder<MantidQt::API::AlgorithmInputHistoryImpl>;
}
} // namespace Mantid

#endif // ALGORITHMINPUTHISTORY_H_
