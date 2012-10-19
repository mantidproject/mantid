#ifndef MANTID_ALGORITHMS_QUERYPULSETIMESTEST_H_
#define MANTID_ALGORITHMS_QUERYPULSETIMESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/DateAndTime.h"
#include "MantidAlgorithms/QueryPulseTimes.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include <boost/assign/list_of.hpp>

using Mantid::Algorithms::QueryPulseTimes;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class QueryPulseTimesTest : public CxxTest::TestSuite
{

private:

  /**
  Helper method to create an event workspace with a set number of distributed events between pulseTimeMax and pulseTimeMin.
  */
  IEventWorkspace_sptr createEventWorkspace(const int numberspectra, const int nDistrubutedEvents, const int pulseTimeMinSecs, const int pulseTimeMaxSecs, const DateAndTime runStart=DateAndTime(int(0)))
  {
    size_t pulseTimeMin = 1e9 * pulseTimeMinSecs;
    size_t pulseTimeMax = 1e9 * pulseTimeMaxSecs;

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numberspectra,1,1);
    double binWidth = std::abs(double(pulseTimeMax - pulseTimeMin)/nDistrubutedEvents);

    //Make fake events
    for (int pix=0; pix<numberspectra; pix++)
    {
      for (int i=0; i<nDistrubutedEvents; i++)
      {
        double tof = 0;
        double pulseTime = ((double)i+0.5)*binWidth; // Stick an event with a pulse_time in the middle of each pulse_time bin.
        retVal->getEventList(pix) += TofEvent(tof, pulseTime);
      }
      retVal->getEventList(pix).addDetectorID(pix);
      retVal->getEventList(pix).setSpectrumNo(pix);
    }
    retVal->doneAddingEventLists();

    // Add the required start time.
    PropertyWithValue<std::string>* testProperty = new PropertyWithValue<std::string>("start_time", runStart.toSimpleString(), Direction::Input);
    retVal->mutableRun().addLogData(testProperty);

    return retVal;
  }

  /**
  Test execution method.

  Sets up the algorithm for rebinning and executes it. Also verifies the results.
  */
  void do_execute_and_check_binning(const int nSpectra, const int pulseTimeMin, const int pulseTimeMax, const int nUniformDistributedEvents, const int nBinsToBinTo)
  {
    IEventWorkspace_sptr inWS = createEventWorkspace(nSpectra, nUniformDistributedEvents, pulseTimeMin, pulseTimeMax);

    // Rebin pameters require the step.
    const int step = (pulseTimeMax - pulseTimeMin)/(nBinsToBinTo); 

    QueryPulseTimes alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(pulseTimeMin)(step)(pulseTimeMax); // Provide rebin arguments.
    alg.setProperty("Params", rebinArgs);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();

    
    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<Workspace2D>("outWS");

    // Check the units of the output workspace.
    Unit_const_sptr expectedUnit(new Units::Time()); 
    TSM_ASSERT_EQUALS("X unit should be Time/s", expectedUnit->unitID(), outWS->getAxis(0)->unit()->unitID());
    for (int i=1; i < outWS->axes(); ++i)
    {
      TSM_ASSERT_EQUALS("Axis units do not match.", inWS->getAxis(i)->unit()->unitID(), outWS->getAxis(i)->unit()->unitID());
    }

    //Validate each spectra
    for(int i = 0; i < nSpectra; ++i)
    {
      // Check that the x-axis has been set-up properly. It should mirror the original rebin parameters.
      const Mantid::MantidVec& X = outWS->readX(i);
      TS_ASSERT_EQUALS(nBinsToBinTo + 1, X.size());
      for(int j = 0; j < X.size(); ++j)
      {
        TS_ASSERT_EQUALS(static_cast<int>(step*j), static_cast<int>(X[j]));
      }

      // Check that the y-axis has been set-up properly.
      
      const Mantid::MantidVec& Y = outWS->readY(i);
      TS_ASSERT_EQUALS(nBinsToBinTo, Y.size());
      for(int j = 0; j < Y.size(); ++j)
      {
        TS_ASSERT_EQUALS(nUniformDistributedEvents/nBinsToBinTo, Y[j]); // Should have 1 event per bin, because that's what the createEventWorkspace() provides and our rebinning params are based on our original creation parameters.
      }
    }
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryPulseTimesTest *createSuite() { return new QueryPulseTimesTest(); }
  static void destroySuite( QueryPulseTimesTest *suite ) { delete suite; }


  void test_Init()
  {
    QueryPulseTimes alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  /*
  Test that the input workspace must be an event workspace, other types of matrix workspace will not do.
  */
  void test_input_workspace2D_throws()
  {
    using Mantid::DataObjects::Workspace2D;
    Workspace_sptr workspace2D = boost::make_shared<Workspace2D>();

    QueryPulseTimes alg;
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", workspace2D), std::invalid_argument);
  }

  void test_execute_with_original_binning()
  {
    const int nSpectra = 1;
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;
    
    const int numberOfBinsToBinTo = 20; // Gives the expected occupancy of each bin, given that the original setup is 1 event per bin.
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents, numberOfBinsToBinTo);
  }

  void test_execute_with_double_sized_bins_binning()
  {
    const int nSpectra = 1;
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;
    
    const int numberOfBinsToBinTo = 10; // The bins are now twice as big.
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents, numberOfBinsToBinTo);
  }

  void test_execute_with_quadruple_sized_bins_binning()
  {
    const int nSpectra = 1;
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;
    
    const int numberOfBinsToBinTo = 5; // The bins are now four times as big.
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents, numberOfBinsToBinTo);
  }

  void test_execute_with_multiple_spectra()
  {
    const int nSpectra = 10; // multipe spectra created in input workspace.
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;
    
    const int numberOfBinsToBinTo = 5; 
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents, numberOfBinsToBinTo);
  }

  void test_execute_with_xmin_larger_than_xmax_throws()
  {
    // Rebin pameters require the step.
    const int step = 1;
    const double pulseTimeMin = 10;
    const double pulseTimeMax = 0;

    QueryPulseTimes alg;
    alg.setRethrows(true);
    alg.initialize();
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(pulseTimeMin)(step)(pulseTimeMax); // Provide rebin arguments.
    TSM_ASSERT_THROWS("Shouldnt be able to have xmin > xmax", alg.setProperty("Params", rebinArgs), std::invalid_argument);
  }

  void test_calculate_xmin_xmax()
  {
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 10;
    const int nUniformDistributedEvents = 10;
    const int nSpectra = 1;
    const int nBinsToBinTo = 10;

    IEventWorkspace_sptr ws = createEventWorkspace(nSpectra, nUniformDistributedEvents, pulseTimeMin, pulseTimeMax);

    // Rebin pameters require the step.
    const int step = static_cast<int>((pulseTimeMax - pulseTimeMin)/(nBinsToBinTo)); 

    QueryPulseTimes alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(pulseTimeMin)(step)(pulseTimeMax); // Provide rebin arguments.
    alg.setProperty("Params", rebinArgs);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();

    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<Workspace2D>("outWS");
    const Mantid::MantidVec& X = outWS->readX(0);

    // Check that xmin and xmax have been caclulated correctly.
    TS_ASSERT_EQUALS(nBinsToBinTo + 1, X.size());
    TS_ASSERT_EQUALS(pulseTimeMin, X.front());
    TS_ASSERT_EQUALS(pulseTimeMax, X.back());
  }

};


#endif /* MANTID_ALGORITHMS_QUERYPULSETIMESTEST_H_ */