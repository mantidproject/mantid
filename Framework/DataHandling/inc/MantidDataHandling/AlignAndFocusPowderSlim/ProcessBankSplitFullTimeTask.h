// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/NexusLoader.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankTaskBase.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/DateAndTime.h"
#include <H5Cpp.h>
#include <MantidAPI/Progress.h>
#include <map>
#include <set>
#include <tbb/tbb.h>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

// We need to include additional pulses because time-of-flight can be greater than a couple of pulse periods.
// This is in nanoseconds. So four pulses of 1/60s is 66666666 ns.
const int64_t PULSETIME_OFFSET = 66666666;

class MANTID_DATAHANDLING_DLL ProcessBankSplitFullTimeTask : public ProcessBankTaskBase {
public:
  ProcessBankSplitFullTimeTask(std::vector<std::string> &bankEntryNames, H5::H5File &h5file,
                               std::shared_ptr<NexusLoader> loader, std::vector<int> &workspaceIndices,
                               std::vector<API::MatrixWorkspace_sptr> &wksps,
                               const BankCalibrationFactory &calibFactory, const size_t events_per_chunk,
                               const size_t grainsize_event,
                               const std::map<Mantid::Types::Core::DateAndTime, int> &splitterMap,
                               std::shared_ptr<API::Progress> &progress);

  void operator()(const tbb::blocked_range<size_t> &range) const;

private:
  H5::H5File m_h5file;
  std::shared_ptr<const NexusLoader> m_loader;
  std::vector<int> m_workspaceIndices;
  std::vector<API::MatrixWorkspace_sptr> m_wksps;
  /// number of events to read from disk at one time
  const size_t m_events_per_chunk;
  const std::map<Mantid::Types::Core::DateAndTime, int> m_splitterMap;
  /// number of events to histogram in a single thread
  const size_t m_grainsize_event;
  std::shared_ptr<API::Progress> m_progress;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
