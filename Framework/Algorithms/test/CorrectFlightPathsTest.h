#ifndef MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_
#define MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CorrectFlightPaths.h"
#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid;
using namespace Kernel;

using Mantid::Algorithms::CorrectFlightPaths;

class CorrectFlightPathsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CorrectFlightPathsTest *createSuite() {
    return new CorrectFlightPathsTest();
  }
  static void destroySuite(CorrectFlightPathsTest *suite) { delete suite; }

  CorrectFlightPathsTest()
      : // alter here if needed
        m_l2(4) {}

  void testTheBasics() {
    CorrectFlightPaths c;
    TS_ASSERT_EQUALS(c.name(), "CorrectFlightPaths");
    TS_ASSERT_EQUALS(c.version(), 1);
  }

  void testExec() {

    std::string inputWSName("test_input_ws");
    std::string outputWSName("test_output_ws");

    std::vector<double> L2(5, 5);
    std::vector<double> polar(5, (30. / 180.) * M_PI);
    polar[0] = 0;
    std::vector<double> azimutal(5, 0);
    azimutal[1] = (45. / 180.) * M_PI;
    azimutal[2] = (90. / 180.) * M_PI;
    azimutal[3] = (135. / 180.) * M_PI;
    azimutal[4] = (180. / 180.) * M_PI;

    int numBins = 10;
    Mantid::API::MatrixWorkspace_sptr dataws =
        WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,
                                                            numBins, -1, 3, 3);

    dataws->getAxis(0)->setUnit("TOF");
    dataws->mutableRun().addProperty("wavelength",
                                     boost::lexical_cast<std::string>(5));

    dataws->instrumentParameters().addString(
        dataws->getInstrument()->getComponentID(), "l2",
        boost::lexical_cast<std::string>(m_l2));

    API::AnalysisDataService::Instance().addOrReplace(inputWSName, dataws);

    // BEFORE
    for (int i = 0; i < 5; i++) {
      Mantid::Geometry::IDetector_const_sptr det = dataws->getDetector(i);
      double r, theta, phi;
      Mantid::Kernel::V3D pos = det->getPos();
      pos.getSpherical(r, theta, phi);
      // Corrected distance to 4!
      TS_ASSERT_DIFFERS(r, m_l2)
    }

    CorrectFlightPaths c;
    if (!c.isInitialized())
      c.initialize();

    c.setPropertyValue("InputWorkspace", inputWSName);
    c.setPropertyValue("OutputWorkspace", outputWSName);
    c.execute();
    TS_ASSERT(c.isExecuted());

	// AFTER
    Mantid::API::MatrixWorkspace_const_sptr input;
	input = Mantid::API::AnalysisDataService::Instance()
                 .retrieveWS<Mantid::API::MatrixWorkspace>(inputWSName);    
	
	Mantid::API::MatrixWorkspace_const_sptr output;
    output = Mantid::API::AnalysisDataService::Instance()
                 .retrieveWS<Mantid::API::MatrixWorkspace>(outputWSName);

	// check some arbitrary values that shouldn't have been changed
	for (size_t i = 0; i < polar.size(); ++i) {
		const auto &inputY = input->y(i);
		const auto &inputE = input->e(i);
		const auto &outputY = output->y(i);
		const auto &outputE = output->e(i);

		for (size_t j = 0; j < azimutal.size(); ++j) {
			TS_ASSERT_EQUALS(inputY[j], outputY[j])
			TS_ASSERT_EQUALS(inputE[j], outputE[j])
		}
	}	

    // test the first tube to see if distance was well corrected to l2
    for (int i = 0; i < 5; i++) {
      Mantid::Geometry::IDetector_const_sptr det = output->getDetector(i);
      double r, theta, phi;
      Mantid::Kernel::V3D pos = det->getPos();
      pos.getSpherical(r, theta, phi);
      // Corrected distance to l2!
      // TS_ASSERT_EQUALS(r, m_l2)
      TS_ASSERT_DELTA(r, m_l2, 0.001)
    }

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(inputWSName);
  }

private:
  int m_l2;
};

class CorrectFlightPathsTestPerformance : public CxxTest::TestSuite {
public:
	void setUp() override {
		// generate large workspace
		generateWorkspace(3000);
	}

	void tearDown() override {
		AnalysisDataService::Instance().remove(outputWSName);
		AnalysisDataService::Instance().remove(inputWSName);
	}
	void testPerformance() {

		CorrectFlightPaths c;
		if (!c.isInitialized())
			c.initialize();

		c.setPropertyValue("InputWorkspace", inputWSName);
		c.setPropertyValue("OutputWorkspace", outputWSName);
		c.execute();
		TS_ASSERT(c.isExecuted());

	}
private:
	std::string inputWSName = "inputWS";
	std::string outputWSName = "outputWS";

	void generateWorkspace(int nHist = 50, int nBins = 50) {
		int m_l2 = 4;
		std::vector<double> L2(nHist, 5);
		std::vector<double> polar(nHist, (30. / 180.) * M_PI);
		polar[0] = 0;
		std::vector<double> azimutal(nHist, 0);
		azimutal[1] = (45. / 180.) * M_PI;
		azimutal[2] = (90. / 180.) * M_PI;
		azimutal[3] = (135. / 180.) * M_PI;
		azimutal[4] = (180. / 180.) * M_PI;

		Mantid::API::MatrixWorkspace_sptr dataws =
			WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,
				nBins, -1, 3, 3);

		dataws->getAxis(0)->setUnit("TOF");
		dataws->mutableRun().addProperty("wavelength",
			boost::lexical_cast<std::string>(5));

		dataws->instrumentParameters().addString(
			dataws->getInstrument()->getComponentID(), "l2",
			boost::lexical_cast<std::string>(m_l2));

		API::AnalysisDataService::Instance().addOrReplace(inputWSName, dataws);
	}
};

#endif /* MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_ */
