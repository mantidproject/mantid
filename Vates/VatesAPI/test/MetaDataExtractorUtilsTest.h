#ifndef METADATAEXTRACTORUTILS_TEST_H
#define METADATAEXTRACTORUTILS_TEST_H

#ifdef _MSC_VER
// Disabling Json warnings regarding non-export of Json::Reader and Json::Writer
#pragma warning(disable : 4275)
#pragma warning(disable : 4251)
#endif

#include "MantidVatesAPI/MetaDataExtractorUtils.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockObjects.h"

#include <qwt_double_interval.h>
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "boost/pointer_cast.hpp"

using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace testing;
using Mantid::DataObjects::MDEventsTestHelper::makeFakeMDHistoWorkspace;

class MetaDataExtractorUtilsTest : public CxxTest::TestSuite {

public:
  // Helper method. Generates and returns a valid IMDEventWorkspace
  static Mantid::API::Workspace_sptr getReal4DWorkspace() {
    AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadMD");
    alg->initialize();
    alg->setRethrows(true);
    alg->setPropertyValue(
        "Filename",
        Mantid::API::FileFinder::Instance().getFullPath("MAPS_MDEW.nxs"));
    alg->setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
    alg->setProperty("FileBackEnd", false);
    alg->execute();
    return AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
  }

  void testShouldNotFindInstrumentForBadWorkspace() {
    // Arrange
    // Return a table workspace.
    Mantid::API::Workspace_sptr workspace =
        WorkspaceFactory::Instance().createTable();
    Mantid::API::IMDHistoWorkspace_sptr histoWorkspace =
        boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(workspace);

    MetaDataExtractorUtils extractor;

    // Act
    std::string instrument = extractor.extractInstrument(histoWorkspace.get());

    // Assert
    TSM_ASSERT("Should find an empty instrment for invalid workspace",
               instrument.empty())
  }
};

#endif
