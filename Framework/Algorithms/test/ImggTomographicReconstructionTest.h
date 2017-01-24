#ifndef MANTID_ALGORITHMS_IMGGTOMOGRAPHICRECONSTRUCTIONTEST_H_
#define MANTID_ALGORITHMS_IMGGTOMOGRAPHICRECONSTRUCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ImggTomographicReconstruction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Exception.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>

using Mantid::Algorithms::ImggTomographicReconstruction;
using namespace Mantid;

class ImggTomographicReconstructionTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ImggTomographicReconstructionTest *createSuite() {
    return new ImggTomographicReconstructionTest();
  }

  static void destroySuite(ImggTomographicReconstructionTest *suite) {
    delete suite;
  }

  void test_init() {
    ImggTomographicReconstruction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    double relax;
    TS_ASSERT_THROWS_NOTHING(relax = alg.getProperty("RelaxationParameter"));
    TS_ASSERT_EQUALS(relax, 0.5);
  }

  void test_errors_options() {
    auto alg = API::AlgorithmManager::Instance().create(
        "ImggTomographicReconstruction");

    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"));

    TS_ASSERT_THROWS(
        alg->setPropertyValue("BitDepth", "this_is_wrong_you_must_fail"),
        std::runtime_error);
  }

  void test_exec_fails_inexistent_workspace() {
    ImggTomographicReconstruction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS(
        alg.setPropertyValue("InputWorkspace", "inexistent_workspace_fails"),
        std::invalid_argument);

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_fails_wrong_workspace() {
    API::MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::createWorkspaceSingleValue(3);

    ImggTomographicReconstruction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", a));

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());

    API::MatrixWorkspace_sptr wsSingle =
        WorkspaceCreationHelper::create2DWorkspace(10, 10);

    ImggTomographicReconstruction algTwo;
    TS_ASSERT_THROWS_NOTHING(algTwo.initialize());
    TS_ASSERT(algTwo.isInitialized());
    TS_ASSERT_THROWS_NOTHING(algTwo.setProperty("InputWorkspace", wsSingle));

    TS_ASSERT_THROWS(algTwo.execute(), std::runtime_error);
    TS_ASSERT(!algTwo.isExecuted());
  }

  void test_exec_fails_single_proj() {
    const std::string projectionsGrpName("only_one_projection");
    API::WorkspaceGroup_sptr projectionsGrp =
        WorkspaceCreationHelper::createWorkspaceGroup(1, 4, 4,
                                                      projectionsGrpName);

    ImggTomographicReconstruction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", projectionsGrp));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinProjectionAngle", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxProjectionAngle", 260.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RelaxationParameter", 1.25));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CenterOfRotation", 4));

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_fails_wrong_center() {
    const std::string projectionsGrpName("only_two_small_projections");
    API::WorkspaceGroup_sptr projectionsGrp =
        WorkspaceCreationHelper::createWorkspaceGroup(2, 4, 4,
                                                      projectionsGrpName);

    ImggTomographicReconstruction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", projectionsGrp));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinProjectionAngle", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxProjectionAngle", 260.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RelaxationParameter", 1.25));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CenterOfRotation", 10000));

    // should throw because of the wrong center / outside of image dimensions
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_runs() {
    const std::string projectionsGrpName("only_four_proj");
    int ysize = 16;
    int xsize = 16;
    int numProj = 4;
    API::WorkspaceGroup_sptr projectionsGrp =
        WorkspaceCreationHelper::createWorkspaceGroup(numProj, ysize, xsize,
                                                      projectionsGrpName);

    for (size_t proj = 0; proj < static_cast<size_t>(proj); ++proj) {
      API::Workspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(ws = projectionsGrp->getItem(proj));
      API::MatrixWorkspace_sptr projWS;
      TS_ASSERT_THROWS_NOTHING(
          projWS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(ws));
      MantidVec &dataY = projWS->dataY(7);
      TS_ASSERT_THROWS_NOTHING(std::fill(dataY.begin(), dataY.end(), 5000.0));
      for (size_t idx = 0; idx < static_cast<size_t>(ysize); ++idx) {
        projWS->dataY(ysize)[ysize - idx - 1] = 987.6;
      }
    }

    ImggTomographicReconstruction alg;
    // getProperty with workspaceGroup isn't working with
    // Algorithm::processGroups. So we need retrieveWS<>() below:
    const std::string reconName = "recon_1";
    auto recon = runWithValidCenter(alg, projectionsGrp, 7, reconName);
    // Fix when this works well
    TSM_ASSERT("Expected that getProperty would return nullptr with "
               "WorkspaceGroup when using processGroups()",
               !recon);

    TS_ASSERT_THROWS_NOTHING(
        recon = API::AnalysisDataService::Instance()
                    .retrieveWS<API::WorkspaceGroup>(reconName));
    TSM_ASSERT("The reconstruction/result workspace is not valid", recon);

    size_t grpSize = recon->size();
    TSM_ASSERT_EQUALS(
        "The number of items in the output/reconstruction workspace is wrong",
        grpSize, ysize);

    const size_t pix1x = 5;
    const size_t pix1y = 14;
    const double referencePix1 = 8.0;
    const size_t pix2x = 7;
    const size_t pix2y = 8;
    const double referencePix2 = 8.0;
    for (size_t idx = 0; idx < grpSize; ++idx) {
      auto wks = recon->getItem(idx);
      TSM_ASSERT(
          "The output workspace group should have valid slice workspaces", wks);
      auto sliceWS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(wks);
      TSM_ASSERT("The slice workspaces should be of type MatrixWorkspace",
                 sliceWS);

      TSM_ASSERT_EQUALS("Unexpected number of columns in output slices",
                        sliceWS->blocksize(), xsize);
      TSM_ASSERT_EQUALS("Unexpected number of rows in output slices",
                        sliceWS->getNumberHistograms(), ysize);
      TSM_ASSERT_DELTA("Unexpected value in output pixel",
                       sliceWS->readY(pix1y)[pix1x], referencePix1, 1e-4);
      TSM_ASSERT_DELTA("Unexpected value in output pixel",
                       sliceWS->readY(pix2y)[pix2x], referencePix2, 1e-4);
    }
  }

  void test_exec_runs0s() {
    const std::string projectionsGrpName("a_couple_0_images");
    int ysize = 8;
    int xsize = 8;
    int numProj = 2;
    API::WorkspaceGroup_sptr projectionsGrp =
        WorkspaceCreationHelper::createWorkspaceGroup(numProj, ysize, xsize,
                                                      projectionsGrpName);

    for (size_t proj = 0; proj < static_cast<size_t>(proj); ++proj) {
      API::Workspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(ws = projectionsGrp->getItem(proj));
      API::MatrixWorkspace_sptr projWS;
      TS_ASSERT_THROWS_NOTHING(
          projWS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(ws));
      for (size_t row = 0; row < static_cast<size_t>(ysize); ++row) {
        MantidVec &dataY = projWS->dataY(row);
        TS_ASSERT_THROWS_NOTHING(std::fill(dataY.begin(), dataY.end(), 0.0));
      }
    }

    ImggTomographicReconstruction alg;
    const std::string reconName = "recon_0";
    auto recon = runWithValidCenter(alg, projectionsGrp, 7, reconName);
    // Fix when this works well
    TSM_ASSERT("Expected that getProperty would return nullptr with "
               "WorkspaceGroup when using processGroups()",
               !recon);

    TS_ASSERT_THROWS_NOTHING(
        recon = API::AnalysisDataService::Instance()
                    .retrieveWS<API::WorkspaceGroup>(reconName));
    TSM_ASSERT("The reconstruction/result workspace is not valid", recon);

    size_t grpSize = recon->size();
    TSM_ASSERT_EQUALS(
        "The number of items in the output/reconstruction workspace is wrong",
        grpSize, ysize);

    const size_t pix1x = 5;
    const size_t pix1y = 2;
    const double referencePix1 = 0.0;
    const size_t pix2x = 7;
    const size_t pix2y = 7;
    const double referencePix2 = 0.0;
    for (size_t idx = 0; idx < grpSize; ++idx) {
      auto wks = recon->getItem(idx);
      TSM_ASSERT(
          "The output workspace group should have valid slice workspaces", wks);
      auto sliceWS = boost::dynamic_pointer_cast<API::MatrixWorkspace>(wks);
      TSM_ASSERT("The slice workspaces should be of type MatrixWorkspace",
                 sliceWS);

      TSM_ASSERT_EQUALS("Unexpected number of columns in output slices",
                        sliceWS->blocksize(), xsize);
      TSM_ASSERT_EQUALS("Unexpected number of rows in output slices",
                        sliceWS->getNumberHistograms(), ysize);
      TSM_ASSERT_DELTA("Unexpected value in output pixel",
                       sliceWS->readY(pix1y)[pix1x], referencePix1, 1e-4);
      TSM_ASSERT_DELTA("Unexpected value in output pixel",
                       sliceWS->readY(pix2y)[pix2x], referencePix2, 1e-4);
    }
  }

private:
  API::WorkspaceGroup_sptr runWithValidCenter(API::Algorithm &alg,
                                              API::WorkspaceGroup_sptr wksg,
                                              int center,
                                              const std::string &outName) {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", wksg));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinProjectionAngle", 0.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxProjectionAngle", 180.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RelaxationParameter", 1.25));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CenterOfRotation", center));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outName));
    TSM_ASSERT_THROWS_NOTHING(
        "execute() throwed for an algorithm with a "
        "supposedly correct center parameter for which no "
        "failure was expected",
        alg.execute());
    TSM_ASSERT("The algorithm execution didn't finish successfully when no "
               "issues where expected ",
               alg.isExecuted());

    API::WorkspaceGroup_sptr result = alg.getProperty("OutputWorkspace");
    return result;
  }
};

#endif /* MANTID_ALGORITHMS_IMGGTOMOGRAPHICRECONSTRUCTIONTEST_H_ */
