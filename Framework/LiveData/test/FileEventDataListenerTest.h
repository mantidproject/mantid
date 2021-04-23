// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class FileEventDataListenerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileEventDataListenerTest *createSuite() { return new FileEventDataListenerTest(); }
  static void destroySuite(FileEventDataListenerTest *suite) { delete suite; }

  // This is just a test class to help with development, so let's keep the test
  // simple and all in one method
  void testTheListener() {
    // Set the properties that are required by this listener
    ConfigService::Instance().setString("fileeventdatalistener.filename", "REF_L_32035_neutron_event.dat");
    ConfigService::Instance().setString("fileeventdatalistener.chunks", "2");

    // Create the listener. Remember: this will call connect()
    ILiveListener_sptr listener = LiveListenerFactory::Instance().create("FileEventDataListener", true);

    // Test the 'property' methods
    TS_ASSERT(listener)
    TS_ASSERT_EQUALS(listener->name(), "FileEventDataListener")
    TS_ASSERT(!listener->supportsHistory())
    TS_ASSERT(listener->buffersEvents())
    TS_ASSERT(listener->isConnected())

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::NoRun)

    TS_ASSERT_THROWS_NOTHING(listener->start(0));

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::BeginRun)

    MatrixWorkspace_const_sptr buffer;
    TS_ASSERT_THROWS_NOTHING(buffer = std::dynamic_pointer_cast<const MatrixWorkspace>(listener->extractData()))
    TS_ASSERT(buffer)
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS(buffer.use_count(), 1)
    TS_ASSERT_EQUALS(buffer->getNumberHistograms(), 77824)

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::Running)

    MatrixWorkspace_const_sptr buffer2;
    // Call extractData again
    TS_ASSERT_THROWS_NOTHING(buffer2 = std::dynamic_pointer_cast<const MatrixWorkspace>(listener->extractData()))
    TS_ASSERT(buffer2)
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS(buffer2.use_count(), 1)
    // Check it's a different workspace to last time
    TS_ASSERT_DIFFERS(buffer.get(), buffer2.get())
    TS_ASSERT_EQUALS(buffer2->getNumberHistograms(), 77824)

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::EndRun)

    // Calling it again will throw as it's the end of the file
    TS_ASSERT_THROWS(listener->extractData(), const std::runtime_error &)
  }

  void testFileListener() {
    // Test that the event listener works for an event nexus file
    // Set the properties that are required by this listener
    const int nchunks = 4;
    ConfigService::Instance().setString("fileeventdatalistener.filename", "EQSANS_89157.nxs.h5");
    ConfigService::Instance().setString("fileeventdatalistener.chunks", std::to_string(nchunks));

    // Create the listener. Remember: this will call connect()
    ILiveListener_sptr listener = LiveListenerFactory::Instance().create("FileEventDataListener", true);

    // Test the 'property' methods
    TS_ASSERT(listener)
    TS_ASSERT_EQUALS(listener->name(), "FileEventDataListener")
    TS_ASSERT(!listener->supportsHistory())
    TS_ASSERT(listener->buffersEvents())
    TS_ASSERT(listener->isConnected())

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::NoRun)

    TS_ASSERT_THROWS_NOTHING(listener->start());

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::BeginRun)

    EventWorkspace_const_sptr buffer;
    TS_ASSERT_THROWS_NOTHING(buffer = std::dynamic_pointer_cast<const EventWorkspace>(listener->extractData()))
    TS_ASSERT(buffer)
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS(buffer.use_count(), 1)
    TS_ASSERT_EQUALS(buffer->getNumberHistograms(), 49152)

    size_t events = buffer->getNumberEvents();

    for (int i = 0; i < nchunks - 1; i++) {
      TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::Running)
      EventWorkspace_const_sptr buffer2;
      // Call extractData again
      TS_ASSERT_THROWS_NOTHING(buffer2 = std::dynamic_pointer_cast<const EventWorkspace>(listener->extractData()))
      TS_ASSERT(buffer2)
      // Check this is the only surviving reference to it
      TS_ASSERT_EQUALS(buffer2.use_count(), 1)
      // Check it's a different workspace to last time
      TS_ASSERT_DIFFERS(buffer.get(), buffer2.get())
      TS_ASSERT_EQUALS(buffer2->getNumberHistograms(), 49152)
      events += buffer2->getNumberEvents();
    }

    TS_ASSERT_EQUALS(events, 14553);

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::EndRun)

    // Calling it again will throw as it's the end of the file
    TS_ASSERT_THROWS(listener->extractData(), const std::runtime_error &)
  }

  void testChunkingProtonCharge() {
    ConfigService::Instance().setString("fileeventdatalistener.filename", "EQSANS_89157.nxs.h5");
    ConfigService::Instance().setString("fileeventdatalistener.chunks", "1");

    ILiveListener_sptr listener = LiveListenerFactory::Instance().create("FileEventDataListener", true);
    TS_ASSERT(listener)
    TS_ASSERT_EQUALS(listener->name(), "FileEventDataListener")
    TS_ASSERT(!listener->supportsHistory())
    TS_ASSERT(listener->buffersEvents())
    TS_ASSERT(listener->isConnected())
    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::NoRun)

    TS_ASSERT_THROWS_NOTHING(listener->start());

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::BeginRun)

    EventWorkspace_const_sptr buffer;
    TS_ASSERT_THROWS_NOTHING(buffer = std::dynamic_pointer_cast<const EventWorkspace>(listener->extractData()))
    TS_ASSERT(buffer)
    TS_ASSERT_EQUALS(buffer.use_count(), 1)
    TS_ASSERT_EQUALS(buffer->getNumberHistograms(), 49152)
    TS_ASSERT_EQUALS(buffer->getNumberEvents(), 14553);
    TS_ASSERT_THROWS(listener->extractData(), const std::runtime_error &)

    // Get the proton charge from the single chunk
    const double pcharge_onechunk = buffer->run().getProtonCharge();

    // Start the data listener again but load two chunks this time
    ConfigService::Instance().setString("fileeventdatalistener.chunks", "2");

    listener = LiveListenerFactory::Instance().create("FileEventDataListener", true);
    TS_ASSERT(listener)
    TS_ASSERT_EQUALS(listener->name(), "FileEventDataListener")
    TS_ASSERT(!listener->supportsHistory())
    TS_ASSERT(listener->buffersEvents())
    TS_ASSERT(listener->isConnected())
    TS_ASSERT_THROWS_NOTHING(listener->start(0));
    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::BeginRun)

    EventWorkspace_const_sptr buffer2;
    TS_ASSERT_THROWS_NOTHING(buffer2 = std::dynamic_pointer_cast<const EventWorkspace>(listener->extractData()))
    TS_ASSERT(buffer2)
    TS_ASSERT_EQUALS(buffer2.use_count(), 1)
    // Load the second chunk from the file
    TS_ASSERT_THROWS_NOTHING(buffer2 = std::dynamic_pointer_cast<const EventWorkspace>(listener->extractData()))
    TS_ASSERT(buffer2)
    TS_ASSERT_EQUALS(buffer2.use_count(), 1)
    TS_ASSERT_EQUALS(buffer2->getNumberHistograms(), 49152)

    // Get the proton charge from this chunk - it should be half of the charge from one chunk
    const double pcharge_twochunk = buffer2->run().getProtonCharge();
    TS_ASSERT_EQUALS(pcharge_onechunk * 0.5, pcharge_twochunk);
  }
};
