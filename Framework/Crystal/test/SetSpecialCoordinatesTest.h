// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidCrystal/SetSpecialCoordinates.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::Crystal::SetSpecialCoordinates;

class SetSpecialCoordinatesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SetSpecialCoordinatesTest *createSuite() { return new SetSpecialCoordinatesTest(); }
  static void destroySuite(SetSpecialCoordinatesTest *suite) { delete suite; }

  void test_Init() {
    SetSpecialCoordinates alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_CannotUseAnyWorkspaceType() {
    Workspace_sptr tableWS = std::make_shared<Mantid::DataObjects::TableWorkspace>(1);

    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("SpecialCoordinates", "Q (lab frame)");

    alg.setProperty("InputWorkspace", tableWS);

    TSM_ASSERT_THROWS("Only IMDWorkspaces and IPeaksWorkspaces are acceptable inputs.", alg.execute(),
                      std::invalid_argument &);
  }

  void test_qLabAllowed() {
    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("SpecialCoordinates", "Q (lab frame)"));
  }

  void test_qSampleAllowed() {
    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("SpecialCoordinates", "Q (sample frame)"));
  }

  void test_HKLAllowed() {
    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("SpecialCoordinates", "HKL"));
  }

  void test_junkCoordinateSystemDisallowed() {
    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS(alg.setPropertyValue("SpecialCoordinates", "Junk"), std::invalid_argument &);
    TS_ASSERT(alg.isInitialized());
  }

  void test_ModifyMDEventWorkspace() {
    IMDEventWorkspace_sptr inWS = Mantid::DataObjects::MDEventsTestHelper::makeMDEW<1>(1, 0, 1, 1);
    AnalysisDataService::Instance().add("inWS", inWS);

    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("SpecialCoordinates", "Q (sample frame)");
    alg.setPropertyValue("InputWorkspace", "inWS");
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("inWS");
    TSM_ASSERT_EQUALS("Should still be still with the same special coordinate system",
                      inWS->getSpecialCoordinateSystem(), outWS->getSpecialCoordinateSystem());
    AnalysisDataService::Instance().remove("inWS");
  }

  void test_ModifyMDHistoWorkspace() {
    IMDHistoWorkspace_sptr inWS = Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1);
    AnalysisDataService::Instance().add("inWS", inWS);

    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("SpecialCoordinates", "Q (sample frame)");
    alg.setPropertyValue("InputWorkspace", "inWS");
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("inWS");
    TSM_ASSERT_EQUALS("Should still be still with the same special coordinate system",
                      inWS->getSpecialCoordinateSystem(), outWS->getSpecialCoordinateSystem());
    AnalysisDataService::Instance().remove("inWS");
  }

  void test_ModifyPeaksWorkspace() {
    IPeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace(1);
    AnalysisDataService::Instance().add("inWS", inWS);

    SetSpecialCoordinates alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("SpecialCoordinates", "Q (sample frame)");
    alg.setPropertyValue("InputWorkspace", "inWS");
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>("inWS");
    TS_ASSERT_EQUALS(Mantid::Kernel::QSample, outWS->getSpecialCoordinateSystem());
    AnalysisDataService::Instance().remove("inWS");
  }
};
