// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MASKBINSTEST_H_
#define MASKBINSTEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaskBins.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

class MaskBinsTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(masker.name(), "MaskBins"); }

  void testVersion() { TS_ASSERT_EQUALS(masker.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(masker.initialize());
    TS_ASSERT(masker.isInitialized());
  }

  void testCommonBins() {
    if (!masker.isInitialized())
      masker.initialize();

    // Create a dummy workspace
    const std::string workspaceName("forMasking");
    const std::string resultWorkspaceName("masked");
    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    ads.add(workspaceName,
            WorkspaceCreationHelper::create2DWorkspaceBinned(5, 25, 0.0));

    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("InputWorkspace", workspaceName));
    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("OutputWorkspace", resultWorkspaceName));

    // Check that execution fails if XMin & XMax not set
    TS_ASSERT_THROWS(masker.execute(), const std::runtime_error &);
    TS_ASSERT(!masker.isExecuted());

    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("XMin", "20.0"));
    TS_ASSERT_THROWS_NOTHING(masker.setPropertyValue("XMax", "22.5"));

    TS_ASSERT_THROWS_NOTHING(masker.execute());
    TS_ASSERT(masker.isExecuted());

    MatrixWorkspace_const_sptr outputWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            ads.retrieve(resultWorkspaceName));

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      TS_ASSERT(outputWS->hasMaskedBins(i));
      const MatrixWorkspace::MaskList &mask = outputWS->maskedBins(i);
      TS_ASSERT_EQUALS(mask.size(), 3);
      MatrixWorkspace::MaskList::const_iterator it;
      int k = 20;
      for (it = mask.begin(); it != mask.end(); ++it, ++k) {
        TS_ASSERT_EQUALS((*it).first, k);
        TS_ASSERT_EQUALS((*it).second, 1.0);
      }

      const size_t numBins = outputWS->blocksize();
      for (size_t j = 0; j < numBins; ++j) {
        if (j >= 20 && j < 23) {
          TS_ASSERT_EQUALS(outputWS->y(i)[j], 0.0);
          TS_ASSERT_EQUALS(outputWS->e(i)[j], 0.0);
        } else {
          TS_ASSERT_EQUALS(outputWS->y(i)[j], 2.0);
          TS_ASSERT_DELTA(outputWS->e(i)[j], M_SQRT2, 0.0001);
        }
        TS_ASSERT_EQUALS(outputWS->x(i)[j], j);
      }
    }

    // Clean up
    ads.remove(workspaceName);
    ads.remove(resultWorkspaceName);
  }

  void testRaggedBins() {
    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();

    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(3, 10, 0.0);
    // Now change one set of bin boundaries so that the don't match the others
    WS->mutableX(1) += -10;
    AnalysisDataService::Instance().add(workspaceName, WS);

    TS_ASSERT_THROWS_NOTHING(
        masker2.setPropertyValue("InputWorkspace", workspaceName));
    TS_ASSERT_THROWS_NOTHING(
        masker2.setPropertyValue("OutputWorkspace", workspaceName));
    TS_ASSERT_THROWS_NOTHING(masker2.setPropertyValue("XMin", "-11.0"));
    TS_ASSERT_THROWS_NOTHING(masker2.setPropertyValue("XMax", "-8.5"));

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(masker2.isExecuted());

    TS_ASSERT(!WS->hasMaskedBins(0));
    TS_ASSERT(WS->hasMaskedBins(1));
    TS_ASSERT(!WS->hasMaskedBins(2));

    const MatrixWorkspace::MaskList &mask = WS->maskedBins(1);
    TS_ASSERT_EQUALS(mask.size(), 2);
    auto &Y = WS->y(1);
    auto &E = WS->e(1);
    MatrixWorkspace::MaskList::const_iterator it;
    int k = 0;
    for (it = mask.begin(); it != mask.end(); ++it, ++k) {
      TS_ASSERT_EQUALS((*it).first, k);
      TS_ASSERT_EQUALS((*it).second, 1.0);
      TS_ASSERT_EQUALS(Y[k], 0.0);
      TS_ASSERT_EQUALS(E[k], 0.0);
    }

    AnalysisDataService::Instance().remove(workspaceName);
  }

  void testSpectraList_out_of_range() {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(10, 10, 0.0);
    AnalysisDataService::Instance().add(workspaceName, WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace", workspaceName);
    masker2.setPropertyValue("OutputWorkspace", workspaceName);
    masker2.setPropertyValue("XMin", "-11.0");
    masker2.setPropertyValue("XMax", "-8.5");
    masker2.setPropertyValue("SpectraList", "1,8-12");

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(!masker2.isExecuted());
    AnalysisDataService::Instance().remove(workspaceName);
  }

  void testSpectraList_WS2D() {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName, WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace", workspaceName);
    masker2.setPropertyValue("OutputWorkspace", workspaceName);
    masker2.setPropertyValue("XMin", "3.0");
    masker2.setPropertyValue("XMax", "6.0");
    masker2.setPropertyValue("SpectraList", "1-3");

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(masker2.isExecuted());

    for (int wi = 1; wi <= 3; wi++)
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(WS->y(wi)[bin], 0.0);
      }

    AnalysisDataService::Instance().remove(workspaceName);
  }

  /*
   * A more sserious check
   */
  void testSpectraList_WS2D_OutPlace() {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    const std::string opWSName("maskedWS");

    int nBins = 10;
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(5, nBins, 0.0);
    AnalysisDataService::Instance().add(workspaceName, WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace", workspaceName);
    masker2.setPropertyValue("OutputWorkspace", opWSName);
    masker2.setPropertyValue("XMin", "3.0");
    masker2.setPropertyValue("XMax", "6.0");
    masker2.setPropertyValue("SpectraList", "1-3");

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(masker2.isExecuted());

    // Get output workspace and compare
    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(opWSName));
    TS_ASSERT(outWS);
    if (!outWS)
      return;
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 5);

    for (size_t wi = 0; wi < 5; ++wi) {
      for (size_t bin = 0; bin < size_t(nBins); ++bin) {
        if (wi >= 1 && wi <= 3 && bin >= 3 && bin < 6) {
          TS_ASSERT_EQUALS(outWS->y(wi)[bin], 0.0);
        } else {
          TS_ASSERT_EQUALS(outWS->y(wi)[bin], WS->y(wi)[bin]);
        }
      }
    } // ENDFOR wi

    AnalysisDataService::Instance().remove(workspaceName);
    AnalysisDataService::Instance().remove(opWSName);
  }

  void testEventWorkspace_SpectraList() {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int numHist = 5;
    EventWorkspace_sptr WS =
        WorkspaceCreationHelper::createEventWorkspace(numHist, nBins);
    AnalysisDataService::Instance().add(workspaceName, WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace", workspaceName);
    masker2.setPropertyValue("OutputWorkspace", workspaceName);
    masker2.setPropertyValue("XMin", "3.0");
    masker2.setPropertyValue("XMax", "6.0");
    masker2.setPropertyValue("SpectraList", "1-3");

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(masker2.isExecuted());

    EventWorkspace_const_sptr constWS =
        boost::dynamic_pointer_cast<const EventWorkspace>(WS);
    for (int wi = 1; wi <= 3; wi++)
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(constWS->y(wi)[bin], 0.0);
      }

    AnalysisDataService::Instance().remove(workspaceName);
  }

  /*
   * A more serious test to compare all bins
   */
  void testEventWorkspace_SpectraList_OutPlace() {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    const std::string opWSName("maskedWorkspace");

    int nBins = 10;
    int numHist = 5;
    EventWorkspace_sptr WS =
        WorkspaceCreationHelper::createEventWorkspace(numHist, nBins);
    AnalysisDataService::Instance().add(workspaceName, WS);

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace", workspaceName);
    masker2.setPropertyValue("OutputWorkspace", opWSName);
    masker2.setPropertyValue("XMin", "3.0");
    masker2.setPropertyValue("XMax", "6.0");
    masker2.setPropertyValue("SpectraList", "1-3");

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(masker2.isExecuted());

    EventWorkspace_const_sptr constWS =
        boost::dynamic_pointer_cast<const EventWorkspace>(
            AnalysisDataService::Instance().retrieve(opWSName));

    for (size_t wi = 0; wi < 5; ++wi) {
      for (size_t bin = 0; bin < size_t(nBins); ++bin) {
        auto &Y = constWS->y(wi);
        auto &oY = WS->y(wi);
        if (wi >= 1 && wi <= 3 && bin >= 3 && bin < 6) {
          TS_ASSERT_EQUALS(Y[bin], 0.0);
        } else {
          TS_ASSERT_EQUALS(Y[bin], oY[bin]);
        }
      }
    } // ENDFOR wi

    AnalysisDataService::Instance().remove(workspaceName);
    AnalysisDataService::Instance().remove(opWSName);
  }

  void testEventWorkspace_No_SpectraList() {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int numHist = 5;
    EventWorkspace_sptr WS =
        WorkspaceCreationHelper::createEventWorkspace(numHist, nBins);
    AnalysisDataService::Instance().add(workspaceName, WS);
    std::size_t events_before = WS->getNumberEvents();

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace", workspaceName);
    masker2.setPropertyValue("OutputWorkspace", workspaceName);
    masker2.setPropertyValue("XMin", "3.0");
    masker2.setPropertyValue("XMax", "6.0");
    masker2.setPropertyValue("SpectraList", ""); // Do all

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(masker2.isExecuted());

    EventWorkspace_const_sptr constWS =
        boost::dynamic_pointer_cast<const EventWorkspace>(WS);
    std::size_t events_after = constWS->getNumberEvents();

    for (int wi = 0; wi < numHist; wi++)
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(constWS->y(wi)[bin], 0.0);
      }
    // Fewer events now; I won't go through all of them
    TS_ASSERT_LESS_THAN(events_after, events_before);

    AnalysisDataService::Instance().remove(workspaceName);
  }

  void testEventWorkspace_copiedOutput_No_SpectraList() {
    // Create a dummy workspace
    const std::string workspaceName("raggedMask");
    int nBins = 10;
    int numHist = 5;
    EventWorkspace_sptr WS =
        WorkspaceCreationHelper::createEventWorkspace(numHist, nBins);
    AnalysisDataService::Instance().add(workspaceName, WS);
    std::size_t events_before = WS->getNumberEvents();

    Mantid::Algorithms::MaskBins masker2;
    masker2.initialize();
    masker2.setPropertyValue("InputWorkspace", workspaceName);
    masker2.setPropertyValue("OutputWorkspace", workspaceName + "2");
    masker2.setPropertyValue("XMin", "3.0");
    masker2.setPropertyValue("XMax", "6.0");
    masker2.setPropertyValue("SpectraList", ""); // Do all

    TS_ASSERT_THROWS_NOTHING(masker2.execute());
    TS_ASSERT(masker2.isExecuted());

    EventWorkspace_const_sptr constWS =
        AnalysisDataService::Instance().retrieveWS<const EventWorkspace>(
            workspaceName + "2");
    std::size_t events_after = constWS->getNumberEvents();

    for (int wi = 0; wi < numHist; wi++)
      for (int bin = 3; bin < 6; bin++) {
        TS_ASSERT_EQUALS(constWS->y(wi)[bin], 0.0);
      }
    // Fewer events now; I won't go through all of them
    TS_ASSERT_LESS_THAN(events_after, events_before);

    AnalysisDataService::Instance().remove(workspaceName);
    AnalysisDataService::Instance().remove(workspaceName + "2");
  }

private:
  Mantid::Algorithms::MaskBins masker;
};

#endif /*MASKBINSTEST_H_*/
