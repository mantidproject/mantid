#ifndef MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_
#define MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CorrectFlightPaths.h"
#include "MantidDataHandling/LoadILL.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"


using namespace Mantid::API;
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
			m_testFile("ILLIN5_104007.nxs"),
			m_l2(4){
	}

	void testTheBasics() {
		CorrectFlightPaths c;
		TS_ASSERT_EQUALS( c.name(), "CorrectFlightPaths");
		TS_ASSERT_EQUALS( c.version(), 1);
	}

	void testExec() {
		std::string inputWSName("test_input_ws");
		std::string outputWSName("test_output_ws");

		// Start by loading our NXS file
		Mantid::API::IAlgorithm* loader = Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadILL");
		loader->setPropertyValue("Filename", m_testFile);
		loader->setPropertyValue("OutputWorkspace", inputWSName);
		loader->execute();
		TS_ASSERT( loader->isExecuted());

		CorrectFlightPaths c;
		if (!c.isInitialized())
			c.initialize();

		c.setPropertyValue("InputWorkspace", inputWSName);
		c.setPropertyValue("OutputWorkspace", outputWSName);
		c.execute();
		TS_ASSERT( c.isExecuted());

		Mantid::API::MatrixWorkspace_const_sptr output;
		output = Mantid::API::AnalysisDataService::Instance().retrieveWS <Mantid::API::MatrixWorkspace> (outputWSName);

		// test the first tube to see if distance was well corrected to l2
		for (int i = 0; i < 128; i++) {
			Mantid::Geometry::IDetector_const_sptr det = output->getDetector(1);
			double r, theta, phi;
			Mantid::Kernel::V3D pos = det->getPos();
			pos.getSpherical(r, theta, phi);
			// Corrected distance to 4!
			TS_ASSERT_EQUALS(r, m_l2)
		}

		AnalysisDataService::Instance().remove(outputWSName);
		AnalysisDataService::Instance().remove(inputWSName);
	}

private:
	std::string m_testFile;
	int m_l2;

};

#endif /* MANTID_ALGORITHMS_CORRECTFLIGHTPATHSTEST_H_ */
