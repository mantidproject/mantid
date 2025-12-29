// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once
#include "MantidAPI/ISpectrum.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

class ProcessBankTaskBase {
public:
  ProcessBankTaskBase(std::vector<std::string> &bankEntryNames, const BankCalibrationFactory &calibFactory);
  const std::string &bankName(const size_t wksp_index) const;
  BankCalibration getCalibration(const std::string &tof_unit, const size_t wksp_index) const;

private:
  const std::vector<std::string> m_bankEntries;
  /// used to generate actual calibration
  const BankCalibrationFactory &m_calibFactory;
};

void copyDataToSpectrum(const std::vector<uint32_t> &y_temp, API::ISpectrum *spectrum);
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
