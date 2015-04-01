#ifndef MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_
#define MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_

#include <cxxtest/TestSuite.h>

#include "ClusterIntegrationBaseTest.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/IntegratePeaksHybrid.h"

#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/WorkspaceGroup.h"

#include <boost/tuple/tuple.hpp>

using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

namespace
{
  typedef boost::tuple<WorkspaceGroup_sptr, IPeaksWorkspace_sptr> AlgorithmOutputs;

  // Execute the clustering integration algorithm
  AlgorithmOutputs execute_integration(const MDEventPeaksWSTuple& inputWorkspaces,
      const double& backgroundOuterRadius, const int& numberOfBins)
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

  IntegratePeaksHybridTest()
  {
    FrameworkManager::Instance();
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
    IMDEventWorkspace_sptr mdws = boost::static_pointer_cast<IMDEventWorkspace>(
        MDEventsTestHelper::makeMDEW<3>(10, 0, 10));

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

    IMDEventWorkspace_sptr mdws = boost::static_pointer_cast<IMDEventWorkspace>(
        MDEventsTestHelper::makeMDEW<3>(10, 0, 10));

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
    IMDEventWorkspace_sptr mdws = boost::static_pointer_cast<IMDEventWorkspace>(
        MDEventsTestHelper::makeMDEW<3>(10, 0, 10));

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
    hklValues.push_back(V3D(2, 2, 2));
    const double peakRadius = 1;
    const double backgroundOuterRadius = peakRadius * 3;
    const size_t nBins = 10;
    const size_t nEventsInPeak = 10000;
    MDEventPeaksWSTuple inputWorkspaces = make_peak_and_mdew(hklValues, -10, 10, peakRadius,
        nEventsInPeak);
    //-------- Execute the integration
    AlgorithmOutputs integratedWorkspaces = execute_integration(inputWorkspaces, backgroundOuterRadius,
        nBins);
    // ------- Get the integrated results
    WorkspaceGroup_sptr outClustersWorkspaces = integratedWorkspaces.get<0>();
    IPeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    TSM_ASSERT_EQUALS("Expect one output image", 1, outClustersWorkspaces->size());

    IMDHistoWorkspace_sptr outClustersWS = boost::dynamic_pointer_cast<IMDHistoWorkspace>(
        outClustersWorkspaces->getItem(0));

    // ------- Check the results.
    // Basic checks
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    TS_ASSERT_EQUALS(outPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
    TS_ASSERT_EQUALS(nBins * nBins * nBins, outClustersWS->getNPoints());
    // Check clusters by extracting unique label ids.
    std::set<Mantid::signal_t> labelIds;
    for (size_t i = 0; i < outClustersWS->getNPoints(); ++i)
    {
      labelIds.insert(outClustersWS->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Only one peak present, so should only have two unique label ids", 2,
        labelIds.size());

    TSM_ASSERT_DELTA("Integrated intensity should be almost the same as original peak intensity",
        outPeaksWS->getPeak(0).getIntensity(), nEventsInPeak, 300);
    TSM_ASSERT_DELTA("Integrated error should be almost the same as original peak intensity error",
        outPeaksWS->getPeak(0).getSigmaIntensity(), std::sqrt( nEventsInPeak ), 300);

    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds, 0));
  }

  void test_integrate_with_differnent_outer_radius()
  {
    // ------- Make the fake input
    std::vector<V3D> hklValues;
    // Add a single peak.
    hklValues.push_back(V3D(2, 2, 2));
    const double peakRadius = 1;
    const size_t nBins = 10;
    const size_t nEventsInPeak = 10000;

    MDEventPeaksWSTuple inputWorkspaces = make_peak_and_mdew(hklValues, -10, 10, peakRadius,
        nEventsInPeak);

    //-------- Execute the integration. Tight radius, so Background threshold will be very high. As a result, integrated value should be low.
    AlgorithmOutputs integratedWorkspaces1 = execute_integration(inputWorkspaces, peakRadius * 1.5,
        nBins);
    //-------- Execute the integration. Less conservative radius.
    AlgorithmOutputs integratedWorkspaces2 = execute_integration(inputWorkspaces, peakRadius * 2.5,
        nBins);

    //-------- Execute the integration. Liberal radius.
    AlgorithmOutputs integratedWorkspaces3 = execute_integration(inputWorkspaces, peakRadius * 3.5,
        nBins);

    IPeaksWorkspace_sptr outPeaksWS1 = integratedWorkspaces1.get<1>();
    IPeaksWorkspace_sptr outPeaksWS2 = integratedWorkspaces2.get<1>();
    IPeaksWorkspace_sptr outPeaksWS3 = integratedWorkspaces3.get<1>();

    TSM_ASSERT("Conservative intensities should lead to lower integrated values.",
        outPeaksWS1->getPeak(0).getIntensity() <  outPeaksWS2->getPeak(0).getIntensity());

    TSM_ASSERT("Conservative intensities should lead to lower integrated values.",
            outPeaksWS2->getPeak(0).getIntensity() <  outPeaksWS3->getPeak(0).getIntensity());

  }

  void test_integrate_two_separate_but_identical_peaks()
  {
    // ------- Make the fake input
    std::vector<V3D> hklValues;
    // Add a single peak.
    hklValues.push_back(V3D(2, 2, 2));
    // Add another peak
    hklValues.push_back(V3D(5, 5, 5));

    const double peakRadius = 1;
    const double backgroundOuterRadius = peakRadius * 3;
    const size_t nBins = 10;
    const size_t nEventsInPeak = 10000;
    MDEventPeaksWSTuple inputWorkspaces = make_peak_and_mdew(hklValues, -10, 10, peakRadius,
        nEventsInPeak);
    //-------- Execute the integration
    AlgorithmOutputs integratedWorkspaces = execute_integration(inputWorkspaces, backgroundOuterRadius,
        nBins);
    // ------- Get the integrated results
    WorkspaceGroup_sptr outClustersWorkspaces = integratedWorkspaces.get<0>();
    IPeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    TSM_ASSERT_EQUALS("Expect two output images", 2, outClustersWorkspaces->size());

    IMDHistoWorkspace_sptr outClustersWS1 = boost::dynamic_pointer_cast<IMDHistoWorkspace>(
        outClustersWorkspaces->getItem(0));

    IMDHistoWorkspace_sptr outClustersWS2 = boost::dynamic_pointer_cast<IMDHistoWorkspace>(
        outClustersWorkspaces->getItem(1));

    // ------- Check the results.
    // Basic checks
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    TS_ASSERT_EQUALS(outPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
    TS_ASSERT_EQUALS(nBins * nBins * nBins, outClustersWS1->getNPoints());
    TS_ASSERT_EQUALS(nBins * nBins * nBins, outClustersWS2->getNPoints());
    // Check clusters by extracting unique label ids.
    std::set<Mantid::signal_t> labelIds1;
    for (size_t i = 0; i < outClustersWS1->getNPoints(); ++i)
    {
      labelIds1.insert(outClustersWS1->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Only one peak present in the region, so should only have two unique label ids", 2,
        labelIds1.size());

    std::set<Mantid::signal_t> labelIds2;
    for (size_t i = 0; i < outClustersWS2->getNPoints(); ++i)
    {
      labelIds2.insert(outClustersWS2->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Only one peak present in the region, so should only have two unique label ids", 2,
        labelIds2.size());

    TSM_ASSERT_DELTA("Integrated intensity should be almost the same as original peak intensity",
        outPeaksWS->getPeak(0).getIntensity(), nEventsInPeak, 300);
    TSM_ASSERT_DELTA("Integrated error should be almost the same as original peak intensity error",
        outPeaksWS->getPeak(0).getSigmaIntensity(), std::sqrt( nEventsInPeak ), 300);

    TSM_ASSERT_EQUALS("Peaks are identical, so integrated values should be identical",
        outPeaksWS->getPeak(0).getIntensity(), outPeaksWS->getPeak(1).getIntensity());
    TSM_ASSERT_EQUALS("Peaks are identical, so integrated error values should be identical",
        outPeaksWS->getPeak(0).getSigmaIntensity(), outPeaksWS->getPeak(1).getSigmaIntensity());

    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds1, 0));
    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds2, 0));
  }

  void test_integrate_two_peaks_of_different_magnitude()
  {
    // ------- Make the fake input
    std::vector<V3D> hklValues;
    // Add a single peak.
    hklValues.push_back(V3D(2, 2, 2));
    // Add another peak
    hklValues.push_back(V3D(5, 5, 5));

    const std::vector<double> peakRadiusVec(2, 1.0);
    const double backgroundOuterRadius = peakRadiusVec[0] * 3;
    const size_t nBins = 10;
    std::vector<size_t> nEventsInPeakVec;
    nEventsInPeakVec.push_back(10000);
    nEventsInPeakVec.push_back(20000); // Second peak has DOUBLE the intensity of the firse one.

    MDEventPeaksWSTuple inputWorkspaces = make_peak_and_mdew(hklValues, -10, 10, peakRadiusVec,
        nEventsInPeakVec);
    //-------- Execute the integration
    AlgorithmOutputs integratedWorkspaces = execute_integration(inputWorkspaces, backgroundOuterRadius,
        nBins);
    // ------- Get the integrated results
    WorkspaceGroup_sptr outClustersWorkspaces = integratedWorkspaces.get<0>();
    IPeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    TSM_ASSERT_EQUALS("Expect two output images", 2, outClustersWorkspaces->size());

    IMDHistoWorkspace_sptr outClustersWS1 = boost::dynamic_pointer_cast<IMDHistoWorkspace>(
        outClustersWorkspaces->getItem(0));

    IMDHistoWorkspace_sptr outClustersWS2 = boost::dynamic_pointer_cast<IMDHistoWorkspace>(
        outClustersWorkspaces->getItem(1));

    // ------- Check the results.
    // Basic checks
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    TS_ASSERT_EQUALS(outPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
    TS_ASSERT_EQUALS(nBins * nBins * nBins, outClustersWS1->getNPoints());
    TS_ASSERT_EQUALS(nBins * nBins * nBins, outClustersWS2->getNPoints());
    // Check clusters by extracting unique label ids.
    std::set<Mantid::signal_t> labelIds1;
    for (size_t i = 0; i < outClustersWS1->getNPoints(); ++i)
    {
      labelIds1.insert(outClustersWS1->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Only one peak present in the region, so should only have two unique label ids", 2,
        labelIds1.size());

    std::set<Mantid::signal_t> labelIds2;
    for (size_t i = 0; i < outClustersWS2->getNPoints(); ++i)
    {
      labelIds2.insert(outClustersWS2->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Only one peak present in the region, so should only have two unique label ids", 2,
        labelIds2.size());

    TSM_ASSERT_DELTA("Second peak is twice as 'bright'", outPeaksWS->getPeak(0).getIntensity() * 2,
        outPeaksWS->getPeak(1).getIntensity(), 100);

    TSM_ASSERT_DELTA("Second peak is twice as 'bright'", outPeaksWS->getPeak(0).getSigmaIntensity() * 2,
        outPeaksWS->getPeak(1).getSigmaIntensity(), 100);
  }

};

//=====================================================================================
// Performance Tests
//=====================================================================================
class IntegratePeaksHybridTestPerformance: public CxxTest::TestSuite, public ClusterIntegrationBaseTest
{

private:

  // Input data
  MDEventPeaksWSTuple m_inputWorkspaces;
  int m_nBins;
  double m_backgroundOuterRadius;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksHybridTestPerformance *createSuite()
  {
    return new IntegratePeaksHybridTestPerformance();
  }
  static void destroySuite(IntegratePeaksHybridTestPerformance *suite)
  {
    delete suite;
  }
  IntegratePeaksHybridTestPerformance()
  {
    FrameworkManager::Instance();

    std::vector<V3D> hklValues;
    for(double i = -10; i < 10; i+=4)
    {
      for(double j = -10; j < 10; j+=4)
      {
        for(double k = -10; k < 10; k+=4)
        {
          hklValues.push_back(V3D(i,j,k));
        }
      }
    }

    double peakRadius = 1;
    m_backgroundOuterRadius = peakRadius * 3;
    m_nBins = 5;
    const size_t nEventsInPeak = 1000;
    m_inputWorkspaces = make_peak_and_mdew(hklValues, -10, 10, peakRadius, nEventsInPeak);

  }

  void test_execute()
  {
    // Just run the integration. Functional tests handled in separate suite.
    execute_integration(m_inputWorkspaces, m_backgroundOuterRadius, m_nBins);
  }

};


#endif /* MANTID_CRYSTAL_INTEGRATEPEAKSHYBRIDTEST_H_ */
