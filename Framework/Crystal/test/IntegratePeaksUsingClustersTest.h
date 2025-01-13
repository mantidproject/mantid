// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ClusterIntegrationBaseTest.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/IntegratePeaksUsingClusters.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Crystal;

namespace {

// Execute the clustering integration algorithm
MDHistoPeaksWSTuple execute_integration(const MDHistoPeaksWSTuple &inputWorkspaces, const double &threshold) {
  auto mdWS = inputWorkspaces.get<0>();
  auto peaksWS = inputWorkspaces.get<1>();
  // ------- Integrate the fake data
  IntegratePeaksUsingClusters alg;
  alg.initialize();
  alg.setRethrows(true);
  alg.setChild(true);
  alg.setProperty("InputWorkspace", mdWS);
  alg.setProperty("PeaksWorkspace", peaksWS);
  alg.setProperty("Threshold", threshold);
  alg.setProperty("Normalization", "NoNormalization");
  alg.setPropertyValue("OutputWorkspace", "out_ws");
  alg.setPropertyValue("OutputWorkspaceMD", "out_ws_md");
  alg.execute();
  // ------- Get the integrated results
  PeaksWorkspace_sptr outPeaksWS = alg.getProperty("OutputWorkspace");
  IMDHistoWorkspace_sptr outClustersWS = alg.getProperty("OutputWorkspaceMD");
  return MDHistoPeaksWSTuple(outClustersWS, outPeaksWS);
}
} // namespace

//=====================================================================================
// Functional Tests
//=====================================================================================
class IntegratePeaksUsingClustersTest : public CxxTest::TestSuite, public ClusterIntegrationBaseTest {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksUsingClustersTest *createSuite() { return new IntegratePeaksUsingClustersTest(); }
  static void destroySuite(IntegratePeaksUsingClustersTest *suite) { delete suite; }

  IntegratePeaksUsingClustersTest() { FrameworkManager::Instance(); }

  void test_Init() {
    IntegratePeaksUsingClusters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_peaks_workspace_mandatory() {
    IMDHistoWorkspace_sptr mdws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1);

    IntegratePeaksUsingClusters alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", mdws);
    alg.setPropertyValue("OutputWorkspaceMD", "out_md");
    alg.setProperty("Threshold", 0.01);
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    TSM_ASSERT_THROWS("PeaksWorkspace required", alg.execute(), std::runtime_error &);
  }

  void test_input_md_workspace_mandatory() {
    auto peaksws = WorkspaceCreationHelper::createPeaksWorkspace(2);

    IntegratePeaksUsingClusters alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("PeaksWorkspace", peaksws);
    alg.setPropertyValue("OutputWorkspaceMD", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    alg.setProperty("Threshold", 0.01);
    TSM_ASSERT_THROWS("InputWorkspace required", alg.execute(), std::runtime_error &);
  }

  void test_throw_if_special_coordinates_unknown() {
    auto peaksws = WorkspaceCreationHelper::createPeaksWorkspace(2);
    IMDHistoWorkspace_sptr mdws = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1);

    IntegratePeaksUsingClusters alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", mdws);
    alg.setProperty("PeaksWorkspace", peaksws);
    alg.setPropertyValue("OutputWorkspaceMD", "out_md");
    alg.setPropertyValue("OutputWorkspace", "out_peaks");
    alg.setProperty("Threshold", 0.01);
    TSM_ASSERT_THROWS("Unknown special coordinates", alg.execute(), std::invalid_argument &);
  }

  void test_threshold_too_high_gives_no_peaks() {
    // ------- Make the fake input
    std::vector<V3D> hklValues{{2, 2, 2}};
    const double peakRadius = 1;
    const double threshold = 10000; // Threshold will filter out everything
                                    // given the nEventsInPeak restriction.
    const size_t nEventsInPeak = 10000;
    MDHistoPeaksWSTuple inputWorkspaces = make_peak_and_md_ws(hklValues, -10, 10, peakRadius, nEventsInPeak);
    //-------- Execute the integratioin
    MDHistoPeaksWSTuple integratedWorkspaces = execute_integration(inputWorkspaces, threshold);
    // ------- Get the integrated results
    IMDHistoWorkspace_sptr outClustersWS = integratedWorkspaces.get<0>();
    PeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    std::unordered_set<Mantid::signal_t> labelIds;
    for (size_t i = 0; i < outClustersWS->getNPoints(); ++i) {
      labelIds.insert(outClustersWS->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Should only have one type of label", 1, labelIds.size());
    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds, 0));

    TSM_ASSERT_EQUALS("Integrated intensity should be zero since no integration has occured", 0,
                      outPeaksWS->getPeak(0).getIntensity());
    TSM_ASSERT_EQUALS("Integrated intensity should be zero since no integration has occured", 0,
                      outPeaksWS->getPeak(0).getSigmaIntensity());
  }

  void test_integrate_single_peak() {
    // ------- Make the fake input
    std::vector<V3D> hklValues{{2, 2, 2}};
    const double peakRadius = 1;
    const double threshold = 100;
    const size_t nEventsInPeak = 10000;
    MDHistoPeaksWSTuple inputWorkspaces = make_peak_and_md_ws(hklValues, -10, 10, peakRadius, nEventsInPeak);
    //-------- Execute the integratioin
    MDHistoPeaksWSTuple integratedWorkspaces = execute_integration(inputWorkspaces, threshold);
    // ------- Get the integrated results
    IMDHistoWorkspace_sptr outClustersWS = integratedWorkspaces.get<0>();
    PeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    // ------- Check the results.
    // Basic checks
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    TS_ASSERT_EQUALS(outPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
    TS_ASSERT_EQUALS(mdWS->getNPoints(), outClustersWS->getNPoints());
    // Check clusters by extracting unique label ids.
    std::unordered_set<Mantid::signal_t> labelIds;
    for (size_t i = 0; i < outClustersWS->getNPoints(); ++i) {
      labelIds.insert(outClustersWS->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("Only one peak present, so should only have two unique label ids", 2, labelIds.size());

    TSM_ASSERT_EQUALS("Integrated intensity should be same as original peak intensity",
                      outPeaksWS->getPeak(0).getIntensity(), nEventsInPeak);
    TSM_ASSERT_EQUALS("Integrated error should same as original peak intensity error",
                      outPeaksWS->getPeak(0).getSigmaIntensity(), std::sqrt(nEventsInPeak));

    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds, 0));
  }

  void test_integrate_two_separate_but_identical_peaks() {
    // ------- Make the fake input
    // Add several peaks. These are NOT overlapping.
    std::vector<V3D> hklValues{{1, 1, 1}, {6, 6, 6}};
    const double peakRadius = 1;
    const double threshold = 100;
    const size_t nEventsInPeak = 10000;
    MDHistoPeaksWSTuple inputWorkspaces = make_peak_and_md_ws(hklValues, -10, 10, peakRadius, nEventsInPeak);
    //-------- Execute the integratioin
    MDHistoPeaksWSTuple integratedWorkspaces = execute_integration(inputWorkspaces, threshold);
    // ------- Get the integrated results
    IMDHistoWorkspace_sptr outClustersWS = integratedWorkspaces.get<0>();
    PeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    // ------- Check the results.
    // Basic checks
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    TS_ASSERT_EQUALS(outPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
    TS_ASSERT_EQUALS(mdWS->getNPoints(), outClustersWS->getNPoints());
    // Check clusters by extracting unique label ids.
    std::unordered_set<Mantid::signal_t> labelIds;
    for (size_t i = 0; i < outClustersWS->getNPoints(); ++i) {
      labelIds.insert(outClustersWS->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("N peaks present, so should only have n+1 unique label ids", 3, labelIds.size());

    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds, 0));

    // Two peaks are identical, so integrated values should be the same.
    TSM_ASSERT_EQUALS("Integrated intensity should be same as original peak intensity",
                      outPeaksWS->getPeak(0).getIntensity(), nEventsInPeak);
    TSM_ASSERT_EQUALS("Integrated error should same as original peak intensity error",
                      outPeaksWS->getPeak(0).getSigmaIntensity(), std::sqrt(nEventsInPeak));
    TSM_ASSERT_EQUALS("Peaks are identical, so integrated values should be identical",
                      outPeaksWS->getPeak(0).getIntensity(), outPeaksWS->getPeak(1).getIntensity());
    TSM_ASSERT_EQUALS("Peaks are identical, so integrated error values should be identical",
                      outPeaksWS->getPeak(0).getSigmaIntensity(), outPeaksWS->getPeak(1).getSigmaIntensity());
  }

  void test_integrate_two_peaks_of_different_magnitude() {
    // ------- Make the fake input
    // Add several peaks. These are NOT overlapping.
    std::vector<V3D> hklValues{{1, 1, 1}, {6, 6, 6}};
    const double peakRadius = 1;
    const double threshold = 100;
    std::vector<size_t> nEventsInPeakVec;
    nEventsInPeakVec.emplace_back(10000);
    nEventsInPeakVec.emplace_back(20000); // Second peak has DOUBLE the intensity of the firse one.

    MDHistoPeaksWSTuple inputWorkspaces =
        make_peak_and_md_ws(hklValues, -10, 10, std::vector<double>(hklValues.size(), peakRadius), nEventsInPeakVec);
    //-------- Execute the integration
    MDHistoPeaksWSTuple integratedWorkspaces = execute_integration(inputWorkspaces, threshold);
    // ------- Get the integrated results
    IMDHistoWorkspace_sptr outClustersWS = integratedWorkspaces.get<0>();
    PeaksWorkspace_sptr outPeaksWS = integratedWorkspaces.get<1>();

    // ------- Check the results.
    // Basic checks
    auto mdWS = inputWorkspaces.get<0>();
    auto peaksWS = inputWorkspaces.get<1>();
    TS_ASSERT_EQUALS(outPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
    TS_ASSERT_EQUALS(mdWS->getNPoints(), outClustersWS->getNPoints());
    // Check clusters by extracting unique label ids.
    std::unordered_set<Mantid::signal_t> labelIds;
    for (size_t i = 0; i < outClustersWS->getNPoints(); ++i) {
      labelIds.insert(outClustersWS->getSignalAt(i));
    }
    TSM_ASSERT_EQUALS("N peaks present, so should only have n+1 unique label ids", 3, labelIds.size());

    TSM_ASSERT("Should have 'empy' label", does_contain(labelIds, 0));

    // Two peaks are identical, so integrated values should be the same.
    TSM_ASSERT_EQUALS("Integrated intensity should be same as original peak intensity",
                      outPeaksWS->getPeak(0).getIntensity(), nEventsInPeakVec[0]);
    TSM_ASSERT_EQUALS("Integrated error should same as original peak intensity error",
                      outPeaksWS->getPeak(0).getSigmaIntensity(), std::sqrt(nEventsInPeakVec[0]));

    TSM_ASSERT_EQUALS("Second peak is twice as 'bright'", outPeaksWS->getPeak(0).getIntensity() * 2,
                      outPeaksWS->getPeak(1).getIntensity());
    TSM_ASSERT_EQUALS("Second peak is twice as 'bright'", std::pow(outPeaksWS->getPeak(0).getSigmaIntensity(), 2) * 2,
                      std::pow(outPeaksWS->getPeak(1).getSigmaIntensity(), 2));
  }
};

//=====================================================================================
// Performance Tests
//=====================================================================================
class IntegratePeaksUsingClustersTestPerformance : public CxxTest::TestSuite, public ClusterIntegrationBaseTest {

private:
  // Input data
  MDHistoPeaksWSTuple m_inputWorkspaces;
  double m_peakRadius;
  double m_threshold;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksUsingClustersTestPerformance *createSuite() {
    return new IntegratePeaksUsingClustersTestPerformance();
  }
  static void destroySuite(IntegratePeaksUsingClustersTestPerformance *suite) { delete suite; }

  IntegratePeaksUsingClustersTestPerformance() {
    FrameworkManager::Instance();

    std::vector<V3D> hklValues;
    for (double i = -10; i < 10; i += 4) {
      for (double j = -10; j < 10; j += 4) {
        for (double k = -10; k < 10; k += 4) {
          hklValues.emplace_back(i, j, k);
        }
      }
    }

    m_peakRadius = 1;
    m_threshold = 10;
    const size_t nEventsInPeak = 1000;
    m_inputWorkspaces = make_peak_and_md_ws(hklValues, -10, 10, m_peakRadius, nEventsInPeak, 50);
  }

  void test_execute() {
    // Just run the integration. Functional tests handled in separate suite.
    execute_integration(m_inputWorkspaces, m_threshold);
  }
};
