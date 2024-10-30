// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/V3D.h"
#include "MantidSINQ/PoldiIndexKnownCompounds.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid::Poldi;
using namespace Mantid::API;

class PoldiIndexKnownCompoundsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiIndexKnownCompoundsTest *createSuite() { return new PoldiIndexKnownCompoundsTest(); }
  static void destroySuite(PoldiIndexKnownCompoundsTest *suite) { delete suite; }

  PoldiIndexKnownCompoundsTest() { FrameworkManager::Instance(); }

  void test_Init() {
    PoldiIndexKnownCompounds alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void testExecOnePhase() {
    /* In this test, peaks from PoldiMockInstrumentHelpers (Silicon)
     * are indexed using theoretical Si-peaks.
     */
    auto wsMeasuredSi = PoldiPeakCollectionHelpers::createPoldiPeakTableWorkspace();
    WorkspaceCreationHelper::storeWS("measured_SI", wsMeasuredSi);
    auto wsSi = PoldiPeakCollectionHelpers::createTheoreticalPeakCollectionSilicon()->asTableWorkspace();
    WorkspaceCreationHelper::storeWS("Si", wsSi);

    std::string outWSName("PoldiIndexKnownCompoundsTest_OutputWS");

    PoldiIndexKnownCompounds alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "measured_SI"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CompoundWorkspaces", "Si"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerances", "0.005"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ScatteringContributions", "1.0"));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Workspace is a group
    WorkspaceGroup_sptr group = std::dynamic_pointer_cast<WorkspaceGroup>(ws);
    TS_ASSERT(group);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // Has two entries - indexed Si peaks and unindexed peaks
    Workspace_sptr indexedSi = group->getItem(0);
    TS_ASSERT(indexedSi);

    // Table with peaks that can be attributed to Si
    ITableWorkspace_sptr tableSi = std::dynamic_pointer_cast<ITableWorkspace>(indexedSi);
    TS_ASSERT(tableSi);
    TS_ASSERT_EQUALS(tableSi->getName(), "measured_SI_indexed_Si");
    TS_ASSERT_EQUALS(tableSi->rowCount(), 4);

    // Make sure unit cell and point group is carried over from compound ws
    ITableWorkspace_sptr si = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Si");
    TS_ASSERT_EQUALS(tableSi->getLogs()->getProperty("PointGroup")->value(),
                     si->getLogs()->getProperty("PointGroup")->value());
    TS_ASSERT_EQUALS(tableSi->getLogs()->getProperty("UnitCell")->value(),
                     si->getLogs()->getProperty("UnitCell")->value());

    Workspace_sptr unindexed = group->getItem(1);
    TS_ASSERT(unindexed);

    // Table with peaks that can be attributed to Si
    ITableWorkspace_sptr tableUnindexed = std::dynamic_pointer_cast<ITableWorkspace>(unindexed);
    TS_ASSERT(tableUnindexed);
    TS_ASSERT_EQUALS(tableUnindexed->getName(), "measured_SI_unindexed");
    TS_ASSERT_EQUALS(tableUnindexed->rowCount(), 0);

    AnalysisDataService::Instance().remove(outWSName);
    WorkspaceCreationHelper::removeWS("measured_SI");
    WorkspaceCreationHelper::removeWS("Si");
  }

  void testSetMeasuredPeaks() {
    PoldiPeakCollection_sptr silicon = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();

    TestablePoldiIndexKnownCompounds alg;
    alg.setMeasuredPeaks(silicon);

    TS_ASSERT_EQUALS(alg.m_measuredPeaks, silicon)
  }

  void testSetExpectedPhases() {
    std::vector<PoldiPeakCollection_sptr> expectedPeaks;
    expectedPeaks.emplace_back(PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum());
    expectedPeaks.emplace_back(PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum());

    TestablePoldiIndexKnownCompounds alg;
    alg.setExpectedPhases(expectedPeaks);

    TS_ASSERT_EQUALS(alg.m_expectedPhases.size(), expectedPeaks.size());
    for (size_t i = 0; i < expectedPeaks.size(); ++i) {
      TS_ASSERT_EQUALS(alg.m_expectedPhases[i], expectedPeaks[i]);
    }
  }

  void testInitializeUnindexedPeaks() {
    TestablePoldiIndexKnownCompounds alg;
    TS_ASSERT(!alg.m_unindexedPeaks);

    alg.initializeUnindexedPeaks();

    TS_ASSERT(alg.m_unindexedPeaks);
  }

  void testInitializeIndexedPeaks() {
    std::vector<PoldiPeakCollection_sptr> expectedPeaks;
    expectedPeaks.emplace_back(PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum());
    expectedPeaks.emplace_back(PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum());

    TestablePoldiIndexKnownCompounds alg;
    TS_ASSERT_EQUALS(alg.m_indexedPeaks.size(), 0);

    TS_ASSERT_THROWS(alg.initializeIndexedPeaks(expectedPeaks), const std::runtime_error &);

    PoldiPeakCollection_sptr peaks = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();
    alg.setMeasuredPeaks(peaks);

    TS_ASSERT_THROWS_NOTHING(alg.initializeIndexedPeaks(expectedPeaks));

    TS_ASSERT_EQUALS(alg.m_indexedPeaks.size(), expectedPeaks.size());
    for (size_t i = 0; i < expectedPeaks.size(); ++i) {
      TS_ASSERT(alg.m_indexedPeaks[i]);
      TS_ASSERT_EQUALS(alg.m_indexedPeaks[i]->getProfileFunctionName(), peaks->getProfileFunctionName());
      TS_ASSERT_EQUALS(alg.m_indexedPeaks[i]->intensityType(), peaks->intensityType());
    }
  }

  void testGetWorkspaces() {
    setupWorkspaceStructure();

    TestablePoldiIndexKnownCompounds alg;

    // test single workspaces
    std::vector<std::string> singleWsNames = getStringVector("test1,test3,test4");

    TS_ASSERT_THROWS_NOTHING(alg.getWorkspaces(singleWsNames));
    std::vector<Workspace_sptr> singleWs = alg.getWorkspaces(singleWsNames);
    TS_ASSERT_EQUALS(singleWs.size(), 3);

    // test group workspace
    std::vector<std::string> groupWsName = getStringVector("group1");

    TS_ASSERT_THROWS_NOTHING(alg.getWorkspaces(groupWsName));
    std::vector<Workspace_sptr> groupWs = alg.getWorkspaces(groupWsName);
    TS_ASSERT_EQUALS(groupWs.size(), 2);

    // test 2 group workspaces
    std::vector<std::string> groupWsNames = getStringVector("group1,group2");

    TS_ASSERT_THROWS_NOTHING(alg.getWorkspaces(groupWsNames));
    std::vector<Workspace_sptr> groupWs2 = alg.getWorkspaces(groupWsNames);
    TS_ASSERT_EQUALS(groupWs2.size(), 4);

    // test group of group workspaces
    std::vector<std::string> groupsWsName = getStringVector("group3");

    TS_ASSERT_THROWS_NOTHING(alg.getWorkspaces(groupsWsName));
    std::vector<Workspace_sptr> groupsWs = alg.getWorkspaces(groupsWsName);
    TS_ASSERT_EQUALS(groupsWs.size(), 4);

    tearDownWorkspaceStructure();
  }

  void testGetWorkspaceNames() {
    setupWorkspaceStructure();

    std::vector<std::string> groupWsName = getStringVector("group1");

    TestablePoldiIndexKnownCompounds alg;
    std::vector<Workspace_sptr> groupWs = alg.getWorkspaces(groupWsName);

    std::vector<std::string> workspaceNames = alg.getWorkspaceNames(groupWs);

    TS_ASSERT_EQUALS(workspaceNames.size(), 2);
    TS_ASSERT_EQUALS(workspaceNames.front(), "test1");
    TS_ASSERT_EQUALS(workspaceNames.back(), "test2");

    tearDownWorkspaceStructure();
  }

  void testGetPeakCollections() {
    std::vector<Workspace_sptr> goodWorkspaces;
    goodWorkspaces.emplace_back(PoldiPeakCollectionHelpers::createPoldiPeakTableWorkspace());
    goodWorkspaces.emplace_back(PoldiPeakCollectionHelpers::createPoldiPeakTableWorkspace());

    TestablePoldiIndexKnownCompounds alg;
    TS_ASSERT_THROWS_NOTHING(alg.getPeakCollections(goodWorkspaces));

    std::vector<Workspace_sptr> badWorkspaces;
    badWorkspaces.emplace_back(PoldiPeakCollectionHelpers::createPoldiPeakTableWorkspace());
    badWorkspaces.emplace_back(WorkspaceCreationHelper::create1DWorkspaceRand(10, true));

    TS_ASSERT_THROWS(alg.getPeakCollections(badWorkspaces), const std::invalid_argument &);
  }

  void testReshapeVector() {
    std::vector<double> one(1, 0.1);
    std::vector<double> two(2, 0.1);
    two[1] = 0.2;

    std::vector<double> empty;

    TestablePoldiIndexKnownCompounds alg;
    TS_ASSERT_THROWS(alg.reshapeVector(empty, 1), const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(alg.reshapeVector(one, 1));
    TS_ASSERT_THROWS_NOTHING(alg.reshapeVector(one, 2));
    TS_ASSERT_THROWS_NOTHING(alg.reshapeVector(two, 1));
    TS_ASSERT_THROWS_NOTHING(alg.reshapeVector(two, 2));
    TS_ASSERT_THROWS_NOTHING(alg.reshapeVector(two, 3));
    TS_ASSERT_THROWS(alg.reshapeVector(one, 0), const std::invalid_argument &);

    std::vector<double> t11 = alg.reshapeVector(one, 1);
    TS_ASSERT_EQUALS(t11.size(), 1);
    TS_ASSERT_EQUALS(t11[0], 0.1);

    std::vector<double> t12 = alg.reshapeVector(one, 2);
    TS_ASSERT_EQUALS(t12.size(), 2);
    TS_ASSERT_EQUALS(t12[0], 0.1);
    TS_ASSERT_EQUALS(t12[1], 0.1);

    std::vector<double> t21 = alg.reshapeVector(two, 1);
    TS_ASSERT_EQUALS(t21.size(), 1);
    TS_ASSERT_EQUALS(t21[0], 0.1);

    std::vector<double> t22 = alg.reshapeVector(two, 2);
    TS_ASSERT_EQUALS(t22.size(), 2);
    TS_ASSERT_EQUALS(t22[0], 0.1);
    TS_ASSERT_EQUALS(t22[1], 0.2);

    std::vector<double> t23 = alg.reshapeVector(two, 3);
    TS_ASSERT_EQUALS(t23.size(), 3);
    TS_ASSERT_EQUALS(t23[0], 0.1);
    TS_ASSERT_EQUALS(t23[1], 0.2);
    TS_ASSERT_EQUALS(t23[2], 0.2);
  }

  void testGetTolerances() {
    TestablePoldiIndexKnownCompounds alg;
    alg.initialize();

    alg.setPropertyValue("Tolerances", "");
    TS_ASSERT_THROWS(alg.getTolerances(1), const std::invalid_argument &);

    alg.setPropertyValue("Tolerances", "0.1");
    TS_ASSERT_THROWS_NOTHING(alg.getTolerances(1));
    TS_ASSERT_THROWS_NOTHING(alg.getTolerances(2));

    alg.setPropertyValue("Tolerances", "0.1,0.2");
    TS_ASSERT_THROWS_NOTHING(alg.getTolerances(1));
    TS_ASSERT_THROWS_NOTHING(alg.getTolerances(2));
    TS_ASSERT_THROWS_NOTHING(alg.getTolerances(3));

    TS_ASSERT_THROWS(alg.getTolerances(0), const std::invalid_argument &);
  }

  void testGetContributions() {
    TestablePoldiIndexKnownCompounds alg;
    alg.initialize();

    alg.setPropertyValue("ScatteringContributions", "");
    TS_ASSERT_THROWS(alg.getContributions(1), const std::invalid_argument &);

    alg.setPropertyValue("ScatteringContributions", "0.1");
    TS_ASSERT_THROWS_NOTHING(alg.getContributions(1));
    TS_ASSERT_THROWS_NOTHING(alg.getContributions(2));

    alg.setPropertyValue("ScatteringContributions", "0.1,0.2");
    TS_ASSERT_THROWS_NOTHING(alg.getContributions(1));
    TS_ASSERT_THROWS_NOTHING(alg.getContributions(2));
    TS_ASSERT_THROWS_NOTHING(alg.getContributions(3));

    TS_ASSERT_THROWS(alg.getContributions(0), const std::invalid_argument &);
  }

  void testGetNormalizedContributions() {
    std::vector<double> contributions;
    contributions.emplace_back(4.0);
    contributions.emplace_back(1.0);

    TestablePoldiIndexKnownCompounds alg;
    TS_ASSERT_THROWS_NOTHING(alg.getNormalizedContributions(contributions));

    std::vector<double> normalized = alg.getNormalizedContributions(contributions);
    TS_ASSERT_EQUALS(normalized.size(), 2);
    TS_ASSERT_EQUALS(normalized[0], 0.8);
    TS_ASSERT_EQUALS(normalized[1], 0.2);

    std::vector<double> empty;
    TS_ASSERT_THROWS(alg.getNormalizedContributions(empty), const std::invalid_argument &);

    std::vector<double> negative(1, -2.0);
    TS_ASSERT_THROWS(alg.getNormalizedContributions(negative), const std::invalid_argument &);
  }

  void testScaleIntensityEstimates() {
    TestablePoldiIndexKnownCompounds alg;
    alg.initialize();

    PoldiPeakCollection_sptr null;
    TS_ASSERT_THROWS(alg.scaleIntensityEstimates(null, 0.1), const std::invalid_argument &);

    PoldiPeakCollection_sptr indexedSilicon = PoldiPeakCollectionHelpers::createTheoreticalPeakCollectionSilicon();
    TS_ASSERT_THROWS_NOTHING(alg.scaleIntensityEstimates(indexedSilicon, 2.0));

    PoldiPeakCollection_sptr reference = PoldiPeakCollectionHelpers::createTheoreticalPeakCollectionSilicon();

    for (size_t i = 0; i < indexedSilicon->peakCount(); ++i) {
      TS_ASSERT_EQUALS(indexedSilicon->peak(i)->intensity(), 2.0 * reference->peak(i)->intensity());
    }

    // test override with vectors
    std::vector<PoldiPeakCollection_sptr> phases(4, indexedSilicon);
    std::vector<double> goodContributions(4, 0.4);
    std::vector<double> badContributions(2, 0.4);

    TS_ASSERT_THROWS_NOTHING(alg.scaleIntensityEstimates(phases, goodContributions));
    TS_ASSERT_THROWS(alg.scaleIntensityEstimates(phases, badContributions), const std::invalid_argument &);
  }

  void testFwhmSigmaConversion() {
    double sigma = 1.0;
    double fwhm = 2.354820045;

    TS_ASSERT_DELTA(PoldiIndexKnownCompounds::sigmaToFwhm(sigma), fwhm, 1e-9);
    TS_ASSERT_DELTA(PoldiIndexKnownCompounds::fwhmToSigma(fwhm), sigma, 1e-9);

    TS_ASSERT_EQUALS(PoldiIndexKnownCompounds::sigmaToFwhm(PoldiIndexKnownCompounds::fwhmToSigma(fwhm)), fwhm);
  }

  void testAssignFwhmEstimates() {
    TestablePoldiIndexKnownCompounds alg;
    alg.initialize();

    PoldiPeakCollection_sptr null;
    TS_ASSERT_THROWS(alg.assignFwhmEstimates(null, 0.1), const std::invalid_argument &);

    PoldiPeakCollection_sptr indexedSilicon = PoldiPeakCollectionHelpers::createTheoreticalPeakCollectionSilicon();
    TS_ASSERT_THROWS_NOTHING(alg.assignFwhmEstimates(indexedSilicon, 0.1));

    for (size_t i = 0; i < indexedSilicon->peakCount(); ++i) {
      TS_ASSERT_EQUALS(indexedSilicon->peak(i)->fwhm(PoldiPeak::Relative), alg.sigmaToFwhm(0.1));
    }

    // test override with vectors
    std::vector<PoldiPeakCollection_sptr> phases(4, indexedSilicon);
    std::vector<double> goodFwhms(4, 0.4);
    std::vector<double> badFwhms(2, 0.4);

    TS_ASSERT_THROWS_NOTHING(alg.assignFwhmEstimates(phases, goodFwhms));
    TS_ASSERT_THROWS(alg.scaleIntensityEstimates(phases, badFwhms), const std::invalid_argument &);
  }

  void testInPeakSet() {
    PoldiPeak_sptr peak1 = PoldiPeak::create(1.0);
    PoldiPeak_sptr peak2 = PoldiPeak::create(1.0);
    PoldiPeak_sptr peak3 = PoldiPeak::create(1.0);

    std::set<PoldiPeak_sptr> peakSet;
    peakSet.insert(peak1);
    peakSet.insert(peak2);

    TestablePoldiIndexKnownCompounds alg;

    TS_ASSERT(alg.inPeakSet(peakSet, peak1));
    TS_ASSERT(alg.inPeakSet(peakSet, peak2));

    TS_ASSERT(!alg.inPeakSet(peakSet, peak3));
  }

  void testIsCandidate() {
    TestablePoldiIndexKnownCompounds alg;

    PoldiPeak_sptr peak = PoldiPeak::create(Conversions::dToQ(2.0));

    PoldiPeak_sptr candidate1 = PoldiPeak::create(Conversions::dToQ(2.02)); // +2 * sigma
    candidate1->setFwhm(UncertainValue(alg.sigmaToFwhm(0.01)), PoldiPeak::AbsoluteD);
    TS_ASSERT(alg.isCandidate(peak, candidate1));

    PoldiPeak_sptr candidate2 = PoldiPeak::create(Conversions::dToQ(1.971)); // -2.99 * sigma
    candidate2->setFwhm(UncertainValue(alg.sigmaToFwhm(0.01)), PoldiPeak::AbsoluteD);
    TS_ASSERT(alg.isCandidate(peak, candidate2));

    // More than 3sigma away - no candidate.
    PoldiPeak_sptr candidate3 = PoldiPeak::create(Conversions::dToQ(1.97)); // -3 sigma
    candidate3->setFwhm(UncertainValue(alg.sigmaToFwhm(0.01)), PoldiPeak::AbsoluteD);
    TS_ASSERT(!alg.isCandidate(peak, candidate3));

    // throw if one of the peaks is invalid
    PoldiPeak_sptr null;
    TS_ASSERT_THROWS(alg.isCandidate(null, candidate1), const std::invalid_argument &);
    TS_ASSERT_THROWS(alg.isCandidate(peak, null), const std::invalid_argument &);
  }

  void testGetIndexCandidatePairs() {
    PoldiPeakCollection_sptr measuredSi = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();
    std::vector<PoldiPeakCollection_sptr> theoreticalSi(
        2, PoldiPeakCollectionHelpers::createTheoreticalPeakCollectionSilicon());

    TestablePoldiIndexKnownCompounds alg;
    alg.initialize();
    alg.scaleIntensityEstimates(theoreticalSi, std::vector<double>(2, 1.0));
    alg.assignFwhmEstimates(theoreticalSi, std::vector<double>(2, 0.005));

    // Get candidates for one peak
    std::vector<IndexCandidatePair> candidates = alg.getIndexCandidatePairs(measuredSi->peak(0), theoreticalSi);

    // Should be twice the same candidate from both collections
    TS_ASSERT_EQUALS(candidates.size(), 2);

    // Tolerance is too small, no candidates.
    alg.assignFwhmEstimates(theoreticalSi, std::vector<double>(2, 0.00001));
    candidates = alg.getIndexCandidatePairs(measuredSi->peak(0), theoreticalSi);
    TS_ASSERT_EQUALS(candidates.size(), 0);
  }

  void testGetAllIndexCandidatePairs() {
    PoldiPeakCollection_sptr measuredSi = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();
    std::vector<PoldiPeakCollection_sptr> theoreticalSi(
        2, PoldiPeakCollectionHelpers::createTheoreticalPeakCollectionSilicon());

    TestablePoldiIndexKnownCompounds alg;
    alg.initialize();

    alg.scaleIntensityEstimates(theoreticalSi, std::vector<double>(2, 1.0));
    alg.assignFwhmEstimates(theoreticalSi, std::vector<double>(2, 0.005));

    // Get candidates for all peaks
    std::vector<IndexCandidatePair> candidates = alg.getAllIndexCandidatePairs(measuredSi, theoreticalSi);

    // Each peak has two candidates - one from each theoretical Si-collection
    TS_ASSERT_EQUALS(candidates.size(), measuredSi->peakCount() * theoreticalSi.size());

    // Tolerance is too small, no candidates.
    alg.assignFwhmEstimates(theoreticalSi, std::vector<double>(2, 0.00001));

    /* unindexed peaks must be initialized, because they are sorted out by
     * getAllCandidates() and placed in that collection, otherwise an exception
     * is thrown.
     */
    TS_ASSERT_THROWS(alg.getAllIndexCandidatePairs(measuredSi, theoreticalSi), const std::runtime_error &);

    // After initialization, everything is fine.
    alg.initializeUnindexedPeaks();

    candidates = alg.getAllIndexCandidatePairs(measuredSi, theoreticalSi);
    TS_ASSERT_EQUALS(candidates.size(), 0);
  }

  void testCollectUnindexedPeak() {
    TestablePoldiIndexKnownCompounds alg;

    PoldiPeak_sptr peak = PoldiPeak::create(1.0);
    TS_ASSERT_THROWS(alg.collectUnindexedPeak(peak), const std::runtime_error &);

    alg.initializeUnindexedPeaks();

    TS_ASSERT_THROWS_NOTHING(alg.collectUnindexedPeak(peak));
  }

  void testIndexCandidatePair() {
    IndexCandidatePair defaultConstructed;
    TS_ASSERT(!defaultConstructed.observed);
    TS_ASSERT(!defaultConstructed.observed);
    TS_ASSERT_EQUALS(defaultConstructed.positionMatch, 0.0);
    TS_ASSERT_EQUALS(defaultConstructed.candidateCollectionIndex, 0);
  }

  void testIndexCandidatePairValues() {
    TestablePoldiIndexKnownCompounds alg;

    PoldiPeak_sptr null;
    PoldiPeak_sptr noFwhm = PoldiPeak::create(3.0);
    PoldiPeak_sptr peak = PoldiPeak::create(Conversions::dToQ(2.0));

    /* Two candidates for indexing with different
     * distances from the peak position, but equal tolerances.
     */
    PoldiPeak_sptr candidate1 = PoldiPeak::create(Conversions::dToQ(2.04), 1.0);
    candidate1->setFwhm(UncertainValue(alg.sigmaToFwhm(0.1)), PoldiPeak::Relative);

    PoldiPeak_sptr candidate2 = PoldiPeak::create(Conversions::dToQ(1.92), 1.0);
    candidate2->setFwhm(UncertainValue(alg.sigmaToFwhm(0.1)), PoldiPeak::Relative);

    // Null peaks don't work
    TS_ASSERT_THROWS(IndexCandidatePair(peak, null, 0), const std::invalid_argument &);
    TS_ASSERT_THROWS(IndexCandidatePair(null, candidate1, 0), const std::invalid_argument &);

    // Range error when no fwhm is set (division by zero).
    TS_ASSERT_THROWS(IndexCandidatePair(peak, noFwhm, 0), const std::range_error &);

    IndexCandidatePair peakCandidate1(peak, candidate1, 1);
    TS_ASSERT_EQUALS(peakCandidate1.observed, peak);
    TS_ASSERT_EQUALS(peakCandidate1.candidate, candidate1);
    TS_ASSERT_EQUALS(peakCandidate1.candidateCollectionIndex, 1);

    IndexCandidatePair peakCandidate2(peak, candidate2, 1);
    TS_ASSERT_EQUALS(peakCandidate2.observed, peak);
    TS_ASSERT_EQUALS(peakCandidate2.candidate, candidate2);
    TS_ASSERT_EQUALS(peakCandidate2.candidateCollectionIndex, 1);

    // Test comparison operator
    TS_ASSERT(peakCandidate2 < peakCandidate1);
    TS_ASSERT(!(peakCandidate1 < peakCandidate2));
  }

  void testIndexPeaks() {
    std::vector<PoldiPeakCollection_sptr> expected(
        1, PoldiPeakCollectionHelpers::createTheoreticalPeakCollectionSilicon());
    PoldiPeakCollection_sptr measured = PoldiPeakCollectionHelpers::createPoldiPeakCollectionMaximum();
    // This peak does not exist in the pattern of Si. It will remain unindexed.
    measured->addPeak(PoldiPeak::create(Conversions::dToQ(0.99111)));

    /* This is very close to 422 (d=1.108539), but slightly further away than
     * the one already present in the collection.
     */
    measured->addPeak(PoldiPeak::create(Conversions::dToQ(1.10880)));

    TestablePoldiIndexKnownCompounds alg;
    alg.initialize();

    alg.setMeasuredPeaks(measured);
    alg.initializeUnindexedPeaks();
    alg.initializeIndexedPeaks(expected);

    alg.scaleIntensityEstimates(expected, std::vector<double>(1, 1.0));
    alg.assignFwhmEstimates(expected, std::vector<double>(1, 0.005));

    TS_ASSERT_THROWS_NOTHING(alg.indexPeaks(measured, expected));
    TS_ASSERT_EQUALS(alg.m_unindexedPeaks->peakCount(), 2);

    PoldiPeakCollection_sptr indexedSi = alg.getIntensitySortedPeakCollection(alg.m_indexedPeaks[0]);
    TS_ASSERT_EQUALS(indexedSi->peakCount(), 4);

    // Peaks are ordered by intensity
    TS_ASSERT_EQUALS(indexedSi->peak(3)->hkl(), MillerIndices(3, 3, 1));
    // Make sure this is the correct peak (not the one introduced above)
    TS_ASSERT_EQUALS(indexedSi->peak(2)->hkl(), MillerIndices(3, 1, 1));
    TS_ASSERT_EQUALS(indexedSi->peak(1)->hkl(), MillerIndices(2, 2, 0));
    TS_ASSERT_EQUALS(indexedSi->peak(0)->hkl(), MillerIndices(4, 2, 2));
    TS_ASSERT_EQUALS(indexedSi->peak(0)->d(), 1.108644);
  }

private:
  class TestablePoldiIndexKnownCompounds : public PoldiIndexKnownCompounds {
    friend class PoldiIndexKnownCompoundsTest;

  public:
    TestablePoldiIndexKnownCompounds() : PoldiIndexKnownCompounds() {}
    ~TestablePoldiIndexKnownCompounds() override = default;
  };

  void setupWorkspaceStructure() {
    /* Workspace structure:
     * group3
     *  +----- group1
     *  |       +----- test1
     *  |       +----- test2
     *  |
     *  +----- group2
     *          +----- test3
     *          +----- test4
     */
    std::vector<std::string> testWorkspaces = getStringVector("test1,test2,test3,test4");

    storeRandomWorkspaces(testWorkspaces);

    WorkspaceGroup_sptr group1 = std::make_shared<WorkspaceGroup>();
    WorkspaceCreationHelper::storeWS("group1", group1);
    group1->add("test1");
    group1->add("test2");

    WorkspaceGroup_sptr group2 = std::make_shared<WorkspaceGroup>();
    WorkspaceCreationHelper::storeWS("group2", group2);
    group2->add("test3");
    group2->add("test4");

    WorkspaceGroup_sptr group3 = std::make_shared<WorkspaceGroup>();
    WorkspaceCreationHelper::storeWS("group3", group3);
    group3->add("group1");
    group3->add("group2");
  }

  void storeRandomWorkspaces(const std::vector<std::string> &wsNames) {
    for (const auto &wsName : wsNames) {
      auto ws = WorkspaceCreationHelper::create1DWorkspaceRand(10, true);
      WorkspaceCreationHelper::storeWS(wsName, ws);
    }
  }

  void tearDownWorkspaceStructure() {
    removeRandomWorkspaces(getStringVector("test1,test2,test3,test4"));

    WorkspaceCreationHelper::removeWS("group1");
    WorkspaceCreationHelper::removeWS("group2");
    WorkspaceCreationHelper::removeWS("group3");
  }

  void removeRandomWorkspaces(const std::vector<std::string> &wsNames) {
    for (const auto &wsName : wsNames) {
      WorkspaceCreationHelper::removeWS(wsName);
    }
  }

  std::vector<std::string> getStringVector(const std::string &string) {
    std::vector<std::string> stringVector;
    boost::split(stringVector, string, boost::is_any_of(", "), boost::token_compress_on);

    return stringVector;
  }
};
