// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once
#include "MantidAPI/ISpectrum.h"

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

class ProcessBankTaskBase {
public:
  ProcessBankTaskBase(std::vector<std::string> &bankEntryNames);
  const std::string &bankName(const size_t wksp_index) const;

private:
  const std::vector<std::string> m_bankEntries;
};

void copyDataToSpectrum(const std::vector<uint32_t> &y_temp, API::ISpectrum *spectrum);
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
