// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MASKBINSFROMWORKSPACETEST_H_
#define MASKBINSFROMWORKSPACETEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MaskBinsFromWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::MantidVec;

class MaskBinsFromWorkspaceTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(masker.name(), "MaskBinsFromWorkspace"); }

  void testVersion() { TS_ASSERT_EQUALS(masker.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(masker.initialize());
    TS_ASSERT(masker.isInitialized());
  }

  /*
  Test that when the MaskedWorkspace does
  not contain masked bins on its 0th spectrum,
  the output workspace has no bin masking.
  */
  void testUnmaskedWorkspace() {
    if (!masker.isInitialized())
      masker.initialize();

    // Create a dummy workspace
    const std::string workspaceName("forMasking");
    const std::string maskedWorkspaceName("forCopyingMasks");
    const std::string resultWorkspaceName("masked");
    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    ads.add(workspaceName,
            WorkspaceCreationHelper::create2DWorkspaceBinned(5, 25, 0.0));
    ads.add(maskedWorkspaceName,
            WorkspaceCreationHelper::create2DWorkspaceBinned(5, 25 ,0.0));

    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("InputWorkspace", workspaceName));
    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("MaskedWorkspace", maskedWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("OutputWorkspace", resultWorkspaceName));

    masker.execute();
    TS_ASSERT(masker.isExecuted());

    MatrixWorkspace_const_sptr outputWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            ads.retrieve(resultWorkspaceName));

    for (size_t i = 0; i < outputWS->getNumberHistograms(); ++i) {
      TS_ASSERT(!outputWS->hasMaskedBins(i));
    }

    // Clean up
    ads.remove(workspaceName);
    ads.remove(maskedWorkspaceName);
    ads.remove(resultWorkspaceName);
  }

  /*
  Test that when MaskedWorkspace contains masked bins in its 0th spectrum,
  the masked bins are copied over to every spectrum in the output workspace.
  */
  void testMaskedWorkspace() {
    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

    // Create the input workspace
    const std::string workspaceName("forMasking");
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(3, 10, 0.0);
    ads.add(workspaceName, WS);

    // Create a masked workspace
    const std::string maskedWorkspaceName("forCopyingMasks");
    MatrixWorkspace_sptr maskedWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 10, 0.0);
    maskedWS->flagMasked(0, 0);
    maskedWS->flagMasked(0, 1);
    maskedWS->flagMasked(0, 2);
    ads.add(maskedWorkspaceName, maskedWS);

    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("InputWorkspace", workspaceName));
    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("MaskedWorkspace", maskedWorkspaceName));
    TS_ASSERT_THROWS_NOTHING(
        masker.setPropertyValue("OutputWorkspace", workspaceName));

    TS_ASSERT_THROWS_NOTHING(masker.execute());
    TS_ASSERT(masker.isExecuted());

    TS_ASSERT(WS->hasMaskedBins(0));
    TS_ASSERT(WS->hasMaskedBins(1));
    TS_ASSERT(WS->hasMaskedBins(2));

    for (int wi = 0; wi < 3; ++wi) {
      const MatrixWorkspace::MaskList &mask = WS->maskedBins(wi);
      MatrixWorkspace::MaskList::const_iterator it;
      int k = 0;
      for (it = mask.begin(); it != mask.end(); ++it, ++k) {
        TS_ASSERT_EQUALS((*it).first, k);
        TS_ASSERT_EQUALS((*it).second, 1.0);
      }
    }

    ads.remove(workspaceName);
    ads.remove(maskedWorkspaceName);
  }

private:
  Mantid::Algorithms::MaskBinsFromWorkspace masker;
};

#endif /*MASKBINSFROMWORKSPACETEST_H_*/
