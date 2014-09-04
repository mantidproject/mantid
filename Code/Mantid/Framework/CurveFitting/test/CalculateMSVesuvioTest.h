#ifndef MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_
#define MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CalculateMSVesuvio.h"
#include "MantidGeometry/Instrument/Goniometer.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "ComptonProfileTestHelpers.h"

using Mantid::CurveFitting::CalculateMSVesuvio;

class CalculateMSVesuvioTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateMSVesuvioTest *createSuite() { return new CalculateMSVesuvioTest(); }
  static void destroySuite( CalculateMSVesuvioTest *suite ) { delete suite; }

  // ------------------------ Success Cases -----------------------------------------

  void test_init()
  {
    CalculateMSVesuvio alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec_with_flat_plate_sample_and_no_goniometer()
  {
    auto alg = createTestAlgorithm(createFlatPlateSampleWS());

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_exec_with_flat_plate_sample_and_goniometer()
  {
    auto testWS = createFlatPlateSampleWS();
    Mantid::Geometry::Goniometer sampleRot;
    // 45.0 deg rotation around Y
    sampleRot.pushAxis("phi", 0.0, 1.0, 0.0, 45.0,
                       Mantid::Geometry::CW, Mantid::Geometry::angDegrees);
    testWS->mutableRun().setGoniometer(sampleRot, false);
    auto alg = createTestAlgorithm(testWS);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  // ------------------------ Failure Cases -----------------------------------------

  void test_setting_input_workspace_not_in_tof_throws_invalid_argument()
  {
    CalculateMSVesuvio alg;
    alg.initialize();

    auto testWS = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", testWS), std::invalid_argument);
  }

  void test_setting_workspace_with_no_sample_shape_throws_invalid_argument()
  {
    CalculateMSVesuvio alg;
    alg.initialize();

    auto testWS = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    testWS->getAxis(0)->setUnit("TOF");
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", testWS), std::invalid_argument);
  }

  void test_setting_nmasses_zero_or_negative_throws_invalid_argument()
  {
    CalculateMSVesuvio alg;
    alg.initialize();

    TS_ASSERT_THROWS(alg.setProperty("NoOfMasses", -1), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setProperty("NoOfMasses", 0), std::invalid_argument);
  }

  void test_setting_sampledensity_zero_or_negative_throws_invalid_argument()
  {
    CalculateMSVesuvio alg;
    alg.initialize();

    TS_ASSERT_THROWS(alg.setProperty("SampleDensity", -1), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setProperty("SampleDensity", 0), std::invalid_argument);
  }


  void test_setting_atomic_properties_not_length_three_times_nmasses_throws_invalid_argument_on_execute()
  {
    auto alg = createTestAlgorithm(createFlatPlateSampleWS());

    alg->setProperty("NoOfMasses", 2);
    const double sampleProps[5] = {1.007900, 0.9272392, 5.003738, 16.00000, 3.2587662E-02};
    alg->setProperty("AtomicProperties", std::vector<double>(sampleProps, sampleProps + 5));

    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

  void test_setting_zero_or_negative_beam_radius_values_throws_invalid_argument()
  {
    CalculateMSVesuvio alg;
    alg.initialize();

    TS_ASSERT_THROWS(alg.setProperty("BeamUmbraRadius", -1.5), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setProperty("BeamUmbraRadius", 0.0), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setProperty("BeamPenumbraRadius", -1.5), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setProperty("BeamPenumbraRadius", 0.0), std::invalid_argument);
  }

  void test_setting_umbra_less_than_penumbra_throws_invalid_argument()
  {
    auto testWS = createFlatPlateSampleWS();
    auto alg = createTestAlgorithm(testWS);

    alg->setProperty("BeamUmbraRadius", 2.5);
    alg->setProperty("BeamPenumbraRadius", 1.5);

    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }

private:

  Mantid::API::IAlgorithm_sptr createTestAlgorithm(const Mantid::API::MatrixWorkspace_sptr & inputWS)
  {
    Mantid::API::IAlgorithm_sptr alg = boost::shared_ptr<Mantid::API::IAlgorithm>(new CalculateMSVesuvio);
    alg->initialize();
    alg->setRethrows(true);
    alg->setChild(true);
    // inputs
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("NoOfMasses", 3);
    alg->setProperty("SampleDensity", 241.0);
    const double sampleProps[9] = {1.007900, 0.9272392, 5.003738, 16.00000, 3.2587662E-02, 13.92299, 27.50000, 4.0172841E-02, 15.07701};
    alg->setProperty("AtomicProperties", std::vector<double>(sampleProps, sampleProps + 9));
    alg->setProperty("BeamUmbraRadius", 1.5);
    alg->setProperty("BeamPenumbraRadius", 2.5);
    // outputs
    alg->setPropertyValue("TotalScatteringWS", "__unused_for_child");
    alg->setPropertyValue("MultipleScatteringWS", "__unused_for_child");

    return alg;
  }

  struct ones
  {
    double operator()(const double, size_t) { return 1.0; } // don't care about Y values, just use 1.0 everywhere
  };

  Mantid::API::MatrixWorkspace_sptr createFlatPlateSampleWS()
  {
    auto testWS = createTestWorkspace();
    // Sample shape
    const double height(0.05), width(0.01), thick(0.0025);
    auto sampleShape = ComponentCreationHelper::createCuboid(width, height, thick);
    testWS->mutableSample().setShape(*sampleShape);

    return testWS;
  }


  Mantid::API::MatrixWorkspace_sptr createTestWorkspace()
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    const int nhist(1);
    const double x0(50.0), x1(562.0), dx(1.0);
    const bool singleMassSpec(false), foilChanger(false);
    auto ws2d = ComptonProfileTestHelpers::createTestWorkspace(nhist, x0, x1, dx, singleMassSpec, foilChanger);

    return ws2d;
  }

};


#endif /* MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_ */
