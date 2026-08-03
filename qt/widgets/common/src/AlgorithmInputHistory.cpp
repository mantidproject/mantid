// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------
// Includes
//----------------------------------
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidAPI/IAlgorithm.h"

#include <QSettings>
#include <utility>

using namespace MantidQt::API;

//----------------------------------
// Public member functions
//----------------------------------

AlgorithmInputHistorySettings::AlgorithmInputHistorySettings(InputHistory lastInput, QString previousDirectory)
    : m_lastInput(std::move(lastInput)), m_previousDirectory(std::move(previousDirectory)) {}

AlgorithmInputHistorySettings::InputHistory const &AlgorithmInputHistorySettings::lastInput() const {
  return m_lastInput;
}

QString const &AlgorithmInputHistorySettings::previousDirectory() const { return m_previousDirectory; }

/**
 * Constructor
 */
AbstractAlgorithmInputHistory::AbstractAlgorithmInputHistory(const QString &settingsGroup)
    : m_lastInput(), m_previousDirectory(""), m_algorithmsGroup(settingsGroup), m_dirKey("LastDirectory") {
  // Fill the stored map from the QSettings information
  initializeSettings();
}

/**
 * Destructor
 */
AbstractAlgorithmInputHistory::~AbstractAlgorithmInputHistory() {
  // Can't write the history out here since, in Linux, the singletons are
  // destroyed after
  // the QApplication object and then we get a crash
}

/**
 * Update the stored map with new property value. If the algorithm
 * doesn't exist then it is appended to the list otherwise the previous
 * value is overwritten.
 * @param algName :: The name of the algorithm
 * @param property :: A pair containing <name,value> of a property
 */
void AbstractAlgorithmInputHistory::storeNewValue(const QString &algName, const std::pair<QString, QString> &property) {
  m_lastInput[algName][property.first] = property.second;
}

/**
 * Clear all stored values associated with a particular algorithm
 */
void AbstractAlgorithmInputHistory::clearAlgorithmInput(const QString &algName) {
  if (m_lastInput.contains(algName))
    m_lastInput[algName].clear();
}

/**
 * Retrieve an old parameter value
 * @param algName :: The name of the algorithm
 * @param propName :: The name of the property
 */
QString AbstractAlgorithmInputHistory::previousInput(const QString &algName, const QString &propName) const {
  if (!m_lastInput.contains(algName))
    return "";

  if (m_lastInput.value(algName).contains(propName))
    return m_lastInput.value(algName).value(propName);
  else
    return "";
}

/**
 * Set the directory that was accessed when the previous open file dialog was
 * used
 * @param lastdir :: A QString giving the path of the directory that was last
 * accessed with a file dialog
 */
void AbstractAlgorithmInputHistory::setPreviousDirectory(const QString &lastdir) { m_previousDirectory = lastdir; }

/// Get the directory that was accessed when the previous open file dialog was
/// used
const QString &AbstractAlgorithmInputHistory::getPreviousDirectory() const { return m_previousDirectory; }

/**
 * Save the stored information to persistent storage
 */
void AbstractAlgorithmInputHistory::save() const {
  QSettings settings;
  saveSettings(settings, captureSettings());
}

AlgorithmInputHistorySettings AbstractAlgorithmInputHistory::readSettings(const QSettings &storage) const {
  auto prefix = m_algorithmsGroup;
  if (!prefix.isEmpty() && !prefix.endsWith('/'))
    prefix.append('/');

  AlgorithmInputHistorySettings::InputHistory inputHistory;
  QString previousDirectory;
  for (auto const &qualifiedName : storage.allKeys()) {
    if (!qualifiedName.startsWith(prefix))
      continue;
    auto const relativeName = qualifiedName.mid(prefix.size());
    if (relativeName == m_dirKey) {
      previousDirectory = storage.value(qualifiedName).toString();
      continue;
    }
    auto const separator = relativeName.indexOf('/');
    if (separator <= 0 || relativeName.indexOf('/', separator + 1) >= 0)
      continue;
    auto const value = storage.value(qualifiedName).toString();
    if (!value.isEmpty())
      inputHistory[relativeName.left(separator)].insert(relativeName.mid(separator + 1), value);
  }
  return AlgorithmInputHistorySettings(std::move(inputHistory), std::move(previousDirectory));
}

void AbstractAlgorithmInputHistory::restoreSettings(const AlgorithmInputHistorySettings &settings) {
  m_lastInput = settings.lastInput();
  m_previousDirectory = settings.previousDirectory();
}

AlgorithmInputHistorySettings AbstractAlgorithmInputHistory::captureSettings() const {
  return AlgorithmInputHistorySettings(m_lastInput, m_previousDirectory);
}

void AbstractAlgorithmInputHistory::saveSettings(QSettings &storage,
                                                 const AlgorithmInputHistorySettings &settings) const {
  auto prefix = m_algorithmsGroup;
  if (!prefix.isEmpty() && !prefix.endsWith('/'))
    prefix.append('/');
  QHashIterator<QString, QHash<QString, QString>> inputHistory(settings.lastInput());
  while (inputHistory.hasNext()) {
    inputHistory.next();
    auto const algorithmName = prefix + inputHistory.key();
    storage.remove(algorithmName);
    auto const algorithmPrefix = algorithmName + '/';
    for (auto itr = inputHistory.value().cbegin(); itr != inputHistory.value().cend(); ++itr)
      storage.setValue(algorithmPrefix + itr.key(), itr.value());
  }
  storage.setValue(prefix + m_dirKey, settings.previousDirectory());
}

//----------------------------------
// Private member functions
//----------------------------------

/**
 * Load any values that are available from persistent storage. Note: this
 * clears all currently values stored
 */
void AbstractAlgorithmInputHistory::initializeSettings() {
  QSettings settings;
  restoreSettings(readSettings(settings));
}
