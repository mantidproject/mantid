#ifndef MANTID_API_EXPERIMENTINFOTEST_H_
#define MANTID_API_EXPERIMENTINFOTEST_H_

#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/NexusTestHelper.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/DirectoryIterator.h>
#include <Poco/RegularExpression.h>
#include <set>
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::Kernel::NexusTestHelper;

class ExperimentInfoTest : public CxxTest::TestSuite
{
public:


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



  void do_compare_ExperimentInfo(ExperimentInfo & ws, ExperimentInfo & ws2)
  {
    TS_ASSERT_EQUALS( ws2.sample().getName(), "test" );
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().a(), 1.0, 1e-4);
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().b(), 2.0, 1e-4);
    TS_ASSERT_DELTA( ws2.sample().getOrientedLattice().c(), 3.0, 1e-4);
    TS_ASSERT_DELTA( ws2.run().getProtonCharge(), 1.234, 0.001);
    TS_ASSERT_EQUALS( ws2.getInstrument()->getName(), "MyTestInst");

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
    ws.mutableSample().setOrientedLattice( new OrientedLattice(1,2,3,90,90,90) );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    ExperimentInfo ws2;
    ws2.copyExperimentInfoFrom( &ws );
    do_compare_ExperimentInfo(ws,ws2);
  }

  void test_clone()
  {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    ws.mutableSample().setOrientedLattice( new OrientedLattice(1,2,3,90,90,90) );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    ExperimentInfo * ws2 = ws.cloneExperimentInfo();
    do_compare_ExperimentInfo(ws,*ws2);
  }

  void test_clone_then_copy()
  {
    ExperimentInfo ws;
    ws.mutableRun().setProtonCharge(1.234);
    ws.mutableSample().setName("test");
    ws.mutableSample().setOrientedLattice( new OrientedLattice(1,2,3,90,90,90) );
    boost::shared_ptr<Instrument> inst1(new Instrument());
    inst1->setName("MyTestInst");
    ws.setInstrument(inst1);

    ExperimentInfo * ws2 = ws.cloneExperimentInfo();

    ExperimentInfo ws3;
    ws3.copyExperimentInfoFrom(ws2);

    do_compare_ExperimentInfo(ws,ws3);
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

    Poco::RegularExpression regex(".*_Definition.*\\.xml", Poco::RegularExpression::RE_CASELESS );
    Poco::DirectoryIterator end_iter;
    for ( Poco::DirectoryIterator dir_itr(ConfigService::Instance().getString("instrumentDefinition.directory")); dir_itr != end_iter; ++dir_itr )
    {
          if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

          std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

          if ( regex.match(l_filenamePart) )
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
              ft.to.setFromISO8601("2100-01-01");

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
            if ( it2->second.from == it1->second.from )
            {
              // some more intelligent stuff here later
              TS_ASSERT_EQUALS("Two IDFs for one instrument have equal valid-from dates", "0");
            }
          }
        }

      }
    }
  }

  //
  void testHelperFunctions()
  {
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


};


#endif /* MANTID_API_EXPERIMENTINFOTEST_H_ */

