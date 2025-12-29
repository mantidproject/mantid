// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankTaskBase.h"
#include "MantidKernel/Unit.h"

namespace {
const std::string MICROSEC("microseconds");
}

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {
ProcessBankTaskBase::ProcessBankTaskBase(std::vector<std::string> &bankEntryNames,
                                         const BankCalibrationFactory &calibFactory)
    : m_bankEntries(bankEntryNames), m_calibFactory(calibFactory) {}

const std::string &ProcessBankTaskBase::bankName(const size_t wksp_index) const { return m_bankEntries[wksp_index]; }
BankCalibration ProcessBankTaskBase::getCalibration(const std::string &tof_unit, const size_t wksp_index) const {
  // determine time conversion as a double
  const double time_conversion = Kernel::Units::timeConversionValue(tof_unit, MICROSEC);

  // now the calibration for the output group can be created
  // which detectors go into the current group - assumes ouput spectrum number is one more than workspace index
  return m_calibFactory.getCalibration(time_conversion, wksp_index);
}

void copyDataToSpectrum(const std::vector<uint32_t> &y_temp, API::ISpectrum *spectrum) {
  // copy the data out into the correct spectrum and calculate errors
  auto &y_values = spectrum->dataY();
  std::copy(y_temp.cbegin(), y_temp.cend(), y_values.begin());
  auto &e_values = spectrum->dataE();
  std::transform(y_temp.cbegin(), y_temp.cend(), e_values.begin(),
                 [](uint32_t y) { return std::sqrt(static_cast<double>(y)); });
}
}; // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
