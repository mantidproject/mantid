#ifndef MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_
#define MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_

#include <cxxtest/TestSuite.h>

#include "boost/version.hpp"

#include "MantidCurveFitting/CalculateMSVesuvio.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

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
// seed has different effect in boost version 1.56 and newer
#if (BOOST_VERSION / 100000) == 1 && (BOOST_VERSION / 100 % 1000) >= 56
    checkOutputValuesAsExpected(alg, 0.0111204555, 0.0019484356);
#else
    checkOutputValuesAsExpected(alg, 0.0099824991, 0.0020558473);
#endif
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

    TS_ASSERT_THROWS(alg.setProperty("BeamRadius", -1.5), std::invalid_argument);
    TS_ASSERT_THROWS(alg.setProperty("BeamRadius", 0.0), std::invalid_argument);
  }

  void test_input_workspace_with_detector_that_has_no_shape_throws_exception()
  {
    auto alg = createTestAlgorithm(createFlatPlateSampleWS(false));

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
    const double sampleProps[9] = {1.007900, 0.9272392, 5.003738, 16.00000, 3.2587662E-02, 13.92299,
                                   27.50000, 4.0172841E-02, 15.07701};
    alg->setProperty("AtomicProperties", std::vector<double>(sampleProps, sampleProps + 9));
    alg->setProperty("BeamRadius", 2.5);
    // reduce number of events for test purposes
    alg->setProperty("NumEventsPerRun", 10000);

    // outputs
    alg->setPropertyValue("TotalScatteringWS", "__unused_for_child");
    alg->setPropertyValue("MultipleScatteringWS", "__unused_for_child");

    return alg;
  }

  Mantid::API::MatrixWorkspace_sptr createFlatPlateSampleWS(const bool detShape = true)
  {
    auto testWS = createTestWorkspace(detShape);
    // Sample shape
    const double halfHeight(0.05), halfWidth(0.05), halfThick(0.0025);
    std::ostringstream sampleShapeXML;
    sampleShapeXML <<  " <cuboid id=\"detector-shape\"> "
      << "<left-front-bottom-point x=\"" << halfWidth << "\" y=\"" << -halfHeight << "\" z=\"" << -halfThick <<"\"  /> "
      << "<left-front-top-point  x=\"" << halfWidth <<"\" y=\"" << halfHeight <<"\" z=\"" << -halfThick << "\"  /> "
      << "<left-back-bottom-point  x=\"" << halfWidth <<"\" y=\"" << -halfHeight << "\" z=\"" << halfThick <<"\"  /> "
      << "<right-front-bottom-point  x=\"" << -halfWidth <<"\" y=\"" << -halfHeight << "\" z=\"" << -halfThick << "\"  /> "
      << "</cuboid>";
    auto sampleShape = Mantid::Geometry::ShapeFactory().createShape(sampleShapeXML.str());
    testWS->mutableSample().setShape(*sampleShape);

    return testWS;
  }


  Mantid::API::MatrixWorkspace_sptr createTestWorkspace(const bool detShape = true)
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    const int nhist(1);
    const double x0(50.0), x1(562.0), dx(1.0);
    const bool singleMassSpec(false), foilChanger(true);
    auto ws2d = ComptonProfileTestHelpers::createTestWorkspace(nhist, x0, x1, dx,
                                                               singleMassSpec, foilChanger);

    if(detShape)
    {
      // replace instrument with one that has a detector with a shape
      const std::string shapeXML = \
        "<cuboid id=\"shape\">"
          "<left-front-bottom-point x=\"0.0125\" y=\"-0.0395\" z= \"0.0045\" />"
          "<left-front-top-point x=\"0.0125\" y=\"0.0395\" z= \"0.0045\" />"
          "<left-back-bottom-point x=\"0.0125\" y=\"-0.0395\" z= \"-0.0045\" />"
          "<right-front-bottom-point x=\"-0.0125\" y=\"-0.0395\" z= \"0.0045\" />"
        "</cuboid>"
        "<algebra val=\"shape\" />";
      const auto pos = ws2d->getDetector(0)->getPos();
      auto instrument = ComptonProfileTestHelpers::createTestInstrumentWithFoilChanger(1, pos, shapeXML);
      ws2d->setInstrument(instrument);
      ComptonProfileTestHelpers::addResolutionParameters(ws2d, 1);
      ComptonProfileTestHelpers::addFoilResolution(ws2d, "foil-pos0");
    }

    return ws2d;
  }

  void checkOutputValuesAsExpected(const Mantid::API::IAlgorithm_sptr & alg,
                                   const double expectedTotal, const double expectedMS)
  {
    using Mantid::API::MatrixWorkspace_sptr;
    const size_t checkIdx = 100;
    const double tolerance(1e-8);
    
    // Values for total scattering
    MatrixWorkspace_sptr totScatter = alg->getProperty("TotalScatteringWS");
    TS_ASSERT(totScatter);
    const auto & totY = totScatter->readY(0);
    TS_ASSERT_DELTA(expectedTotal, totY[checkIdx], tolerance);
    const auto & totX = totScatter->readX(0);
    TS_ASSERT_DELTA(150.0, totX[checkIdx], tolerance); // based on workspace setup

    // Values for multiple scatters
    MatrixWorkspace_sptr multScatter = alg->getProperty("MultipleScatteringWS");
    TS_ASSERT(multScatter);
    const auto & msY = multScatter->readY(0);
    TS_ASSERT_DELTA(expectedMS, msY[checkIdx], tolerance);
    const auto & msX = multScatter->readX(0);
    TS_ASSERT_DELTA(150.0, msX[checkIdx], tolerance); // based on workspace setup

  }
};


#endif /* MANTID_CURVEFITTING_CALCULATEMSVESUIVIOTEST_H_ */
