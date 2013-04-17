#ifndef MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_
#define MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using Mantid::Algorithms::DetectorEfficiencyCorUser;

using namespace Mantid;
using namespace Mantid::API;
using namespace Kernel;

class DetectorEfficiencyCorUserTest: public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static DetectorEfficiencyCorUserTest *createSuite() {
		return new DetectorEfficiencyCorUserTest();
	}
	static void destroySuite(DetectorEfficiencyCorUserTest *suite) {
		delete suite;
	}

	// contructor
	DetectorEfficiencyCorUserTest() :
			m_inWSName("input_workspace"), m_outWSName("output_workspace") {
		m_Ei = 3.27;

		createInputWorkSpace();

	}

	void test_Init() {
		DetectorEfficiencyCorUser alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
	}

	void test_exec() {

		DetectorEfficiencyCorUser alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize())
		TS_ASSERT(alg.isInitialized())
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("InputWorkspace", m_inWSName));
		TS_ASSERT_THROWS_NOTHING(
				alg.setPropertyValue("OutputWorkspace", m_outWSName));
		TS_ASSERT_THROWS_NOTHING(alg.execute()
		; );
		TS_ASSERT(alg.isExecuted());

		// Retrieve the output workspace from data service.
		MatrixWorkspace_sptr outWS;
		TS_ASSERT_THROWS_NOTHING(
				outWS = AnalysisDataService::Instance().retrieveWS<
						MatrixWorkspace>(m_outWSName));
		TS_ASSERT(outWS);
		if (!outWS)
			return;

		// Retrieve the output workspace from data service.
		MatrixWorkspace_sptr inWS;
		TS_ASSERT_THROWS_NOTHING(
				inWS = AnalysisDataService::Instance().retrieveWS<
						MatrixWorkspace>(m_inWSName));
		TS_ASSERT(inWS);
		if (!inWS)
			return;


		TS_ASSERT_DELTA(outWS->readY(0).front(), inWS->readY(0).front(), 0.3);

		// Remove workspace from the data service.
		AnalysisDataService::Instance().remove(m_outWSName);
	}

private:
	double m_Ei;
	const std::string m_inWSName, m_outWSName;

	void createInputWorkSpace() {
		//createWorkspace2D();
		DataObjects::Workspace2D_sptr dataws =
				WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(2, 10, 20);
		dataws->getAxis(0)->setUnit("Energy");

		dataws->mutableRun().addProperty("Ei",
				boost::lexical_cast<std::string>(m_Ei));
		dataws->instrumentParameters().addString(
				dataws->getInstrument()->getChild(0).get(), "formula_eff0",
				"exp(-0.0565/sqrt(e0))*(1.-exp(-3.284/sqrt(e0)))");
		dataws->instrumentParameters().addString(
				dataws->getInstrument()->getChild(0).get(), "formula_eff",
				"1.0/eff0*exp(-0.0565/sqrt(e))*(1.0-exp(-3.284/sqrt(e)))");

		API::AnalysisDataService::Instance().addOrReplace(m_inWSName, dataws);

	}

};

#endif /* MANTID_ALGORITHMS_DETECTOREFFICIENCYCORUSERTEST_H_ */
