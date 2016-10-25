#ifndef MANTID_ALGORITHMS_CONVERTTOCONSTANTL2TEST_H_
#define MANTID_ALGORITHMS_CONVERTTOCONSTANTL2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ConvertToConstantL2.h"
#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid;
using namespace Kernel;

using Mantid::Algorithms::ConvertToConstantL2;

class ConvertToConstantL2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToConstantL2Test *createSuite() {
    return new ConvertToConstantL2Test();
  }
  static void destroySuite(ConvertToConstantL2Test *suite) { delete suite; }

  ConvertToConstantL2Test() : m_l2(4) {}

  void testTheBasics() {
    ConvertToConstantL2 c;
    TS_ASSERT_EQUALS(c.name(), "ConvertToConstantL2");
    TS_ASSERT_EQUALS(c.version(), 1);
  }

  void testExec() {
    std::string inputWSName("test_input_ws");
    std::string outputWSName("test_output_ws");

    int numberOfAngles = 5;
    int numberOfBins = 10;

    auto inputWS =
        createTestWorkspace(numberOfAngles, numberOfBins, inputWSName);

    // BEFORE - check the L2 values differ
    for (int i = 0; i < numberOfAngles; i++) {
      Mantid::Geometry::IDetector_const_sptr det = inputWS->getDetector(i);
      double r, theta, phi;
      Mantid::Kernel::V3D pos = det->getPos();
      pos.getSpherical(r, theta, phi);
      TS_ASSERT_DIFFERS(r, m_l2)
    }

    ConvertToConstantL2 c;
    if (!c.isInitialized())
      c.initialize();

    c.setPropertyValue("InputWorkspace", inputWSName);
    c.setPropertyValue("OutputWorkspace", outputWSName);
    c.execute();
    TS_ASSERT(c.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(outputWSName);

    // AFTER - test the first tube to see if distance was well corrected to l2
    for (int i = 0; i < numberOfAngles; i++) {
      Mantid::Geometry::IDetector_const_sptr det = outputWS->getDetector(i);
      double r, theta, phi;
      Mantid::Kernel::V3D pos = det->getPos();
      pos.getSpherical(r, theta, phi);
      // Corrected distance to l2!
      // TS_ASSERT_EQUALS(r, m_l2)
      TS_ASSERT_DELTA(r, m_l2, 0.001)
    }

    // Check the contents of the y and e parts of the histogram are the same,
    // and x differs
    for (int i = 0; i < numberOfAngles; i++) {
      TS_ASSERT_DIFFERS(outputWS->x(i).rawData(), inputWS->x(i).rawData());
      TS_ASSERT_EQUALS(outputWS->y(i).rawData(), inputWS->y(i).rawData());
      TS_ASSERT_EQUALS(outputWS->e(i).rawData(), inputWS->e(i).rawData());
    }

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(inputWSName);
  }

private:
  int m_l2;

  MatrixWorkspace_sptr createTestWorkspace(int numberOfAngles, int numberOfBins,
                                           std::string inputWSName) {
    std::vector<double> L2(numberOfAngles, 5);
    std::vector<double> polar(numberOfAngles, (30. / 180.) * M_PI);
    polar[0] = 0;
    std::vector<double> azimutal(numberOfAngles, 0);
    azimutal[1] = (45. / 180.) * M_PI;
    azimutal[2] = (90. / 180.) * M_PI;
    azimutal[3] = (135. / 180.) * M_PI;
    azimutal[4] = (180. / 180.) * M_PI;

    Mantid::API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createProcessedInelasticWS(
            L2, polar, azimutal, numberOfBins, -1, 3, 3);

    inputWS->getAxis(0)->setUnit("TOF");
    inputWS->mutableRun().addProperty(
        "wavelength", boost::lexical_cast<std::string>(numberOfAngles));

    inputWS->instrumentParameters().addString(
        inputWS->getInstrument()->getComponentID(), "l2",
        boost::lexical_cast<std::string>(m_l2));

    API::AnalysisDataService::Instance().addOrReplace(inputWSName, inputWS);

    return inputWS;
  }
};

#endif /* MANTID_ALGORITHMS_CONVERTTOCONSTANTL2TEST_H_ */
