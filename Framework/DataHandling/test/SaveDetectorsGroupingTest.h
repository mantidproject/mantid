// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidDataHandling/LoadDetectorsGroupingFile.h"
#include "MantidDataHandling/SaveDetectorsGrouping.h"
#include "Poco/File.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class SaveDetectorsGroupingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveDetectorsGroupingTest *createSuite() { return new SaveDetectorsGroupingTest(); }
  static void destroySuite(SaveDetectorsGroupingTest *suite) { delete suite; }

  void test_Initialize() {
    SaveDetectorsGrouping savegroup;
    savegroup.initialize();
    TS_ASSERT(savegroup.isInitialized());
  }

  void test_SaveXMLFile() {

    // 1. Get an object for SaveDetectorsGrouping
    SaveDetectorsGrouping savegroup;
    savegroup.initialize();

    // 2. Create a Workspace from an XML file
    LoadDetectorsGroupingFile load;
    load.initialize();

    TS_ASSERT(load.setProperty("InputFile", "vulcangroup.xml"));
    TS_ASSERT(load.setProperty("OutputWorkspace", "Vulcan_Group"));

    load.execute();
    TS_ASSERT(load.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws = std::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(
        API::AnalysisDataService::Instance().retrieve("Vulcan_Group"));

    // 3. Save
    savegroup.setProperty("InputWorkspace", gws);
    savegroup.setProperty("OutputFile", "grouptemp.xml");

    savegroup.execute();
    TS_ASSERT(savegroup.isExecuted());

    // Retrieve the full path to file
    std::string file1 = savegroup.getPropertyValue("OutputFile");

    // 4. Verify
    LoadDetectorsGroupingFile load2;
    load2.initialize();

    TS_ASSERT(load2.setProperty("InputFile", file1));
    TS_ASSERT(load2.setProperty("OutputWorkspace", "Vulcan_Group2"));

    load2.execute();
    TS_ASSERT(load2.isExecuted());

    DataObjects::GroupingWorkspace_sptr gws2 = std::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(
        API::AnalysisDataService::Instance().retrieve("Vulcan_Group2"));

    TS_ASSERT_DELTA(gws2->y(0)[0], 1.0, 1.0E-5);
    // TS_ASSERT_DELTA(gws2->y(3695)[0], 2.0, 1.0E-5);
    TS_ASSERT_DELTA(gws2->y(3696)[0], 2.0, 1.0E-5);
    // TS_ASSERT_DELTA(gws2->y(7000)[0], 0.0, 1.0E-5);

    // 5. Clear
    Poco::File file(file1);
    file.remove(false);

    API::AnalysisDataService::Instance().remove("Vulcan_Group");
    API::AnalysisDataService::Instance().remove("Vulcan_Group2");
  }

  void test_SaveNamingAndDescription() {
    std::string testWs = "GroupingWorkspace";

    // Load the grouping to test with
    LoadDetectorsGroupingFile load;
    load.initialize();

    TS_ASSERT(load.setProperty("InputFile", "MUSRGrouping.xml"));
    TS_ASSERT(load.setProperty("OutputWorkspace", testWs));

    load.execute();
    TS_ASSERT(load.isExecuted());

    // Save the test workspace
    SaveDetectorsGrouping save;
    save.initialize();

    TS_ASSERT(save.setProperty("InputWorkspace", testWs));
    TS_ASSERT(save.setProperty("OutputFile", "grouptemp.xml"));

    save.execute();
    TS_ASSERT(save.isExecuted());

    // Get full path of the file
    std::string testFile = save.getPropertyValue("OutputFile");

    // Remove saved workspace
    API::AnalysisDataService::Instance().remove(testWs);

    // Load it again to verify
    LoadDetectorsGroupingFile load2;
    load2.initialize();

    TS_ASSERT(load2.setProperty("InputFile", testFile));
    TS_ASSERT(load2.setProperty("OutputWorkspace", testWs));

    load2.execute();
    TS_ASSERT(load2.isExecuted());

    // Get GroupingWorkspace instance
    auto gws = std::dynamic_pointer_cast<DataObjects::GroupingWorkspace>(
        API::AnalysisDataService::Instance().retrieve(testWs));

    // Check that description was saved
    TS_ASSERT_EQUALS(gws->run().getProperty("Description")->value(), "musr longitudinal (64 detectors)");

    // Check that group names were saved
    TS_ASSERT_EQUALS(gws->run().getProperty("GroupName_1")->value(), "fwd");
    TS_ASSERT_EQUALS(gws->run().getProperty("GroupName_2")->value(), "bwd");

    // Clean-up
    API::AnalysisDataService::Instance().remove(testWs);

    Poco::File file(testFile);
    file.remove();
  }

  void test_SaveUngroupedDetectors() {
    // Create a grouping workspace with a single detector in a single group
    auto alg = Mantid::API::AlgorithmFactory::Instance().create("CreateGroupingWorkspace", 1);
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InstrumentName", "IRIS");
    alg->setProperty("ComponentName", "graphite");
    alg->setProperty("CustomGroupingString", "42");
    alg->setProperty("OutputWorkspace", "unused");
    alg->execute();

    DataObjects::GroupingWorkspace_sptr output = alg->getProperty("OutputWorkspace");

    // Save the test workspace, first saving ungrouped detectors
    SaveDetectorsGrouping save;
    save.initialize();
    TS_ASSERT(save.setProperty("InputWorkspace", output));
    TS_ASSERT(save.setProperty("OutputFile", "grouptemp.xml"));
    save.execute();
    TS_ASSERT(save.isExecuted());

    // Get full path of the file
    std::string testFile = save.getPropertyValue("OutputFile");

    // Check that both group 0 and 1 are in the file
    std::string xmlText = Kernel::Strings::loadFile(testFile);
    TS_ASSERT_DIFFERS(xmlText.find("<group ID=\"0\">"), std::string::npos)             // group 0 found
    TS_ASSERT_DIFFERS(xmlText.find("<detids>3-41,43-112</detids>"), std::string::npos) // group 0 found
    TS_ASSERT_DIFFERS(xmlText.find("<group ID=\"1\">"), std::string::npos)             // group 1 found
    TS_ASSERT_DIFFERS(xmlText.find("<detids>42</detids>"), std::string::npos)          // group 1 found

    // Now repeat but don't save ungrouped detectors
    SaveDetectorsGrouping save2;
    save2.initialize();
    TS_ASSERT(save2.setProperty("InputWorkspace", output));
    TS_ASSERT(save2.setProperty("OutputFile", "grouptemp.xml"));
    TS_ASSERT(save2.setProperty("SaveUngroupedDetectors", false));
    save2.execute();
    TS_ASSERT(save2.isExecuted());

    testFile = save2.getPropertyValue("OutputFile");

    // Check that only group 1 and not 0 is in the file
    xmlText = Kernel::Strings::loadFile(testFile);
    TS_ASSERT_EQUALS(xmlText.find("<group ID=\"0\">"), std::string::npos)             // group 0 NOT found
    TS_ASSERT_EQUALS(xmlText.find("<detids>3-41,43-112</detids>"), std::string::npos) // group 0 NOT found
    TS_ASSERT_DIFFERS(xmlText.find("<group ID=\"1\">"), std::string::npos)            // group 1 found
    TS_ASSERT_DIFFERS(xmlText.find("<detids>42</detids>"), std::string::npos)         // group 1 found

    Poco::File file(testFile);
    file.remove();
  }
};
