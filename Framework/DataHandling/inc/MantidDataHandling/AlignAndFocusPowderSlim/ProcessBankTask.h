// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/NexusLoader.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankTaskBase.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/SpectraProcessingData.h"
#include "MantidGeometry/IDTypes.h"
#include <H5Cpp.h>
#include <map>
#include <set>
#include <tbb/tbb.h>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

class ProcessBankTask : public ProcessBankTaskBase {
public:
  ProcessBankTask(std::vector<std::string> &bankEntryNames, H5::H5File &h5file, std::shared_ptr<NexusLoader> loader,
                  SpectraProcessingData &processingData, const BankCalibrationFactory &calibFactory,
                  const size_t events_per_chunk, const size_t grainsize_event,
                  std::shared_ptr<API::Progress> &progress);

  void operator()(const tbb::blocked_range<size_t> &range) const;

private:
  H5::H5File m_h5file;
  SpectraProcessingData &m_processingData;
  /// number of events to read from disk at one time
  const size_t m_events_per_chunk;
  /// number of events to histogram in a single thread
  const size_t m_grainsize_event;
  std::shared_ptr<API::Progress> m_progress;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
