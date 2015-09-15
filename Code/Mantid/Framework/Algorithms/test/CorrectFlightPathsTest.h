#ifndef MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_
#define MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CorrectFlightPaths.h"
#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid;
using namespace Kernel;

using Mantid::Algorithms::CorrectFlightPaths;

class CorrectFlightPathsTest: public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static CorrectFlightPathsTest *createSuite() {
		return new CorrectFlightPathsTest();
	}
	static void destroySuite(CorrectFlightPathsTest *suite) {
		delete suite;
	}

	CorrectFlightPathsTest() :
			// alter here if needed
			m_l2(4) {
	}

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
		Mantid::API::MatrixWorkspace_sptr dataws = WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar,
				azimutal, numBins, -1, 3, 3);

		dataws->getAxis(0)->setUnit("TOF");
		dataws->mutableRun().addProperty("wavelength",boost::lexical_cast<std::string>(5));

		dataws->instrumentParameters().addString(dataws->getInstrument()->getComponentID(),"l2",boost::lexical_cast<std::string>(m_l2) );


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

		Mantid::API::MatrixWorkspace_const_sptr output;
		output = Mantid::API::AnalysisDataService::Instance().retrieveWS<
				Mantid::API::MatrixWorkspace>(outputWSName);

		// AFTER
		// test the first tube to see if distance was well corrected to l2
		for (int i = 0; i < 5; i++) {
			Mantid::Geometry::IDetector_const_sptr det = output->getDetector(i);
			double r, theta, phi;
			Mantid::Kernel::V3D pos = det->getPos();
			pos.getSpherical(r, theta, phi);
			// Corrected distance to l2!
			// TS_ASSERT_EQUALS(r, m_l2)
			TS_ASSERT_DELTA(r, m_l2,0.001)
		}

		AnalysisDataService::Instance().remove(outputWSName);
		AnalysisDataService::Instance().remove(inputWSName);
	}

private:
	int m_l2;

};

#endif /* MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_ */
