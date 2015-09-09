#include "MantidDataHandling/EventWorkspaceCollection.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
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
void copyLogs(const EventWorkspace_sptr& from,
                                  EventWorkspace_sptr& to)
{
  // from the logs, get all the properties that don't overwrite any
  // prop. already set in the sink workspace (like 'filename').
  auto props = from->mutableRun().getLogData();
  for (size_t j=0; j<props.size(); j++) {
    if (!to->mutableRun().hasProperty(props[j]->name())) {
      to->mutableRun().addLogData(props[j]->clone());
    }
  }
}

}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
EventWorkspaceCollection::EventWorkspaceCollection(): m_WsVec(1, createEmptyEventWorkspace()) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
EventWorkspaceCollection::~EventWorkspaceCollection() {}

//-----------------------------------------------------------------------------
/**
* Create a blank event workspace
* @returns A shared pointer to a new empty EventWorkspace object
*/
EventWorkspace_sptr EventWorkspaceCollection::createEmptyEventWorkspace() const {
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

void EventWorkspaceCollection::setNPeriods(size_t nPeriods, std::unique_ptr<const TimeSeriesProperty<int> >& periodLog) {

  // Create vector where size is the number of periods and initialize workspaces in each.
  auto temp = m_WsVec[0];
  m_WsVec = std::vector<DataObjects::EventWorkspace_sptr>(
      nPeriods);

  std::vector<int> periodNumbers = periodLog->valuesAsVector();
  std::set<int> uniquePeriods(periodNumbers.begin(), periodNumbers.end());
  const bool addBoolTimeSeries = (uniquePeriods.size() == nPeriods);

  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    const int periodNumber = int(i+1);
    m_WsVec[i] =  createEmptyEventWorkspace();
    m_WsVec[i]->copyExperimentInfoFrom(temp.get());
    if(addBoolTimeSeries) {
        std::stringstream buffer;
        buffer << "period " << periodNumber;
        auto * periodBoolLog = new Kernel::TimeSeriesProperty<bool>(buffer.str());
        for(int j = 0; j < int(periodLog->size()); ++j){
            periodBoolLog->addValue(periodLog->nthTime(j), periodNumber == periodLog->nthValue(j));
        }
        Run& mutableRun =  m_WsVec[i]->mutableRun();
        mutableRun.addProperty(periodBoolLog);


        Kernel::PropertyWithValue<int> *currentPeriodProperty =
            new Kernel::PropertyWithValue<int>("current_period", periodNumber);
        mutableRun.addProperty(currentPeriodProperty);
    }

    copyLogs(temp, m_WsVec[i]); // Copy all logs from dummy workspace to period workspaces.
    m_WsVec[i]->setInstrument(temp->getInstrument());
  }
}


void EventWorkspaceCollection::reserveEventListAt(size_t wi, size_t size){
    for(size_t i = 0; i < m_WsVec.size(); ++i){
         m_WsVec[i]->getEventList(wi).reserve(size);
    }
}

size_t EventWorkspaceCollection::nPeriods() const {
    return m_WsVec.size();
}

DataObjects::EventWorkspace_sptr EventWorkspaceCollection::getSingleHeldWorkspace(){return m_WsVec.front();}

API::Workspace_sptr EventWorkspaceCollection::combinedWorkspace(){
    API::Workspace_sptr final;
    if( this->nPeriods() == 1 ){
        final = getSingleHeldWorkspace();
    }
    else{
         auto wsg = boost::make_shared<API::WorkspaceGroup>();
         for(size_t i = 0; i < m_WsVec.size(); ++i){
             wsg->addWorkspace(m_WsVec[i]);
         }
         final = wsg;
    }
    return final;
}

Geometry::Instrument_const_sptr EventWorkspaceCollection::getInstrument() const
{
    return m_WsVec[0]->getInstrument();
}
const API::Run &EventWorkspaceCollection::run() const
{
    return m_WsVec[0]->run();
}
API::Run &EventWorkspaceCollection::mutableRun() {
    return m_WsVec[0]->mutableRun();
}
API::Sample &EventWorkspaceCollection::mutableSample() {
  return m_WsVec[0]->mutableSample();
}
Mantid::API::ISpectrum* EventWorkspaceCollection::getSpectrum(const size_t index)  {
    return m_WsVec[0]->getSpectrum(index);
}
const Mantid::API::ISpectrum *EventWorkspaceCollection::getSpectrum(const size_t index) const{
    return m_WsVec[0]->getSpectrum(index);
}
void EventWorkspaceCollection::setSpectrumNumbersFromUniqueSpectra(const std::set<int> uniqueSpectra){
  // For each workspace, update all the spectrum numbers
  for (auto ws = m_WsVec.begin(); ws != m_WsVec.end(); ++ws) {
    size_t counter = 0;
    for (auto it = uniqueSpectra.begin(); it !=uniqueSpectra.end(); ++it) {
      (*ws)->getSpectrum(counter)->setSpectrumNo(*it);
      ++counter;
    }
  }
}

void EventWorkspaceCollection::setSpectrumNumberForAllPeriods(const size_t spectrumNumber, const specid_t specid) {
  for (auto ws = m_WsVec.begin(); ws != m_WsVec.end(); ++ws) {
    auto spec = (*ws)->getSpectrum(spectrumNumber);
    spec->setSpectrumNo(specid);
  }
}

void EventWorkspaceCollection::setDetectorIdsForAllPeriods(const size_t spectrumNumber, const detid_t id) {
  for (auto ws = m_WsVec.begin(); ws != m_WsVec.end(); ++ws) {
    auto spec = (*ws)->getSpectrum(spectrumNumber);
    spec->setDetectorID(id);
  }
}

Mantid::API::Axis* EventWorkspaceCollection::getAxis(const size_t& i) const {
    return m_WsVec[0]->getAxis(i);
}
size_t EventWorkspaceCollection::getNumberHistograms() const  {
    return m_WsVec[0]->getNumberHistograms();
}
const DataObjects::EventList& EventWorkspaceCollection::getEventList(const size_t workspace_index) const {
    return m_WsVec[0]->getEventList(workspace_index); // TODO need to know PERIOD number TOO
}

const DataObjects::EventList& EventWorkspaceCollection::getEventList(const size_t workspace_index, const size_t periodNumber) const {
    return m_WsVec[periodNumber]->getEventList(workspace_index);
}

DataObjects::EventList& EventWorkspaceCollection::getEventList(const size_t workspace_index, const size_t periodNumber)  {
    return m_WsVec[periodNumber]->getEventList(workspace_index);
}

DataObjects::EventList &EventWorkspaceCollection::getEventList(const std::size_t workspace_index){
    return m_WsVec[0]->getEventList(workspace_index); // TODO need to know PERIOD number TOO
}

void EventWorkspaceCollection::getSpectrumToWorkspaceIndexVector(std::vector<size_t>&out, Mantid::specid_t& offset) const  {
    return m_WsVec[0]->getSpectrumToWorkspaceIndexVector(out, offset);
}
void EventWorkspaceCollection::getDetectorIDToWorkspaceIndexVector(std::vector<size_t>&out, Mantid::specid_t& offset, bool dothrow) const{
    return m_WsVec[0]->getDetectorIDToWorkspaceIndexVector(out, offset, dothrow);
}

Kernel::DateAndTime EventWorkspaceCollection::getFirstPulseTime() const  {
    return m_WsVec[0]->getFirstPulseTime();
}
void EventWorkspaceCollection::setAllX(Kernel::cow_ptr<MantidVec>& x)  {
    for(size_t i = 0; i < m_WsVec.size(); ++i){
         m_WsVec[i]->setAllX(x);
    }
}
size_t EventWorkspaceCollection::getNumberEvents() const  {
    return m_WsVec[0]->getNumberEvents(); // Should be the sum across all periods?
}
void EventWorkspaceCollection::resizeTo(const size_t size)  {
    for(size_t i = 0; i < m_WsVec.size(); ++i){
         m_WsVec[i]->resizeTo(size); // Creates the EventLists
    }
}
void EventWorkspaceCollection::padSpectra(const std::vector<int32_t>& padding)  {
    for(size_t i = 0; i < m_WsVec.size(); ++i){
         m_WsVec[i]->padSpectra(padding); // Set detector ids and spectrum numbers
    }
}
void EventWorkspaceCollection::setInstrument(const Geometry::Instrument_const_sptr& inst)  {
    for(size_t i = 0; i < m_WsVec.size(); ++i){
         m_WsVec[i]->setInstrument(inst);
    }
}
void EventWorkspaceCollection::setMonitorWorkspace(const boost::shared_ptr<API::MatrixWorkspace>& monitorWS)  {
    for(size_t i = 0; i < m_WsVec.size(); ++i){
         m_WsVec[i]->setMonitorWorkspace(monitorWS); // TODO, do we really set the same monitor on all periods???
    }
}
void EventWorkspaceCollection::updateSpectraUsing(const API::SpectrumDetectorMapping& map)  {
    for(size_t i = 0; i < m_WsVec.size(); ++i){
         m_WsVec[i]->updateSpectraUsing(map);
    }

}

DataObjects::EventList* EventWorkspaceCollection::getEventListPtr(size_t i){
    return m_WsVec[0]->getEventListPtr(i);  // TODO, just take from the first workspace
}

void EventWorkspaceCollection::populateInstrumentParameters() {
  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    m_WsVec[i]->populateInstrumentParameters();
  }
}

void EventWorkspaceCollection::setGeometryFlag(const int flag) {
  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    m_WsVec[i]->mutableSample().setGeometryFlag(flag);
  }
}

void EventWorkspaceCollection::setThickness(const float flag) {
  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    m_WsVec[i]->mutableSample().setThickness(flag);
  }
}
void EventWorkspaceCollection::setHeight(const float flag) {
  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    m_WsVec[i]->mutableSample().setHeight(flag);
  }
}
void EventWorkspaceCollection::setWidth(const float flag) {
  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    m_WsVec[i]->mutableSample().setWidth(flag);
  }
}

void EventWorkspaceCollection::setTitle(std::string title) {
  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    m_WsVec[i]->setTitle(title);
  }
}

void EventWorkspaceCollection::applyFilter(boost::function<void (MatrixWorkspace_sptr)> func) {
  for (size_t i = 0; i < m_WsVec.size(); ++i) {
    func(m_WsVec[i]);
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
