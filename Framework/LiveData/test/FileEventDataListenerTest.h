// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_LIVEDATA_FILEEVENTDATALISTENERTEST_H_
#define MANTID_LIVEDATA_FILEEVENTDATALISTENERTEST_H_

#include "MantidAPI/LiveListenerFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FileEventDataListenerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FileEventDataListenerTest *createSuite() {
    return new FileEventDataListenerTest();
  }
  static void destroySuite(FileEventDataListenerTest *suite) { delete suite; }

  // This is just a test class to help with development, so let's keep the test
  // simple and all in one method
  void testTheListener() {
    // Set the properties that are required by this listener
    ConfigService::Instance().setString("fileeventdatalistener.filename",
                                        "REF_L_32035_neutron_event.dat");
    ConfigService::Instance().setString("fileeventdatalistener.chunks", "2");

    // Create the listener. Remember: this will call connect()
    ILiveListener_sptr listener =
        LiveListenerFactory::Instance().create("FileEventDataListener", true);

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
    TS_ASSERT_THROWS_NOTHING(
        buffer = boost::dynamic_pointer_cast<const MatrixWorkspace>(
            listener->extractData()))
    TS_ASSERT(buffer)
    // Check this is the only surviving reference to it
    TS_ASSERT_EQUALS(buffer.use_count(), 1)
    TS_ASSERT_EQUALS(buffer->getNumberHistograms(), 77824)

    TS_ASSERT_EQUALS(listener->runStatus(), ILiveListener::Running)

    MatrixWorkspace_const_sptr buffer2;
    // Call extractData again
    TS_ASSERT_THROWS_NOTHING(
        buffer2 = boost::dynamic_pointer_cast<const MatrixWorkspace>(
            listener->extractData()))
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
};

#endif /* MANTID_LIVEDATA_FILEEVENTDATALISTENERTEST_H_ */
