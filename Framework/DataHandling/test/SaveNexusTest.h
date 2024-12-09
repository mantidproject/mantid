// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <fstream>

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataHandling/SaveNexus.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveNexusTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExecOnMuon() {
    Mantid::DataHandling::LoadNexus nxLoad;
    std::string outputSpace, inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("Filename", inputFile);
    outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // get workspace
    //
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    //
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    // specify parameters to algorithm
    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    outputFile = "testOfSaveNexus.nxs";
    algToBeTested.setPropertyValue("Filename", outputFile);
    outputFile = algToBeTested.getPropertyValue("Filename");
    title = "Testing SaveNexus with Muon data";
    algToBeTested.setPropertyValue("Title", title);
    // comment line below to check the contents of the o/p file manually
    remove(outputFile.c_str());

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("Filename"));
    TS_ASSERT(!result.compare(outputFile));
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("Title"));
    TS_ASSERT(!result.compare(title));
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("InputWorkspace"));
    TS_ASSERT(!result.compare(outputSpace));

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // test writing two entries to one nexus file
    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    remove(outputFile.c_str());
  }

  void test_pass_inputworkspace_as_pointer() {
    Workspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace123(2, 5);

    SaveNexus alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("Filename", "out.nxs");

    std::string outputFile = alg.getPropertyValue("Filename");

    const bool executed = alg.execute();
    TSM_ASSERT("SaveNexus did not execute successfully", executed)
    if (executed)
      Poco::File(outputFile).remove();
  }

private:
  SaveNexus algToBeTested;
  std::string outputFile;
  std::string title;
};
