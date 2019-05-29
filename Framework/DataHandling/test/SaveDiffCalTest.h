// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEDIFFCALTEST_H_
#define MANTID_DATAHANDLING_SAVEDIFFCALTEST_H_

#include <Poco/File.h>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/SaveDiffCal.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::DataHandling::SaveDiffCal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;

namespace {
const size_t NUM_BANK = 5;
const std::string FILENAME = "SaveDiffCalTest.h5";
} // namespace

class SaveDiffCalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveDiffCalTest *createSuite() { return new SaveDiffCalTest(); }
  static void destroySuite(SaveDiffCalTest *suite) { delete suite; }

  void test_Init() {
    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  Instrument_sptr createInstrument() {
    return ComponentCreationHelper::createTestInstrumentCylindrical(NUM_BANK);
  }

  GroupingWorkspace_sptr createGrouping(Instrument_sptr instr,
                                        bool single = true) {
    GroupingWorkspace_sptr groupWS =
        boost::make_shared<GroupingWorkspace>(instr);
    if (single) {
      // set all of the groups to one
      size_t numHist = groupWS->getNumberHistograms();
      for (size_t i = 0; i < numHist; ++i) {
        const auto &detIds = groupWS->getDetectorIDs(i);
        groupWS->setValue(*(detIds.begin()), 1);
      }
    } else {
      groupWS->setValue(1, 12);
      groupWS->setValue(2, 23);
      groupWS->setValue(3, 45);
    }
    return groupWS;
  }

  MaskWorkspace_sptr createMasking(Instrument_sptr instr) {
    MaskWorkspace_sptr maskWS = boost::make_shared<MaskWorkspace>(instr);
    maskWS->getSpectrum(0).clearData();
    maskWS->mutableSpectrumInfo().setMasked(0, true);
    return maskWS;
  }

  TableWorkspace_sptr createCalibration(const size_t numRows) {
    TableWorkspace_sptr wksp = boost::make_shared<TableWorkspace>();
    wksp->addColumn("int", "detid");
    wksp->addColumn("double", "difc");
    wksp->addColumn("double", "difa");
    wksp->addColumn("double", "tzero");
    wksp->addColumn("double", "tofmin");

    for (size_t i = 0; i < numRows; ++i) {
      TableRow row = wksp->appendRow();

      row << static_cast<int>(i) // detid
          << 0.                  // difc
          << 0.                  // difa
          << 0.                  // tzero
          << 0.;                 // tofmin
    }
    return wksp;
  }

  void test_no_cal_wksp() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);

    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_empty_cal_wksp() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);
    TableWorkspace_sptr calWS = createCalibration(0);

    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalibrationWorkspace", calWS));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_no_mask() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);
    TableWorkspace_sptr calWS =
        createCalibration(NUM_BANK * 9); // nine components per bank

    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalibrationWorkspace", calWS));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // confirm that it exists
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());

    // cleanup
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_no_grouping() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);
    TableWorkspace_sptr calWS =
        createCalibration(NUM_BANK * 9); // nine components per bank

    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalibrationWorkspace", calWS));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // confirm that it exists
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());

    // cleanup
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void test_exec() {
    Instrument_sptr inst = createInstrument();
    GroupingWorkspace_sptr groupWS = createGrouping(inst);
    MaskWorkspace_sptr maskWS = createMasking(inst);
    TableWorkspace_sptr calWS =
        createCalibration(NUM_BANK * 9); // nine components per bank

    SaveDiffCal alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupingWorkspace", groupWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", FILENAME));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CalibrationWorkspace", calWS));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // confirm that it exists
    std::string filename = alg.getPropertyValue("Filename");
    TS_ASSERT(Poco::File(filename).exists());

    // cleanup
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }
};

#endif /* MANTID_DATAHANDLING_SAVEDIFFCALTEST_H_ */
