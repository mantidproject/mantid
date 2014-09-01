#ifndef MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_
#define MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CalculateMSVesuvio.h"

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

  void test_exec_with_flat_plate_sample()
  {
    auto alg = createTestAlgorithm(createFlatPlateSampleWS());

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

  void test_input_workspace_and_sampleshape_not_cuboid_or_cylinder_throws_invalid_argument_on_execution()
  {
    auto testWS = createFlatPlateSampleWS();
    auto sampleShape = ComponentCreationHelper::createSphere(1, Mantid::Kernel::V3D(),
                                                             "sample-shape");
    testWS->mutableSample().setShape(*sampleShape); // overwrite shape
    auto alg = createTestAlgorithm(testWS);

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
    const double height(0.1), width(0.02), thick(0.005);
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
