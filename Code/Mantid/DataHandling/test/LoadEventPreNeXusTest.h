/*
 * LoadEventPreNeXusTest.h
 *
 *  Created on: Jun 23, 2010
 *      Author: janik zikovsky
 */

#ifndef LOADEVENTPRENEXUSTEST_H_
#define LOADEVENTPRENEXUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include <boost/timer.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Exception;
using namespace Mantid::API;
using namespace Mantid::Geometry;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;
using std::endl;
using namespace boost::posix_time;



//==========================================================================================
class LoadEventPreNeXusTest : public CxxTest::TestSuite
{
public:
  LoadEventPreNeXus * eventLoader;

  LoadEventPreNeXusTest()
  {

  }

  void setUp()
  {
    eventLoader = new LoadEventPreNeXus();
    eventLoader->initialize();
  }

  void xtest_file_not_found()
  {
    TS_ASSERT_THROWS(
        eventLoader->setPropertyValue("EventFilename", "this_file_doesnt_exist.blabla.data") ,
        std::invalid_argument );
    //Execut fails since the properties aren't set correctly.
    TS_ASSERT_THROWS( eventLoader->execute() , std::runtime_error);

  }

  void xtest_LoadPreNeXus()
  {
    std::string eventfile( "../../../../Test/Data/event_data/TOPAZ_1249_neutron_event.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setPropertyValue("MappingFilename",
          "../../../../Test/Data/event_data/TOPAZ_TS_2010_04_16.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "topaz1249");

    //Get the event file size
    struct stat filestatus;
    stat(eventfile.c_str(), &filestatus);

    //std::cout << "***** executing *****" << std::endl;
    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("topaz1249"));

    //Matching class name
    TS_ASSERT_EQUALS(ew->id(), "EventWorkspace");
    //Can't call getEventList cause you finished loading data
    TS_ASSERT_THROWS( ew->getEventList(12), std::runtime_error );

    //The # of events = size of the file / 8 bytes (per event)
    TS_ASSERT_EQUALS( ew->getNumberEvents(), filestatus.st_size / 8);

    //TOPAZ has 14*256*256 pixels; but the mapping file goes up to
    //  15**256*256 because there is no detector 0.
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), 15*256*256);

    //TS_ASSERT_EQUALS( ew->getEventListAtWorkspaceIndex(111).getNumberEvents(), 1)

//    std::cout << "name:" << ew->id() << std::endl;
//    std::cout << "num histo:" << ew->getNumberHistograms() << std::endl;
//    std::cout << "num events:" << ew->getNumberEvents() << std::endl;
//    std::cout << "**********" << std::endl;

    // end of LoadEventPreNeXus test
  }

};

#endif /* LOADEVENTPRENEXUSTEST_H_ */

