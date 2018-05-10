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

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
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
  void loadEventIndex(::NeXus::File &file, std::vector<uint64_t> &event_index);
  void prepareEventId(::NeXus::File &file, size_t &start_event,
                      size_t &stop_event, std::vector<uint64_t> &event_index);
  void loadEventId(::NeXus::File &file);
  void loadTof(::NeXus::File &file);
  void loadEventWeights(::NeXus::File &file);
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
  std::vector<int> m_loadStart;
  /// How much to load in the file
  std::vector<int> m_loadSize;
  /// Event pixel ID data
  uint32_t *m_event_id;
  /// Minimum pixel ID in this data
  uint32_t m_min_id;
  /// Maximum pixel ID in this data
  uint32_t m_max_id;
  /// TOF data
  float *m_event_time_of_flight;
  /// Flag for simulated data
  bool m_have_weight;
  /// Event weights
  float *m_event_weight;
  /// Frame period numbers
  const std::vector<int> m_framePeriodNumbers;
}; // END-DEF-CLASS LoadBankFromDiskTask

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADBANKFROMDISKTASK_H_ */
