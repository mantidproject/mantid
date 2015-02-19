#ifndef COPYDETECTORMAPPINGTEST_H_
#define COPYDETECTORMAPPINGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/CopyDetectorMapping.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MantidVec;

class CopyDetectorMappingTest : public CxxTest::TestSuite
{
public:
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( copyMapping.initialize() )
    TS_ASSERT( copyMapping.isInitialized() )
  }

  void testAdd()
  {
    using namespace Mantid;
    using namespace Mantid::API;
    using namespace Mantid::Kernel;

    auto toMatch = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    // Set the detector map for a spectra in the to match workspace
    std::set<detid_t> detIDs;
    detIDs.insert(5);
    detIDs.insert(9);
    detIDs.insert(6);
    detIDs.insert(2);
    toMatch->getSpectrum(0)->setDetectorIDs(detIDs);

    // Add workspaces to ADS
    AnalysisDataService::Instance().add("to_match", toMatch);
    AnalysisDataService::Instance().add("to_remap", WorkspaceCreationHelper::Create2DWorkspace(10, 10));

    // Run algorithm
    TS_ASSERT_THROWS_NOTHING( copyMapping.setPropertyValue("WorkspaceToMatch", "to_match") );
    TS_ASSERT_THROWS_NOTHING( copyMapping.setPropertyValue("WorkspaceToRemap", "to_remap") );

    TS_ASSERT_THROWS_NOTHING( copyMapping.execute() );
    TS_ASSERT( copyMapping.isExecuted() );

    // Check the detector map in the to remap workspace matches that of the to match workspace
    MatrixWorkspace_const_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<MatrixWorkspace>
                                (AnalysisDataService::Instance().retrieve("to_remap")) );
    std::set<detid_t> resultDetIDs = result->getSpectrum(0)->getDetectorIDs();
    TS_ASSERT( detIDs == resultDetIDs );

    // Clean up workspace
    AnalysisDataService::Instance().remove("to_match");
    AnalysisDataService::Instance().remove("to_remap");
  }

private:
  Mantid::Algorithms::CopyDetectorMapping copyMapping;

};

#endif /*COPYDETECTORMAPPINGTEST_H_*/
