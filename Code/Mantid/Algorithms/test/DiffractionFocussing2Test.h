#ifndef DIFFRACTIONFOCUSSING2TEST_H_
#define DIFFRACTIONFOCUSSING2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/AlignDetectors.h"
#include "MantidAlgorithms/MaskBins.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class DiffractionFocussing2Test : public CxxTest::TestSuite
{
public:
	void testName()
	{
		TS_ASSERT_EQUALS( focus.name(), "DiffractionFocussing" );
	}

	void testVersion()
	{
	  TS_ASSERT_EQUALS( focus.version(), 2 );
	}

	void testCategory()
	{
    TS_ASSERT_EQUALS( focus.category(), "Diffraction" );
	}

	void testInit()
	{
	  focus.initialize();
	  TS_ASSERT( focus.isInitialized() );
	}

	void testExec()
	{
    Mantid::DataHandling::LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/Data/HRP38692.RAW");

    std::string outputSpace = "tofocus";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.setPropertyValue("SpectrumMin","50");
    loader.setPropertyValue("SpectrumMax","100");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT( loader.isExecuted() );
    
    // Have to align because diffraction focussing wants d-spacing
    Mantid::DataHandling::AlignDetectors align;
    align.initialize();
    align.setPropertyValue("InputWorkspace",outputSpace);
    align.setPropertyValue("OutputWorkspace",outputSpace);
    align.setPropertyValue("CalibrationFile","../../../../Test/Data/hrpd_new_072_01.cal");
    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS" );
    focus.setPropertyValue("GroupingFileName","../../../../Test/Data/hrpd_new_072_01.cal");

	  TS_ASSERT_THROWS_NOTHING( focus.execute() );
	  TS_ASSERT( focus.isExecuted() );

		MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("focusedWS")) );

		// only 2 groups for this limited range of spectra
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 2 );
    
    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("focusedWS");
	}

	void testEventWorkspace()
	{
    //----- Load some event data --------
	  LoadEventPreNeXus * eventLoader;
    eventLoader = new LoadEventPreNeXus();
    eventLoader->initialize();
    std::string eventfile( "../../../../Test/Data/sns_event_prenexus/REF_L_32035_neutron_event.dat" );
    std::string pulsefile( "../../../../Test/Data/sns_event_prenexus/REF_L_32035_pulseid.dat" );
    eventLoader->setPropertyValue("EventFilename", eventfile);
    eventLoader->setProperty("PulseidFilename", pulsefile);
    eventLoader->setPropertyValue("MappingFilename",
          "../../../../Test/Data/sns_event_prenexus/REF_L_TS_2010_02_19.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "refl");
    TS_ASSERT( eventLoader->execute() );

    //Check on the input workspace
    EventWorkspace_sptr inputW = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("refl"));
    int numpixels_with_events = 4753;
    TS_ASSERT_EQUALS( inputW->getNumberHistograms(), numpixels_with_events);

    //Fake a d-spacing unit in the data.
    inputW->getAxis(0)->unit() =UnitFactory::Instance().create("dSpacing");

    /*
    // Have to align because diffraction focussing wants d-spacing
    Mantid::DataHandling::AlignDetectors align;
    align.initialize();
    align.setPropertyValue("InputWorkspace", "refl");
    align.setPropertyValue("OutputWorkspace", "refl");
    align.setPropertyValue("CalibrationFile","../../../../Test/Data/refl_fake.cal");
    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );
    */

    focus.setPropertyValue("InputWorkspace", "refl");
    focus.setPropertyValue("OutputWorkspace", "focusedWS" );

    //This fake calibration file was generated using DiffractionFocussing2Test_helper.py
    focus.setPropertyValue("GroupingFileName","../../../../Test/Data/refl_fake.cal");

    //OK, run the algorithm
    focus.execute();
    TS_ASSERT( focus.isExecuted() );

    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("focusedWS"));
    //The fake grouping file has 100 groups, starting at 1, so there'll be 100 histograms
    int numgroups = 100;
    TS_ASSERT_EQUALS( output->getNumberHistograms(), numgroups);
    //This means that the map between workspace index and spectrum # is just off by 1
    TS_ASSERT_EQUALS( output->getAxis(1)->length(), numgroups);
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(0), 1);
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(numgroups-1), numgroups);

    //Because no pixels are rejected or anything, the total # of events should stay the same.
    TS_ASSERT_EQUALS( inputW->getNumberEvents(), output->getNumberEvents());

    //Now let's test the grouping of detector UDETS to groups
    for (int group=1; group<numgroups; group++)
    {
      std::vector<int> mylist = output->spectraMap().getDetectors(group);
      //std::cout << mylist.size() << " group " << group << "\n";
      //Each group has around 47 detectors, but there is some variation. They are all above 35 though
      TS_ASSERT_LESS_THAN(35, mylist.size());
      int numevents = 0;
      //This is to find the workspace index for a given original spectrum #
      Mantid::API::Axis::spec2index_map mymap;
      inputW->getAxis(1)->getSpectraIndexMap(mymap);

      for (int i=0; i<mylist.size(); i++)
      {
        //The formula for assigning fake group #
        TS_ASSERT_EQUALS( (mylist[i] % numgroups)+1, group );
        //The workspace index in the input workspace for this detector #
        int workspaceIndex = mymap[ mylist[i] ];
        //Add up the events
        numevents += inputW->getEventListAtWorkspaceIndex(workspaceIndex).getNumberEvents();
      }
      //Look up how many events in the output, summed up spectrum (workspace index = group-1)
      TS_ASSERT_EQUALS(numevents, output->getEventListAtWorkspaceIndex(group-1).getNumberEvents());
    }



	}

private:
  DiffractionFocussing2 focus;
};

#endif /*DIFFRACTIONFOCUSSING2TEST_H_*/
