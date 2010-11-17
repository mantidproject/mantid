#ifndef DIFFRACTIONFOCUSSING2TEST_H_
#define DIFFRACTIONFOCUSSING2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/Rebin.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/MaskBins.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAPI/SpectraAxis.h"

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
    loader.setPropertyValue("Filename", "../../../../Test/AutoTestData/HRP38692.raw");

    std::string outputSpace = "tofocus";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.setPropertyValue("SpectrumMin","50");
    loader.setPropertyValue("SpectrumMax","100");
    TS_ASSERT_THROWS_NOTHING( loader.execute() );
    TS_ASSERT( loader.isExecuted() );
    
    // Have to align because diffraction focussing wants d-spacing
    Mantid::Algorithms::AlignDetectors align;
    align.initialize();
    align.setPropertyValue("InputWorkspace",outputSpace);
    align.setPropertyValue("OutputWorkspace",outputSpace);
    align.setPropertyValue("CalibrationFile","../../../../Test/AutoTestData/hrpd_new_072_01.cal");
    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS" );
    focus.setPropertyValue("GroupingFileName","../../../../Test/AutoTestData/hrpd_new_072_01.cal");

	  TS_ASSERT_THROWS_NOTHING( focus.execute() );
	  TS_ASSERT( focus.isExecuted() );

		MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("focusedWS")) );

		// only 2 groups for this limited range of spectra
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 2 );
    
    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("focusedWS");
	}

  void testEventWorkspaceSameOutputWS()
  {
    //----- Load some event data --------
    LoadEventPreNeXus * eventLoader;
    eventLoader = new LoadEventPreNeXus();
    eventLoader->initialize();
    eventLoader->setPropertyValue("EventFilename", "../../../../Test/AutoTestData/REF_L_32035_neutron_event.dat");
    eventLoader->setProperty("PulseidFilename",  "../../../../Test/AutoTestData/REF_L_32035_pulseid.dat");
    eventLoader->setPropertyValue("MappingFilename", "../../../../Test/AutoTestData/REF_L_TS_2010_02_19.dat");
    eventLoader->setPropertyValue("OutputWorkspace", "refl");
    TS_ASSERT( eventLoader->execute() );

    //Check on the input workspace
    EventWorkspace_sptr inputW = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve("refl"));
    int numpixels_with_events = 4753;
    TS_ASSERT_EQUALS( inputW->getNumberHistograms(), numpixels_with_events);

    //Fake a d-spacing unit in the data.
    inputW->getAxis(0)->unit() =UnitFactory::Instance().create("dSpacing");

    //Create a DIFFERENT x-axis for each pixel. Starting bin = the input workspace index #
    for (int pix=0; pix < numpixels_with_events; pix++)
    {
      Kernel::cow_ptr<MantidVec> axis;
      MantidVec& xRef = axis.access();
      xRef.resize(5);
      for (int i = 0; i < 5; ++i)
        xRef[i] = 1 + pix + i*1.0;
      xRef[4] = 1e6;

      //Set an X-axis
      inputW->setX(pix, axis);
    }

    focus.setPropertyValue("InputWorkspace", "refl");
    std::string outputws( "refl2" );
    focus.setPropertyValue("OutputWorkspace", outputws);

    //This fake calibration file was generated using DiffractionFocussing2Test_helper.py
    focus.setPropertyValue("GroupingFileName","../../../../Test/AutoTestData/refl_fake.cal");

    //OK, run the algorithm
    focus.execute();
    TS_ASSERT( focus.isExecuted() );

    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputws));

    //The fake grouping file has 100 groups, starting at 1, so there'll be 100 histograms
    int numgroups = 100;
    TS_ASSERT_EQUALS( output->getNumberHistograms(), numgroups);
    if (output->getNumberHistograms() != numgroups)
      return;

    //The map between workspace index and spectrum # is still 1:1
    TS_ASSERT_EQUALS( output->getAxis(1)->length(), numgroups);
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(0), 0);
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(numgroups-1), numgroups-1);

    //Because no pixels are rejected or anything, the total # of events should stay the same.
    TS_ASSERT_EQUALS( inputW->getNumberEvents(), output->getNumberEvents());

    //List of the expected total # of events in each group
    int * expected_total_events = new int[numgroups+1];

    //Now let's test the grouping of detector UDETS to groups
    for (int group=1; group<=numgroups; group++)
    {
      int workspaceindex_in_output = group-1;

      //This is the list of the detectors (grouped)
      std::vector<int> mylist = output->spectraMap().getDetectors(workspaceindex_in_output);

      //Each group has around 47 detectors, but there is some variation. They are all above 35 though
      TS_ASSERT_LESS_THAN(35, mylist.size());
      int numevents = 0;

      //This is to find the workspace index for a given original spectrum #
      Mantid::API::IndexToIndexMap * mymap = inputW->getDetectorIDToWorkspaceIndexMap(true);

      for (int i=0; i<mylist.size(); i++)
      {
        //The formula for assigning fake group #
        TS_ASSERT_EQUALS( (mylist[i] % numgroups)+1, group );
        //The workspace index in the input workspace for this detector #
        int workspaceIndex_in_input = (*mymap)[ mylist[i] ];
        //Add up the events
        numevents += inputW->getEventList(workspaceIndex_in_input).getNumberEvents();
      }
      //Look up how many events in the output, summed up spectrum (workspace index = group-1)
      TS_ASSERT_EQUALS(numevents, output->getEventList(workspaceindex_in_output).getNumberEvents());

      //The first X bin of each group corresponds to the lowest workspace index - since the limits in X are used.
      const MantidVec & X = (*output->refX(workspaceindex_in_output));
      TS_ASSERT_EQUALS( X.size(), 5);
      TS_ASSERT_EQUALS( X[0], (*mymap)[ mylist[0] ] + 1);

      //Save the # of events for later
      expected_total_events[workspaceindex_in_output] = numevents;

      delete mymap;

    }

    //Now let's try to rebin using log parameters
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", outputws);
    rebin.setPropertyValue("OutputWorkspace", outputws);
    // Check it fails if "Params" property not set
    rebin.setPropertyValue("Params", "1.0,-1.0,32768");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    /* Get the output ws again */
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputws));


    for (int workspace_index=0; workspace_index<output->getNumberHistograms(); workspace_index++)
    {
      //should be 16 bins
      TS_ASSERT_EQUALS( output->refX(workspace_index)->size(), 16);
      //There should be some data in the bins
      int events_after_binning = 0;
      for (int i=0; i<15; i++)
        events_after_binning += output->dataY(workspace_index)[i];
//      std::cout << workspace_index << " workspace_index \n";
      TS_ASSERT_EQUALS( events_after_binning, expected_total_events[workspace_index]);
    }


    delete [] expected_total_events;
  }


  //Warning: can be a slow test.
  void testEventWorkspace_PG3()
  {
    std::string outputws( "pg3" );

    //----- Load some event data --------
    LoadEventPreNeXus * eventLoader;
    eventLoader = new LoadEventPreNeXus();
    eventLoader->initialize();
    eventLoader->setPropertyValue("EventFilename", "../../../../Test/AutoTestData/PG3_732_neutron_event.dat" );
    eventLoader->setProperty("PulseidFilename", "../../../../Test/AutoTestData/PG3_732_pulseid.dat");
    eventLoader->setPropertyValue("MappingFilename","");
//    eventLoader->setProperty("InstrumentFilename", "../../../../Test/Instrument/PG3_Definition.xml");
    eventLoader->setMaxEventsToLoad(100000); //This makes loading events faster.
    eventLoader->setPropertyValue("OutputWorkspace", outputws);
    TS_ASSERT( eventLoader->execute() );

    //Check on the input workspace
    EventWorkspace_sptr inputW = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve(outputws));
    int numpixels_with_events = 14233;
    TS_ASSERT_EQUALS( inputW->getNumberHistograms(), numpixels_with_events);

    // Have to align because diffraction focussing wants d-spacing
    Mantid::Algorithms::AlignDetectors align;
    align.initialize();
    align.setPropertyValue("InputWorkspace", outputws);
    align.setPropertyValue("OutputWorkspace", outputws);
    align.setPropertyValue("CalibrationFile","../../../../Test/AutoTestData/pg3_mantid_det.cal");
    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );

    // Now do the focussing
    focus.setPropertyValue("InputWorkspace", outputws);
    focus.setPropertyValue("OutputWorkspace", outputws);
    focus.setPropertyValue("GroupingFileName","../../../../Test/AutoTestData/pg3_mantid_det.cal");
    focus.execute();
    TS_ASSERT( focus.isExecuted() );

    //Checks on the output workspace
    EventWorkspace_const_sptr output;
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputws));

    //There should be 4 groups (including group 4, which should be empty)
    int numgroups = 4;
    TS_ASSERT_EQUALS( output->getNumberHistograms(), numgroups);

    //Because no pixels are rejected or anything, the total # of events should stay the same.
    TS_ASSERT_EQUALS( inputW->getNumberEvents(), output->getNumberEvents());


    //Now let's try to rebin using log parameters
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", outputws);
    rebin.setPropertyValue("OutputWorkspace", outputws);
    rebin.setPropertyValue("Params", "0.0001,-1.0,3.2768");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());


    //Now let's test rebinning
    for ( int wi=0; wi<output->getNumberHistograms(); wi++)
    {
      //should be 16 bins
      TS_ASSERT_EQUALS( output->refX(wi)->size(), 16);
      //There should be some data in the bins
      int events_after_binning = 0;
      const MantidVec thisY = output->dataY(wi);
      for (int i=0; i<15; i++)
        events_after_binning += thisY[i];

      //Don't test the last workspace - it is empty
      if (wi < 3)
      {
        TS_ASSERT_LESS_THAN(0, events_after_binning);
      }
      else
      { //Group 4 is empty
        TS_ASSERT_EQUALS(0, events_after_binning);
      }
    }

  }



private:
  DiffractionFocussing2 focus;
};

#endif /*DIFFRACTIONFOCUSSING2TEST_H_*/
