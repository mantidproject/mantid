#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"

#include <vector>
#include <set>
#include <memory>
#include <boost/bind.hpp>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {

/**
 * Copy all logData properties from the 'from' workspace to the 'to'
 * workspace. Does not use CopyLogs as a child algorithm (this is a
 * simple copy and the workspace is not yet in the ADS).
 *
 * @param from source of log entries
 * @param to workspace where to add the log entries
 */
void copyLogs(const EventWorkspace_sptr &from, EventWorkspace_sptr &to) {
  // from the logs, get all the properties that don't overwrite any
  // prop. already set in the sink workspace (like 'filename').
  auto props = from->mutableRun().getLogData();
  for (auto &prop : props) {
    if (!to->mutableRun().hasProperty(prop->name())) {
      to->mutableRun().addLogData(prop->clone());
    }
  }
}
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
EventWorkspaceCollection::EventWorkspaceCollection()
    : m_WsVec(1, createEmptyEventWorkspace()) {}

//-----------------------------------------------------------------------------
/**
* Create a blank event workspace
* @returns A shared pointer to a new empty EventWorkspace object
*/
EventWorkspace_sptr
EventWorkspaceCollection::createEmptyEventWorkspace() const {
  // Create the output workspace
  EventWorkspace_sptr eventWS(new EventWorkspace());
  // Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't
  //   matter
  eventWS->initialize(1, 1, 1);

  // Set the units
  eventWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  eventWS->setYUnit("Counts");

  return eventWS;
}

void EventWorkspaceCollection::setNPeriods(
    size_t nPeriods,
    std::unique_ptr<const TimeSeriesProperty<int>> &periodLog) {

  // Create vector where size is the number of periods and initialize workspaces
  // in each.
  auto temp = m_WsVec[0];
  m_WsVec = std::vector<DataObjects::EventWorkspace_sptr>(nPeriods);

  std::vector<int> periodNumbers = periodLog->valuesAsVector();
  std::unordered_set<int> uniquePeriods(periodNumbers.begin(),
                                        periodNumbers.end());
  const bool addBoolTimeSeries = (uniquePeriods.size() == nPeriods);

  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    const int periodNumber = int(i + 1);
    m_WsVec[i] = createEmptyEventWorkspace();
    m_WsVec[i]->copyExperimentInfoFrom(temp.get());
    if (addBoolTimeSeries) {
      std::stringstream buffer;
      buffer << "period " << periodNumber;
      auto *periodBoolLog = new Kernel::TimeSeriesProperty<bool>(buffer.str());
      for (int j = 0; j < int(periodLog->size()); ++j) {
        periodBoolLog->addValue(periodLog->nthTime(j),
                                periodNumber == periodLog->nthValue(j));
      }
      Run &mutableRun = m_WsVec[i]->mutableRun();
      mutableRun.addProperty(periodBoolLog);

      Kernel::PropertyWithValue<int> *currentPeriodProperty =
          new Kernel::PropertyWithValue<int>("current_period", periodNumber);
      mutableRun.addProperty(currentPeriodProperty);
    }

    copyLogs(
        temp,
        m_WsVec[i]); // Copy all logs from dummy workspace to period workspaces.
    m_WsVec[i]->setInstrument(temp->getInstrument());
  }
}

void EventWorkspaceCollection::reserveEventListAt(size_t wi, size_t size) {
  for (auto &ws : m_WsVec) {
    ws->getEventList(wi).reserve(size);
  }
}

size_t EventWorkspaceCollection::nPeriods() const { return m_WsVec.size(); }

DataObjects::EventWorkspace_sptr
EventWorkspaceCollection::getSingleHeldWorkspace() {
  return m_WsVec.front();
}

API::Workspace_sptr EventWorkspaceCollection::combinedWorkspace() {
  API::Workspace_sptr final;
  if (this->nPeriods() == 1) {
    final = getSingleHeldWorkspace();
  } else {
    auto wsg = boost::make_shared<API::WorkspaceGroup>();
    for (auto &ws : m_WsVec) {
      wsg->addWorkspace(ws);
    }
    final = wsg;
  }
  return final;
}

Geometry::Instrument_const_sptr
EventWorkspaceCollection::getInstrument() const {
  return m_WsVec[0]->getInstrument();
}
const API::Run &EventWorkspaceCollection::run() const {
  return m_WsVec[0]->run();
}
API::Run &EventWorkspaceCollection::mutableRun() {
  return m_WsVec[0]->mutableRun();
}
API::Sample &EventWorkspaceCollection::mutableSample() {
  return m_WsVec[0]->mutableSample();
}
Mantid::API::ISpectrum *
EventWorkspaceCollection::getSpectrum(const size_t index) {
  return m_WsVec[0]->getSpectrum(index);
}
const Mantid::API::ISpectrum *
EventWorkspaceCollection::getSpectrum(const size_t index) const {
  return m_WsVec[0]->getSpectrum(index);
}
void EventWorkspaceCollection::setSpectrumNumbersFromUniqueSpectra(
    const std::set<int> uniqueSpectra) {
  // For each workspace, update all the spectrum numbers
  for (auto &ws : m_WsVec) {
    size_t counter = 0;
    for (auto spectrum : uniqueSpectra) {
      ws->getSpectrum(counter)->setSpectrumNo(spectrum);
      ++counter;
    }
  }
}

void EventWorkspaceCollection::setSpectrumNumberForAllPeriods(
    const size_t spectrumNumber, const specnum_t specid) {
  for (auto &ws : m_WsVec) {
    auto spec = ws->getSpectrum(spectrumNumber);
    spec->setSpectrumNo(specid);
  }
}

void EventWorkspaceCollection::setDetectorIdsForAllPeriods(
    const size_t spectrumNumber, const detid_t id) {
  for (auto &ws : m_WsVec) {
    auto spec = ws->getSpectrum(spectrumNumber);
    spec->setDetectorID(id);
  }
}

Mantid::API::Axis *EventWorkspaceCollection::getAxis(const size_t &i) const {
  return m_WsVec[0]->getAxis(i);
}
size_t EventWorkspaceCollection::getNumberHistograms() const {
  return m_WsVec[0]->getNumberHistograms();
}
const DataObjects::EventList &
EventWorkspaceCollection::getEventList(const size_t workspace_index) const {
  return m_WsVec[0]->getEventList(
      workspace_index); // TODO need to know PERIOD number TOO
}

const DataObjects::EventList &
EventWorkspaceCollection::getEventList(const size_t workspace_index,
                                       const size_t periodNumber) const {
  return m_WsVec[periodNumber]->getEventList(workspace_index);
}

DataObjects::EventList &
EventWorkspaceCollection::getEventList(const size_t workspace_index,
                                       const size_t periodNumber) {
  return m_WsVec[periodNumber]->getEventList(workspace_index);
}

DataObjects::EventList &
EventWorkspaceCollection::getEventList(const std::size_t workspace_index) {
  return m_WsVec[0]->getEventList(
      workspace_index); // TODO need to know PERIOD number TOO
}

std::vector<size_t> EventWorkspaceCollection::getSpectrumToWorkspaceIndexVector(
    Mantid::specnum_t &offset) const {
  return m_WsVec[0]->getSpectrumToWorkspaceIndexVector(offset);
}
std::vector<size_t>
EventWorkspaceCollection::getDetectorIDToWorkspaceIndexVector(
    Mantid::specnum_t &offset, bool dothrow) const {
  return m_WsVec[0]->getDetectorIDToWorkspaceIndexVector(offset, dothrow);
}

Kernel::DateAndTime EventWorkspaceCollection::getFirstPulseTime() const {
  return m_WsVec[0]->getFirstPulseTime();
}
void EventWorkspaceCollection::setAllX(HistogramData::BinEdges &x) {
  for (auto &ws : m_WsVec) {
    ws->setAllX(x);
  }
}
size_t EventWorkspaceCollection::getNumberEvents() const {
  return m_WsVec[0]->getNumberEvents(); // Should be the sum across all periods?
}
void EventWorkspaceCollection::resizeTo(const size_t size) {
  for (auto &ws : m_WsVec) {
    ws->resizeTo(size); // Creates the EventLists
  }
}
void EventWorkspaceCollection::padSpectra(const std::vector<int32_t> &padding) {
  for (auto &ws : m_WsVec) {
    ws->padSpectra(padding); // Set detector ids and spectrum numbers
  }
}
void EventWorkspaceCollection::setInstrument(
    const Geometry::Instrument_const_sptr &inst) {
  for (auto &ws : m_WsVec) {
    ws->setInstrument(inst);
  }
}
void EventWorkspaceCollection::setMonitorWorkspace(
    const boost::shared_ptr<API::MatrixWorkspace> &monitorWS) {
  for (auto &ws : m_WsVec) {
    ws->setMonitorWorkspace(
        monitorWS); // TODO, do we really set the same monitor on all periods???
  }
}
void EventWorkspaceCollection::updateSpectraUsing(
    const API::SpectrumDetectorMapping &map) {
  for (auto &ws : m_WsVec) {
    ws->updateSpectraUsing(map);
  }
}

DataObjects::EventList *EventWorkspaceCollection::getEventListPtr(size_t i) {
  return m_WsVec[0]->getEventListPtr(
      i); // TODO, just take from the first workspace
}

void EventWorkspaceCollection::populateInstrumentParameters() {
  for (auto &ws : m_WsVec) {
    ws->populateInstrumentParameters();
  }
}

void EventWorkspaceCollection::setGeometryFlag(const int flag) {
  for (auto &ws : m_WsVec) {
    ws->mutableSample().setGeometryFlag(flag);
  }
}

void EventWorkspaceCollection::setThickness(const float flag) {
  for (auto &ws : m_WsVec) {
    ws->mutableSample().setThickness(flag);
  }
}
void EventWorkspaceCollection::setHeight(const float flag) {
  for (auto &ws : m_WsVec) {
    ws->mutableSample().setHeight(flag);
  }
}
void EventWorkspaceCollection::setWidth(const float flag) {
  for (auto &ws : m_WsVec) {
    ws->mutableSample().setWidth(flag);
  }
}

void EventWorkspaceCollection::setTitle(std::string title) {
  for (auto &ws : m_WsVec) {
    ws->setTitle(title);
  }
}

void EventWorkspaceCollection::applyFilter(
    boost::function<void(MatrixWorkspace_sptr)> func) {
  for (auto &ws : m_WsVec) {
    func(ws);
  }
}

//-----------------------------------------------------------------------------
/** Returns true if the EventWorkspace is safe for multithreaded operations.
 */
bool EventWorkspaceCollection::threadSafe() const {
  // Since there is a mutex lock around sorting, EventWorkspaces are always
  // safe.
  return true;
}

} // namespace DataHandling
} // namespace Mantid
