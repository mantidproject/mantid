#ifndef MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_
#define MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_

#include <cxxtest/TestSuite.h>

#include "ClusterIntegrationBaseTest.h"
#include "MantidCrystal/IntegratePeaksHybrid.h"

#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/WorkspaceGroup.h"

#include <boost/tuple/tuple.hpp>


using namespace Mantid::Crystal;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

namespace
{
  typedef boost::tuple<WorkspaceGroup_sptr, IPeaksWorkspace_sptr> AlgorithmOutputs;

  // Execute the clustering integration algorithm
  AlgorithmOutputs execute_integration(const MDEventPeaksWSTuple& inputWorkspaces, const double& backgroundOuterRadius, const int& numberOfBins)
  {
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    // ------- Integrate the fake data
    IntegratePeaksHybrid alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", mdWS);
    alg.setProperty("PeaksWorkspace", peaksWS);
    alg.setProperty("BackgroundOuterRadius", backgroundOuterRadius);
    alg.setProperty("NumberOfBins", numberOfBins);
    alg.setPropertyValue("OutputWorkspace", "out_ws");
    alg.setPropertyValue("OutputWorkspaces", "out_ws_md");
    alg.execute();
    // ------- Get the integrated results
    IPeaksWorkspace_sptr outPeaksWS = alg.getProperty("OutputWorkspace");
    WorkspaceGroup_sptr outClustersWS = alg.getProperty("OutputWorkspaces");
    return AlgorithmOutputs(outClustersWS, outPeaksWS);
  }
}

//=====================================================================================
// Functional Tests
//=====================================================================================
class IntegratePeaksHybridTest: public CxxTest::TestSuite, public ClusterIntegrationBaseTest
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksHybridTest *createSuite()
  {
    return new IntegratePeaksHybridTest();
  }
  static void destroySuite(IntegratePeaksHybridTest *suite)
  {
    delete suite;
  }

  void test_Init()
  {
    IntegratePeaksHybrid alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_n_bins_mustbe_greater_than_zero()
  {
    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("NumberOfBins", -1), std::invalid_argument&);
  }

  void test_radius_mustbe_greater_than_zero()
  {
    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("BackgroundOuterRadius", -1.0), std::invalid_argument&);
  }

  void test_peaks_workspace_mandatory()
  {
    IMDEventWorkspace_sptr mdws = boost::static_pointer_cast<IMDEventWorkspace>( MDEventsTestHelper::makeMDEW<3>(10, 0, 10) );

    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", mdws);
    alg.setProperty("BackgroundOuterRadius", 1.0);
    alg.setPropertyValue("OutputWorkspaces", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    TSM_ASSERT_THROWS("PeaksWorkspace required", alg.execute(), std::runtime_error&);
  }

  void test_input_md_workspace_mandatory()
  {
    auto peaksws = WorkspaceCreationHelper::createPeaksWorkspace();

    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PeaksWorkspace", peaksws);
    alg.setPropertyValue("OutputWorkspaces", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    alg.setProperty("BackgroundOuterRadius", 1.0);
    TSM_ASSERT_THROWS("InputWorkspace required", alg.execute(), std::runtime_error&);
  }

  void test_outer_radius_mandatory()
  {
    auto peaksws = WorkspaceCreationHelper::createPeaksWorkspace();

    IMDEventWorkspace_sptr mdws = boost::static_pointer_cast<IMDEventWorkspace>( MDEventsTestHelper::makeMDEW<3>(10, 0, 10) );

    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PeaksWorkspace", peaksws);
    alg.setProperty("InputWorkspace", mdws);
    alg.setPropertyValue("OutputWorkspaces", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    TSM_ASSERT_THROWS("InputWorkspace required", alg.execute(), std::runtime_error&);
  }

  void test_throw_if_special_coordinates_unknown()
  {
    auto peaksws = WorkspaceCreationHelper::createPeaksWorkspace();
    IMDEventWorkspace_sptr mdws = boost::static_pointer_cast<IMDEventWorkspace>( MDEventsTestHelper::makeMDEW<3>(10, 0, 10) );

    IntegratePeaksHybrid alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", mdws);
    alg.setProperty("PeaksWorkspace", peaksws);
    alg.setPropertyValue("OutputWorkspaces", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    alg.setProperty("BackgroundOuterRadius", 0.01);
    TSM_ASSERT_THROWS("Unknown special coordinates", alg.execute(), std::invalid_argument&);
  }

  void test_integrate_single_peak()
  {
    // ------- Make the fake input
    std::vector<V3D> hklValues;
    // Add a single peak.
    hklValues.push_back(V3D(2,2,2));
    const double peakRadius = 1;
    const double backgroundOuterRadius = peakRadius * 3;
    const size_t nBins = 10;
    const size_t nEventsInPeak = 10000;
    MDEventPeaksWSTuple inputWorkspaces = make_peak_and_mdew(hklValues, -10, 10, peakRadius, nEventsInPeak);
    //-------- Execute the integratioin
    AlgorithmOutputs integratedWorkspaces = execute_integration(inputWorkspaces, backgroundOuterRadius, nBins);
    // ------- Get the integrated results
    WorkspaceGroup_sptr outClustersWorkspaces = integratedWorkspaces.get<0>();
    IPeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    TSM_ASSERT_EQUALS("Expect one output image", 1, outClustersWorkspaces->size());

    IMDHistoWorkspace_sptr outClustersWS = boost::dynamic_pointer_cast<IMDHistoWorkspace>( outClustersWorkspaces->getItem(0) );

    // ------- Check the results.
    // Basic checks
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    TS_ASSERT_EQUALS(outPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
    TS_ASSERT_EQUALS(nBins * nBins * nBins,  outClustersWS->getNPoints());
    // Check clusters by extracting unique label ids.
    std::set<Mantid::signal_t> labelIds;
    for(size_t i = 0; i < outClustersWS->getNPoints(); ++i)
    {
      labelIds.insert(outClustersWS->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Only one peak present, so should only have two unique label ids", 2, labelIds.size());

    TSM_ASSERT_DELTA("Integrated intensity should be almost the same as original peak intensity", outPeaksWS->getPeak(0).getIntensity(), nEventsInPeak, 300);
    TSM_ASSERT_DELTA("Integrated error should be almost the same as original peak intensity error", outPeaksWS->getPeak(0).getSigmaIntensity(), nEventsInPeak, 300);

    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds, 0));
  }

};

#endif /* MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_ */
