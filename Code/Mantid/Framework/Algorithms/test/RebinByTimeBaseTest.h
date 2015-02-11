/*
 Base class for common rebinning testing performed by test clases such as RebinByPulseTime and RebinByTimeAtSample
 */

#ifndef MANTID_ALGORITHMS_REBINBYTIMEBASETEST_H_
#define MANTID_ALGORITHMS_REBINBYTIMEBASETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/DateAndTime.h"
#include "MantidAlgorithms/RebinByTimeAtSample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/make_shared.hpp>
#include <boost/assign/list_of.hpp>
#include <gmock/gmock.h>

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace
{
  /**
   Helper method to create an event workspace with a set number of distributed events between pulseTimeMax and pulseTimeMin.
   */
  IEventWorkspace_sptr createEventWorkspace(const int numberspectra, const int nDistrubutedEvents,
      const int pulseTimeMinSecs, const int pulseTimeMaxSecs,
      const DateAndTime runStart = DateAndTime(int(1)))
  {
    uint64_t pulseTimeMin = uint64_t(1e9) * pulseTimeMinSecs;
    uint64_t pulseTimeMax = uint64_t(1e9) * pulseTimeMaxSecs;

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numberspectra, 1, 1);
    double binWidth = std::abs(double(pulseTimeMax - pulseTimeMin) / nDistrubutedEvents);

    //Make fake events
    for (int pix = 0; pix < numberspectra; pix++)
    {
      for (int i = 0; i < nDistrubutedEvents; i++)
      {
        double tof = 0;
        uint64_t pulseTime = uint64_t(((double) i + 0.5) * binWidth); // Stick an event with a pulse_time in the middle of each pulse_time bin.
        retVal->getEventList(pix) += TofEvent(tof, pulseTime);
      }
    }

    // Add the required start time.
    PropertyWithValue<std::string>* testProperty = new PropertyWithValue<std::string>("start_time",
        runStart.toSimpleString(), Direction::Input);
    retVal->mutableRun().addLogData(testProperty);

    V3D samplePosition(10, 0, 0);
    V3D sourcePosition(0, 0, 0);
    std::vector<V3D> detectorPositions(numberspectra, V3D(20, 0, 0));

    WorkspaceCreationHelper::createInstrumentForWorkspaceWithDistances(retVal, samplePosition,
        sourcePosition, detectorPositions);

    return retVal;
  }

  /*
   This type is an IEventWorkspace, but not an EventWorkspace.
   */
  class MockIEventWorkspace: public Mantid::API::IEventWorkspace
  {
  public:
    MOCK_CONST_METHOD0(getNumberEvents, std::size_t());
    MOCK_CONST_METHOD0(getTofMin, double());
    MOCK_CONST_METHOD0(getTofMax, double());
    MOCK_CONST_METHOD0(getPulseTimeMin, DateAndTime());
    MOCK_CONST_METHOD0(getPulseTimeMax, DateAndTime());
    MOCK_CONST_METHOD1(getTimeAtSampleMax, DateAndTime(double));
    MOCK_CONST_METHOD1(getTimeAtSampleMin, DateAndTime(double));
    MOCK_CONST_METHOD0(getEventType, EventType());
    MOCK_METHOD1(getEventListPtr, IEventList*(const std::size_t));
    MOCK_CONST_METHOD5(generateHistogram, void(const std::size_t, const Mantid::MantidVec&, Mantid::MantidVec&, Mantid::MantidVec&, bool));
    MOCK_METHOD0(clearMRU, void());
    MOCK_CONST_METHOD0(clearMRU, void());
    MOCK_CONST_METHOD0(blocksize, std::size_t());
    MOCK_CONST_METHOD0(size, std::size_t());
    MOCK_CONST_METHOD0(getNumberHistograms, std::size_t());
    MOCK_METHOD1(getSpectrum, Mantid::API::ISpectrum*(const std::size_t));
    MOCK_CONST_METHOD1(getSpectrum, const Mantid::API::ISpectrum*(const std::size_t));
    MOCK_METHOD3(init, void(const size_t&, const size_t&, const size_t&));
    MOCK_CONST_METHOD0(getSpecialCoordinateSystem, Mantid::Kernel::SpecialCoordinateSystem());
    virtual ~MockIEventWorkspace()
    {}
  };}
//=====================================================================================
// Functional Tests
//=====================================================================================
template<class AlgorithmType>
class RebinByTimeBaseTest
{

public:

  virtual ~RebinByTimeBaseTest()
  {
  }

private:

  /**
   Test execution method.

   Sets up the algorithm for rebinning and executes it. Also verifies the results.
   */
  void do_execute_and_check_binning(const int nSpectra, const int pulseTimeMin, const int pulseTimeMax,
      const int nUniformDistributedEvents, const int nBinsToBinTo)
  {
    IEventWorkspace_sptr inWS = createEventWorkspace(nSpectra, nUniformDistributedEvents, pulseTimeMin,
        pulseTimeMax);

    // Rebin pameters require the step.
    const int step = (pulseTimeMax - pulseTimeMin) / (nBinsToBinTo);

    AlgorithmType alg;
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
    TSM_ASSERT_EQUALS("X unit should be Time/s", expectedUnit->unitID(),
        outWS->getAxis(0)->unit()->unitID());
    for (int i = 1; i < outWS->axes(); ++i)
    {
      TSM_ASSERT_EQUALS("Axis units do not match.", inWS->getAxis(i)->unit()->unitID(),
          outWS->getAxis(i)->unit()->unitID());
    }

    //Validate each spectra
    for (int i = 0; i < nSpectra; ++i)
    {
      // Check that the x-axis has been set-up properly. It should mirror the original rebin parameters.
      const Mantid::MantidVec& X = outWS->readX(i);
      TS_ASSERT_EQUALS(nBinsToBinTo + 1, X.size());
      for (uint64_t j = 0; j < X.size(); ++j)
      {
        TS_ASSERT_EQUALS(static_cast<int>(step*j), static_cast<int>(X[j]));
      }

      // Check that the y-axis has been set-up properly.

      const Mantid::MantidVec& Y = outWS->readY(i);
      TS_ASSERT_EQUALS(nBinsToBinTo, Y.size());
      for (uint64_t j = 0; j < Y.size(); ++j)
      {
        TS_ASSERT_EQUALS(nUniformDistributedEvents/nBinsToBinTo, Y[j]);
        // Should have 1 event per bin, because that's what the createEventWorkspace() provides and our rebinning params are based on our original creation parameters.
      }
    }
  }

public:

  // Constructor
  RebinByTimeBaseTest()
  {
  }

  void test_Init()
  {
    AlgorithmType alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_not_a_event_workspace_throws()
  {
    IEventWorkspace_sptr ws(new MockIEventWorkspace);

    AlgorithmType alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(1);
    alg.setProperty("Params", rebinArgs);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    try
    {
      alg.execute();
    } catch (std::invalid_argument&)
    {
      return;
    }
  }

  void do_test_bad_step_throws(const double& badStep)
  {
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 4;
    const int nUniformDistributedEvents = 4;
    const int nSpectra = 1;

    IEventWorkspace_sptr ws = createEventWorkspace(nSpectra, nUniformDistributedEvents, pulseTimeMin,
        pulseTimeMax); // Create an otherwise valid input workspace.

    AlgorithmType alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    Mantid::MantidVec rebinArgs1 = boost::assign::list_of<double>(badStep);  // Step is zero!.
    alg.setProperty("Params", rebinArgs1);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    try
    {
      alg.execute();
    } catch (std::invalid_argument&)
    {
      return;
    }
  }

  void test_zero_step_throws()
  {
    do_test_bad_step_throws(0);
  }

  void test_less_than_zero_step_throws()
  {
    do_test_bad_step_throws(-1);
  }

  /*
   Test that the input workspace must be an event workspace, other types of matrix workspace will not do.
   */
  void test_input_workspace2D_throws()
  {
    using Mantid::DataObjects::Workspace2D;
    Workspace_sptr workspace2D = boost::make_shared<Workspace2D>();

    AlgorithmType alg;
    alg.initialize();
    try
    {
      alg.setProperty("InputWorkspace", workspace2D);
    } catch (std::invalid_argument&)
    {
      return;
    }
  }

  /**
   Test setup description.

   Bins set up with no offset and a spacing of 1e9 according to the rebin parameters.
   The events in the workspace are created such that they sit in the middle of each bin. They are uniformly distributed from 0.5e9 to 19.5e9, so binning should occur as follows:

   0      1e9   2e9   3e9   4e9   5e9 .... 20e9
   |     |     |     |     |                 X array
   ^      ^      ^     ^
   |      |      |     |                   TOF pulse times
   0.5e9  1.5e9  2.5e9 3.5e9 ... 19e9

   so Y array should work out to be [1, 1, 1, ...] counts.
   */
  void test_execute_with_original_binning()
  {
    const int nSpectra = 1;
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;

    const int numberOfBinsToBinTo = 20; // Gives the expected occupancy of each bin, given that the original setup is 1 event per bin.
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents,
        numberOfBinsToBinTo);
  }

  /**
   Test setup description.

   Bins set up with no offset and a spacing of 2*e9 according to the rebin parameters.
   The events in the workspace are created such that they sit in the middle of each bin. They are uniformly distributed from 0.5e9 to 19.5e9, so binning should occur as follows:

   0          2e9            4e9   .... 20e9
   |           |              |                 X array
   ^      ^      ^     ^
   |      |      |     |                      TOF pulse times
   0.5e9  1.5e9  2.5e9 3.5e9 ... 19e9

   so Y array should work out to be [2, 2, 2, ...] counts.
   */
  void test_execute_with_double_sized_bins_binning()
  {
    const int nSpectra = 1;
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;

    const int numberOfBinsToBinTo = 10; // The bins are now twice as big!
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents,
        numberOfBinsToBinTo);
  }

  /**
   Test setup description.

   Bins set up with no offset and a spacing of 4*e9 according to the rebin parameters.
   The events in the workspace are created such that they sit in the middle of each bin. They are uniformly distributed from 0.5e9 to 19.5e9, so binning should occur as follows:

   0                     4e9   .... 20e9
   |                        |                 X array
   ^      ^      ^     ^
   |      |      |     |                      TOF pulse times
   0.5e9  1.5e9  2.5e9 3.5e9 ... 19e9

   so Y array should work out to be [4, 4, 4, ...] counts.
   */
  void test_execute_with_quadruple_sized_bins_binning()
  {
    const int nSpectra = 1;
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;

    const int numberOfBinsToBinTo = 5; // The bins are now four times as big.
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents,
        numberOfBinsToBinTo);
  }

  void test_execute_with_multiple_spectra()
  {
    const int nSpectra = 10; // multipe spectra created in input workspace.
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 20;
    const int nUninformDistributedEvents = 20;

    const int numberOfBinsToBinTo = 5;
    do_execute_and_check_binning(nSpectra, pulseTimeMin, pulseTimeMax, nUninformDistributedEvents,
        numberOfBinsToBinTo);
  }

  void test_execute_with_xmin_larger_than_xmax_throws()
  {
    // Rebin pameters require the step.
    const int step = 1;
    const double pulseTimeMin = 10;
    const double pulseTimeMax = 0;

    AlgorithmType alg;
    alg.setRethrows(true);
    alg.initialize();
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(pulseTimeMin)(step)(pulseTimeMax); // Provide rebin arguments.

    try
    {
      alg.setProperty("Params", rebinArgs);
    } catch (std::invalid_argument&)
    {
      return;
    }
  }

  void test_calculate_xmin_xmax()
  {
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 10;
    const int nUniformDistributedEvents = 10;
    const int nSpectra = 1;
    const int nBinsToBinTo = 10;

    IEventWorkspace_sptr ws = createEventWorkspace(nSpectra, nUniformDistributedEvents, pulseTimeMin,
        pulseTimeMax);

    // Rebin pameters require the step.
    const int step = static_cast<int>((pulseTimeMax - pulseTimeMin) / (nBinsToBinTo));

    AlgorithmType alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(step); // Provide rebin arguments. Note we are only providing the step, xmin and xmax are calculated internally!
    alg.setProperty("Params", rebinArgs);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();

    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<Workspace2D>("outWS");
    const Mantid::MantidVec& X = outWS->readX(0);

    // Check that xmin and xmax have been caclulated correctly.
    TS_ASSERT_EQUALS(nBinsToBinTo, X.size());
    // Added 1 nanosecond to start time
    TS_ASSERT_EQUALS(pulseTimeMin + double(step)/2 - 1.e-9, X.front());
    TS_ASSERT_EQUALS(pulseTimeMax - double(step)/2 - 1.e-9, X.back());
  }

  /*
   Test setup description.

   Bins set up with 1e9 offset according to the rebin parameters.
   But the events in the workspace are created without the offset, they have uniformly distributed pulse times from 0.5e9 to 3.5e9, so binning should occur as follows:

   1e9   2e9   3e9   4e9   5e9
   |     |     |     |     |         X array
   ^      ^      ^     ^
   |      |      |     |           TOF pulse times
   0.5e9  1.5e9  2.5e9 3.5e9

   so Y array should work out to be [1, 1, 1, 0] counts.
   */
  void test_calculate_non_zero_offset()
  {
    const int pulseTimeMin = 0;
    const int pulseTimeMax = 4;
    const int nUniformDistributedEvents = 4;
    const int nSpectra = 1;
    const int nBinsToBinTo = 4;
    DateAndTime offSet(size_t(1e9)); // Offset (start_time).

    IEventWorkspace_sptr ws = createEventWorkspace(nSpectra, nUniformDistributedEvents, pulseTimeMin,
        pulseTimeMax, offSet);

    // Rebin pameters require the step.
    const int step = static_cast<int>((pulseTimeMax - pulseTimeMin) / (nBinsToBinTo));

    AlgorithmType alg;
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

    const Mantid::MantidVec& Y = outWS->readY(0);
    TS_ASSERT_EQUALS(nBinsToBinTo, Y.size());

    TS_ASSERT_EQUALS(nUniformDistributedEvents/nBinsToBinTo, Y[0]);
    TS_ASSERT_EQUALS(nUniformDistributedEvents/nBinsToBinTo, Y[1]);
    TS_ASSERT_EQUALS(nUniformDistributedEvents/nBinsToBinTo, Y[2]);
    TS_ASSERT_EQUALS(0, Y[3]);

  }
};

//=====================================================================================
// Performance Tests
//=====================================================================================
template<class AlgorithmType>
class RebinByTimeBaseTestPerformance
{
private:

  IEventWorkspace_sptr m_ws;
  const int pulseTimeMin;
  const int pulseTimeMax;
  const int nUniformDistributedEvents;
  const int nSpectra;
  const int nBinsToBinTo;

public:

  RebinByTimeBaseTestPerformance() :
      pulseTimeMin(0), pulseTimeMax(4), nUniformDistributedEvents(10000), nSpectra(1000), nBinsToBinTo(
          100)
  {
  }

  void setUp()
  {
    // Make a reasonably sized workspace to rebin.
    m_ws = createEventWorkspace(nSpectra, nUniformDistributedEvents, pulseTimeMin, pulseTimeMax);
  }

  void testExecution()
  {
    const double dPulseTimeMax = pulseTimeMax;
    const double dPulseTimeMin = pulseTimeMin;
    const double step = (dPulseTimeMax - dPulseTimeMin) / (nBinsToBinTo);

    AlgorithmType alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", m_ws);
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(dPulseTimeMin)(step)(dPulseTimeMax); // Provide rebin arguments.
    alg.setProperty("Params", rebinArgs);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Simple tests. Functionality tests cover this much better.
    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<Workspace2D>("outWS");
    TS_ASSERT(outWS != NULL);
  }
};

#endif /* MANTID_ALGORITHMS_REBINBYTIMEBASETEST_H_ */
