// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadErrorEventsNexus.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/PulseIndexer.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/NexusIOHelper.h"

namespace Mantid {
namespace DataHandling {
using namespace API;
using Mantid::Kernel::Direction;
using Mantid::Kernel::TimeSeriesProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadErrorEventsNexus)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadErrorEventsNexus::name() const { return "LoadErrorEventsNexus"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadErrorEventsNexus::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadErrorEventsNexus::category() const { return "DataHandling\\Nexus"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadErrorEventsNexus::summary() const { return "Load error events from NeXus file"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadErrorEventsNexus::init() {
  const std::vector<std::string> exts{".nxs.h5", ".nxs", "_event.nxs"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Event NeXus file to read, including its full or "
                  "relative path. ");
  declareProperty(std::make_unique<WorkspaceProperty<Mantid::DataObjects::EventWorkspace>>("OutputWorkspace", "",
                                                                                           Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadErrorEventsNexus::exec() {
  std::string filename = getPropertyValue("Filename");

  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create("EventWorkspace", 1, 2, 1);

  const Kernel::NexusHDF5Descriptor descriptor(filename);

  if (!descriptor.isEntry("/entry/bank_error_events"))
    throw std::runtime_error("entry bank_error_events does not exist");

  // load logs
  int nPeriods = 1;
  auto periodLog = std::make_unique<const TimeSeriesProperty<int>>("period_log");
  LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, outWS, *this, false, nPeriods, periodLog);

  if (nPeriods != 1)
    g_log.warning("This algorithm does not correctly handle period data");

  // Load the instrument
  LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, outWS, "entry", this, &descriptor);

  // load run metadata
  try {
    LoadEventNexus::loadEntryMetadata(filename, outWS, "entry", descriptor);
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading meta data: " << e.what() << '\n';
  }

  // load the data
  ::NeXus::File file(filename);

  file.openPath("/");
  file.openGroup("entry", "NXentry");
  file.openGroup("bank_error_events", "NXevent_data");

  const auto event_times = Mantid::NeXus::NeXusIOHelper::readNexusVector<float>(file, "event_time_offset");
  const auto event_index = std::make_shared<std::vector<uint64_t>>(
      Mantid::NeXus::NeXusIOHelper::readNexusVector<uint64_t>(file, "event_index"));
  const auto bankPulseTimes = std::make_shared<BankPulseTimes>(boost::ref(file), periodLog->valuesAsVector());

  file.closeGroup(); // bank_error_events
  file.closeGroup(); // entry
  file.close();

  // add event data to output workspace

  const auto numEvents = event_times.size();

  // this assumes that pulse indices are sorted
  if (!std::is_sorted(event_index->cbegin(), event_index->cend()))
    throw std::runtime_error("Event index is not sorted");

  auto eventWS = std::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(outWS);
  auto &ev = eventWS->getSpectrum(0);

  auto min_tof = std::numeric_limits<double>::max();
  auto max_tof = std::numeric_limits<double>::lowest();

  const PulseIndexer pulseIndexer(event_index, event_index->at(0), numEvents, "bank_error_events",
                                  std::vector<size_t>());

  for (const auto &pulseIter : pulseIndexer) {
    // Save the pulse time at this index for creating those events
    const auto &pulsetime = bankPulseTimes->pulseTime(pulseIter.pulseIndex);

    // loop through events associated with a single pulse
    for (std::size_t eventIndex = pulseIter.eventIndexStart; eventIndex < pulseIter.eventIndexStop; ++eventIndex) {
      const auto tof = static_cast<double>(event_times[eventIndex]);
      ev.addEventQuickly(Mantid::Types::Event::TofEvent(tof, pulsetime));
      min_tof = std::min(min_tof, tof);
      max_tof = std::max(max_tof, tof);
    }
  }

  g_log.information() << "Loaded " << numEvents << " events with TOF min = " << min_tof << ", max = " << max_tof
                      << "\n";

  eventWS->setAllX(HistogramData::BinEdges{min_tof, max_tof});

  outWS->getAxis(0)->setUnit("TOF");
  outWS->setYUnit("Counts");
  outWS->mutableRun().addProperty("Filename", filename);

  setProperty("OutputWorkspace", outWS);
}

} // namespace DataHandling
} // namespace Mantid
