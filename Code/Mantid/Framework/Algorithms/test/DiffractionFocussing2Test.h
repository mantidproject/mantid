#ifndef DIFFRACTIONFOCUSSING2TEST_H_
#define DIFFRACTIONFOCUSSING2TEST_H_

#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAlgorithms/MaskBins.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include <cxxtest/TestSuite.h>
#include "MantidKernel/cow_ptr.h"

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
    loader.setPropertyValue("Filename", "HRP38692.raw");

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
    align.setPropertyValue("CalibrationFile","hrpd_new_072_01.cal");
    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS" );
    focus.setPropertyValue("GroupingFileName","hrpd_new_072_01.cal");

	  TS_ASSERT_THROWS_NOTHING( focus.execute() );
	  TS_ASSERT( focus.isExecuted() );

		MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("focusedWS")) );

		// only 2 groups for this limited range of spectra
    TS_ASSERT_EQUALS( output->getNumberHistograms(), 2 );
    
    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("focusedWS");
	}



  void test_EventWorkspace_SameOutputWS()
  {
    dotestEventWorkspace(true, 2);
  }

  void test_EventWorkspace_DifferentOutputWS()
  {
    dotestEventWorkspace(false, 2);
  }
  void test_EventWorkspace_SameOutputWS_oneGroup()
  {
    dotestEventWorkspace(true, 1);
  }

  void test_EventWorkspace_DifferentOutputWS_oneGroup()
  {
    dotestEventWorkspace(false, 1);
  }

  void dotestEventWorkspace(bool inplace, int numgroups)
  {
    std::string nxsWSname("DiffractionFocussing2Test_ws");
    //----- Load some event data --------
    AlgorithmHelper::runAlgorithm("LoadEventNexus", 4,
        "Filename", "CNCS_7860_event.nxs",
        "OutputWorkspace", nxsWSname.c_str());

    //-------- Check on the input workspace ---------------
    EventWorkspace_sptr inputW = boost::dynamic_pointer_cast<EventWorkspace>
            (AnalysisDataService::Instance().retrieve(nxsWSname));
    TS_ASSERT(inputW);
    if (!inputW) return;
    int old_numevents = inputW->getNumberEvents();

    //Fake a d-spacing unit in the data.
    inputW->getAxis(0)->unit() =UnitFactory::Instance().create("dSpacing");

    //Create a DIFFERENT x-axis for each pixel. Starting bin = the input workspace index #
    for (int pix=0; pix < inputW->getNumberHistograms(); pix++)
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

    // ------------ Create a grouping workspace by name -------------
    std::string GroupNames = "bank36,bank37";
    if (numgroups == 1) GroupNames = "bank36";
    std::string groupWSName("DiffractionFocussing2Test_group");
    AlgorithmHelper::runAlgorithm("CreateGroupingWorkspace", 6,
        "InputWorkspace",  nxsWSname.c_str(),
        "GroupNames", GroupNames.c_str(),
        "OutputWorkspace", groupWSName.c_str());

    // ------------ Create a grouping workspace by name -------------
    DiffractionFocussing2 focus;
    focus.initialize();
    TS_ASSERT_THROWS_NOTHING( focus.setPropertyValue("InputWorkspace", nxsWSname) );
    std::string outputws = nxsWSname + "_focussed";
    if (inplace) outputws = nxsWSname;
    TS_ASSERT_THROWS_NOTHING( focus.setPropertyValue("OutputWorkspace", outputws) );

    //This fake calibration file was generated using DiffractionFocussing2Test_helper.py
    TS_ASSERT_THROWS_NOTHING( focus.setPropertyValue("GroupingWorkspace", groupWSName) );

    //OK, run the algorithm
    TS_ASSERT_THROWS_NOTHING( focus.execute(); )
    TS_ASSERT( focus.isExecuted() );

    EventWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING( output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputws)) );
    if (!output) return;

    TS_ASSERT_EQUALS( output->getNumberHistograms(), numgroups);
    if (output->getNumberHistograms() != numgroups)
      return;

    //The map between workspace index and spectrum # is still 1:1
    TS_ASSERT_EQUALS( output->getAxis(1)->length(), numgroups);
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(0), 0);
    TS_ASSERT_EQUALS( output->getAxis(1)->spectraNo(numgroups-1), numgroups-1);

    //Events in these two banks alone
    TS_ASSERT_EQUALS(output->getNumberEvents(), (numgroups==2) ? 16260 : 7274);

    //Now let's test the grouping of detector UDETS to groups
    for (int group=1; group<=numgroups; group++)
    {
      int workspaceindex_in_output = group-1;
      //This is the list of the detectors (grouped)
      std::vector<int> mylist = output->spectraMap().getDetectors(workspaceindex_in_output);
      //1024 pixels in a bank
      TS_ASSERT_EQUALS(mylist.size(), 1024);
    }

    //Now let's try to rebin using log parameters (this used to fail?)
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", outputws);
    rebin.setPropertyValue("OutputWorkspace", outputws);
    // Check it fails if "Params" property not set
    rebin.setPropertyValue("Params", "2.0,-1.0,65535");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    /* Get the output ws again */
    output = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputws));
    double events_after_binning = 0;
    for (int workspace_index=0; workspace_index<output->getNumberHistograms(); workspace_index++)
    {
      //should be 16 bins
      TS_ASSERT_EQUALS( output->refX(workspace_index)->size(), 16);
      //There should be some data in the bins
      for (int i=0; i<15; i++)
        events_after_binning += output->dataY(workspace_index)[i];
    }
    // The count sums up to the same as the number of events
    TS_ASSERT_DELTA( events_after_binning, (numgroups==2) ? 16260.0 : 7274.0, 1e-4);
  }






private:
  DiffractionFocussing2 focus;
};



class DiffractionFocussing2TestPerformance : public CxxTest::TestSuite
{
public:
  EventWorkspace_sptr ws;

  void setUp()
  {
    IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    alg->initialize();
    alg->setPropertyValue("Filename", "SNAP_Definition.xml");
    alg->setPropertyValue("OutputWorkspace", "SNAP_empty");
    alg->setPropertyValue("MakeEventWorkspace", "1");
    alg->execute();
    ws = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SNAP_empty"));
    ws->sortAll(TOF_SORT, NULL);

    // Fill a whole bunch of events
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i < ws->getNumberHistograms(); i++)
    {
      EventList & el = ws->getEventList(i);
      for (size_t j=0; j < 20; j++)
      {
        el.addEventQuickly( TofEvent(double(j)*1e-3) );
      }
    }
    ws->getAxis(0)->setUnit("dSpacing");

    alg = AlgorithmFactory::Instance().create("CreateGroupingWorkspace", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupNames", "bank1");
    alg->setPropertyValue("OutputWorkspace", "SNAP_group_bank1");
    alg->execute();

    alg = AlgorithmFactory::Instance().create("CreateGroupingWorkspace", 1);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupNames", "bank1,bank2,bank3,bank4,bank5,bank6");
    alg->setPropertyValue("OutputWorkspace", "SNAP_group_several");
    alg->execute();
  }

  void test_SNAP_event_one_group()
  {
    IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_bank1");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "1");
    alg->execute();
    EventWorkspace_sptr outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SNAP_focus"));

    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS( outWS->getNumberEvents(), 20*65536);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }

  void test_SNAP_event_six_groups()
  {
    IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_several");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "1");
    alg->execute();
    EventWorkspace_sptr outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SNAP_focus"));

    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS( outWS->getNumberEvents(), 6*20*65536);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }

  void test_SNAP_event_one_group_dontPreserveEvents()
  {
    IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_bank1");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "0");
    alg->execute();
    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("SNAP_focus"));

    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 1);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }

  void test_SNAP_event_six_groups_dontPreserveEvents()
  {
    IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("DiffractionFocussing", 2);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "SNAP_empty");
    alg->setPropertyValue("GroupingWorkspace", "SNAP_group_several");
    alg->setPropertyValue("OutputWorkspace", "SNAP_focus");
    alg->setPropertyValue("PreserveEvents", "0");
    alg->execute();
    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("SNAP_focus"));

    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 6);
    AnalysisDataService::Instance().remove("SNAP_focus");
  }

};


#endif /*DIFFRACTIONFOCUSSING2TEST_H_*/
