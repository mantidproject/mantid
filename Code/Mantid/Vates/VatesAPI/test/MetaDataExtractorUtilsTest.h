#ifndef METADATAEXTRACTORUTILS_TEST_H
#define METADATAEXTRACTORUTILS_TEST_H

#include "MantidVatesAPI/MetaDataExtractorUtils.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h> 
#include "MockObjects.h"

#include "qwt/qwt_double_interval.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/FileFinder.h"
#include "boost/pointer_cast.hpp"

using namespace Mantid::VATES;
using namespace Mantid::API;
using namespace testing;
using Mantid::MDEvents::MDEventsTestHelper::makeFakeMDHistoWorkspace;

class MetaDataExtractorUtilsTest : public CxxTest::TestSuite
{

  public:

    // Helper method. Generates and returns a valid IMDEventWorkspace
    static Mantid::API::Workspace_sptr getReal4DWorkspace()
    {
      AnalysisDataService::Instance().remove("MD_EVENT_WS_ID");
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadMD");	
      alg->initialize();
      alg->setRethrows(true);
      alg->setPropertyValue("Filename", Mantid::API::FileFinder::Instance().getFullPath("MAPS_MDEW.nxs"));
      alg->setPropertyValue("OutputWorkspace", "MD_EVENT_WS_ID");
      alg->setProperty("FileBackEnd", false); 
      alg->execute(); 
      return AnalysisDataService::Instance().retrieve("MD_EVENT_WS_ID");
    }

    void testShouldExtractMinAndMaxFromWorkspaceForMDHisto()
    {
      // Arrange
      double maxValue = 1.3;
      Mantid::MDEvents::MDHistoWorkspace_sptr histoWorkspace = makeFakeMDHistoWorkspace(1.0, 4, 5, maxValue, 0.1,"MD_HISTO_WS");

      // Act
      MetaDataExtractorUtils extractor;
      QwtDoubleInterval minMax = extractor.getMinAndMax(histoWorkspace);

      // Assert
      TSM_ASSERT("Should find the a min which is smaller/equal to max ", minMax.minValue() <= minMax.maxValue())
    }

    void testShouldExtractMinAndMaxFromWorkspaceForMDEvent()
    {
      // Arrange
      Mantid::API::Workspace_sptr workspace = getReal4DWorkspace();
      Mantid::API::IMDEventWorkspace_sptr eventWorkspace = boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(workspace);
      MetaDataExtractorUtils extractor;
      
      // Act
      QwtDoubleInterval minMax = extractor.getMinAndMax(eventWorkspace);

      // Assert
      TSM_ASSERT("Should find the a min which is smaller/equal to max ", minMax.minValue() <= minMax.maxValue())
    }

    void testShouldNotFindInstrumentForBadWorkspace()
    {
      // Arrange
      // Return a table workspace.
      Mantid::API::Workspace_sptr workspace = WorkspaceFactory::Instance().createTable();
      Mantid::API::IMDHistoWorkspace_sptr histoWorkspace = boost::dynamic_pointer_cast<Mantid::API::IMDHistoWorkspace>(workspace);

      MetaDataExtractorUtils extractor;

      // Act
      std::string instrument = extractor.extractInstrument(histoWorkspace);

      // Assert
      TSM_ASSERT("Should find an empty instrment for invalid workspace", instrument.empty())
    }
};

#endif