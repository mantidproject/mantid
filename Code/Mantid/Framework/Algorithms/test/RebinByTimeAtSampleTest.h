#ifndef MANTID_ALGORITHMS_REBINBYTIMEATSAMPLETEST_H_
#define MANTID_ALGORITHMS_REBINBYTIMEATSAMPLETEST_H_

#include <cxxtest/TestSuite.h>
#include "RebinByTimeBaseTest.h"
#include "MantidAlgorithms/RebinByTimeAtSample.h"
#include <numeric>

using Mantid::Algorithms::RebinByTimeAtSample;

namespace
{
  /**
   Helper method to create an event workspace around some different geometries (for each detector) L1 and L2. with uniform TOFs for each and pulse time of zero.
   */
  IEventWorkspace_sptr createSinglePulseEventWorkspace(const V3D& sourcePosition,
      const V3D& samplePosition, const std::vector<V3D>& detectorPositions,
      const std::vector<double>& allSpectraTOF)
  {
    const size_t numberspectra = detectorPositions.size();

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numberspectra, 1, 1);

    //Make fake events
    for (size_t pix = 0; pix < numberspectra; pix++)
    {
      for (size_t i = 0; i < allSpectraTOF.size(); i++)
      {
        const double tof = allSpectraTOF[i];
        uint64_t pulseTime(0); // Pulse time is always zero. Same pulse.
        retVal->getEventList(pix) += TofEvent(tof, pulseTime);
      }
    }

    // Add the required start time.
    const DateAndTime runStart(int(1));
    PropertyWithValue<std::string>* testProperty = new PropertyWithValue<std::string>("start_time",
        runStart.toSimpleString(), Direction::Input);
    retVal->mutableRun().addLogData(testProperty);

    WorkspaceCreationHelper::createInstrumentForWorkspaceWithDistances(retVal, samplePosition,
        sourcePosition, detectorPositions);

    return retVal;
  }
}

//=====================================================================================
// Functional Tests
//=====================================================================================
typedef RebinByTimeBaseTest<RebinByTimeAtSample> Super;
class RebinByTimeAtSampleTest: public CxxTest::TestSuite, public Super
{

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinByTimeAtSampleTest *createSuite()
  {
    return new RebinByTimeAtSampleTest();
  }
  static void destroySuite(RebinByTimeAtSampleTest *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    Super::test_Init();
  }

  void test_not_a_event_workspace_throws()
  {
    Super::test_not_a_event_workspace_throws();
  }

  void do_test_bad_step_throws(const double& badStep)
  {
    Super::do_test_bad_step_throws(badStep);
  }

  void test_zero_step_throws()
  {
    Super::test_zero_step_throws();
  }

  void test_less_than_zero_step_throws()
  {
    Super::test_less_than_zero_step_throws();
  }

  /*
   Test that the input workspace must be an event workspace, other types of matrix workspace will not do.
   */
  void test_input_workspace2D_throws()
  {
    Super::test_input_workspace2D_throws();
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
    Super::test_execute_with_original_binning();
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
    Super::test_execute_with_double_sized_bins_binning();
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
    Super::test_execute_with_quadruple_sized_bins_binning();
  }

  void test_execute_with_multiple_spectra()
  {
    Super::test_execute_with_multiple_spectra();
  }

  void test_execute_with_xmin_larger_than_xmax_throws()
  {
    Super::test_execute_with_xmin_larger_than_xmax_throws();
  }

  void test_calculate_xmin_xmax()
  {
    Super::test_calculate_xmin_xmax();
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
    Super::test_calculate_non_zero_offset();
  }

  void test_filter_spectra_with_harmonic_L1_over_L1_plus_L2_ratios_all_other_affects_being_equal()
  {
    const std::vector<double> tofValues(1, 5000); // 1 tof event per spectra with TOF of 5 micro seconds. Incidentally 5 micro seconds corresponds to the speed that thermal neutrons 2.2km/s would take to cover a 10m distance.

    const double L1 = 10; // 10 meters
    const double L2_spec1 = 0; // Therefore L1 / (L1 + L2) == 1
    const double L2_spec2 = L1; // Therefore L1 / (L1 + L2) == 1 / 2
    const double L2_spec3 = 2 * L1; // Therefore L1 / (L1 + L2) == 1 / 3

    V3D sample(L1, 0, 0); // Sample at L1
    V3D source(0, 0, 0);

    std::vector<V3D> detectorPositions;
    detectorPositions.push_back(V3D(L2_spec1, 0, 0));
    detectorPositions.push_back(V3D(L2_spec2, 0, 0));
    detectorPositions.push_back(V3D(L2_spec3, 0, 0));

    auto inWS = createSinglePulseEventWorkspace(source, sample, detectorPositions, tofValues);

    /*
     Since TOF is 5E-3 seconds for all spectra. and the distance ratios find the form of a harmonic sequence,
     We should expect events when rebinned by time at sample to sit at:

     5 * 1/1 = 5
     5 * 1/2 = 2.5
     5 * 1/3 = 1.66

     */

    RebinByTimeAtSample alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    Mantid::MantidVec rebinArgs = boost::assign::list_of<double>(0)(1e-3)(6e-3); // Provide rebin arguments. Arguments are in seconds.
    alg.setProperty("Params", rebinArgs);
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.execute();
    MatrixWorkspace_sptr result = alg.getProperty("OutputWorkspace");

    /*
     Test output description.

     Bins set up between 0 microseconds and 6000 microseconds with a 1000 microsecond step

     0e-3    1e-3   2e-3    3e-3    4e-3    5e-3    6e-3
     |       |      |       |       |       |       |         X array
     -----------^      ^                   ^
     -----------|      |                   |                  TOF pulse times
     ----------5*1/3  5*1/2               5*1/1
     ----------spec3  spec2               spec1

     */

    TSM_ASSERT_EQUALS("Should not loose spectrum", 3, result->getNumberHistograms());

    auto y1 = result->readY(0);
    auto y1Sum = std::accumulate(y1.begin(), y1.end(), 0);

    auto y2 = result->readY(1);
    auto y2Sum = std::accumulate(y2.begin(), y2.end(), 0);

    auto y3 = result->readY(2);
    auto y3Sum = std::accumulate(y3.begin(), y3.end(), 0);

    TSM_ASSERT_EQUALS("Spectrum 1 not rebinned to sample time correctly", 1.0, y1[4]);
    TSM_ASSERT_EQUALS("Spectrum 2 not rebinned to sample time correctly", 1.0, y2[2]);
    TSM_ASSERT_EQUALS("Spectrum 3 not rebinned to sample time correctly", 1.0, y3[1]);

    TSM_ASSERT_EQUALS("Spectrum 1 should only contain one count", 1.0, y1Sum);
    TSM_ASSERT_EQUALS("Spectrum 2 should only contain one count", 1.0, y2Sum);
    TSM_ASSERT_EQUALS("Spectrum 3 should only contain one count", 1.0, y3Sum);

  }
};

//=====================================================================================
// Performance Tests
//=====================================================================================
class RebinByTimeAtSampleTestPerformance: public CxxTest::TestSuite,
    public RebinByTimeBaseTestPerformance<RebinByTimeAtSample>
{

public:
  static RebinByTimeAtSampleTestPerformance *createSuite()
  {
    return new RebinByTimeAtSampleTestPerformance();
  }
  static void destroySuite(RebinByTimeAtSampleTestPerformance *suite)
  {
    delete suite;
  }

  RebinByTimeAtSampleTestPerformance()
  {
  }

  void setUp()
  {
    RebinByTimeBaseTestPerformance<RebinByTimeAtSample>::setUp();
  }

  void testExecution()
  {
    RebinByTimeBaseTestPerformance<RebinByTimeAtSample>::testExecution();
  }
};

#endif /* MANTID_ALGORITHMS_REBINBYTIMEATSAMPLETEST_H_ */
