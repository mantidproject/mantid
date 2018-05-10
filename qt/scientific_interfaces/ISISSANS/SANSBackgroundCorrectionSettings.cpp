#include "SANSBackgroundCorrectionSettings.h"

namespace MantidQt {
namespace CustomInterfaces {
SANSBackgroundCorrectionSettings::SANSBackgroundCorrectionSettings(
    QString runNumber, bool useMean, bool useMon, QString monNumber)
    : m_runNumber(runNumber), m_useMean(useMean), m_useMon(useMon),
      m_monNumber(monNumber) {
  hasValidSettings();
}

/**
 * Check if the settings stored in the object are valid
 * @returns true if they are valid, else false
 */
bool SANSBackgroundCorrectionSettings::hasValidSettings() {
  if (!m_hasValidSettings) {
    // The run number must not be empty
    m_hasValidSettings = m_runNumber.isEmpty() ? false : true;
  }

  return m_hasValidSettings.get();
}

/**
 * Get the run number
 * @returns a run number or an empty string
 */
QString SANSBackgroundCorrectionSettings::getRunNumber() const {
  return m_hasValidSettings ? m_runNumber : QString();
}

/**
 * Get a string list with monitor numbers
 * @returns a run number or an empty string
 */
QString SANSBackgroundCorrectionSettings::getMonNumber() const {
  return m_hasValidSettings ? m_monNumber : QString();
}

/**
 * Get the setting if mean is to be used
 * @returns the setting or default to false
 */
bool SANSBackgroundCorrectionSettings::getUseMean() const {
  return m_hasValidSettings ? m_useMean : false;
}

/**
 * Get the setting if monitors or detectors are to be used
 * @returns the setting or default to false
 */
bool SANSBackgroundCorrectionSettings::getUseMon() const {
  return m_hasValidSettings ? m_useMon : false;
}
} // namespace CustomInterfaces
} // namespace MantidQt
