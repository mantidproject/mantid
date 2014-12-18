#ifndef MANTID_API_EXPERIMENTINFOTEST_H_
#define MANTID_API_EXPERIMENTINFOTEST_H_

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/ModeratorModel.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Matrix.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/NexusTestHelper.h"

#include <cxxtest/TestSuite.h>
#include <boost/regex.hpp>
#include <Poco/DirectoryIterator.h>

#include <iomanip>
#include <iostream>
#include <set>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class FakeChopper : public Mantid::API::ChopperModel
{
public:
  boost::shared_ptr<ChopperModel> clone() const { return boost::make_shared<FakeChopper>(*this); }

  double calculatePulseTimeVariance() const
  {
    return 0.0;
  }
  double sampleTimeDistribution(const double) const
  {
    return 0.0;
  }
  double sampleJitterDistribution(const double) const
  {
    return 0.0;
  }

private:
  void setParameterValue(const std::string &,const std::string&) {};
};

class FakeSource : public Mantid::API::ModeratorModel
{
public:
  boost::shared_ptr<ModeratorModel> clone() const { return boost::make_shared<FakeSource>(*this); }
  double emissionTimeMean() const { return 0.0;}
  double emissionTimeVariance() const { return 0.0; }
  double sampleTimeDistribution(const double) const { return 0.0; }
private:
  void setParameterValue(const std::string &,const std::string&) {};
};

class ExperimentInfoTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentInfoTest *createSuite() { return new ExperimentInfoTest(); }
  static void destroySuite( ExperimentInfoTest *suite ) { delete suite; }

  ExperimentInfoTest()
  {
    ConfigService::Instance().updateFacilities();
  }

  void test_GetInstrument_default()
  {
    ExperimentInfo ws;
    boost::shared_ptr<const Instrument> i = ws.getInstrument();
    TSM_ASSERT( "ExperimentInfo gets a default, empty Instrument.", i);
    TS_ASSERT_EQUALS( ws.getInstrument()->type(), "Instrument" );
  }

  void test_GetSetInstrument_default()
  {
    ExperimentInfo ws;
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    // Instruments don't point to the same base place since you get back a parameterized one
    boost::shared_ptr<const Instrument> inst2 = ws.getInstrument();
    TS_ASSERT_EQUALS( inst2->getName(), "MyTestInst");

    // But the base instrument does!
    boost::shared_ptr<const Instrument> inst3 = inst2->baseInstrument();
    TS_ASSERT_EQUALS( inst3.get(), inst1.get());
    TS_ASSERT_EQUALS( inst3->getName(), "MyTestInst");
  }

  void test_Setting_A_New_Source_With_NULL_Ptr_Throws()
  {
    ExperimentInfo ws;

    TS_ASSERT_THROWS(ws.setModeratorModel(NULL), std::invalid_argument);
  }

  void test_Retrieving_Source_Properties_Before_Set_Throws()
  {
    ExperimentInfo ws;

    TS_ASSERT_THROWS(ws.moderatorModel(), std::runtime_error);
  }


  void test_Setting_New_Source_Description_With_Valid_Object_Does_Not_Throw()
  {
    using namespace Mantid::API;
    ExperimentInfo ws;

    ModeratorModel * source = new FakeSource;
    TS_ASSERT_THROWS_NOTHING(ws.setModeratorModel(source));
    const ModeratorModel & fetched = ws.moderatorModel();
    const ModeratorModel & constInput = const_cast<const Mantid::API::ModeratorModel&>(*source);
    TS_ASSERT_EQUALS(&fetched, &constInput);
  }

  void test_Setting_A_New_Chopper_With_NULL_Ptr_Throws()
  {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS(ws->setChopperModel(NULL), std::invalid_argument);
  }

  void test_Setting_A_New_Chopper_To_Point_Lower_Point_Succeeds()
  {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS_NOTHING(ws->setChopperModel(new FakeChopper));
    TS_ASSERT_THROWS_NOTHING(ws->chopperModel(0));
  }

  void test_Setting_A_New_Chopper_To_Existing_Index_Replaces_Current()
  {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS_NOTHING(ws->setChopperModel(new FakeChopper));
    TS_ASSERT_THROWS(ws->chopperModel(1), std::invalid_argument);
  }

  void test_Getting_Chopper_At_Index_Greater_Than_Descriptions_Added_Throws()
  {
    ExperimentInfo_sptr ws = createTestInfoWithChopperPoints(1);

    TS_ASSERT_THROWS(ws->chopperModel(2), std::invalid_argument);
  }

  void test_GetSetSample()
  {
    ExperimentInfo ws;
    TS_ASSERT( &ws.sample() );
    ws.mutableSample().setName("test");
    TS_ASSERT_EQUALS( ws.sample().getName(), "test" );
  }

  void test_GetSetRun()
  {
    ExperimentInfo ws;
    TS_ASSERT( &ws.run() );
    ws.mutableRun().setProtonCharge(1.234);
    TS_ASSERT_DELTA( ws.run().getProtonCharge(), 1.234, 0.001);
  }

  void test_GetLog_Throws_If_No_Log_Or_Instrument_Parameter_Exists()
  {
    ExperimentInfo expt;

    TS_ASSERT_THROWS(expt.getLog("__NOTALOG__"), std::invalid_argument);
  }

  void test_GetLog_Throws_If_Instrument_Contains_LogName_Parameter_But_Log_Does_Not_Exist()
  {
    ExperimentInfo expt;
    const std::string instPar = "temperature_log";
    const std::string actualLogName = "SAMPLE_TEMP";
    addInstrumentWithParameter(expt, instPar, actualLogName);

    TS_ASSERT_THROWS(expt.getLog(instPar), Exception::NotFoundError);
  }

  void test_GetLog_Returns_Value_Of_Log_Named_In_Instrument_Parameter_If_It_Exists_And_Actual_Log_Entry_Exists()
  {
    ExperimentInfo expt;
    const std::string instPar = "temperature_log";
    const std::string actualLogName = "SAMPLE_TEMP";
    const double logValue(7.4);
    addRunWithLog(expt, actualLogName, logValue);
    addInstrumentWithParameter(expt, instPar, actualLogName);

    Property *log(NULL);
    TS_ASSERT_THROWS_NOTHING(log = expt.getLog(instPar));
    TS_ASSERT_EQUALS(log->name(), actualLogName);
  }

  void test_GetLog_Picks_Run_Log_Over_Instrument_Parameter_Of_Same_Name()
  {
    ExperimentInfo expt;
    const std::string actualLogName = "SAMPLE_TEMP";
    const double logValue(7.4);
    addRunWithLog(expt, actualLogName, logValue);
    addInstrumentWithParameter(expt, actualLogName, "some  value");

    Property *log(NULL);
    TS_ASSERT_THROWS_NOTHING(log = expt.getLog(actualLogName));
    TS_ASSERT_EQUALS(log->name(), actualLogName);
  }

  void test_GetLogAsSingleValue_Throws_If_No_Log_Or_Instrument_Parameter_Exists()
  {
    ExperimentInfo expt;

    TS_ASSERT_THROWS(expt.getLogAsSingleValue("__NOTALOG__"), std::invalid_argument);
  }

  void test_GetLogAsSingleValue_Throws_If_Instrument_Contains_LogName_Parameter_But_Log_Does_Not_Exist()
  {
    ExperimentInfo expt;
    const std::string instPar = "temperature_log";
    const std::string actualLogName = "SAMPLE_TEMP";
    addInstrumentWithParameter(expt, instPar, actualLogName);

    TS_ASSERT_THROWS(expt.getLogAsSingleValue(instPar), Exception::NotFoundError);
  }

  void test_GetLogAsSingleValue_Returns_Value_Of_Log_Named_In_Instrument_Parameter_If_It_Exists_And_Actual_Log_Entry_Exists()
  {
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

  void test_GetLogAsSingleValue_Picks_Run_Log_Over_Instrument_Parameter_Of_Same_Name()
  {
    ExperimentInfo expt;
    const std::string actualLogName = "SAMPLE_TEMP";
    const double logValue(11.5);
    addInstrumentWithParameter(expt, actualLogName, "some  value");
    addRunWithLog(expt, actualLogName, logValue);

    double value(-1.0);
    TS_ASSERT_THROWS_NOTHING(value = expt.getLogAsSingleValue(actualLogName));
    TS_ASSERT_DELTA(value, logValue, 1e-12);
  }


  void do_compare_ExperimentInfo(ExperimentInfo & ws, ExperimentInfo & ws2)
  {
    TS_ASSERT_EQUALS( ws2.sample().getName(), "test" );
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().a(), 1.0, 1e-4);
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().b(), 2.0, 1e-4);
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().c(), 3.0, 1e-4);
    TS_ASSERT_DELTA( ws2.run().getProtonCharge(), 1.234, 0.001);
    TS_ASSERT_EQUALS( ws2.getInstrument()->getName(), "MyTestInst");
    TS_ASSERT_DIFFERS(&ws.moderatorModel(), &ws2.moderatorModel());

    // Changing stuff in the original workspace...
    ws.mutableSample().setName("test1");
    ws.mutableRun().setProtonCharge(2.345);

    // ... does not change the copied one.
    TS_ASSERT_EQUALS( ws2.sample().getName(), "test" );
    TS_ASSERT_DELTA( ws2.run().getProtonCharge(), 1.234, 0.001);

    // The original oriented lattice is ok
    TS_ASSERT_DELTA( ws.sample().getOrientedLattice().a(), 1.0, 1e-4);
    TS_ASSERT_DELTA( ws.sample().getOrientedLattice().b(), 2.0, 1e-4);
    TS_ASSERT_DELTA( ws.sample().getOrientedLattice().c(), 3.0, 1e-4);
  }


  void test_copyExperimentInfoFrom()
  {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    OrientedLattice latt(1,2,3,90,90,90);
    ws.mutableSample().setOrientedLattice( &latt  );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);
    ws.setModeratorModel(new FakeSource);
    ws.setChopperModel(new FakeChopper);

    ExperimentInfo ws2;
    ws2.copyExperimentInfoFrom( &ws );
    do_compare_ExperimentInfo(ws,ws2);
  }

  void test_clone()
  {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    OrientedLattice latt(1,2,3,90,90,90);
    ws.mutableSample().setOrientedLattice( &latt  );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);
    ws.setModeratorModel(new FakeSource);
    ws.setChopperModel(new FakeChopper);
    
    ExperimentInfo * ws2 = ws.cloneExperimentInfo();
    do_compare_ExperimentInfo(ws,*ws2);
    delete ws2;
  }

  void test_clone_then_copy()
  {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    OrientedLattice latt(1,2,3,90,90,90);
    ws.mutableSample().setOrientedLattice( &latt  );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);
    ws.setModeratorModel(new FakeSource);
    ws.setChopperModel(new FakeChopper);

    ExperimentInfo * ws2 = ws.cloneExperimentInfo();

    ExperimentInfo ws3;
    ws3.copyExperimentInfoFrom(ws2);

    do_compare_ExperimentInfo(ws,ws3);

    delete ws2;
  }

  void test_default_emode_is_elastic()
  {
    ExperimentInfo exptInfo;

    TS_ASSERT_EQUALS(exptInfo.getEMode(), Mantid::Kernel::DeltaEMode::Elastic);
  }

  void test_runlog_with_emode_returns_correct_mode()
  {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();

    TS_ASSERT_EQUALS(exptInfo->getEMode(), Mantid::Kernel::DeltaEMode::Direct);
  }

  void test_runlog_with_emode_overrides_instrument_emode()
  {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();
    addInstrumentWithIndirectEmodeParameter(exptInfo);

    TS_ASSERT_EQUALS(exptInfo->getEMode(), Mantid::Kernel::DeltaEMode::Direct);
  }

  void test_runlog_with_only_instrument_emode_uses_this()
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    addInstrumentWithIndirectEmodeParameter(exptInfo);

    TS_ASSERT_EQUALS(exptInfo->getEMode(), Mantid::Kernel::DeltaEMode::Indirect);
  }

  void test_getEFixed_throws_exception_if_detID_does_not_exist()
  {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();

    TS_ASSERT_THROWS(exptInfo->getEFixed(1), Mantid::Kernel::Exception::NotFoundError);
  }

  void test_correct_efixed_value_is_returned_for_direct_run()
  {
    ExperimentInfo_sptr exptInfo = createTestInfoWithDirectEModeLog();
    const double test_ei(15.1);
    exptInfo->mutableRun().addProperty("Ei", test_ei);

    TS_ASSERT_EQUALS(exptInfo->getEFixed(), test_ei);
  }

  void test_getEfixed_throws_for_indirect_mode_and_no_detector_passed()
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    Instrument_sptr inst = addInstrumentWithIndirectEmodeParameter(exptInfo);

    TS_ASSERT_THROWS(exptInfo->getEFixed(), std::runtime_error);
  }

  void test_getEfixed_throws_for_indirect_mode_when_passed_a_detector_without_parameter()
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    addInstrumentWithIndirectEmodeParameter(exptInfo);
    IDetector_const_sptr det = exptInfo->getInstrument()->getDetector(3);

    TS_ASSERT_THROWS(exptInfo->getEFixed(det), std::runtime_error);
  }

  void test_getEfixed_in_indirect_mode_returns_detector_level_EFixed_parameter()
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    Instrument_sptr inst = addInstrumentWithIndirectEmodeParameter(exptInfo);
    const double test_ef(32.7);
    const Mantid::detid_t test_id = 3;
    IDetector_const_sptr det = exptInfo->getInstrument()->getDetector(test_id);
    ParameterMap & pmap = exptInfo->instrumentParameters();
    pmap.addDouble(det.get(), "Efixed", test_ef);

    TS_ASSERT_EQUALS(exptInfo->getEFixed(det), test_ef);
    TS_ASSERT_EQUALS(exptInfo->getEFixed(test_id), test_ef);

  }

  void test_getEfixed_in_indirect_mode_looks_recursively_for_Efixed_parameter()
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    Instrument_sptr inst = addInstrumentWithIndirectEmodeParameter(exptInfo);
    const double test_ef(32.7);
    const Mantid::detid_t test_id = 3;
    exptInfo->instrumentParameters().addDouble(inst.get(), "Efixed", test_ef);
    IDetector_const_sptr det = exptInfo->getInstrument()->getDetector(test_id);

    TS_ASSERT_EQUALS(exptInfo->getEFixed(det), test_ef);
    TS_ASSERT_EQUALS(exptInfo->getEFixed(test_id), test_ef);
  }

  void test_getDetectorByID()
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    addInstrumentWithParameter(*exptInfo, "a", "b");

    IDetector_const_sptr det;
    TS_ASSERT_THROWS_NOTHING(det = exptInfo->getDetectorByID(1));
    TS_ASSERT(det);
    
    // Set a mapping
    std::vector<Mantid::detid_t> group(2, 1);
    group[1] = 2;
    Mantid::det2group_map mapping;
    mapping.insert(std::make_pair(1, group));
    exptInfo->cacheDetectorGroupings(mapping);
    
    TS_ASSERT_THROWS_NOTHING(det = exptInfo->getDetectorByID(1));
    TS_ASSERT(det);
    TS_ASSERT(boost::dynamic_pointer_cast<const DetectorGroup>(det));
  }

  void test_Setting_Group_Lookup_To_Empty_Map_Does_Not_Throw()
  {
    ExperimentInfo expt;
    std::map<Mantid::detid_t, std::vector<Mantid::detid_t>> mappings;

    TS_ASSERT_THROWS_NOTHING(expt.cacheDetectorGroupings(mappings));
  }

  void test_Getting_Group_Members_For_Unknown_ID_Throws()
  {
    ExperimentInfo expt;

    TS_ASSERT_THROWS(expt.getGroupMembers(1), std::runtime_error);
  }

  void test_Setting_Group_Lookup_To_Non_Empty_Map_Allows_Retrieval_Of_Correct_IDs()
  {
    ExperimentInfo expt;
    std::map<Mantid::detid_t, std::vector<Mantid::detid_t>> mappings;
    mappings.insert(std::make_pair(1, std::vector<Mantid::detid_t>(1,2)));
    expt.cacheDetectorGroupings(mappings);

    std::vector<Mantid::detid_t> ids;
    TS_ASSERT_THROWS_NOTHING(ids = expt.getGroupMembers(1));
  }

  struct fromToEntry
  {
    std::string path;
    DateAndTime from;
    DateAndTime to;
  };

  // Test that all the IDFs contain valid-to and valid-from dates and that
  // for a single instrument none of the valid-from dates are equal
  void testAllDatesInIDFs()
  {
    ExperimentInfo helper;

    // Collect all IDF filenames and put them in a multimap where the instrument
    // identifier is the key
    std::multimap<std::string, fromToEntry> idfFiles;
    std::set<std::string> idfIdentifiers;

    boost::regex regex(".*_Definition.*\\.xml", boost::regex_constants::icase );
    Poco::DirectoryIterator end_iter;
    for ( Poco::DirectoryIterator dir_itr(ConfigService::Instance().getString("instrumentDefinition.directory")); dir_itr != end_iter; ++dir_itr )
    {
          if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

          std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

          if ( boost::regex_match(l_filenamePart, regex) )
          {
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

            idfFiles.insert( std::pair<std::string,fromToEntry>(l_filenamePart.substr(0,found),
              ft) );
            idfIdentifiers.insert(l_filenamePart.substr(0,found));
          }
    }

    // iterator to browse through the multimap: paramInfoFromIDF
    std::multimap<std::string,fromToEntry> :: const_iterator it1, it2;
    std::pair<std::multimap<std::string,fromToEntry>::iterator,
    std::multimap<std::string,fromToEntry>::iterator> ret;

    std::set<std::string>::iterator setIt;
    for (setIt=idfIdentifiers.begin(); setIt != idfIdentifiers.end(); setIt++)
    {
      ret = idfFiles.equal_range(*setIt);
      for (it1 = ret.first; it1 != ret.second; ++it1)
      {
        for (it2 = ret.first; it2 != ret.second; ++it2)
        {
          if (it1 != it2)
          {
            // some more intelligent stuff here later
            std::stringstream messageBuffer;
            messageBuffer << "Two IDFs for one instrument have equal valid-from dates" << 
              "IDFs are: " << it1->first << " and " << it2->first <<
              " Date One: "<< it1->second.from.toFormattedString() <<
              " Date Two: "<< it2->second.from.toFormattedString();
            TSM_ASSERT_DIFFERS(messageBuffer.str(), it2->second.from, it1->second.from);
          }
        }

      }
    }
  }

  //
  void testHelperFunctions()
  {
    ConfigService::Instance().updateFacilities();
    ExperimentInfo helper;
    std::string boevs = helper.getInstrumentFilename("BIOSANS", "2100-01-31 22:59:59");
    TS_ASSERT(!boevs.empty());
  }

  //
  void testHelper_TOPAZ_No_To_Date()
  {
    ExperimentInfo helper;
    std::string boevs = helper.getInstrumentFilename("TOPAZ", "2011-01-31 22:59:59");
    TS_ASSERT(!boevs.empty());
  }

  void testHelper_ValidDateOverlap()
  {
    const std::string instDir = ConfigService::Instance().getInstrumentDirectory();
    const std::string testDir = instDir + "IDFs_for_UNIT_TESTING";
    ConfigService::Instance().setString("instrumentDefinition.directory", testDir);
    ExperimentInfo helper;
    std::string boevs = helper.getInstrumentFilename("ARGUS", "1909-01-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST1_ValidDateOverlap"),std::string::npos);
    boevs = helper.getInstrumentFilename("ARGUS", "1909-03-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST2_ValidDateOverlap"),std::string::npos);
    boevs = helper.getInstrumentFilename("ARGUS", "1909-05-31 22:59:59");
    TS_ASSERT_DIFFERS(boevs.find("TEST1_ValidDateOverlap"),std::string::npos);
    ConfigService::Instance().setString("instrumentDefinition.directory", instDir);
  }


  void test_nexus()
  {
    NexusTestHelper th(true);
    th.createFile("ExperimentInfoTest1.nxs");
    ExperimentInfo ws;
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("GEM");
    inst1->setFilename("GEM_Definition.xml");
    inst1->setXmlText("");
    ws.setInstrument(inst1);

    TS_ASSERT_THROWS_NOTHING( ws.saveExperimentInfoNexus(th.file); );

    // ------------------------ Re-load the contents ----------------------
    ExperimentInfo ws2;
    std::string parameterStr;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING( ws2.loadExperimentInfoNexus(th.file, parameterStr) );
    Instrument_const_sptr inst = ws2.getInstrument();
    TS_ASSERT_EQUALS( inst->getName(), "GEM" );
    TS_ASSERT( inst->getFilename().find("GEM_Definition.xml",0) != std::string::npos );
    TS_ASSERT_EQUALS( parameterStr, "" );
  }


  void test_nexus_empty_instrument()
  {
    NexusTestHelper th(true);
    th.createFile("ExperimentInfoTest2.nxs");
    ExperimentInfo ws;
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("");
    inst1->setFilename("");
    inst1->setXmlText("");
    ws.setInstrument(inst1);

    TS_ASSERT_THROWS_NOTHING( ws.saveExperimentInfoNexus(th.file); );

    // ------------------------ Re-load the contents ----------------------
    ExperimentInfo ws2;
    std::string parameterStr;
    th.reopenFile();
    TS_ASSERT_THROWS_NOTHING( ws2.loadExperimentInfoNexus(th.file, parameterStr) );
    Instrument_const_sptr inst = ws2.getInstrument();
    TS_ASSERT_EQUALS( inst->getName(), "" );
    TS_ASSERT_EQUALS( parameterStr, "" );
  }

  void testNexus_W_matrix()
  {
      NexusTestHelper th(true);
      th.createFile("ExperimentInfoWMatrixTest.nxs");
      ExperimentInfo ei;

      DblMatrix WTransf(3,3,true);
      // let's add some tricky stuff to w-transf
      WTransf[0][1]=0.5;
      WTransf[0][2]=2.5;
      WTransf[1][0]=10.5;
      WTransf[1][2]=12.5;
      WTransf[2][0]=20.5;
      WTransf[2][1]=21.5;

      auto wTrVector = WTransf.getVector();

      // this occurs in ConvertToMD, copy methadata
      ei.mutableRun().addProperty("W_MATRIX",wTrVector,true);

      TS_ASSERT_THROWS_NOTHING(ei.saveExperimentInfoNexus(th.file));

      th.reopenFile();

      ExperimentInfo other;
      std::string InstrParameters;
      TS_ASSERT_THROWS_NOTHING(other.loadExperimentInfoNexus(th.file,InstrParameters));

      std::vector<double> wMatrRestored=other.run().getPropertyValueAsType<std::vector<double> >("W_MATRIX");

      for(int i=0;i<9;i++)
      {
          TS_ASSERT_DELTA(wTrVector[i],wMatrRestored[i],1.e-9);
      }

  }

private:

  void addInstrumentWithParameter(ExperimentInfo & expt, const std::string & name, const std::string & value)
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    expt.setInstrument(inst);
    expt.instrumentParameters().addString(inst.get(), name, value);
  }

  void addRunWithLog(ExperimentInfo & expt, const std::string & name, const double value)
  {
    expt.mutableRun().addProperty(name, value);
  }

  ExperimentInfo_sptr createTestInfoWithDirectEModeLog()
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    exptInfo->mutableRun().addProperty("deltaE-mode", std::string("direct"));
    return exptInfo;
  }

  Instrument_sptr addInstrumentWithIndirectEmodeParameter(ExperimentInfo_sptr exptInfo)
  {
    Instrument_sptr inst = addInstrument(exptInfo);
    exptInfo->instrumentParameters().addString(inst.get(), "deltaE-mode", "indirect");
    return inst;
  }

  Instrument_sptr addInstrument(ExperimentInfo_sptr exptInfo)
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    exptInfo->setInstrument(inst);
    return inst;
  }

  ExperimentInfo_sptr createTestInfoWithChopperPoints(const size_t npoints)
  {
    ExperimentInfo_sptr exptInfo(new ExperimentInfo);
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    auto source = new ObjComponent("source");
    inst1->add(source);
    inst1->markAsSource(source);

    for(size_t i = 0; i < npoints; ++i)
    {
      auto chopperPoint = new ObjComponent("ChopperPoint");
      inst1->add(chopperPoint);
      inst1->markAsChopperPoint(chopperPoint);
    }
    exptInfo->setInstrument(inst1);
    return exptInfo;
  }

};


#endif /* MANTID_API_EXPERIMENTINFOTEST_H_ */

