// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RINGPROFILETEST_H_
#define MANTID_ALGORITHMS_RINGPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NumericAxis.h"
#include "MantidAlgorithms/RingProfile.h"
#include "MantidDataHandling/SaveNexus.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::RingProfile;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class RingProfileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RingProfileTest *createSuite() { return new RingProfileTest(); }
  static void destroySuite(RingProfileTest *suite) { delete suite; }

  void test_wrongInputs() {
    std::string outWSName("RingProfileTest_OutputWS");
    RingProfile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // check numbins is only integer > 1
    TS_ASSERT_THROWS(alg.setProperty("NumBins", -3), const std::invalid_argument &);
    // ange between [-360, 360]
    TS_ASSERT_THROWS(alg.setProperty("StartAngle", 500.0),
                     const std::invalid_argument &);

    // centre must be 2 or 3 values (x,y) or (x,y,z)
    std::vector<double> justOne(1);
    justOne[0] = -0.35;
    // TS_ASSERT_THROWS(alg.setProperty("Centre",justOne),
    // const std::invalid_argument &);

    std::vector<double> fourInputs(4, -0.45);
    // TS_ASSERT_THROWS(alg.setProperty("Centre", fourInputs),
    // const std::invalid_argument &);

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    // change to a 2d workspace
    MatrixWorkspace_sptr goodWS = create_2d_workspace();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", goodWS));

    // centre must be inside the limits of the workspace
    std::vector<double> twoInputs(2, 0);
    // set the centre outside the matrix workspace
    twoInputs[0] = goodWS->readX(0)[0] - 3.5;
    twoInputs[1] = goodWS->getAxis(1)->getMin() - 4.5;
    // it is a valid input because it has just two inputs
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", twoInputs));

    TS_ASSERT_EQUALS(alg.execute(), false);
  }
  void test_wrongInputs_SpectraInput() {

    std::string outWSName("RingProfileTest_OutputWS");
    RingProfile alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    // change to a 2d workspace
    MatrixWorkspace_sptr goodWS = create_rectangular_instrument_workspace();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", goodWS));

    // centre must be inside the limits of the workspace
    std::vector<double> twoInputs(3, 0);
    // set the centre outside the matrix workspace
    twoInputs[0] = goodWS->readX(0)[0] - 3.5;
    twoInputs[1] = goodWS->getAxis(1)->getMin() - 4.5;
    // it is a valid input because it has just two inputs
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", twoInputs));

    TS_ASSERT_EQUALS(alg.execute(), false);
  }

  /**
     This will create the following input 2D workspace:
     ------------>X
     | 0 2 0 1 0
     | 2 0 2 0 1
     | 0 3 0 1 0
     | 3 0 4 0 4
     | 0 3 0 4 0
    \/
     Y

     With the limits being: X = [ -0.3, -0.18, -0.06, 0.06, 0.18, 0.3 ]
    x0=-0.3,delta=0.12
                            Y = [-0.24, -0.12,  0   , 0.12, 0.24]


     To be easy to have a visual interpretation I will put the Y axis growing
    towards up:
    Y
     | 0 3 0 4 0
     | 3 0 4 0 4
     | 0 3 0 1 0
     | 2 0 2 0 1
     | 0 2 0 1 0
     ------------>X
   */
  static MatrixWorkspace_sptr create_2d_workspace() {
    MatrixWorkspace_sptr goodWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(5, 5, -0.3, 0.12);
    NumericAxis *yAxis = new NumericAxis(5);

    for (int i = 0; i < 5; ++i) {
      yAxis->setValue(i, -0.24 + 0.12 * i);
    }
    goodWS->replaceAxis(1, yAxis);

    NumericAxis *xAxis = new NumericAxis(6);
    for (int i = 0; i < 6; i++)
      xAxis->setValue(i, -0.3 + i * 0.12);
    goodWS->replaceAxis(0, xAxis);

    // 0 values
    goodWS->dataY(0)[0] = goodWS->dataY(0)[2] = goodWS->dataY(0)[4] = 0;
    goodWS->dataY(1)[1] = goodWS->dataY(1)[3] = 0;
    goodWS->dataY(2)[0] = goodWS->dataY(2)[2] = goodWS->dataY(2)[4] = 0;
    goodWS->dataY(3)[1] = goodWS->dataY(3)[3] = 0;
    goodWS->dataY(4)[0] = goodWS->dataY(4)[2] = goodWS->dataY(4)[4] = 0;

    // 2 values
    goodWS->dataY(0)[1] = goodWS->dataY(1)[0] = goodWS->dataY(1)[2] = 2;
    // 1 values
    goodWS->dataY(0)[3] = goodWS->dataY(1)[4] = goodWS->dataY(2)[3] = 1;
    // 3 values
    goodWS->dataY(2)[1] = goodWS->dataY(3)[0] = goodWS->dataY(4)[1] = 3;
    // 4 values
    goodWS->dataY(3)[2] = goodWS->dataY(3)[4] = goodWS->dataY(4)[3] = 4;

    return goodWS;
  }

  void configure_ring_profile(RingProfile &alg, MatrixWorkspace_sptr inws,
                              std::vector<double> centre, int num_bins,
                              double start_angle = 0, double min_radius = 0,
                              double max_radius = 1000, bool anticlock = true) {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    std::string outWSName("RingProfileTest_OutputWS");
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));

    // set centre
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Centre", centre));

    // set NumBins
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumBins", num_bins));

    // set start angle = -45
    if (start_angle < -0.00001 || start_angle > 0.00001)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartAngle", start_angle));

    // set ring to get just the first group
    if (min_radius > 0.00001)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinRadius", min_radius));
    if (max_radius < 999)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxRadius", max_radius));
    if (!anticlock)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("Sense", "ClockWise"));
  }

  static MatrixWorkspace_sptr basic_checkup_on_output_workspace(Algorithm &alg,
                                                                int num_bins) {
    MatrixWorkspace_sptr outws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            alg.getPropertyValue("OutputWorkspace"));
    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outws->readY(0).size(), num_bins);
    TS_ASSERT_EQUALS(outws->readX(0).size(), num_bins + 1);
    return outws;
  }

  void test__profile_of_2d_workspace_startangle_minus45_anticlock() {

    MatrixWorkspace_sptr goodWS = create_2d_workspace();
    RingProfile alg;

    // selecting 4 bins from -45 degrees, in anti-clockwise sense,
    // and for the ring that will get only the numbers 1, 4, 3, 2
    // set start angle = -45
    // set NumBins = 4
    configure_ring_profile(alg, goodWS, std::vector<double>(2, 0.0), 4, -45.0,
                           0.115, 0.13, true);

    // execute the algorithm
    TS_ASSERT_EQUALS(alg.execute(), true);

    // check the result
    MatrixWorkspace_sptr outputWS = basic_checkup_on_output_workspace(alg, 4);

    // specific checks:

    // check angles:
    for (int i = 0; i < 5; i++)
      TS_ASSERT_DELTA(outputWS->readX(0)[i], 90 * i, 0.1);

    // check that Y = [1, 4, 3, 2]
    TS_ASSERT_DELTA(outputWS->readY(0)[0], 1, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[1], 4, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[2], 3, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[3], 2, 0.1);
  }

  void test__profile_of_2d_workspace_startangle_45_anticlock() {

    MatrixWorkspace_sptr goodWS = create_2d_workspace();
    RingProfile alg;

    // selecting 4 bins from 45 degrees, in anti-clockwise sense,
    // and for the ring that will get only the numbers 1, 4, 3, 2
    // set start angle = 45
    // set NumBins = 4
    configure_ring_profile(alg, goodWS, std::vector<double>(2, 0.0), 4, 45.0,
                           0.115, 0.13, true);

    // execute the algorithm
    TS_ASSERT_EQUALS(alg.execute(), true);

    // check the result
    MatrixWorkspace_sptr outputWS = basic_checkup_on_output_workspace(alg, 4);

    // specific checks:

    // check angles:
    for (int i = 0; i < 5; i++)
      TS_ASSERT_DELTA(outputWS->readX(0)[i], 90 * i, 0.1);

    // check that Y = [4, 3, 2, 1]
    for (int i = 0; i < 4; i++)
      TS_ASSERT_DELTA(outputWS->readY(0)[i], 4 - i, 0.1);
  }

  void test__profile_of_2d_workspace_startangle_45_clock() {
    MatrixWorkspace_sptr goodWS = create_2d_workspace();
    RingProfile alg;

    // selecting 4 bins from 45 degres in clockwise sense
    // and for the ring that will get only the numbers 1,2,3,4

    configure_ring_profile(alg, goodWS, std::vector<double>(2, 0.0), 4, 45.0,
                           0.115, 0.13, false);

    TS_ASSERT_EQUALS(alg.execute(), true);
    // check the result
    MatrixWorkspace_sptr outputWS = basic_checkup_on_output_workspace(alg, 4);

    // now, the result expected is [1, 2, 3, 4]
    for (int i = 0; i < 4; i++)
      TS_ASSERT_DELTA(outputWS->readY(0)[i], i + 1, 0.1);
  }

  void test__profile_of_2d_workspace_bigger_ring_and_more_bins() {
    MatrixWorkspace_sptr goodWS = create_2d_workspace();
    RingProfile alg;
    int num_bins = 4;
    std::vector<double> centre(2);
    centre[0] = 0.18;
    centre[1] = 0.18;
    // selecting 10 bins;  centre 0,0 ; start_angle = -10; low_ring = 0;
    // high_ring=10; anti-clock;
    configure_ring_profile(alg, goodWS, centre, num_bins, 0, 0.05, 0.12, true);

    TS_ASSERT_EQUALS(alg.execute(), true);
    // check the result
    MatrixWorkspace_sptr outputWS =
        basic_checkup_on_output_workspace(alg, num_bins);

    // the expected results
    const double exp_angles[] = {0, 90, 180, 270, 360};
    const double exp_results[] = {0, 4, 0, 4};

    for (int i = 0; i < num_bins; i++)
      TS_ASSERT_DELTA(outputWS->readY(0)[i], exp_results[i], 0.1);
    for (int i = 0; i < num_bins + 1; i++)
      TS_ASSERT_DELTA(outputWS->readX(0)[i], exp_angles[i], 0.1);
  }

  void test__profile_of_2d_workspace_bigger_different_centre() {
    MatrixWorkspace_sptr goodWS = create_2d_workspace();
    RingProfile alg;
    int num_bins = 10;
    // selecting 10 bins;  centre 0,0 ; start_angle = -10; low_ring = 0;
    // high_ring=10; anti-clock;
    configure_ring_profile(alg, goodWS, std::vector<double>(2, 0.0), num_bins,
                           -10.0, 0, 10.0, true);

    TS_ASSERT_EQUALS(alg.execute(), true);
    // check the result
    MatrixWorkspace_sptr outputWS =
        basic_checkup_on_output_workspace(alg, num_bins);
  }

  /**
     This will create the following input 2D workspace with rectangular
     instrument:

       ------------> Y
       | 0 2 0 1 0
       | 2 0 2 0 1
       | 0 3 0 1 0
       | 3 0 4 0 4
       | 0 3 0 4 0
      \/
       X

       With the positions in X = [ 0, 0.008, 0.016, 0.024, 0.032]
                             Y = [ 0, 0.008, 0.016, 0.024, 0.032]

       To be easy to have a visual interpretation I will put the Y axis growing
     towards up:
       Y
       | 0 1 0 4 0
       | 1 0 1 0 4
       | 0 2 0 4 0
       | 2 0 3 0 3
       | 0 2 0 3 0
       ------------>X
     */
  static MatrixWorkspace_sptr create_rectangular_instrument_workspace() {
    MatrixWorkspace_sptr goodWS =
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            1, 5, 1);

    // 0 values
    goodWS->dataY(0)[0] = goodWS->dataY(2)[0] = goodWS->dataY(4)[0] = 0;
    goodWS->dataY(6)[0] = goodWS->dataY(8)[0] = 0;
    goodWS->dataY(10)[0] = goodWS->dataY(12)[0] = goodWS->dataY(14)[0] = 0;
    goodWS->dataY(16)[0] = goodWS->dataY(18)[0] = 0;
    goodWS->dataY(20)[0] = goodWS->dataY(22)[0] = goodWS->dataY(24)[0] = 0;

    // 2 values
    goodWS->dataY(1)[0] = goodWS->dataY(5)[0] = goodWS->dataY(7)[0] = 2;
    // 1 values
    goodWS->dataY(3)[0] = goodWS->dataY(9)[0] = goodWS->dataY(13)[0] = 1;
    // 3 values
    goodWS->dataY(11)[0] = goodWS->dataY(15)[0] = goodWS->dataY(21)[0] = 3;
    // 4 values
    goodWS->dataY(17)[0] = goodWS->dataY(19)[0] = goodWS->dataY(23)[0] = 4;

    return goodWS;
  }

  void test__profile_of_rectangular_startangle_minus45_anticlock() {
    MatrixWorkspace_sptr goodWS = create_rectangular_instrument_workspace();
    RingProfile alg;
    // put the centre in the midle of the panel, but in the origin in Z
    // (X=0.016, Y= 0.016, Z=0.0)
    std::vector<double> centre(3, 0.016);
    centre[2] = 0.0;
    // selecting 4 bins from -45 degrees in anti-clock sense
    // select the minRadius and maxRadius (the thetha limits) small to get only
    // the values 1,2,3,4
    configure_ring_profile(alg, goodWS, centre, 4, -45, 0.0078, 0.009, true);

    TS_ASSERT_EQUALS(alg.execute(), true);

    // check the result
    MatrixWorkspace_sptr outputWS = basic_checkup_on_output_workspace(alg, 4);
    // specific checks:
    // check that Y = [4, 1, 2, 3]
    TS_ASSERT_DELTA(outputWS->readY(0)[0], 4, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[1], 1, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[2], 2, 0.1);
    TS_ASSERT_DELTA(outputWS->readY(0)[3], 3, 0.1);
  }

  void test__profile_of_rectangular_startangle_45_anticlock() {
    MatrixWorkspace_sptr goodWS = create_rectangular_instrument_workspace();
    RingProfile alg;
    // put the centre in the midle of the panel, but in the origin in Z
    // (X=0.016, Y= 0.016, Z=0.0)
    std::vector<double> centre(3, 0.016);
    centre[2] = 0.0;
    // selecting 4 bins from 45 degrees in anti-clock sense
    // select the minRadius and maxRadius (the thetha limits) small to get only
    // the values 1,2,3,4
    configure_ring_profile(alg, goodWS, centre, 4, 45, 0.0078, 0.009, true);

    TS_ASSERT_EQUALS(alg.execute(), true);

    // check the result
    MatrixWorkspace_sptr outputWS = basic_checkup_on_output_workspace(alg, 4);

    for (int i = 0; i < 4; i++)
      TS_ASSERT_DELTA(outputWS->readY(0)[i], i + 1, 0.1);
  }

  void test__profile_of_rectangular_startangle_45_clock() {
    MatrixWorkspace_sptr goodWS = create_rectangular_instrument_workspace();
    RingProfile alg;
    // put the centre in the midle of the panel, but in the origin in Z
    // (X=0.016, Y= 0.016, Z=0.0)
    std::vector<double> centre(3, 0.016);
    centre[2] = 0.0;
    // selecting 4 bins from 45 degrees in anti-clock sense
    // select the minRadius and maxRadius (the thetha limits) small to get only
    // the values 1,2,3,4
    configure_ring_profile(alg, goodWS, centre, 4, 45, 0.0078, 0.009, false);

    TS_ASSERT_EQUALS(alg.execute(), true);

    // check the result
    MatrixWorkspace_sptr outputWS = basic_checkup_on_output_workspace(alg, 4);
    // 4, 3, 2, 1
    for (int i = 0; i < 4; i++)
      TS_ASSERT_DELTA(outputWS->readY(0)[i], 4 - i, 0.1);
  }

  void test__profile_of_rectangular_bigger_ring_and_more_bins() {
    MatrixWorkspace_sptr goodWS = create_rectangular_instrument_workspace();
    RingProfile alg;
    // put the centre in the midle of the panel, but in the origin in Z
    // (X=0.016, Y= 0.016, Z=0.0)
    std::vector<double> centre(3, 0.016);
    centre[2] = 0.0;
    // selecting 10 bins from 0 degrees in anti-clock sense
    configure_ring_profile(alg, goodWS, centre, 10, 0, 0, 10, true);

    TS_ASSERT_EQUALS(alg.execute(), true);

    // check the result
    MatrixWorkspace_sptr outputWS = basic_checkup_on_output_workspace(alg, 10);
    const double exp_results[] = {8, 4, 1, 1, 1, 4, 2, 3, 3, 3};
    // angles: 0, 36, 72, 108, 144, 180, 216, 252, 288, 324, 360
    for (int i = 0; i < 10; i++)
      TS_ASSERT_DELTA(outputWS->readY(0)[i], exp_results[i], 0.1);
  }
};

#endif /* MANTID_ALGORITHMS_RINGPROFILETEST_H_ */
