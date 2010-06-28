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
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include <sys/stat.h>

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


  void xtest_LoadPreNeXus_TOPAZ()
  {
    std::string eventfile( "../../../../Test/Data/sns_event_prenexus/TOPAZ_1249_neutron_event.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setPropertyValue("MappingFilename",
          "../../../../Test/Data/sns_event_prenexus/TOPAZ_TS_2010_04_16.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "topaz1249");

    //Get the event file size
    struct stat filestatus;
    stat(eventfile.c_str(), &filestatus);

    //std::cout << "***** executing *****" << std::endl;
    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("topaz1249"));

    //The # of events = size of the file / 8 bytes (per event)
    TS_ASSERT_EQUALS( ew->getNumberEvents(), filestatus.st_size / 8);

    //Only some of the pixels were loaded, because of lot of them are empty
    int numpixels_with_events = 199824;
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), numpixels_with_events);

    //Mapping between workspace index and spectrum number
    //Is the length good?
    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), numpixels_with_events);
    //Depends on which was the first pixel with events. BUT it has to be
    // more than 65536, because the 0th detector has no events (does not exist).
    TS_ASSERT( ew->getAxis(1)->spectraNo(0) >= 65536);
    //And the spectra # grow monotonically
    TS_ASSERT( ew->getAxis(1)->spectraNo(1) > ew->getAxis(1)->spectraNo(0));
    TS_ASSERT( ew->getAxis(1)->spectraNo(numpixels_with_events-1) < 15*256*256);
  }


  void xtest_LoadPreNeXus_REFL()
  {
    std::string eventfile( "../../../../Test/Data/sns_event_prenexus/REF_L_32035_neutron_event.dat" );
    std::string pulsefile( "../../../../Test/Data/sns_event_prenexus/REF_L_32035_pulseid.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setProperty("PulseidFilename", pulsefile);
    eventLoader->setPropertyValue("MappingFilename",
          "../../../../Test/Data/sns_event_prenexus/REF_L_TS_2010_02_19.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "refl");

    //Get the event file size
    struct stat filestatus;
    stat(eventfile.c_str(), &filestatus);

    //std::cout << "***** executing *****" << std::endl;
    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("refl"));

    //The # of events = size of the file / 8 bytes (per event)
    TS_ASSERT_EQUALS( ew->getNumberEvents(), filestatus.st_size / 8);

    //Only some of the pixels were loaded, because of lot of them are empty
    int numpixels_with_events = 4753;
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), numpixels_with_events);

    //--- DAS Pixel ID to our PixelID mapping ---
    //Directly look at the DAS pixel map to check THAT (values taken from the read-out).
    TS_ASSERT_EQUALS( eventLoader->pixelmap[0], 77568);
    TS_ASSERT_EQUALS( eventLoader->pixelmap[1], 77569);
    TS_ASSERT_EQUALS( eventLoader->pixelmap[255], 77823);
    TS_ASSERT_EQUALS( eventLoader->pixelmap[256], 77312);
    TS_ASSERT_EQUALS( eventLoader->pixelmap[304*256-1], 255);
    TS_ASSERT_EQUALS( eventLoader->pixelmap[304*255], 464);

    //Mapping between workspace index and spectrum number
    //Is the length good?
    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), numpixels_with_events);

    //First pixel with events.
    TS_ASSERT_EQUALS( ew->getAxis(1)->spectraNo(1), 12085);
    //And the spectra # grow monotonically
    TS_ASSERT( ew->getAxis(1)->spectraNo(1) > ew->getAxis(1)->spectraNo(0));
    //The detector has 304x256 pixels
    TS_ASSERT_LESS_THAN( ew->getAxis(1)->spectraNo(numpixels_with_events-1), 304*256);
    int max_pixel_id = ew->getAxis(1)->spectraNo(numpixels_with_events-1);

    //Now the mapping of spectrum # to detector #; is a simple 1-1 map.
    TS_ASSERT_EQUALS( ew->spectraMap().nElements(), max_pixel_id);
    TS_ASSERT_EQUALS( ew->spectraMap().getDetectors(0)[0], 0);
    TS_ASSERT_EQUALS( ew->spectraMap().getDetectors(2)[0], 2);
    TS_ASSERT_EQUALS( ew->spectraMap().getDetectors(256)[0], 256);
  }



};

#endif /* LOADEVENTPRENEXUSTEST_H_ */

