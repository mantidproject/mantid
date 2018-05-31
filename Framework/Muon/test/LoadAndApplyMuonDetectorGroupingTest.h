#ifndef MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPINGTEST_H_
#define MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/GroupingLoader.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidMuon/ApplyMuonDetectorGrouping.h"
#include "MantidMuon/LoadAndApplyMuonDetectorGrouping.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using Mantid::DataHandling::LoadMuonNexus2;

class LoadAndApplyMuonDetectorGroupingTest : public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static LoadAndApplyMuonDetectorGroupingTest *createSuite() {
		return new LoadAndApplyMuonDetectorGroupingTest();
	};
	static void destroySuite(LoadAndApplyMuonDetectorGroupingTest *suite) {
		delete suite;
	};

	void test_init() {
		LoadAndApplyMuonDetectorGroupingTest alg;
		TS_ASSERT_THROWS_NOTHING(alg.initialize());
		TS_ASSERT(alg.isInitialized());
	};

	void test_workspaceProduced() {
		// Simple XML with only one group, with only single detector
	}

	void test_workspaceOverwrite() {
		// Simple XML with only one group, with only single detector. 
		// ensure overwrites a workspace with the same name
	}

	void test_workspaceGroupDefaultName() {
		// Give no workspace group and check that the default 
		// name matches the previous code (i.e. EMUnnnn).
	}

	void test_singleGroupDoesntChangeData() {
		// Simple XML with only one group, with only single detector.
		// Ensure workspace is unaltered from the input data.
	}

	void test_groupingWithCounts() {
		// 
	}

	void test_fileWithGroupingOnly() {
		// XML file with at least three groups. Check number of workspaces.
	}

	void test_fileWithPairing() {
		// XML file with at least three groups and three pairs. Check number of workspaces.
	}

	void test_fileWithMisnamedGroup() {
		// Deliberately reference a group by the wrong name. check throws error.
	}

	void test_fileWithTooManyDetectors() {
		// Ensure error thrown if number of detectors exceeds what was in the input ws.
	}

};

#endif /* MANTID_MUON_LOADANDAPPLYMUONDETECTORGROUPING_H_ */