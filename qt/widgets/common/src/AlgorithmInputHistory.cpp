//----------------------------------
// Includes
//----------------------------------
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidAPI/IAlgorithm.h"

#include <QSettings>
#include <QStringList>

using namespace MantidQt::API;

//----------------------------------
// Public member functions
//----------------------------------

/**
 * Constructor
 */
AbstractAlgorithmInputHistory::AbstractAlgorithmInputHistory(
    QString settingsGroup)
    : m_lastInput(), m_previousDirectory(""), m_algorithmsGroup(settingsGroup),
      m_dirKey("LastDirectory") {
  // Fill the stored map from the QSettings information
  load();
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
void AbstractAlgorithmInputHistory::storeNewValue(
    const QString &algName, const QPair<QString, QString> &property) {
  m_lastInput[algName][property.first] = property.second;
}

/**
 * Clear all stored values associated with a particular algorithm
 */
void AbstractAlgorithmInputHistory::clearAlgorithmInput(
    const QString &algName) {
  if (m_lastInput.contains(algName))
    m_lastInput[algName].clear();
}

/**
 * Retrieve an old parameter value
 * @param algName :: The name of the algorithm
 * @param propName :: The name of the property
 */
QString
AbstractAlgorithmInputHistory::previousInput(const QString &algName,
                                             const QString &propName) const {
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
void AbstractAlgorithmInputHistory::setPreviousDirectory(
    const QString &lastdir) {
  m_previousDirectory = lastdir;
}

/// Get the directory that was accessed when the previous open file dialog was
/// used
const QString &AbstractAlgorithmInputHistory::getPreviousDirectory() const {
  return m_previousDirectory;
}

/**
 * Save the stored information to persistent storage
 */
void AbstractAlgorithmInputHistory::save() const {
  QSettings settings;
  settings.beginGroup(m_algorithmsGroup);
  QHashIterator<QString, QHash<QString, QString>> inputHistory(m_lastInput);
  while (inputHistory.hasNext()) {
    inputHistory.next();
    settings.beginGroup(inputHistory.key());
    // Remove all keys for this group that exist at the moment
    settings.remove("");
    QHash<QString, QString>::const_iterator iend = inputHistory.value().end();
    for (QHash<QString, QString>::const_iterator itr =
             inputHistory.value().begin();
         itr != iend; ++itr) {
      settings.setValue(itr.key(), itr.value());
    }
    settings.endGroup();
  }

  // Store the previous directory
  settings.setValue(m_dirKey, m_previousDirectory);

  settings.endGroup();
}

//----------------------------------
// Private member functions
//----------------------------------

/**
 * Load any values that are available from persistent storage. Note: this
 * clears all currently values stored
 */
void AbstractAlgorithmInputHistory::load() {
  m_lastInput.clear();
  QSettings settings;
  settings.beginGroup(m_algorithmsGroup);
  //  QStringList algorithms = settings.childGroups();
  QListIterator<QString> algNames(settings.childGroups());

  // Each property is a key of the algorithm group
  while (algNames.hasNext()) {
    QHash<QString, QString> algorithmProperties;
    QString group = algNames.next();
    settings.beginGroup(group);
    QListIterator<QString> properties(settings.childKeys());
    while (properties.hasNext()) {
      QString propName = properties.next();
      QString value = settings.value(propName).toString();
      if (!value.isEmpty())
        algorithmProperties.insert(propName, value);
    }
    m_lastInput.insert(group, algorithmProperties);
    settings.endGroup();
  }

  // The previous dir
  m_previousDirectory = settings.value(m_dirKey).toString();

  settings.endGroup();
}
