#ifndef SAVENXTOMOTEST_H_
#define SAVENXTOMOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveNXTomo.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

using Mantid::DataObjects::Workspace2D_sptr;

class SaveNXTomoTest : public CxxTest::TestSuite {
public:
  static SaveNXTomoTest *createSuite() { return new SaveNXTomoTest(); }
  static void destroySuite(SaveNXTomoTest *suite) { delete suite; }

  SaveNXTomoTest() {
    m_inputWS = "saveNXTomo_test";
    m_outputFile = "SaveNXTomoTestFile.nxs";
    m_axisSize = 50;
    m_saver = FrameworkManager::Instance().createAlgorithm("SaveNXTomo");
  }

  void testName() { TS_ASSERT_EQUALS(m_saver->name(), "SaveNXTomo"); }

  void testVersion() { TS_ASSERT_EQUALS(m_saver->version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(m_saver->initialize());
    TS_ASSERT(m_saver->isInitialized());
  }

  void testWriteSingleCreating(bool deleteWhenComplete = true) {
    // Test creating a new file from a single WS
    // Create a small test workspace
    Workspace_sptr input = makeWorkspaceSingle(m_inputWS);

    TS_ASSERT_THROWS_NOTHING(
        m_saver->setProperty<Workspace_sptr>("InputWorkspaces", input));
    TS_ASSERT_THROWS_NOTHING(
        m_saver->setPropertyValue("Filename", m_outputFile));
    m_outputFile = m_saver->getPropertyValue("Filename"); // get absolute path

    // Set to overwrite to ensure creation not append
    TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("OverwriteFile", true));
    TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("IncludeError", false));

    TS_ASSERT_THROWS_NOTHING(m_saver->execute());
    TS_ASSERT(m_saver->isExecuted());

    // Check file exists
    Poco::File file(m_outputFile);
    TS_ASSERT(file.exists());

    // Check that the structure of the nxTomo file is correct
    checkNXTomoStructure();

    // Check count of entries for data / run_title / rotation_angle / image_key
    checkNXTomoDimensions(1);

    // Check rotation values
    checkNXTomoRotations(1);

    // Check main data values
    checkNXTomoData(1);

    if (deleteWhenComplete) {
      if (file.exists())
        file.remove();
    }
  }

  void testWriteGroupCreating() {
    // Test creating a new file from a WS Group
    // Create small test workspaces
    std::vector<Workspace2D_sptr> wspaces(3);
    WorkspaceGroup_sptr input = makeWorkspacesInGroup(m_inputWS, wspaces);
    AnalysisDataService::Instance().add(m_inputWS + "0", input);

    TS_ASSERT_THROWS_NOTHING(
        m_saver->setPropertyValue("InputWorkspaces", input->getName()));
    TS_ASSERT_THROWS_NOTHING(
        m_saver->setPropertyValue("Filename", m_outputFile));
    m_outputFile = m_saver->getPropertyValue("Filename"); // get absolute path

    // Set to overwrite to ensure creation not append
    TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("OverwriteFile", true));
    TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("IncludeError", false));

    TS_ASSERT_THROWS_NOTHING(m_saver->execute());
    TS_ASSERT(m_saver->isExecuted());

    // Check file exists
    Poco::File file(m_outputFile);
    TS_ASSERT(file.exists());

    checksOnNXTomoFormat(3);

    // Tidy up
    AnalysisDataService::Instance().remove(input->getName());
    if (file.exists())
      file.remove();
  }

  // using image workspaces with one spectrum per row (as opposed to
  // one spectrum per pixel)
  void testWriteGroupCreatingFromRectImage() {
    std::vector<Workspace2D_sptr> wspaces(2);
    const std::string wsgName = "dummy_test_rect_images";
    WorkspaceGroup_sptr input =
        makeWorkspacesInGroup(wsgName, wspaces, 0, true);
    AnalysisDataService::Instance().add(wsgName + "0", input);

    TS_ASSERT_THROWS_NOTHING(
        m_saver->setPropertyValue("InputWorkspaces", input->getName()));
    TS_ASSERT_THROWS_NOTHING(
        m_saver->setPropertyValue("Filename", m_outputFile));
    m_outputFile = m_saver->getPropertyValue("Filename"); // get absolute path

    // Set to overwrite to ensure creation not append
    TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("OverwriteFile", true));
    TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("IncludeError", false));

    TS_ASSERT_THROWS_NOTHING(m_saver->execute());
    TS_ASSERT(m_saver->isExecuted());

    // Check file exists
    Poco::File file(m_outputFile);
    TS_ASSERT(file.exists());

    checksOnNXTomoFormat(2);

    // Tidy up
    AnalysisDataService::Instance().remove(input->getName());
    if (file.exists())
      file.remove();
  }

  void testWriteGroupAppending() {
    // this needs to be called, cxxtest won't run it when it has an argument
    testWriteSingleCreating(true);

    // Run the single workspace test again, without deleting the file at the end
    // (to test append)
    testWriteSingleCreating(false);

    // Test appending a ws group to an existing file
    if (Poco::File(m_outputFile).exists()) {
      int numberOfPriorWS = 1; // Count of current workspaces in the file
      // Create small test workspaces
      std::vector<Workspace2D_sptr> wspaces(3);
      WorkspaceGroup_sptr input =
          makeWorkspacesInGroup(m_inputWS, wspaces, numberOfPriorWS);
      AnalysisDataService::Instance().add(
          m_inputWS + boost::lexical_cast<std::string>(numberOfPriorWS), input);

      TS_ASSERT_THROWS_NOTHING(
          m_saver->setPropertyValue("InputWorkspaces", input->getName()));
      TS_ASSERT_THROWS_NOTHING(
          m_saver->setPropertyValue("Filename", m_outputFile));
      m_outputFile = m_saver->getPropertyValue("Filename"); // get absolute path

      // Ensure append not create
      TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("OverwriteFile", false));
      TS_ASSERT_THROWS_NOTHING(m_saver->setProperty("IncludeError", false));

      TS_ASSERT_THROWS_NOTHING(m_saver->execute());
      TS_ASSERT(m_saver->isExecuted());

      // Check file exists
      Poco::File file(m_outputFile);
      TS_ASSERT(file.exists());

      checksOnNXTomoFormat(static_cast<int>(wspaces.size()) + numberOfPriorWS);

      // Tidy up
      AnalysisDataService::Instance().remove(input->getName());
      file.remove();
    }
  }

private:
  Workspace_sptr makeWorkspaceSingle(const std::string &input) {
    // Create a single workspace
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(
        m_axisSize * m_axisSize, 1, 1.0);
    ws->setTitle(input);

    // Add axis sizes
    ws->mutableRun().addLogData(
        new PropertyWithValue<int>("Axis1", m_axisSize));
    ws->mutableRun().addLogData(
        new PropertyWithValue<int>("Axis2", m_axisSize));

    // Add log values
    ws->mutableRun().addLogData(
        new PropertyWithValue<double>("Rotation", 1 * 5));

    return ws;
  }

  /**
   * Builds a group
   *
   * @param input name of the input workspace
   * @param wspaces workspaces to populate
   * @param wsIndOffset start/offset of workspace, to calculate the
   * rotation value
   * @param specPerRow If true, the images are in the workspaces with
   * one spectrum per row. By default, there is one spectrum per pixel
   *
   * @return a group of workspaces with properties set
   */
  WorkspaceGroup_sptr
  makeWorkspacesInGroup(const std::string &input,
                        std::vector<Workspace2D_sptr> &wspaces,
                        int wsIndOffset = 0, bool specPerRow = false) {
    // Create a ws group with 3 workspaces.
    WorkspaceGroup_sptr wsGroup = WorkspaceGroup_sptr(new WorkspaceGroup());
    std::string groupName =
        input + boost::lexical_cast<std::string>(wsIndOffset);
    wsGroup->setTitle(groupName);

    for (uint32_t i = 0; i < static_cast<uint32_t>(wspaces.size()); ++i) {
      if (specPerRow) {
        wspaces[i] = WorkspaceCreationHelper::create2DWorkspaceBinned(
            m_axisSize, m_axisSize + 1, 1.0);

      } else {
        wspaces[i] = WorkspaceCreationHelper::create2DWorkspaceBinned(
            m_axisSize * m_axisSize, 1, 1.0);
      }
      wspaces[i]->setTitle(
          groupName + boost::lexical_cast<std::string>(wsIndOffset + (i + 1)));
      wspaces[i]->mutableRun().addLogData(
          new PropertyWithValue<int>("Axis1", m_axisSize));
      wspaces[i]->mutableRun().addLogData(
          new PropertyWithValue<int>("Axis2", m_axisSize));
      wspaces[i]->mutableRun().addLogData(new PropertyWithValue<double>(
          "Rotation", ((i + 1) + wsIndOffset) * 5));
      wsGroup->addWorkspace(wspaces[i]);
    }

    return wsGroup;
  }

  void checkNXTomoStructure() {
    // Checks the structure of the file - not interested in the data content
    NXhandle fileHandle;
    NXstatus status = NXopen(m_outputFile.c_str(), NXACC_RDWR, &fileHandle);

    TS_ASSERT(status != NX_ERROR);

    if (status != NX_ERROR) {
      ::NeXus::File nxFile(fileHandle);

      // Check for entry1/tomo_entry/control { and data dataset within }
      TS_ASSERT_THROWS_NOTHING(nxFile.openPath("/entry1/tomo_entry/control"));
      TS_ASSERT_THROWS_NOTHING(nxFile.openData("data"));
      try {
        nxFile.closeData();
      } catch (...) {
      }

      // Check for entry1/tomo_entry/data { and data / rotation_angle dataset
      // links within }
      TS_ASSERT_THROWS_NOTHING(nxFile.openPath("/entry1/tomo_entry/data"));
      TS_ASSERT_THROWS_NOTHING(nxFile.openData("data"));
      try {
        nxFile.closeData();
      } catch (...) {
      }
      TS_ASSERT_THROWS_NOTHING(nxFile.openData("rotation_angle"));
      try {
        nxFile.closeData();
      } catch (...) {
      }

      // Check for entry1/tomo_entry/instrument/detector { data and image_key
      // dataset link within }
      TS_ASSERT_THROWS_NOTHING(
          nxFile.openPath("/entry1/tomo_entry/instrument/detector"));
      TS_ASSERT_THROWS_NOTHING(nxFile.openData("data"));
      try {
        nxFile.closeData();
      } catch (...) {
      }
      TS_ASSERT_THROWS_NOTHING(nxFile.openData("image_key"));
      try {
        nxFile.closeData();
      } catch (...) {
      }

      // Check for entry1/tomo_entry/instrument/sample { and rotation_angle
      // dataset link within }
      TS_ASSERT_THROWS_NOTHING(nxFile.openPath("/entry1/tomo_entry/sample"));
      TS_ASSERT_THROWS_NOTHING(nxFile.openData("rotation_angle"));
      try {
        nxFile.closeData();
      } catch (...) {
      }

      // Check for entry1/log_info { and run_title dataset link within }
      TS_ASSERT_THROWS_NOTHING(nxFile.openPath("/entry1/log_info"));
      TS_ASSERT_THROWS_NOTHING(nxFile.openData("run_title"));
      try {
        nxFile.closeData();
      } catch (...) {
      }

      nxFile.close();
    }
  }

  void checksOnNXTomoFormat(int wsCount) {
    // Check that the structure of the nxTomo file is correct
    checkNXTomoStructure();

    // Check count of entries for data / run_title / rotation_angle / image_key
    checkNXTomoDimensions(wsCount);

    // Check rotation values
    checkNXTomoRotations(wsCount);

    // Check main data values
    checkNXTomoData(wsCount);
  }

  void checkNXTomoDimensions(int wsCount) {
    // Check that the dimensions for the datasets are correct for the number of
    // workspaces
    NXhandle fileHandle;
    NXstatus status = NXopen(m_outputFile.c_str(), NXACC_RDWR, &fileHandle);
    if (status != NX_ERROR) {
      ::NeXus::File nxFile(fileHandle);

      nxFile.openPath("/entry1/tomo_entry/data");
      nxFile.openData("data");
      TS_ASSERT_EQUALS(nxFile.getInfo().dims[0], wsCount);
      TS_ASSERT_EQUALS(nxFile.getInfo().dims[1], m_axisSize);
      TS_ASSERT_EQUALS(nxFile.getInfo().dims[2], m_axisSize);
      nxFile.closeData();
      nxFile.openData("rotation_angle");
      TS_ASSERT_EQUALS(nxFile.getInfo().dims[0], wsCount);
      nxFile.closeData();

      nxFile.openPath("/entry1/tomo_entry/instrument/detector");
      nxFile.openData("image_key");
      TS_ASSERT_EQUALS(nxFile.getInfo().dims[0], wsCount);
      nxFile.closeData();

      nxFile.openPath("/entry1/log_info");
      nxFile.openData("run_title");
      TS_ASSERT_EQUALS(nxFile.getInfo().dims[0], wsCount);
      nxFile.closeData();

      nxFile.close();
    }
  }

  void checkNXTomoRotations(int wsCount) {
    // Check that the rotation values are correct for the rotation dataset for
    // the number of workspaces
    NXhandle fileHandle;
    NXstatus status = NXopen(m_outputFile.c_str(), NXACC_RDWR, &fileHandle);
    if (status != NX_ERROR) {
      ::NeXus::File nxFile(fileHandle);

      nxFile.openPath("/entry1/tomo_entry/data");
      nxFile.openData("rotation_angle");
      std::vector<double> data;
      nxFile.getData(data);
      for (int i = 0; i < wsCount; ++i)
        TS_ASSERT_EQUALS(data[i], static_cast<double>((1 + i) * 5));

      nxFile.closeData();

      nxFile.close();
    }
  }

  void checkNXTomoData(int wsCount) {
    // Checks the first {wsCount} data entries are correct - All test data is
    // value 2.0
    NXhandle fileHandle;
    NXstatus status = NXopen(m_outputFile.c_str(), NXACC_RDWR, &fileHandle);
    if (status != NX_ERROR) {
      ::NeXus::File nxFile(fileHandle);

      nxFile.openPath("/entry1/tomo_entry/data");
      nxFile.openData("data");
      std::vector<double> data;
      nxFile.getData(data);
      for (int i = 0; i < wsCount; ++i)
        TS_ASSERT_EQUALS(data[i], 2.0);

      nxFile.closeData();

      nxFile.close();
    }
  }

private:
  IAlgorithm *m_saver;
  std::string m_outputFile;
  std::string m_inputWS;
  int m_axisSize;
};

#endif /*SAVENXTOMOTEST_H_*/
