// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "NexusLoader.h"
#include <H5Cpp.h>
#include <MantidAPI/Progress.h>
#include <map>
#include <set>
#include <tbb/tbb.h>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

class ProcessBankSplitTask {
public:
  ProcessBankSplitTask(std::vector<std::string> &bankEntryNames, H5::H5File &h5file, const bool is_time_filtered,
                       std::vector<int> &workspaceIndices, std::vector<API::MatrixWorkspace_sptr> &wksps,
                       const std::map<detid_t, double> &calibration, const std::set<detid_t> &masked,
                       const size_t events_per_chunk, const size_t grainsize_event,
                       std::vector<std::pair<int, PulseROI>> target_to_pulse_indices,
                       std::shared_ptr<API::Progress> &progress);

  void operator()(const tbb::blocked_range<size_t> &range) const;

private:
  H5::H5File m_h5file;
  const std::vector<std::string> m_bankEntries;
  mutable NexusLoader m_loader;
  std::vector<int> m_workspaceIndices;
  std::vector<API::MatrixWorkspace_sptr> m_wksps;
  const std::map<detid_t, double> m_calibration; // detid: 1/difc
  const std::set<detid_t> m_masked;
  /// number of events to read from disk at one time
  const size_t m_events_per_chunk;
  /// number of events to histogram in a single thread
  const size_t m_grainsize_event;
  std::shared_ptr<API::Progress> m_progress;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
