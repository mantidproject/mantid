// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidKernel/System.h"

#include <QString>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
/** SANSBackgroundCorrectionSettings: A simple communication class between for
the SANSBackgroundCorrectionWidget
*/
class MANTIDQT_ISISSANS_DLL SANSBackgroundCorrectionSettings {
public:
  SANSBackgroundCorrectionSettings(const QString &runNumber, bool useMean, bool useMon, const QString &monNumber);

  bool hasValidSettings();

  QString getRunNumber() const;
  QString getMonNumber() const;
  bool getUseMean() const;
  bool getUseMon() const;

private:
  boost::optional<bool> m_hasValidSettings;
  const QString m_runNumber;
  const bool m_useMean;
  const bool m_useMon;
  const QString m_monNumber;
};

} // namespace CustomInterfaces
} // namespace MantidQt
