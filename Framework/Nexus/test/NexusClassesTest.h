// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidNexus/NexusClasses.h"
#include "MantidNexus/NexusException.h"
#include "test_helper.h"
#include <cxxtest/TestSuite.h>
#include <filesystem>

class NexusClassesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NexusClassesTest *createSuite() { return new NexusClassesTest(); }
  static void destroySuite(NexusClassesTest *suite) { delete suite; }

  void test_EQSANS_89157() {
    const std::string filename = NexusTest::getFullPath("EQSANS_89157.nxs.h5");
    Mantid::Nexus::NXRoot root(filename);

    // groups don't load their attributes

    auto entry = root.openFirstEntry();
    TS_ASSERT_EQUALS(entry.name(), "entry");
    // entry.NX_class() returns the type in "NexusClasses" (i.e. NXentry) rather than what is in the file

    // check NXChar
    auto definition = root.openNXChar("entry/definition"); // relative address
    definition.load();
    TS_ASSERT_EQUALS(std::string(definition()), "NXsnsevent");
    // and from getString
    TS_ASSERT_EQUALS(root.getString("entry/definition"), "NXsnsevent");

    TS_ASSERT(!entry.containsGroup("bank91_events")); // there aren't that many groups
    TS_ASSERT(entry.containsGroup("bank19_events"));

    TS_ASSERT_THROWS(entry.openNXGroup("bank91_events"), const Mantid::Nexus::Exception &); // next call should be fine

    auto bank19 = entry.openNXGroup("bank19_events");
    TS_ASSERT_EQUALS(bank19.name(), "bank19_events");
    // bank19.NX_class() returns the type in "NexusClasses" (i.e. NXClass) rather than what is in the file

    // load time-of-flight as a float because NexusClasses doesn't autoconvert types
    auto time_of_flight = bank19.openNXFloat("event_time_offset");
    TS_ASSERT_EQUALS(time_of_flight.dim0(), 256); // from looking in the file
    TS_ASSERT_EQUALS(time_of_flight.attributes.n(), 2);
    TS_ASSERT_EQUALS(time_of_flight.attributes("units"), "microsecond");
    TS_ASSERT_EQUALS(time_of_flight.attributes("target"), "/entry/instrument/bank19/event_time_offset");
    time_of_flight.load();
    TS_ASSERT_DELTA(time_of_flight[0], 16681.5, .01);
    TS_ASSERT_DELTA(time_of_flight[255], 958.1, .01);
    TS_ASSERT_THROWS_ANYTHING(time_of_flight[256]); // out of bounds

    TS_ASSERT_THROWS(bank19.openNXFloat("timeofflight"), const Mantid::Nexus::Exception &); // next call should be fine

    // load detector ids without letting previous data go out of scope
    auto detid = bank19.openNXDataSet<uint32_t>("event_id"); // type does not have a convenience function
    TS_ASSERT_EQUALS(detid.dim0(), 256);                     // same as number of time-of-flight
    TS_ASSERT_EQUALS(detid.attributes.n(), 1);
    TS_ASSERT_EQUALS(detid.attributes("target"), "/entry/instrument/bank19/event_id");
    detid.load();
    TS_ASSERT_EQUALS(detid[0], 37252);
    TS_ASSERT_EQUALS(detid[255], 37272);
    TS_ASSERT_THROWS_ANYTHING(detid[256]); // out of bounds

    auto duration = root.openNXFloat("/entry/duration"); // absolute address
    TS_ASSERT_EQUALS(duration.attributes.n(), 1);
    TS_ASSERT_EQUALS(duration.attributes("units"), "second");
    duration.load();
    TS_ASSERT_DELTA(duration[0], 7200., .1);
  }

  void test_concurrent_read_address_and_id() {
    std::cout << "test file and nexus classes\n" << std::flush;
    // this test mimics behavior found inside LoadNexusProcessed::loadLeanelasticPeaksEntry
    // and protects against a regression that can occur in tests of LoadNexusProcessed
    // This error occurs when multiple places are trying to access the same file resource,
    // and put the stack of HDF IDs in an inconsistent state.
    std::string filename = NexusTest::getFullPath("SingleCrystalLeanElasticPeakTableNew.nxs");

    // open an NXRoot and a NexusFile
    Mantid::Nexus::NXRoot root(filename);
    Mantid::Nexus::File file(root.m_fileID);

    // in the file, go to one place, in the NXRoot another
    TS_ASSERT_THROWS_NOTHING(file.openGroupAddress("/mantid_workspace_1"));
    TS_ASSERT_THROWS_NOTHING(root.openEntry("mantid_workspace_1"));
    TS_ASSERT_THROWS_NOTHING(file.openGroupAddress("/mantid_workspace_1/peaks_workspace"));
    // If the error is not fixed, the result of the above COULD mean that the file is in an
    // inconsistent situation.  It will then try to open a dataset "definition", which is
    // inside "/mantid_workspace_1", even though the currently opened group is "peaks_workspace"
    // This can cause an error in `File::getEntries()` which can be very hard to pin down
    TS_ASSERT_THROWS_NOTHING(root.openEntry("mantid_workspace_1"));
  }
};
