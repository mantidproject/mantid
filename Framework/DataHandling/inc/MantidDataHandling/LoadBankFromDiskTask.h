// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADBANKFROMDISKTASK_H_
#define MANTID_DATAHANDLING_LOADBANKFROMDISKTASK_H_

#include "MantidAPI/Progress.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"

#include <nexus/NeXusFile.hpp>

class BankPulseTimes;

namespace Mantid {
namespace DataHandling {
class DefaultEventLoader;

/** This task does the disk IO from loading the NXS file, and so will be on a
  disk IO mutex
*/
class MANTID_DATAHANDLING_DLL LoadBankFromDiskTask : public Kernel::Task {

public:
  LoadBankFromDiskTask(DefaultEventLoader &loader,
                       const std::string &entry_name,
                       const std::string &entry_type,
                       const std::size_t numEvents,
                       const bool oldNeXusFileNames, API::Progress *prog,
                       boost::shared_ptr<std::mutex> ioMutex,
                       Kernel::ThreadScheduler &scheduler,
                       const std::vector<int> &framePeriodNumbers);

  void run() override;

private:
  void loadPulseTimes(::NeXus::File &file);
  std::vector<uint64_t> loadEventIndex(::NeXus::File &file);
  void prepareEventId(::NeXus::File &file, int64_t &start_event,
                      int64_t &stop_event,
                      const std::vector<uint64_t> &event_index);
  std::unique_ptr<uint32_t[]> loadEventId(::NeXus::File &file);
  std::unique_ptr<float[]> loadTof(::NeXus::File &file);
  std::unique_ptr<float[]> loadEventWeights(::NeXus::File &file);
  int64_t recalculateDataSize(const int64_t &size);

  /// Algorithm being run
  DefaultEventLoader &m_loader;
  /// NXS path to bank
  std::string entry_name;
  /// NXS type
  std::string entry_type;
  /// Progress reporting
  API::Progress *prog;
  /// ThreadScheduler running this task
  Kernel::ThreadScheduler &scheduler;
  /// Object with the pulse times for this bank
  boost::shared_ptr<BankPulseTimes> thisBankPulseTimes;
  /// Did we get an error in loading
  bool m_loadError;
  /// Old names in the file?
  bool m_oldNexusFileNames;
  /// Index to load start at in the file
  std::vector<int64_t> m_loadStart;
  /// How much to load in the file
  std::vector<int64_t> m_loadSize;
  /// Minimum pixel ID in this data
  uint32_t m_min_id;
  /// Maximum pixel ID in this data
  uint32_t m_max_id;
  /// Flag for simulated data
  bool m_have_weight;
  /// Frame period numbers
  const std::vector<int> m_framePeriodNumbers;
}; // END-DEF-CLASS LoadBankFromDiskTask

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADBANKFROMDISKTASK_H_ */
