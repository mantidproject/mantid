// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "TestDataListener.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MersenneTwister.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Kernel::ConfigService;
using Mantid::Types::Event::TofEvent;

namespace Mantid::LiveData {
DECLARE_LISTENER(TestDataListener)

/// Constructor
TestDataListener::TestDataListener()
    : LiveListener(), m_buffer(),
      m_rand(new Kernel::MersenneTwister(Types::Core::DateAndTime::getCurrentTime().totalNanoseconds(), 40000, 60000)),
      m_changeStatusAfter(0), m_newStatus(ILiveListener::EndRun) {
  // Set up the first workspace buffer
  this->createEmptyWorkspace();

  m_dataReset = false;
  m_timesCalled = 0;
  m_resetAfter = ConfigService::Instance().getValue<int>("testdatalistener.reset_after").value_or(0);
  m_changeStatusAfter = ConfigService::Instance().getValue<int>("testdatalistener.m_changeStatusAfter").value_or(0);
  int temp = ConfigService::Instance().getValue<int>("testdatalistener.m_newStatus").value_or(0);

  switch (temp) {
  case 0:
    m_newStatus = ILiveListener::NoRun;
    break;
  case 1:
    m_newStatus = ILiveListener::BeginRun;
    break;
  case 2:
    m_newStatus = ILiveListener::Running;
    break;
  case 4:
    m_newStatus = ILiveListener::EndRun;
    break;
  }
}

/// Destructor
TestDataListener::~TestDataListener() { delete m_rand; }

bool TestDataListener::connect(const Poco::Net::SocketAddress &) {
  // Do nothing for now. Later, put in stuff to help test failure modes.
  return true;
}

bool TestDataListener::isConnected() {
  // For the time being at least
  return true;
}

bool TestDataListener::dataReset() {
  // No support for reset signal
  return false;
}

ILiveListener::RunStatus TestDataListener::runStatus() {
  // For testing
  if (m_changeStatusAfter > 0 && m_timesCalled == m_changeStatusAfter) {
    return m_newStatus;
  } else
    // In a run by default
    return Running;
}

int TestDataListener::runNumber() const { return 999; }

void TestDataListener::start(Types::Core::DateAndTime /*startTime*/) // Ignore the start time
{}

/** Create the default empty event workspace */
void TestDataListener::createEmptyWorkspace() {
  // Create a new, empty workspace of the same dimensions and assign to the
  // buffer variable
  m_buffer = std::dynamic_pointer_cast<EventWorkspace>(WorkspaceFactory::Instance().create("EventWorkspace", 2, 2, 1));
  // Give detector IDs
  for (size_t i = 0; i < m_buffer->getNumberHistograms(); i++)
    m_buffer->getSpectrum(i).setDetectorID(detid_t(i));
  // Create in TOF units
  m_buffer->getAxis(0)->setUnit("TOF");
  // Load a fake instrument
  Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10, 0.1);
  m_buffer->setInstrument(inst);
  // Set a run number
  m_buffer->mutableRun().addProperty("run_number", std::string("999"));
  // Add a monitor workspace
  auto monitorWS = WorkspaceFactory::Instance().create("EventWorkspace", 1, 2, 1);
  WorkspaceFactory::Instance().initializeFromParent(*m_buffer, *monitorWS, true);
  monitorWS->dataX(0)[0] = 40000;
  monitorWS->dataX(0)[1] = 60000;
  m_buffer->setMonitorWorkspace(monitorWS);
}

std::shared_ptr<Workspace> TestDataListener::extractData() {
  m_dataReset = false;
  // Add a small number of uniformly distributed events to each event list.
  using namespace DataObjects;
  EventList &el1 = m_buffer->getSpectrum(0);
  EventList &el2 = m_buffer->getSpectrum(1);
  for (int i = 0; i < 100; ++i) {
    el1.addEventQuickly(TofEvent(m_rand->nextValue()));
    el2.addEventQuickly(TofEvent(m_rand->nextValue()));
  }
  auto mon_buffer = std::dynamic_pointer_cast<EventWorkspace>(m_buffer->monitorWorkspace());
  mon_buffer->getSpectrum(0).addEventQuickly(TofEvent(m_rand->nextValue()));

  // Copy the workspace pointer to a temporary variable
  EventWorkspace_sptr extracted = m_buffer;
  this->createEmptyWorkspace();

  m_timesCalled++;

  if (m_resetAfter > 0 && m_timesCalled >= m_resetAfter) {
    m_dataReset = true;
    m_timesCalled = 0;
  }

  return extracted;
}

} // namespace Mantid::LiveData
