#ifndef MUONALPHACALCTEST_H_
#define MUONALPHACALCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Column.h"
#include "MantidAlgorithms/CalMuonDeadTime.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class CalMuonDeadTimeTest : public CxxTest::TestSuite
{
public:

  void testName()
  {
    TS_ASSERT_EQUALS( calDeadTime.name(), "CalMuonDeadTime" )
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( calDeadTime.category(), "Muon" )
  }

  void testInit()
  {
    calDeadTime.initialize();
    TS_ASSERT( calDeadTime.isInitialized() )
  }

  void xtestCalDeadTime()
  {
    //Load the muon nexus file
    Mantid::DataHandling::LoadMuonNexus1 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "EMU6473");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT_EQUALS(loader.isExecuted(),true);


    calDeadTime.setPropertyValue("InputWorkspace", "EMU6473");
    calDeadTime.setPropertyValue("DeadTimeTable", "deadtimetable");
    calDeadTime.setPropertyValue("DataFitted", "fittedData");
    calDeadTime.setPropertyValue("FirstGoodData", "1.0");
    calDeadTime.setPropertyValue("LastGoodData", "2.0");

    try 
    {
      TS_ASSERT_EQUALS(calDeadTime.execute(),true);
    }
    catch(std::runtime_error & e)
    {
      TS_FAIL(e.what());
    }

    ITableWorkspace_sptr table = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
                  (Mantid::API::AnalysisDataService::Instance().retrieve("DeadTimeTable"));

    Column_const_sptr col =	table->getColumn(1);
    const Column* tableC = col.get();
    TS_ASSERT_DELTA(tableC->operator[](0),-0.0246,0.0001);

    Mantid::API::AnalysisDataService::Instance().remove("deadtimetable");
    Mantid::API::AnalysisDataService::Instance().remove("fittedData");
    Mantid::API::AnalysisDataService::Instance().remove("EMU6473");


  }


private:
  CalMuonDeadTime calDeadTime;

};

#endif /*MUONALPHACALCTEST_H_*/
