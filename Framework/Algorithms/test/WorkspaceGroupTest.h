#ifndef WORKSPACEGROUP_H_
#define WORKSPACEGROUP_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/FindDeadDetectors.h"
#include "MantidAlgorithms/Plus.h"
#include "MantidAlgorithms/PolynomialCorrection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;

class WorkspaceGroupTest : public CxxTest::TestSuite {
private:
  void checkData(MatrixWorkspace_sptr work_in1, MatrixWorkspace_sptr work_in2,
                 MatrixWorkspace_sptr work_out1) {
    // default to a horizontal loop orientation
    checkData(work_in1, work_in2, work_out1, 0);
  }

  // loopOrientation 0=Horizontal, 1=Vertical
  void checkData(MatrixWorkspace_sptr work_in1, MatrixWorkspace_sptr work_in2,
                 MatrixWorkspace_sptr work_out1, int loopOrientation) {
    if (!work_in1 || !work_in2 || !work_out1) {
      TSM_ASSERT("One or more empty workspace pointers.", 0);
      return;
    }
    size_t ws2LoopCount = 0;
    if (work_in2->size() > 0) {
      ws2LoopCount = work_in1->size() / work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount == 0) ? 1 : ws2LoopCount;

    for (size_t i = 0; i < work_out1->size(); i++) {
      size_t ws2Index = i;

      if (ws2LoopCount > 1) {
        if (loopOrientation == 0) {
          ws2Index = i % ws2LoopCount;
        } else {
          ws2Index = i / ws2LoopCount;
        }
      }
      checkDataItem(work_in1, work_in2, work_out1, i, ws2Index);
    }
  }

  void checkDataItem(MatrixWorkspace_sptr work_in1,
                     MatrixWorkspace_sptr work_in2,
                     MatrixWorkspace_sptr work_out1, size_t i,
                     size_t ws2Index) {
    double sig1 =
        work_in1->dataY(i / work_in1->blocksize())[i % work_in1->blocksize()];
    double sig2 = work_in2->dataY(
        ws2Index / work_in1->blocksize())[ws2Index % work_in2->blocksize()];
    double sig3 =
        work_out1->dataY(i / work_in1->blocksize())[i % work_in1->blocksize()];
    TS_ASSERT_DELTA(
        work_in1->dataX(i / work_in1->blocksize())[i % work_in1->blocksize()],
        work_out1->dataX(i / work_in1->blocksize())[i % work_in1->blocksize()],
        0.0001);

    TS_ASSERT_DELTA(sig1 + sig2, sig3, 0.0001);
    double err1 =
        work_in1->dataE(i / work_in1->blocksize())[i % work_in1->blocksize()];
    double err2 = work_in2->dataE(
        ws2Index / work_in2->blocksize())[ws2Index % work_in2->blocksize()];
    double err3(sqrt((err1 * err1) + (err2 * err2)));
    TS_ASSERT_DELTA(
        err3,
        work_out1->dataE(i / work_in1->blocksize())[i % work_in1->blocksize()],
        0.0001);
  }

public:
  void testExecwithOneGroupandOne2DWorkspace() {

    int nHist = 20, nBins = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);
    MatrixWorkspace_sptr work_in3 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr work_in4 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr) {
      AnalysisDataService::Instance().add("test_in", wsSptr);
      AnalysisDataService::Instance().add("test_in_1", work_in1);
      wsSptr->add("test_in_1");
      AnalysisDataService::Instance().add("test_in_2", work_in2);
      wsSptr->add("test_in_2");
      AnalysisDataService::Instance().add("test_in_3", work_in3);
      wsSptr->add("test_in_3");
      AnalysisDataService::Instance().add("test_in_4", work_in4);
      wsSptr->add("test_in_4");
    }
    WorkspaceGroup_sptr work_in;
    work_in =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("test_in");
    TS_ASSERT_EQUALS(work_in, wsSptr);
    if (work_in) {
      std::vector<std::string> GroupNames = work_in->getNames();
      size_t nSize = GroupNames.size();
      TS_ASSERT_EQUALS(nSize, 4);
    }
    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace", "test_in");
    alg.setPropertyValue("RHSWorkspace", "test_in_1");
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = boost::dynamic_pointer_cast<WorkspaceGroup>(
            AnalysisDataService::Instance().retrieve("test_out")));
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("test_out_1")));
    MatrixWorkspace_sptr work_out2;
    TS_ASSERT_THROWS_NOTHING(
        work_out2 = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("test_out_2")));
    MatrixWorkspace_sptr work_out3;
    TS_ASSERT_THROWS_NOTHING(
        work_out3 = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("test_out_3")));
    MatrixWorkspace_sptr work_out4;
    TS_ASSERT_THROWS_NOTHING(
        work_out4 = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("test_out_4")));

    if (!work_out1)
      return;

    checkData(work_in1, work_in1, work_out1);
    checkData(work_in2, work_in1, work_out2);
    checkData(work_in3, work_in1, work_out3);
    checkData(work_in4, work_in1, work_out4);

    work_out =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("test_out");
    work_out->removeAll();

    AnalysisDataService::Instance().remove("test_in");
    AnalysisDataService::Instance().remove("test_in_1");
    AnalysisDataService::Instance().remove("test_in_2");
    AnalysisDataService::Instance().remove("test_in_3");
    AnalysisDataService::Instance().remove("test_in_4");
    AnalysisDataService::Instance().remove("test_out");
    AnalysisDataService::Instance().remove("test_out_1");
    AnalysisDataService::Instance().remove("test_out_2");
    AnalysisDataService::Instance().remove("test_out_3");
    AnalysisDataService::Instance().remove("test_out_4");
  }

  void testExecOnlyOneGroupInput() {
    int nHist = 20, nBins = 10;
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins, 1);
    Workspace2D_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins, 1);
    Instrument_sptr instr(new Instrument);

    // set some dead detectors
    Counts yDead(nBins, 0);
    CountStandardDeviations eDead(nBins, 0);
    for (int i = 0; i < nBins; i++) {
      if (i % 2 == 0) {
        work_in1->setCounts(i, yDead);
        work_in1->setCountStandardDeviations(i, eDead);
      }
      work_in1->getSpectrum(i).setSpectrumNo(i);
      Mantid::Geometry::Detector *det =
          new Mantid::Geometry::Detector("", i, NULL);
      instr->add(det);
      instr->markAsDetector(det);
    }
    work_in1->setInstrument(instr);
    work_in2->setInstrument(instr);

    for (int i = 0; i < nBins; i++) {
      if (i % 2 == 0) {
        work_in2->setCounts(i, yDead);
        work_in2->setCountStandardDeviations(i, eDead);
      }
      work_in2->getSpectrum(i).setSpectrumNo(i);
    }

    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    AnalysisDataService::Instance().add("testdead_in", wsSptr);
    AnalysisDataService::Instance().add("testdead_in_1", work_in1);
    wsSptr->add("testdead_in_1");
    AnalysisDataService::Instance().add("testdead_in_2", work_in2);
    wsSptr->add("testdead_in_2");

    WorkspaceGroup_sptr work_in;
    work_in = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        "testdead_in");
    TS_ASSERT_EQUALS(work_in, wsSptr);

    FindDeadDetectors alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "testdead_in");
    alg.setPropertyValue("OutputWorkspace", "testdead_out");
    alg.setPropertyValue("DeadThreshold", "0");
    alg.setPropertyValue("LiveValue", "1");
    alg.setPropertyValue("DeadValue", "2");
    std::string filename = "testFile.txt";
    alg.setPropertyValue("OutputFile", filename);
    alg.execute();

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());
    // Get back the output workspace
    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = boost::dynamic_pointer_cast<WorkspaceGroup>(
            AnalysisDataService::Instance().retrieve("testdead_out")));

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("testdead_out_1")));

    MatrixWorkspace_sptr work_out2;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("testdead_out_2")));

    // Get back the output property
    std::vector<int> deadDets;
    TS_ASSERT_THROWS_NOTHING(deadDets = alg.getProperty("FoundDead"))
    TS_ASSERT_EQUALS(deadDets.size(), 0)
    for (int i = 0; i < nBins; i++) {
      const double val = work_out1->readY(i)[0];
      double valExpected = 1;
      if (i % 2 == 0) {
        valExpected = 2;
        if (!deadDets.empty())
          TS_ASSERT_EQUALS(deadDets[i / 2], i)
      }
      TS_ASSERT_DELTA(val, valExpected, 1e-9);
    }

    std::fstream outFile(filename.c_str());
    TS_ASSERT(outFile)
    outFile.close();
    Poco::File(filename).remove();

    work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        "testdead_out");
    work_out->removeAll();

    AnalysisDataService::Instance().remove("testdead_in");
    AnalysisDataService::Instance().remove("testdead_in_1");
    AnalysisDataService::Instance().remove("testdead_in_2");
    AnalysisDataService::Instance().remove("testdead_out");
    AnalysisDataService::Instance().remove("testdead_out_1");
    AnalysisDataService::Instance().remove("testdead_out_2");
  }

  void testExecGroupwithNochildWorkspaces() {
    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    AnalysisDataService::Instance().add("InputWS", wsSptr);
    Mantid::Algorithms::PolynomialCorrection poly3;
    poly3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        poly3.setPropertyValue("InputWorkspace", "InputWS"))
    TS_ASSERT_THROWS_NOTHING(poly3.setPropertyValue("OutputWorkspace", "WSCor"))
    TS_ASSERT_THROWS_NOTHING(
        poly3.setPropertyValue("Coefficients", "3.0,2.0,1.0"))
    TS_ASSERT(!poly3.execute())

    AnalysisDataService::Instance().remove("InputWS");
  }

  void testRenameWorkspaceConsecutiveDuplicates() {
    // Tests that setting a workspace to the name of an existing
    // workspace in a group is correctly handled by the workspace group
    // when they are consecutive

    constexpr int nHist = 20, nBins = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    const std::string ws1Name = "test_ws1";
    const std::string ws2Name = "test_ws2";
    const std::string wsGroupName = "test_group";

    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr) {
      AnalysisDataService::Instance().add(wsGroupName, wsSptr);
      AnalysisDataService::Instance().add(ws1Name, work_in1);
      wsSptr->add(ws1Name);
      AnalysisDataService::Instance().add(ws2Name, work_in2);
      wsSptr->add(ws2Name);
    }

    TS_ASSERT_EQUALS(wsSptr->getNumberOfEntries(), 2);

    // Rename ws2Name to ws1Name
    AnalysisDataService::Instance().rename(ws2Name, ws1Name);

    TS_ASSERT_EQUALS(wsSptr->getNumberOfEntries(), 1);
    TS_ASSERT_EQUALS(wsSptr->contains(work_in2), true);
    TS_ASSERT_EQUALS(wsSptr->contains(work_in1), false);

    AnalysisDataService::Instance().remove(ws1Name);
    AnalysisDataService::Instance().remove(wsGroupName);
  }

  void testRenameWorkspaceNonConsecutiveDuplicates() {
    // Performs the same test as testRenameWorkspaceConsecutiveDuplicates
    // but for workspaces where the duplicates are not adjacent

    // If this test fails but the aforementioned test passes it means
    // the algorithm is only considering consecutive workspaces

    constexpr int nHist = 20, nBins = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr work_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);
    MatrixWorkspace_sptr work_in3 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    const std::string ws1Name = "test_ws1";
    const std::string ws2Name = "test_ws2";
    const std::string ws3Name = "test_ws3";
    const std::string wsGroupName = "test_group";

    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr) {
      AnalysisDataService::Instance().add(wsGroupName, wsSptr);
      AnalysisDataService::Instance().add(ws1Name, work_in1);
      wsSptr->add(ws1Name);
      AnalysisDataService::Instance().add(ws2Name, work_in2);
      wsSptr->add(ws2Name);
      AnalysisDataService::Instance().add(ws3Name, work_in3);
      wsSptr->add(ws3Name);
    }

    TS_ASSERT_EQUALS(wsSptr->getNumberOfEntries(), 3);

    // Rename wsName3 to wsName1
    AnalysisDataService::Instance().rename(ws3Name, ws1Name);

    TS_ASSERT_EQUALS(wsSptr->getNumberOfEntries(), 2);
    TS_ASSERT_EQUALS(wsSptr->contains(work_in3), true);
    TS_ASSERT_EQUALS(wsSptr->contains(work_in2), true);
    TS_ASSERT_EQUALS(wsSptr->contains(work_in1), false);

    AnalysisDataService::Instance().remove(ws1Name);
    AnalysisDataService::Instance().remove(ws2Name);
    AnalysisDataService::Instance().remove(wsGroupName);
  }

  void testRenameWorkspaceDuplicateNotInGroup() {
    // Tests renaming a workspace that is part of a group
    // to a name that exists but not in a group

    constexpr int nHist = 20, nBins = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr inGroupWs1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr inGroupWs2 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr notInGroupWs =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    const std::string inGroupWsName1 = "test_ws1";
    const std::string inGroupWsName2 = "test_ws2";
    const std::string notInGroupWsName = "test_outOfGroup";
    const std::string wsGroupName = "test_group";

    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr) {
      AnalysisDataService::Instance().add(wsGroupName, wsSptr);
      AnalysisDataService::Instance().add(inGroupWsName1, inGroupWs1);
      wsSptr->add(inGroupWsName1);
      AnalysisDataService::Instance().add(inGroupWsName2, inGroupWs2);
      wsSptr->add(inGroupWsName2);
      AnalysisDataService::Instance().add(notInGroupWsName, notInGroupWs);
    }

    TS_ASSERT_EQUALS(wsSptr->getNumberOfEntries(), 2);
    TS_ASSERT_EQUALS(
        AnalysisDataService::Instance().doesExist(notInGroupWsName), true);

    // Rename groupWs to use a name outside of group
    AnalysisDataService::Instance().rename(inGroupWsName1, notInGroupWsName);

    // Test the group didn't change size or pointers only names
    TS_ASSERT_EQUALS(wsSptr->getNumberOfEntries(), 2);
    TS_ASSERT_EQUALS(wsSptr->contains(inGroupWs1), true);
    TS_ASSERT_EQUALS(wsSptr->contains(notInGroupWs), false);
    const auto &groupNames = wsSptr->getNames();
    TS_ASSERT_EQUALS(doesExistInVector(groupNames, inGroupWsName1), false);
    TS_ASSERT_EQUALS(doesExistInVector(groupNames, notInGroupWsName), true);

    // Test the workspace outside the group was deleted
    const auto &allSptrs = AnalysisDataService::Instance().getObjects();
    TS_ASSERT_EQUALS(doesExistInVector(allSptrs, notInGroupWs), false);

    AnalysisDataService::Instance().remove(inGroupWsName1);
    AnalysisDataService::Instance().remove(wsGroupName);
  }

  void testTwoGroupWorkspaces() {
    int nHist = 10, nBins = 20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr worklhs_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr worklhs_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);
    MatrixWorkspace_sptr worklhs_in3 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr worklhs_in4 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    MatrixWorkspace_sptr workrhs_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr workrhs_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);
    MatrixWorkspace_sptr workrhs_in3 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr workrhs_in4 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr) {
      AnalysisDataService::Instance().add("testlhs_in", wsSptr);
      AnalysisDataService::Instance().add("testlhs_in_1", worklhs_in1);
      wsSptr->add("testlhs_in_1");
      AnalysisDataService::Instance().add("testlhs_in_2", worklhs_in2);
      wsSptr->add("testlhs_in_2");
      AnalysisDataService::Instance().add("testlhs_in_3", worklhs_in3);
      wsSptr->add("testlhs_in_3");
      AnalysisDataService::Instance().add("testlhs_in_4", worklhs_in4);
      wsSptr->add("testlhs_in_4");
    }
    WorkspaceGroup_sptr worklhsgrp_in;
    worklhsgrp_in = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        "testlhs_in");
    TS_ASSERT_EQUALS(worklhsgrp_in, wsSptr);
    if (worklhsgrp_in) {
      std::vector<std::string> GroupNames = worklhsgrp_in->getNames();
      size_t nSize = GroupNames.size();
      TS_ASSERT_EQUALS(nSize, 4);
    }

    WorkspaceGroup_sptr wsSptr1 = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr1) {
      AnalysisDataService::Instance().add("testrhs_in", wsSptr1);
      AnalysisDataService::Instance().add("testrhs_in_1", workrhs_in1);
      wsSptr1->add("testrhs_in_1");
      AnalysisDataService::Instance().add("testrhs_in_2", workrhs_in2);
      wsSptr1->add("testrhs_in_2");
      AnalysisDataService::Instance().add("testrhs_in_3", workrhs_in3);
      wsSptr1->add("testrhs_in_3");
      AnalysisDataService::Instance().add("testrhs_in_4", workrhs_in4);
      wsSptr1->add("testrhs_in_4");
    }
    WorkspaceGroup_sptr workrhsgrp_in;
    workrhsgrp_in = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        "testrhs_in");
    TS_ASSERT_EQUALS(worklhsgrp_in, wsSptr);
    if (workrhsgrp_in) {
      std::vector<std::string> GroupNames = workrhsgrp_in->getNames();
      size_t nSize = GroupNames.size();
      TS_ASSERT_EQUALS(nSize, 4);
    }

    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace", "testlhs_in");
    alg.setPropertyValue("RHSWorkspace", "testrhs_in");
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "test_out"));
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_1"));
    MatrixWorkspace_sptr work_out2;
    TS_ASSERT_THROWS_NOTHING(
        work_out2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_2"));
    MatrixWorkspace_sptr work_out3;
    TS_ASSERT_THROWS_NOTHING(
        work_out3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_3"));
    MatrixWorkspace_sptr work_out4;
    TS_ASSERT_THROWS_NOTHING(
        work_out4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_4"));
    if (!work_out4)
      return;

    checkData(worklhs_in1, workrhs_in1, work_out1);
    checkData(worklhs_in2, workrhs_in2, work_out2);
    checkData(worklhs_in3, workrhs_in3, work_out3);
    checkData(worklhs_in4, workrhs_in4, work_out4);

    work_out =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("test_out");
    if (!work_out)
      return;
    work_out->removeAll();

    AnalysisDataService::Instance().remove("testlhs_in");
    AnalysisDataService::Instance().remove("testlhs_in_1");
    AnalysisDataService::Instance().remove("testlhs_in_2");
    AnalysisDataService::Instance().remove("testlhs_in_3");
    AnalysisDataService::Instance().remove("testlhs_in_4");

    AnalysisDataService::Instance().remove("testrhs_in");
    AnalysisDataService::Instance().remove("testrhs_in_1");
    AnalysisDataService::Instance().remove("testrhs_in_2");
    AnalysisDataService::Instance().remove("testrhs_in_3");
    AnalysisDataService::Instance().remove("testrhs_in_4");

    AnalysisDataService::Instance().remove("test_out");
    AnalysisDataService::Instance().remove("test_out_1");
    AnalysisDataService::Instance().remove("test_out_2");
    AnalysisDataService::Instance().remove("test_out_3");
    AnalysisDataService::Instance().remove("test_out_4");
  }

  void testLHS2DWorkspaceandRHSGroupWorkspace() {
    int nHist = 10, nBins = 20;

    MatrixWorkspace_sptr worklhs_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    if (worklhs_in1)
      AnalysisDataService::Instance().add("testlhs_in1", worklhs_in1);

    MatrixWorkspace_sptr workrhs_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr workrhs_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);
    MatrixWorkspace_sptr workrhs_in3 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr workrhs_in4 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    WorkspaceGroup_sptr wsSptr1 = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr1) {
      AnalysisDataService::Instance().add("testrhs_in", wsSptr1);
      AnalysisDataService::Instance().add("testrhs_in_1", workrhs_in1);
      wsSptr1->add("testrhs_in_1");
      AnalysisDataService::Instance().add("testrhs_in_2", workrhs_in2);
      wsSptr1->add("testrhs_in_2");
      AnalysisDataService::Instance().add("testrhs_in_3", workrhs_in3);
      wsSptr1->add("testrhs_in_3");
      AnalysisDataService::Instance().add("testrhs_in_4", workrhs_in4);
      wsSptr1->add("testrhs_in_4");
    }
    WorkspaceGroup_sptr workrhsgrp_in;
    workrhsgrp_in = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        "testrhs_in");
    TS_ASSERT_EQUALS(workrhsgrp_in, wsSptr1);
    if (workrhsgrp_in) {
      std::vector<std::string> GroupNames = workrhsgrp_in->getNames();
      size_t nSize = GroupNames.size();
      TS_ASSERT_EQUALS(nSize, 4);
    }

    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace", "testlhs_in1");
    alg.setPropertyValue("RHSWorkspace", "testrhs_in");
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "test_out"));
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_1"));
    MatrixWorkspace_sptr work_out2;
    TS_ASSERT_THROWS_NOTHING(
        work_out2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_2"));
    MatrixWorkspace_sptr work_out3;
    TS_ASSERT_THROWS_NOTHING(
        work_out3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_3"));
    MatrixWorkspace_sptr work_out4;
    TS_ASSERT_THROWS_NOTHING(
        work_out4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_4"));

    checkData(worklhs_in1, workrhs_in1, work_out1);
    checkData(worklhs_in1, workrhs_in2, work_out2);
    checkData(worklhs_in1, workrhs_in3, work_out3);
    checkData(worklhs_in1, workrhs_in4, work_out4);

    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "test_out");)
    if (!work_out)
      return;
    work_out->removeAll();

    AnalysisDataService::Instance().remove("testlhs_in1");

    AnalysisDataService::Instance().remove("testrhs_in");
    AnalysisDataService::Instance().remove("testrhs_in_1");
    AnalysisDataService::Instance().remove("testrhs_in_2");
    AnalysisDataService::Instance().remove("testrhs_in_3");
    AnalysisDataService::Instance().remove("testrhs_in_4");

    AnalysisDataService::Instance().remove("test_out");
    AnalysisDataService::Instance().remove("test_out_1");
    AnalysisDataService::Instance().remove("test_out_2");
    AnalysisDataService::Instance().remove("test_out_3");
    AnalysisDataService::Instance().remove("test_out_4");
  }

  void testLHSandRHSSameGroupWorkspaces() {
    // this is the test for self addition
    int nHist = 10, nBins = 20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr worklhs_in1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr worklhs_in2 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);
    MatrixWorkspace_sptr worklhs_in3 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins);
    MatrixWorkspace_sptr worklhs_in4 =
        WorkspaceCreationHelper::create2DWorkspace154(nHist, nBins);

    WorkspaceGroup_sptr wsSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
    if (wsSptr) {
      AnalysisDataService::Instance().add("testlhs_in", wsSptr);
      AnalysisDataService::Instance().add("testlhs_in_1", worklhs_in1);
      wsSptr->add("testlhs_in_1");
      AnalysisDataService::Instance().add("testlhs_in_2", worklhs_in2);
      wsSptr->add("testlhs_in_2");
      AnalysisDataService::Instance().add("testlhs_in_3", worklhs_in3);
      wsSptr->add("testlhs_in_3");
      AnalysisDataService::Instance().add("testlhs_in_4", worklhs_in4);
      wsSptr->add("testlhs_in_4");
    }
    WorkspaceGroup_sptr worklhsgrp_in;
    worklhsgrp_in = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        "testlhs_in");
    TS_ASSERT_EQUALS(worklhsgrp_in, wsSptr);
    if (worklhsgrp_in) {
      std::vector<std::string> GroupNames = worklhsgrp_in->getNames();
      size_t nSize = GroupNames.size();
      TS_ASSERT_EQUALS(nSize, 4);
    }

    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace", "testlhs_in");
    alg.setPropertyValue("RHSWorkspace", "testlhs_in");
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "test_out"));
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(
        work_out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_1"));
    MatrixWorkspace_sptr work_out2;
    TS_ASSERT_THROWS_NOTHING(
        work_out2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_2"));
    MatrixWorkspace_sptr work_out3;
    TS_ASSERT_THROWS_NOTHING(
        work_out3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_3"));
    MatrixWorkspace_sptr work_out4;
    TS_ASSERT_THROWS_NOTHING(
        work_out4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_out_4"));

    checkData(worklhs_in1, worklhs_in1, work_out1);
    checkData(worklhs_in2, worklhs_in2, work_out2);
    checkData(worklhs_in3, worklhs_in3, work_out3);
    checkData(worklhs_in4, worklhs_in4, work_out4);

    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "test_out");)
    if (!work_out)
      return;
    work_out->removeAll();

    AnalysisDataService::Instance().remove("testlhs_in");
    AnalysisDataService::Instance().remove("testlhs_in_1");
    AnalysisDataService::Instance().remove("testlhs_in_2");
    AnalysisDataService::Instance().remove("testlhs_in_3");
    AnalysisDataService::Instance().remove("testlhs_in_4");

    AnalysisDataService::Instance().remove("test_out");
    AnalysisDataService::Instance().remove("test_out_1");
    AnalysisDataService::Instance().remove("test_out_2");
    AnalysisDataService::Instance().remove("test_out_3");
    AnalysisDataService::Instance().remove("test_out_4");
  }

private:
  template <typename VT, typename VA, typename T>
  bool doesExistInVector(const std::vector<VT, VA> &vec, const T &val) const {
    // Where VA is for custom VectorAllocators
    return (std::find(vec.cbegin(), vec.cend(), val)) != vec.cend();
  }
};

#endif /*PLUSTEST_H_*/
