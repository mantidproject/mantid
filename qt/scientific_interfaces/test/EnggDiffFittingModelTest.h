#ifndef MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGMODELTEST_H_
#define MANTID_CUSTOMINTERFACES_ENGGDIFFFITTINGMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "../EnggDiffraction/EnggDiffFittingModel.h"

#include <vector>

using namespace Mantid;
using namespace MantidQT::CustomInterfaces;

class EnggDiffFittingModelTest : public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static EnggDiffFittingModelTest *createSuite() { return new EnggDiffFittingModelTest(); }
	static void destroySuite(EnggDiffFittingModelTest *suite) { delete suite; }

	void test_addAndGetWorkspace() {
		auto model = EnggDiffFittingModel();
		API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().
			create("Workspace2D", 1, 10, 10);
		const int runNumber = 100;
		const int bank = 1;
		TS_ASSERT_THROWS_NOTHING(model.addWorkspace(runNumber, bank, ws));
		const auto retrievedWS = model.getWorkspace(runNumber, bank);

		TS_ASSERT(retrievedWS != nullptr);
		TS_ASSERT_EQUALS(ws, retrievedWS);
	}

	void test_getAllRunNumbers() {
		auto model = EnggDiffFittingModel();
		
		API::MatrixWorkspace_sptr ws1 = API::WorkspaceFactory::Instance().
			create("Workspace2D", 1, 10, 10);
		model.addWorkspace(123, 1, ws1);

		API::MatrixWorkspace_sptr ws2 = API::WorkspaceFactory::Instance().
			create("Workspace2D", 1, 10, 10);
		model.addWorkspace(456, 2, ws2);

		API::MatrixWorkspace_sptr ws3 = API::WorkspaceFactory::Instance().
			create("Workspace2D", 1, 10, 10);
		model.addWorkspace(789, 1, ws3);

		API::MatrixWorkspace_sptr ws4 = API::WorkspaceFactory::Instance().
			create("Workspace2D", 1, 10, 10);
		model.addWorkspace(123, 2, ws1);

		const auto runNumbers = model.getAllRunNumbers();

		TS_ASSERT_EQUALS(runNumbers.size(), 3);
		TS_ASSERT_EQUALS(runNumbers[0], 123);
		TS_ASSERT_EQUALS(runNumbers[1], 456);
		TS_ASSERT_EQUALS(runNumbers[2], 789);
	}

};

#endif