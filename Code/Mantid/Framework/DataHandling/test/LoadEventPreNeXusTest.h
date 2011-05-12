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
//#include <boost/date_time/gregorian/gregorian.hpp>
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
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
//using namespace boost::posix_time;



//==========================================================================================
class LoadEventPreNeXusTest : public CxxTest::TestSuite
{
public:
  LoadEventPreNeXus * eventLoader;

  static LoadEventPreNeXusTest *createSuite() { return new LoadEventPreNeXusTest(); }
  static void destroySuite(LoadEventPreNeXusTest *suite) { delete suite; }

  LoadEventPreNeXusTest()
  {

  }



  void setUp()
  {
    eventLoader = new LoadEventPreNeXus();
    eventLoader->initialize();
  }

  void test_file_not_found()
  {
    TS_ASSERT_THROWS(
        eventLoader->setPropertyValue("EventFilename", "this_file_doesnt_exist.blabla.data")
       , std::invalid_argument);
    //Execut fails since the properties aren't set correctly.
    TS_ASSERT_THROWS( eventLoader->execute() , std::runtime_error);

  }

  void test_data_sizes()
  {
    //Make sure the structs are the right size
    TS_ASSERT_EQUALS( sizeof(Pulse), 24);
    TS_ASSERT_EQUALS( sizeof(DasEvent), 8);
  }

  void checkWorkspace(std::string eventfile, std::string WSName, int numpixels_with_events)
  {
    //Get the event file size
    struct stat filestatus;
    stat(eventfile.c_str(), &filestatus);

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(WSName));

    //The # of events = size of the file / 8 bytes (per event)
    TS_ASSERT_EQUALS( ew->getNumberEvents(), filestatus.st_size / 8);

    //Only some of the pixels were loaded, because of lot of them are empty
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), numpixels_with_events);

    //Mapping between workspace index and spectrum number
    //Is the length good?
    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), numpixels_with_events);

  }


  void test_LoadPreNeXus_REFL()
  {
    std::string eventfile( "REF_L_32035_neutron_event.dat" );
    std::string pulsefile( "REF_L_32035_pulseid.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setPropertyValue("PulseidFilename", pulsefile);
    eventLoader->setPropertyValue("MappingFilename", "REF_L_TS_2010_02_19.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "refl");

    //Get the event file size
    struct stat filestatus;
    eventfile = eventLoader->getPropertyValue("EventFilename");
    stat(eventfile.c_str(), &filestatus);

    //std::cout << "***** executing *****" << std::endl;
    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("refl"));

    //The # of events = size of the file / 8 bytes (per event)
    TS_ASSERT_EQUALS( ew->getNumberEvents(), filestatus.st_size / 8);

    //Only some of the pixels weretof loaded, because of lot of them are empty
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

    //WorkspaceIndex to spectrum number is simple 1:1
    TS_ASSERT_EQUALS( ew->getAxis(1)->spectraNo(1), 1);
    TS_ASSERT_EQUALS( ew->getAxis(1)->spectraNo(122), 122);

    //First pixel with events.
    std::vector<int64_t> dets = ew->spectraMap().getDetectors(1);
    TS_ASSERT_EQUALS( dets.size(), 1);
    TS_ASSERT_EQUALS( dets[0], 12085); //This is the pixel ID of the first spectrum

    //And the pixel IDs grow monotonically
    dets = ew->spectraMap().getDetectors(2);
    TS_ASSERT_EQUALS( dets.size(), 1);
    TS_ASSERT( dets[0] > 12085);

    //The detector has 304x256 pixels
    TS_ASSERT_LESS_THAN( ew->getAxis(1)->spectraNo(numpixels_with_events-1), 304*256);
   
    //And the spectramap length matches the # of pixels with events
    TS_ASSERT_EQUALS( ew->spectraMap().nElements(), numpixels_with_events);

  }


  void xtest_LoadPreNeXus_CNCS_7860()
  {
    std::string eventfile( "CNCS_7860_neutron_event.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "cncs");
    eventLoader->setProperty<bool>("PadEmptyPixels", true);

    //Get the event file size
    struct stat filestatus;
    stat(eventfile.c_str(), &filestatus);

    //std::cout << "***** executing *****" << std::endl;
    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("cncs"));


    //Get the start time of all pulses
    Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( ew->mutableRun().getProperty("proton_charge") );
    std::map<DateAndTime, double> logMap = log->valueAsMap();
    std::map<DateAndTime, double>::iterator it, it2;
    it = logMap.begin();
    Kernel::DateAndTime start = it->first;

    std::vector<TofEvent> events1 = ew->getEventListPtr(1000)->getEvents();
    for (size_t i=0; i < events1.size(); i++)
    {
      std::cout << (events1[i].pulseTime()-start) << " sec \n";
    }
  }


  void test_LoadPreNeXus_CNCS()
  {
    std::string eventfile( "CNCS_7860_neutron_event.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "cncs");
    eventLoader->setProperty<bool>("PadEmptyPixels", false);

    //Get the event file size
    struct stat filestatus;
    stat(eventfile.c_str(), &filestatus);

    //std::cout << "***** executing *****" << std::endl;
    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("cncs"));

    //The # of events = size of the file / 8 bytes (per event)
    //This fails cause of errors in events
    //TS_ASSERT_EQUALS( ew->getNumberEvents(), filestatus.st_size / 8);

    //Only some of the pixels weretof loaded, because of lot of them are empty
    int numpixels_with_events = 39733;
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), numpixels_with_events);

    //This seems to be the size of the spectra map.
    //TS_ASSERT_EQUALS( ew->mutableSpectraMap().nElements(), 50172); //or is it 50173
    //TODO: Figure out why that fails there above...

    //Check if the instrument was loaded correctly
    boost::shared_ptr<Instrument> inst = ew->getBaseInstrument();
    TS_ASSERT_EQUALS (  inst->getName(), "CNCS" );

    //Mapping between workspace index and spectrum number
    //Is the length good?
    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), numpixels_with_events);

    //--------------------------------------------------------
    //Now let's test if a copy works too
    EventWorkspace_sptr inputWS = ew;
    TS_ASSERT_EQUALS( inputWS->getInstrument()->getName(), "CNCS");

    //Create a new one
    EventWorkspace_sptr outputWS =
        boost::dynamic_pointer_cast<EventWorkspace>(API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
    TS_ASSERT (outputWS) ; //non-null pointer

    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
    outputWS->mutableSpectraMap().clear();
    //You need to copy over the data as well.
    outputWS->copyDataFrom(*inputWS);

    //Bunch of checks
    TS_ASSERT_EQUALS( outputWS->getNumberEvents(), inputWS->getNumberEvents() );
    TS_ASSERT_EQUALS( outputWS->getNumberHistograms(), inputWS->getNumberHistograms() );
    TS_ASSERT_EQUALS( outputWS->getInstrument()->getName(), "CNCS");

    TS_ASSERT_EQUALS( outputWS->getEventList(0).getEvents()[0].tof(), inputWS->getEventList(0).getEvents()[0].tof() );
    //It should be possible to change an event list and not affect the other one
    outputWS->getEventList(0).convertTof(1.5, 0.2);
    TS_ASSERT_DIFFERS( outputWS->getEventList(0).getEvents()[0].tof(), inputWS->getEventList(0).getEvents()[0].tof() );

    //Setting X should still be possible
    Kernel::cow_ptr<MantidVec> x;
    TS_ASSERT_THROWS_NOTHING( outputWS->setX(0, x) );
    //Accessing Y is still possible
    const MantidVec Y = boost::dynamic_pointer_cast<const EventWorkspace>(outputWS)->dataY(0);


    // Check the run_start property exists and is right.
    Property * p = NULL;
    TS_ASSERT( outputWS->mutableRun().hasProperty("run_start") );
    TS_ASSERT_THROWS_NOTHING( p = outputWS->mutableRun().getProperty("run_start"); )
    if (p)
    {
      TS_ASSERT_EQUALS( p->value(), "2010-03-25T16:08:37") ;
    }
  }


  void test_LoadPreNeXus_CNCS_PadPixels()
  {
    std::string eventfile( "CNCS_7860_neutron_event.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "cncs");
    eventLoader->setProperty("PadEmptyPixels", true);

    //Get the event file size
    struct stat filestatus;
    stat(eventfile.c_str(), &filestatus);

    //std::cout << "***** executing *****" << std::endl;
    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("cncs"));

    //The # of events = size of the file / 8 bytes (per event)
    //This fails cause of errors in events
    //TS_ASSERT_EQUALS( ew->getNumberEvents(), filestatus.st_size / 8);

    //Only some of the pixels weretof loaded, because of lot of them are empty
    int numpixels = 50*8*128; //50 8-packs; monitors are ignored
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), numpixels);

    //This time, the max pixel ID is higher since we padded them.
    TS_ASSERT_EQUALS( ew->mutableSpectraMap().nElements(), numpixels);


    //Check if the instrument was loaded correctly
    boost::shared_ptr<Instrument> inst = ew->getBaseInstrument();
    TS_ASSERT_EQUALS (  inst->getName(), "CNCS" );

    //Mapping between workspace index and spectrum number
    //Is the length good?
    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), numpixels);

  }


  void test_LoadPreNeXus_CNCS_SkipPixels()
  {
    std::string eventfile( "CNCS_7860_neutron_event.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "cncs_skipped");
    //Load just 2 pixels
    eventLoader->setProperty("SpectrumList", "45, 110");
    eventLoader->setProperty("PadEmptyPixels", false);

    TS_ASSERT( eventLoader->execute() );

    EventWorkspace_sptr ew = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("cncs_skipped"));

    //Only some of the pixels weretof loaded, because of lot of them are empty
    int numpixels = 2;
    TS_ASSERT_EQUALS( ew->getNumberHistograms(), numpixels);

    //Mapping between workspace index and spectrum number; simple
    TS_ASSERT_EQUALS( ew->getAxis(1)->spectraNo(0), 0);
    TS_ASSERT_EQUALS( ew->getAxis(1)->spectraNo(1), 1);
    TS_ASSERT_EQUALS( ew->getAxis(1)->length(), 2);

    //Are the pixel IDs ok?
    std::vector<int64_t> dets = ew->spectraMap().getDetectors(0);
    TS_ASSERT_EQUALS( dets.size(), 1);
    TS_ASSERT_EQUALS( dets[0], 45);

    dets = ew->spectraMap().getDetectors(1);
    TS_ASSERT_EQUALS( dets.size(), 1);
    TS_ASSERT_EQUALS( dets[0], 110);
  }



};

#endif /* LOADEVENTPRENEXUSTEST_H_ */

