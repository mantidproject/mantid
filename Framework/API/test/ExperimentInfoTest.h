#ifndef MANTID_API_EXPERIMENTINFOTEST_H_
#define MANTID_API_EXPERIMENTINFOTEST_H_

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/ModeratorModel.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Matrix.h"

#include "MantidAPI/FileFinder.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "PropertyManagerHelper.h"

// clang-format off
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>
// clang-format on

#include <cxxtest/TestSuite.h>
#include <boost/regex.hpp>
#include <Poco/DirectoryIterator.h>

#include <set>
#include <unordered_map>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace NeXus;
using Mantid::Types::Core::DateAndTime;

class FakeChopper : public Mantid::API::ChopperModel {
public:
  boost::shared_ptr<ChopperModel> clone() const override {
    return boost::make_shared<FakeChopper>(*this);
  }

  double calculatePulseTimeVariance() const override { return 0.0; }
  double sampleTimeDistribution(const double) const override { return 0.0; }
  double sampleJitterDistribution(const double) const override { return 0.0; }

private:
  void setParameterValue(const std::string &, const std::string &) override{};
};

class FakeSource : public Mantid::API::ModeratorModel {
public:
  boost::shared_ptr<ModeratorModel> clone() const override {
    return boost::make_shared<FakeSource>(*this);
  }
  double emissionTimeMean() const override { return 0.0; }
  double emissionTimeVariance() const override { return 0.0; }
  double sampleTimeDistribution(const double) const override { return 0.0; }

private:
  void setParameterValue(const std::string &, const std::string &) override{};
};

class ExperimentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentInfoTest *createSuite() { return new ExperimentInfoTest(); }
  static void destroySuite(ExperimentInfoTest *suite) { delete suite; }

  ExperimentInfoTest() { ConfigService::Instance().updateFacilities(); }

  void test_GetInstrument_default() {
    ExperimentInfo ws;
    boost::shared_ptr<const Instrument> i = ws.getInstrument();
    TSM_ASSERT("ExperimentInfo gets a default, empty Instrument.", i);
    TS_ASSERT(i->isEmptyInstrument());
    TS_ASSERT_EQUALS(ws.getInstrument()->type(), "Instrument");

    // Should be set even though we have just an empty instrument.
    TS_ASSERT(i->getParameterMap()->hasDetectorInfo(i->baseInstrument().get()));
  }

  void test_GetSetInstrument_default() {
    ExperimentInfo ws;
    boost::shared_ptr<Instrument> inst1 = boost::make_shared<Instrument>();
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    // Instruments don't point to the same base place since you get back a
    // parameterized one
    boost::shared_ptr<const Instrument> inst2 = ws.getInstrument();
    TS_ASSERT_EQUALS(inst2->getName(), "MyTestInst");

    // But the base instrument does!
    boost::shared_ptr<const Instrument> inst3 = inst2->baseInstrument();
    TS_ASSERT_EQUALS(inst3.get(), inst1.get());
    TS_ASSERT_EQUALS(inst3->getName(), "MyTestInst");
  }

  void test_Setting_A_New_Source_With_NULL_Ptr_Throws() {
    ExperimentInfo ws;

    TS_ASSERT_THROWS(ws.setModeratorModel(nullptr), std::invalid_argument);
  }

  void test_Retrieving_Source_Properties_Before_Set_Throws() {
    ExperimentInfo ws;

    TS_ASSERT_THROWS(ws.moderatorModel(), std::runtime_error);
  }

  void test_Setting_New_Source_Description_With_Valid_Object_Does_Not_Throw() {
    using namespace Mantid::API;
    ExperimentInfo ws;

    ModeratorModel *source = new FakeSource;
    TS_ASSERT_THROWS_NOTHING(ws.setModeratorModel(source));
    const ModeratorModel &fetched = ws.moderatorModel();
    const ModeratorModel &constInput =
        const_cast<const Mantid::API::ModeratorModel &>(*source);
    TS_ASSERT_EQUALS(&fetched, &constInput);
  }

  void test_Setting_A_New_Chopper_With_NULL_Ptr_Throws() {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS(ws->setChopperModel(nullptr), std::invalid_argument);
  }

  void test_Setting_A_New_Chopper_To_Point_Lower_Point_Succeeds() {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS_NOTHING(ws->setChopperModel(new FakeChopper));
    TS_ASSERT_THROWS_NOTHING(ws->chopperModel(0));
  }

  void test_Setting_A_New_Chopper_To_Existing_Index_Replaces_Current() {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS_NOTHING(ws->setChopperModel(new FakeChopper));
    TS_ASSERT_THROWS(ws->chopperModel(1), std::invalid_argument);
  }

  void test_Getting_Chopper_At_Index_Greater_Than_Descriptions_Added_Throws() {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS(ws->chopperModel(2), std::invalid_argument);
  }

  void test_GetSetSample() {
    ExperimentInfo ws;
    TS_ASSERT(&ws.sample());
    ws.mutableSample().setName("test");
    TS_ASSERT_EQUALS(ws.sample().getName(), "test");
  }

  void test_GetSetRun() {
    ExperimentInfo ws;
    TS_ASSERT(&ws.run());
    ws.mutableRun().setProtonCharge(1.234);
    TS_ASSERT_DELTA(ws.run().getProtonCharge(), 1.234, 0.001);
  }

  void test_GetLog_Throws_If_No_Log_Or_Instrument_Parameter_Exists() {
    ExperimentInfo expt;

    TS_ASSERT_THROWS(expt.getLog("__NOTALOG__"), std::invalid_argument);
  }

  void
  test_GetLog_Throws_If_Instrument_Contains_LogName_Parameter_But_Log_Does_Not_Exist() {
    ExperimentInfo expt;
    const std::string instPar = "temperature_log";
    const std::string actualLogName = "SAMPLE_TEMP";
    addInstrumentWithParameter(expt, instPar, actualLogName);

    TS_ASSERT_THROWS(expt.getLog(instPar),
                     Mantid::Kernel::Exception::NotFoundError);
  }

  void
  test_GetLog_Returns_Value_Of_Log_Named_In_Instrument_Parameter_If_It_Exists_And_Actual_Log_Entry_Exists() {
    ExperimentInfo expt;
    const std::string instPar = "temperature_log";
    const std::string actualLogName = "SAMPLE_TEMP";
    const double logValue(7.4);
    addRunWithLog(expt, actualLogName, logValue);
    addInstrumentWithParameter(expt, instPar, actualLogName);

    Property *log = nullptr;
    TS_ASSERT_THROWS_NOTHING(log = expt.getLog(instPar));
    if (log)
      TS_ASSERT_EQUALS(log->name(), actualLogName);
  }

  void test_GetLog_Picks_Run_Log_Over_Instrument_Parameter_Of_Same_Name() {
    ExperimentInfo expt;
    const std::string actualLogName = "SAMPLE_TEMP";
    const double logValue(7.4);
    addRunWithLog(expt, actualLogName, logValue);
    addInstrumentWithParameter(expt, actualLogName, "some  value");

    Property *log = nullptr;
    TS_ASSERT_THROWS_NOTHING(log = expt.getLog(actualLogName));
    if (log)
      TS_ASSERT_EQUALS(log->name(), actualLogName);
  }

  void
  test_GetLogAsSingleValue_Throws_If_No_Log_Or_Instrument_Parameter_Exists() {
    ExperimentInfo expt;

    TS_ASSERT_THROWS(expt.getLogAsSingleValue("__NOTALOG__"),
                     std::invalid_argument);
  }

  void
  test_GetLogAsSingleValue_Throws_If_Instrument_Contains_LogName_Parameter_But_Log_Does_Not_Exist() {
    ExperimentInfo expt;
    const std::string instPar = "temperature_log";
    const std::string actualLogName = "SAMPLE_TEMP";
    addInstrumentWithParameter(expt, instPar, actualLogName);

    TS_ASSERT_THROWS(expt.getLogAsSingleValue(instPar),
                     Mantid::Kernel::Exception::NotFoundError);
  }

  void
  test_GetLogAsSingleValue_Returns_Value_Of_Log_Named_In_Instrument_Parameter_If_It_Exists_And_Actual_Log_Entry_Exists() {
    ExperimentInfo expt;
    const std::string instPar = "temperature_log";
    const std::string actualLogName = "SAMPLE_TEMP";
    const double logValue(9.10);
    addRunWithLog(expt, actualLogName, logValue);
    addInstrumentWithParameter(expt, instPar, actualLogName);

    double value(-1.0);
    TS_ASSERT_THROWS_NOTHING(value = expt.getLogAsSingleValue(instPar));
    TS_ASSERT_DELTA(value, logValue, 1e-12);
  }

  void
  test_GetLogAsSingleValue_Picks_Run_Log_Over_Instrument_Parameter_Of_Same_Name() {
    ExperimentInfo expt;
    const std::string actualLogName = "SAMPLE_TEMP";
    const double logValue(11.5);
    addInstrumentWithParameter(expt, actualLogName, "some  value");
    addRunWithLog(expt, actualLogName, logValue);

    double value(-1.0);
    TS_ASSERT_THROWS_NOTHING(value = expt.getLogAsSingleValue(actualLogName));
    TS_ASSERT_DELTA(value, logValue, 1e-12);
  }

  void do_compare_ExperimentInfo(ExperimentInfo &ws, ExperimentInfo &ws2) {
    TS_ASSERT_EQUALS(ws2.sample().getName(), "test");
    TS_ASSERT_DELTA(ws2.sample().getOrientedLattice().a(), 1.0, 1e-4);
    TS_ASSERT_DELTA(ws2.sample().getOrientedLattice().b(), 2.0, 1e-4);
    TS_ASSERT_DELTA(ws2.sample().getOrientedLattice().c(), 3.0, 1e-4);
    TS_ASSERT_DELTA(ws2.run().getProtonCharge(), 1.234, 0.001);
    TS_ASSERT_EQUALS(ws2.getInstrument()->getName(), "MyTestInst");
    TS_ASSERT_DIFFERS(&ws.moderatorModel(), &ws2.moderatorModel());

    // Changing stuff in the original workspace...
    ws.mutableSample().setName("test1");
    ws.mutableRun().setProtonCharge(2.345);

    // ... does not change the copied one.
    TS_ASSERT_EQUALS(ws2.sample().getName(), "test");
    TS_ASSERT_DELTA(ws2.run().getProtonCharge(), 1.234, 0.001);

    // The original oriented lattice is ok
    TS_ASSERT_DELTA(ws.sample().getOrientedLattice().a(), 1.0, 1e-4);
    TS_ASSERT_DELTA(ws.sample().getOrientedLattice().b(), 2.0, 1e-4);
    TS_ASSERT_DELTA(ws.sample().getOrientedLattice().c(), 3.0, 1e-4);
  }

  void test_copyExperimentInfoFrom() {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    OrientedLattice latt(1, 2, 3, 90, 90, 90);
    ws.mutableSample().setOrientedLattice(&latt);
    boost::shared_ptr<Instrument> inst1 = boost::make_shared<Instrument>();
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);
    ws.setModeratorModel(new FakeSource);
    ws.setChopperModel(new FakeChopper);

    ExperimentInfo ws2;
    ws2.copyExperimentInfoFrom(&ws);
    do_compare_ExperimentInfo(ws, ws2);
  }

  void test_clone() {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    OrientedLattice latt(1, 2, 3, 90, 90, 90);
    ws.mutableSample().setOrientedLattice(&latt);
    boost::shared_ptr<Instrument> inst1 = boost::make_shared<Instrument>();
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);
    ws.setModeratorModel(new FakeSource);
    ws.setChopperModel(new FakeChopper);

    ExperimentInfo *ws2 = ws.cloneExperimentInfo();
    do_compare_ExperimentInfo(ws, *ws2);
    delete ws2;
  }

  void test_clone_then_copy() {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    OrientedLattice latt(1, 2, 3, 90, 90, 90);
    ws.mutableSample().setOrientedLattice(&latt);
    boost::shared_ptr<Instrument> inst1 = boost::make_shared<Instrument>();
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);
    ws.setModeratorModel(new FakeSource);
    ws.setChopperModel(new FakeChopper);

    ExperimentInfo *ws2 = ws.cloneExperimentInfo();

    ExperimentInfo ws3;
    ws3.copyExperimentInfoFrom(ws2);

    do_compare_ExperimentInfo(ws, ws3);

    delete ws2;
  }

  void test_default_emode_is_elastic() {
    ExperimentInfo exptInfo;

    TS_ASSERT_EQUALS(exptInfo.getEMode(), Mantid::Kernel::DeltaEMode::Elastic);
  }

  void test_runlog_with_emode_returns_correct_mode() {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();

    TS_ASSERT_EQUALS(exptInfo->getEMode(), Mantid::Kernel::DeltaEMode::Direct);
  }

  void test_runlog_with_emode_overrides_instrument_emode() {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();
    addInstrumentWithIndirectEmodeParameter(exptInfo);

    TS_ASSERT_EQUALS(exptInfo->getEMode(), Mantid::Kernel::DeltaEMode::Direct);
  }

  void test_runlog_with_only_instrument_emode_uses_this() {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    addInstrumentWithIndirectEmodeParameter(exptInfo);

    TS_ASSERT_EQUALS(exptInfo->getEMode(),
                     Mantid::Kernel::DeltaEMode::Indirect);
  }

  void test_getEFixed_throws_exception_if_detID_does_not_exist() {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();

    TS_ASSERT_THROWS(exptInfo->getEFixed(1),
                     Mantid::Kernel::Exception::NotFoundError);
  }

  void test_correct_efixed_value_is_returned_for_direct_run() {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();
    const double test_ei(15.1);
    exptInfo->mutableRun().addProperty("Ei", test_ei);

    TS_ASSERT_EQUALS(exptInfo->getEFixed(), test_ei);
  }

  void test_getEfixed_throws_for_indirect_mode_and_no_detector_passed() {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    Instrument_sptr inst = addInstrumentWithIndirectEmodeParameter(exptInfo);

    TS_ASSERT_THROWS(exptInfo->getEFixed(), std::runtime_error);
  }

  void
  test_getEfixed_throws_for_indirect_mode_when_passed_a_detector_without_parameter() {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    addInstrumentWithIndirectEmodeParameter(exptInfo);
    IDetector_const_sptr det = exptInfo->getInstrument()->getDetector(3);

    TS_ASSERT_THROWS(exptInfo->getEFixed(det), std::runtime_error);
  }

  void
  test_getEfixed_in_indirect_mode_returns_detector_level_EFixed_parameter() {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    Instrument_sptr inst = addInstrumentWithIndirectEmodeParameter(exptInfo);
    const double test_ef(32.7);
    const Mantid::detid_t test_id = 3;
    IDetector_const_sptr det = exptInfo->getInstrument()->getDetector(test_id);
    ParameterMap &pmap = exptInfo->instrumentParameters();
    pmap.addDouble(det.get(), "Efixed", test_ef);

    TS_ASSERT_EQUALS(exptInfo->getEFixed(det), test_ef);
    TS_ASSERT_EQUALS(exptInfo->getEFixed(test_id), test_ef);
  }

  void
  test_getEfixed_in_indirect_mode_looks_recursively_for_Efixed_parameter() {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    Instrument_sptr inst = addInstrumentWithIndirectEmodeParameter(exptInfo);
    const double test_ef(32.7);
    const Mantid::detid_t test_id = 3;
    exptInfo->instrumentParameters().addDouble(inst.get(), "Efixed", test_ef);
    IDetector_const_sptr det = exptInfo->getInstrument()->getDetector(test_id);

    TS_ASSERT_EQUALS(exptInfo->getEFixed(det), test_ef);
    TS_ASSERT_EQUALS(exptInfo->getEFixed(test_id), test_ef);
  }

  void test_accessing_SpectrumInfo_creates_default_grouping() {
    using namespace Mantid;
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    addInstrumentWithParameter(*exptInfo, "a", "b");

    const auto &spectrumInfo = exptInfo->spectrumInfo();
    const auto *detGroup = dynamic_cast<const Geometry::DetectorGroup *>(
        &spectrumInfo.detector(0));
    TS_ASSERT(!detGroup);
    TS_ASSERT_EQUALS(spectrumInfo.detector(0).getID(), 1);
  }

  void test_cacheDetectorGroupings_creates_correct_SpectrumInfo() {
    using namespace Mantid;
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    addInstrumentWithParameter(*exptInfo, "a", "b");

    // Set a mapping
    std::set<detid_t> group{1, 2};
    Mantid::det2group_map mapping{{1, group}};
    exptInfo->cacheDetectorGroupings(mapping);

    const auto &spectrumInfo = exptInfo->spectrumInfo();
    const auto *detGroup = dynamic_cast<const Geometry::DetectorGroup *>(
        &spectrumInfo.detector(0));
    TS_ASSERT(detGroup);
    TS_ASSERT_EQUALS(detGroup->getDetectorIDs(), (std::vector<detid_t>{1, 2}));
  }

  void test_Setting_Group_Lookup_To_Empty_Map_Does_Not_Throw() {
    ExperimentInfo expt;
    Mantid::det2group_map mappings;

    TS_ASSERT_THROWS_NOTHING(expt.cacheDetectorGroupings(mappings));
  }

  void test_Getting_Group_For_Unknown_ID_Throws() {
    ExperimentInfo expt;

    TS_ASSERT_THROWS(expt.groupOfDetectorID(1), std::out_of_range);
  }

  void
  test_Setting_Group_Lookup_To_Non_Empty_Map_Allows_Retrieval_Of_Correct_Group() {
    ExperimentInfo expt;
    Mantid::det2group_map mappings;
    mappings.emplace(1, std::set<Mantid::detid_t>{2});
    expt.cacheDetectorGroupings(mappings);

    size_t index{0};
    TS_ASSERT_THROWS_NOTHING(index = expt.groupOfDetectorID(1));
    TS_ASSERT_EQUALS(index, 0);
  }

  struct fromToEntry {
    std::string path;
    DateAndTime from;
    DateAndTime to;
  };

  // Test that all the IDFs contain valid-to and valid-from dates and that
  // for a single instrument none of the valid-from dates are equal
  void testAllDatesInIDFs() {
    ExperimentInfo helper;

    // Collect all IDF filenames and put them in a multimap where the instrument
    // identifier is the key
    std::unordered_multimap<std::string, fromToEntry> idfFiles;
    std::unordered_set<std::string> idfIdentifiers;

    boost::regex regex(".*_Definition.*\\.xml", boost::regex_constants::icase);
    Poco::DirectoryIterator end_iter;
    for (Poco::DirectoryIterator dir_itr(ConfigService::Instance().getString(
             "instrumentDefinition.directory"));
         dir_itr != end_iter; ++dir_itr) {
      if (!Poco::File(dir_itr->path()).isFile())
        continue;

      std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

      if (boost::regex_match(l_filenamePart, regex)) {
        std::string validFrom, validTo;
        helper.getValidFromTo(dir_itr->path(), validFrom, validTo);

        size_t found;
        found = l_filenamePart.find("_Definition");
        fromToEntry ft;
        ft.path = dir_itr->path();
        ft.from.setFromISO8601(validFrom);
        // Valid TO is optional
        if (validTo.length() > 0)
          ft.to.setFromISO8601(validTo);
        else
          ft.to.setFromISO8601("2100-01-01T00:00:00");

        idfFiles.emplace(l_filenamePart.substr(0, found), ft);
        idfIdentifiers.insert(l_filenamePart.substr(0, found));
      }
    }

    // iterator to browse through the multimap: paramInfoFromIDF
    std::unordered_multimap<std::string, fromToEntry>::const_iterator it1, it2;
    std::pair<std::unordered_multimap<std::string, fromToEntry>::iterator,
              std::unordered_multimap<std::string, fromToEntry>::iterator> ret;

    for (const auto &idfIdentifier : idfIdentifiers) {
      ret = idfFiles.equal_range(idfIdentifier);
      for (it1 = ret.first; it1 != ret.second; ++it1) {
        for (it2 = ret.first; it2 != ret.second; ++it2) {
          if (it1 != it2) {
            // some more intelligent stuff here later
            std::stringstream messageBuffer;
            messageBuffer
                << "Two IDFs for one instrument have equal valid-from dates"
                << "IDFs are: " << it1->first << " and " << it2->first
                << " Date One: " << it1->second.from.toFormattedString()
                << " Date Two: " << it2->second.from.toFormattedString();
            TSM_ASSERT_DIFFERS(messageBuffer.str(), it2->second.from,
                               it1->second.from);
          }
        }
      }
    }
  }

  //
  void testHelperFunctions() {
    ConfigService::Instance().updateFacilities();
    ExperimentInfo helper;
    std::string boevs =
        helper.getInstrumentFilename("BIOSANS", "2100-01-31 22:59:59");
    TS_ASSERT(!boevs.empty());
  }

  //
  void testHelper_TOPAZ_No_To_Date() {
    ExperimentInfo helper;
    std::string boevs =
        helper.getInstrumentFilename("TOPAZ", "2011-01-31 22:59:59");
    TS_ASSERT(!boevs.empty());
  }

  void testHelper_ValidDateOverlap() {
    const std::string instDir =
        ConfigService::Instance().getInstrumentDirectory();
    const std::string testDir = instDir + "IDFs_for_UNIT_TESTING";
    ConfigService::Instance().setString("instrumentDefinition.directory",
                                        testDir);
    ExperimentInfo helper;
    std::string boevs =
        helper.getInstrumentFilename("ARGUS", "1909-01-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST1_ValidDateOverlap"), std::string::npos);
    boevs = helper.getInstrumentFilename("ARGUS", "1909-03-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST2_ValidDateOverlap"), std::string::npos);
    boevs = helper.getInstrumentFilename("ARGUS", "1909-05-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST1_ValidDateOverlap"), std::string::npos);
    ConfigService::Instance().setString("instrumentDefinition.directory",
                                        instDir);
  }

  void test_nexus() {
    std::string filename = "ExperimentInfoTest1.nxs";
    NexusTestHelper th(true);
    th.createFile(filename);
    ExperimentInfo ws;
    boost::shared_ptr<Instrument> inst1 = boost::make_shared<Instrument>();
    inst1->setName("GEM");
    inst1->setFilename("GEM_Definition.xml");
    inst1->setXmlText("");
    ws.setInstrument(inst1);

    TS_ASSERT_THROWS_NOTHING(ws.saveExperimentInfoNexus(th.file););

    // ------------------------ Re-load the contents ----------------------
    ExperimentInfo ws2;
    std::string parameterStr;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING(
        ws2.loadExperimentInfoNexus(filename, th.file, parameterStr));
    Instrument_const_sptr inst = ws2.getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "GEM");
    TS_ASSERT(inst->getFilename().find("GEM_Definition.xml", 0) !=
              std::string::npos);
    TS_ASSERT_EQUALS(parameterStr, "");
  }

  void test_nexus_empty_instrument() {
    std::string filename = "ExperimentInfoTest2.nxs";
    NexusTestHelper th(true);
    th.createFile(filename);
    ExperimentInfo ws;
    boost::shared_ptr<Instrument> inst1 = boost::make_shared<Instrument>();
    inst1->setName("");
    inst1->setFilename("");
    inst1->setXmlText("");
    ws.setInstrument(inst1);

    TS_ASSERT_THROWS_NOTHING(ws.saveExperimentInfoNexus(th.file););

    // ------------------------ Re-load the contents ----------------------
    ExperimentInfo ws2;
    std::string parameterStr;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING(
        ws2.loadExperimentInfoNexus(filename, th.file, parameterStr));
    Instrument_const_sptr inst = ws2.getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "");
    TS_ASSERT_EQUALS(parameterStr, "");
  }

  void testNexus_W_matrix() {
    std::string filename = "ExperimentInfoWMatrixTest.nxs";
    NexusTestHelper th(true);
    th.createFile(filename);
    ExperimentInfo ei;

    DblMatrix WTransf(3, 3, true);
    // let's add some tricky stuff to w-transf
    WTransf[0][1] = 0.5;
    WTransf[0][2] = 2.5;
    WTransf[1][0] = 10.5;
    WTransf[1][2] = 12.5;
    WTransf[2][0] = 20.5;
    WTransf[2][1] = 21.5;

    auto wTrVector = WTransf.getVector();

    // this occurs in ConvertToMD, copy methadata
    ei.mutableRun().addProperty("W_MATRIX", wTrVector, true);

    TS_ASSERT_THROWS_NOTHING(ei.saveExperimentInfoNexus(th.file));

    th.reopenFile();

    ExperimentInfo other;
    std::string InstrParameters;
    TS_ASSERT_THROWS_NOTHING(
        other.loadExperimentInfoNexus(filename, th.file, InstrParameters));

    std::vector<double> wMatrRestored =
        other.run().getPropertyValueAsType<std::vector<double>>("W_MATRIX");

    for (int i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(wTrVector[i], wMatrRestored[i], 1.e-9);
    }
  }

  void test_nexus_instrument_info() {
    ExperimentInfo ei;

    // We get an instrument group from a test file in the form that would occur
    // in an ISIS Nexus file
    // with an embedded instrument definition and parameters

    // Create the root Nexus class
    std::string testFile = "LOQinstrument.h5";
    std::string path = FileFinder::Instance().getFullPath(testFile);

    // Get nexus file for this.
    ::NeXus::File nxFile(path, NXACC_READ);

    // Load the Nexus IDF info
    std::string params;
    TS_ASSERT_THROWS_NOTHING(
        ei.loadInstrumentInfoNexus(testFile, &nxFile, params));
    Instrument_const_sptr inst = ei.getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "LOQ"); // Check instrument name
    TS_ASSERT_EQUALS(params.size(), 613);     // Check size of parameter string
  }

  void test_nexus_parameters() {
    ExperimentInfo ei;

    // We get an instrument group from a test file in the form that would occur
    // in an ISIS Nexus file
    // with an embedded instrument definition and parameters

    // Create the root Nexus class
    std::string testFile = "LOQinstrument.h5";
    std::string path = FileFinder::Instance().getFullPath(testFile);

    // Get nexus file for this.
    ::NeXus::File nxFile(path, NXACC_READ);
    // Open instrument group
    nxFile.openGroup("instrument", "NXinstrument");

    // Load the Nexus IDF info
    std::string params;
    TS_ASSERT_THROWS_NOTHING(ei.loadInstrumentParametersNexus(&nxFile, params));
    TS_ASSERT_EQUALS(params.size(), 613); // Check size of parameter string
  }

  /**
  * Test declaring an ExperimentInfo property and retrieving as const or
  * non-const
  */
  void testGetProperty_const_sptr() {
    const std::string eiName = "InputEi";
    ExperimentInfo_sptr eiInput(new ExperimentInfo());
    PropertyManagerHelper manager;
    manager.declareProperty(eiName, eiInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    ExperimentInfo_const_sptr eiConst;
    ExperimentInfo_sptr eiNonConst;
    TS_ASSERT_THROWS_NOTHING(
        eiConst = manager.getValue<ExperimentInfo_const_sptr>(eiName));
    TS_ASSERT(eiConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(eiNonConst =
                                 manager.getValue<ExperimentInfo_sptr>(eiName));
    TS_ASSERT(eiNonConst != nullptr);
    TS_ASSERT_EQUALS(eiConst, eiNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, eiName);
    ExperimentInfo_const_sptr eiCastConst;
    ExperimentInfo_sptr eiCastNonConst;
    TS_ASSERT_THROWS_NOTHING(eiCastConst = (ExperimentInfo_const_sptr)val);
    TS_ASSERT(eiCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(eiCastNonConst = (ExperimentInfo_sptr)val);
    TS_ASSERT(eiCastNonConst != nullptr);
    TS_ASSERT_EQUALS(eiCastConst, eiCastNonConst);
  }

  void test_getInstrument_setInstrument_copies_masking() {
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    ExperimentInfo source;
    ExperimentInfo target;
    source.setInstrument(inst);
    target.setInstrument(inst);

    source.mutableDetectorInfo().setMasked(0, true);
    TS_ASSERT(!target.detectorInfo().isMasked(0));
    target.setInstrument(source.getInstrument());
    TS_ASSERT(target.detectorInfo().isMasked(0));
  }

  void test_getInstrument_setInstrument_copy_on_write_not_broken() {
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    ExperimentInfo source;
    ExperimentInfo target;
    source.setInstrument(inst);
    target.setInstrument(inst);

    TS_ASSERT(!target.detectorInfo().isMasked(0));
    target.setInstrument(source.getInstrument());
    source.mutableDetectorInfo().setMasked(0, true);
    TS_ASSERT(!target.detectorInfo().isMasked(0));
  }

  void test_create_componentInfo() {

    const int nPixels = 10;
    auto inst = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixels /*10 by 10 dets in bank*/,
        1 /*sample-bank distance*/);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const Mantid::Geometry::ComponentInfo &compInfo = expInfo.componentInfo();

    size_t nComponents = nPixels * nPixels;
    nComponents += nPixels; // One additional CompAssembly per row.
    nComponents += 1;       // Rectangular Detector (bank)
    nComponents += 1;       // source
    nComponents += 1;       // sample
    nComponents += 1;       // Instrument itself
    TS_ASSERT_EQUALS(compInfo.size(), nComponents);
  }

  void test_component_info_detector_indices_for_assembly_component_types() {

    const int nPixels = 10;
    auto inst = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixels /*10 by 10 dets in bank*/,
        1 /*sample-bank distance*/);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const auto &compInfo = expInfo.componentInfo();
    const auto &detInfo = expInfo.detectorInfo();
    // Test the single bank
    auto bank = inst->getComponentByName("bank1");
    auto bankID = bank->getComponentID();
    auto allBankDetectorIndexes =
        compInfo.detectorsInSubtree(compInfo.indexOf(bankID));

    TSM_ASSERT_EQUALS("Should have all detectors under this bank",
                      allBankDetectorIndexes.size(),
                      detInfo.size()); //

    // Test one of the bank rows
    auto bankRowID =
        boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(bank)
            ->getChild(0)
            ->getComponentID();
    auto allRowDetectorIndexes =
        compInfo.detectorsInSubtree(compInfo.indexOf(bankRowID));

    TSM_ASSERT_EQUALS("Should have all detectors under this row",
                      allRowDetectorIndexes.size(),
                      10); //
  }
  void test_component_info_detector_indices_for_detector_component_types() {

    const int nPixels = 10;
    auto inst = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixels /*10 by 10 dets in bank*/,
        1 /*sample-bank distance*/);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const auto &compInfo = expInfo.componentInfo();
    const auto &detInfo = expInfo.detectorInfo();
    // Test one of the detectors
    const auto targetDetectorIndex = 0;
    const auto detCompId =
        detInfo.detector(targetDetectorIndex).getComponentID();
    TSM_ASSERT_EQUALS(
        "Detector should report the detector index of itself",
        compInfo.detectorsInSubtree(compInfo.indexOf(detCompId)).size(), 1);
    TS_ASSERT_EQUALS(
        compInfo.detectorsInSubtree(compInfo.indexOf(detCompId))[0],
        targetDetectorIndex);

    size_t detectorIndex =
        0; // interchangeable as either component or detector index
    TSM_ASSERT_EQUALS("Gurantee violated of detectorindex == componentIndex",
                      compInfo.detectorsInSubtree(detectorIndex),
                      std::vector<size_t>{detectorIndex});

    detectorIndex = 99; // interchangeable as either component or detector index
    TSM_ASSERT_EQUALS("Gurantee violated of detectorindex == componentIndex",
                      compInfo.detectorsInSubtree(detectorIndex),
                      std::vector<size_t>{detectorIndex});
  }

  void test_component_info_detector_indices_for_generic_component_types() {
    const int nPixels = 10;
    auto inst = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixels /*10 by 10 dets in bank*/,
        1 /*sample-bank distance*/);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const Mantid::Geometry::ComponentInfo &compInfo = expInfo.componentInfo();

    // Test non-detector, non-assembly components
    auto sampleId = inst->getComponentByName("sample")->getComponentID();
    TSM_ASSERT_EQUALS(
        "Sample should not report any nested detector indexes",
        compInfo.detectorsInSubtree(compInfo.indexOf(sampleId)).size(), 0);

    auto sourceId = inst->getComponentByName("source")->getComponentID();
    TSM_ASSERT_EQUALS(
        "Source should not report any nested detector indexes",
        compInfo.detectorsInSubtree(compInfo.indexOf(sourceId)).size(), 0);
  }

  void test_component_info_source_sample_l1() {

    auto inst = ComponentCreationHelper::createMinimalInstrument(
        V3D{-2, 0, 0} /*source*/, V3D{10, 0, 0} /*sample*/,
        V3D{12, 0, 0} /*detector*/);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const Mantid::Geometry::ComponentInfo &compInfo = expInfo.componentInfo();

    TS_ASSERT_EQUALS((V3D{-2, 0, 0}), compInfo.sourcePosition());

    TS_ASSERT_EQUALS((V3D{10, 0, 0}), compInfo.samplePosition());

    TS_ASSERT_DELTA(12, compInfo.l1(), 1e-12);
  }

  void test_component_info_component_index_tree() {

    const int nPixels = 10;
    auto inst = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixels /*10 by 10 dets in bank*/,
        1 /*sample-bank distance*/);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const Mantid::Geometry::ComponentInfo &compInfo = expInfo.componentInfo();

    // Test non-detector, non-assembly components
    auto sampleId = inst->getComponentByName("sample")->getComponentID();
    TS_ASSERT_EQUALS(
        compInfo.componentsInSubtree(compInfo.indexOf(sampleId)).size(), 1);

    auto sourceId = inst->getComponentByName("source")->getComponentID();
    TS_ASSERT_EQUALS(
        compInfo.componentsInSubtree(compInfo.indexOf(sourceId)).size(), 1);

    auto bankId = inst->getComponentByName("bank1")->getComponentID();
    TSM_ASSERT_EQUALS(
        "Bank should yield entire sub-tree of component indices",
        compInfo.componentsInSubtree(compInfo.indexOf(bankId)).size(),
        (nPixels * nPixels) + nPixels + 1);

    auto instrumentId = inst->getComponentID();
    size_t nComponents = nPixels * nPixels;
    nComponents += nPixels; // One additional CompAssembly per row.
    nComponents += 1;       // Rectangular Detector (bank)
    nComponents += 1;       // source
    nComponents += 1;       // sample
    nComponents += 1;       // self
    TSM_ASSERT_EQUALS(
        "Instrument should yield entire tree of component indices",
        compInfo.componentsInSubtree(compInfo.indexOf(instrumentId)).size(),
        nComponents);

    TS_ASSERT_EQUALS(compInfo.indexOf(inst->getComponentID()),
                     compInfo.parent(compInfo.indexOf(bankId)));
  }

  void test_readParameterMap_semicolons_in_value() {
    auto inst = ComponentCreationHelper::createMinimalInstrument(
        V3D{-2, 0, 0} /*source*/, V3D{10, 0, 0} /*sample*/,
        V3D{12, 0, 0} /*detector*/);
    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    expInfo.readParameterMap("detID:1;string;par;11;22;33;44");
    auto &pmap = expInfo.instrumentParameters();
    auto det = expInfo.getInstrument()->getDetector(1);
    auto value = pmap.getString(det.get(), "par");
    TS_ASSERT_EQUALS(value, "11;22;33;44");
  }

private:
  void addInstrumentWithParameter(ExperimentInfo &expt, const std::string &name,
                                  const std::string &value) {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);
    expt.setInstrument(inst);
    expt.instrumentParameters().addString(inst.get(), name, value);
  }

  void addRunWithLog(ExperimentInfo &expt, const std::string &name,
                     const double value) {
    expt.mutableRun().addProperty(name, value);
  }

  ExperimentInfo_sptr createTestInfoWithDirectEModeLog() {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    exptInfo->mutableRun().addProperty("deltaE-mode", std::string("direct"));
    return exptInfo;
  }

  Instrument_sptr
  addInstrumentWithIndirectEmodeParameter(ExperimentInfo_sptr exptInfo) {
    Instrument_sptr inst = addInstrument(exptInfo);
    exptInfo->instrumentParameters().addString(inst.get(), "deltaE-mode",
                                               "indirect");
    return inst;
  }

  Instrument_sptr addInstrument(ExperimentInfo_sptr exptInfo) {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);
    exptInfo->setInstrument(inst);
    return inst;
  }

  ExperimentInfo_sptr createTestInfoWithChopperPoints(const size_t npoints) {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    boost::shared_ptr<Instrument> inst1 = boost::make_shared<Instrument>();
    inst1->setName("MyTestInst");
    auto source = new ObjComponent("source");
    inst1->add(source);
    inst1->markAsSource(source);

    for (size_t i = 0; i < npoints; ++i) {
      auto chopperPoint = new ObjComponent("ChopperPoint");
      inst1->add(chopperPoint);
      inst1->markAsChopperPoint(chopperPoint);
    }
    exptInfo->setInstrument(inst1);
    return exptInfo;
  }
};

class ExperimentInfoTestPerformance : public CxxTest::TestSuite {
private:
  boost::shared_ptr<Mantid::Geometry::Instrument> m_bareInstrument;
  boost::shared_ptr<const Mantid::Geometry::Instrument> m_provisionedInstrument;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentInfoTestPerformance *createSuite() {
    return new ExperimentInfoTestPerformance();
  }
  static void destroySuite(ExperimentInfoTestPerformance *suite) {
    delete suite;
  }

  ExperimentInfoTestPerformance() {

    const int nPixels = 1000;
    m_bareInstrument = ComponentCreationHelper::createTestInstrumentRectangular(
        1 /*n banks*/, nPixels, 1 /*sample-bank distance*/);

    ExperimentInfo tmp;
    tmp.setInstrument(m_bareInstrument);
    m_provisionedInstrument = tmp.getInstrument();
  }

  void
  test_setInstrument_when_instrument_lacks_detectorInfo_and_componentInfo() {
    /*
     * This is similar to what will happen during LoadEmptyInstrument
     */
    ExperimentInfo expInfo;
    expInfo.setInstrument(m_bareInstrument);
  }
  void test_setInstrument_when_new_instrument_is_fully_provisioned() {
    /*
     * This should be the case for any workspaces after they have initially had
     * an instrument
     * set upon them via setInstrument.
     */
    ExperimentInfo expInfo;
    expInfo.setInstrument(m_provisionedInstrument);
  }

  void test_getBoundingBox_once() {
    BoundingBox box;
    m_provisionedInstrument->getBoundingBox(box);
  }

  void test_getBoundingBox_twice() {
    BoundingBox box;
    m_provisionedInstrument->getBoundingBox(box);
  }
};
#endif /* MANTID_API_EXPERIMENTINFOTEST_H_ */
