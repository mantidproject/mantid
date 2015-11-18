#include "MantidQtCustomInterfaces/SANSBackgroundCorrectionSettings.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  SANSBackgroundCorrectionSettings::SANSBackgroundCorrectionSettings(QString runNumber, bool useMean,
    bool useDet, bool useMon, QString monNumber) : m_runNumber(runNumber), m_useMean(useMean), m_useDet(useDet), m_useMon(useMon), m_monNumber(monNumber) {
    hasValidSettings();
  }

  /**
   * Check if the settings stored in the object are valid
   * @returns true if they are valid, else false
   */
  bool SANSBackgroundCorrectionSettings::hasValidSettings() {
    if (!m_hasValidSettings) {
      auto isValid = true;

      // The run number must not be empty
      isValid = m_runNumber.isEmpty() ? false : true;

      // At least the detector or the monitor selection needs to be enabled
      isValid = m_useDet == false && m_useMon == false ? false : true;

      m_hasValidSettings = isValid;
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
  * Get the setting if the entire detector is to be used
  * @returns the setting or default to true
  */
  bool SANSBackgroundCorrectionSettings::getUseDet() const {
    return m_hasValidSettings ? m_useDet : true;
  }

  /**
  * Get the setting if monitors are to be used
  * @returns the setting or default to false
  */
  bool SANSBackgroundCorrectionSettings::getUseMon() const {
    return m_hasValidSettings ? m_useMon : false;
  }
}
}