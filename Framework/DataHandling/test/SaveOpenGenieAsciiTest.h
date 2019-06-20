// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SAVEOPENGENIEASCIITEST_H_
#define SAVEOPENGENIEASCIITEST_H_
#include "MantidDataHandling/SaveOpenGenieAscii.h"

#include "MantidDataHandling/Load.h"
#include "MantidTestHelpers/FileComparisonHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/TemporaryFile.h>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <memory>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

class SaveOpenGenieAsciiTest : public CxxTest::TestSuite {
public:
  void testUnfocusedWsThrows() {
    // If number of spectra is > 1 we cannot save it as it is unfocused
    const int numBins(10);
    const int numHist(2);
    const auto ws =
        WorkspaceCreationHelper::create2DWorkspace(numHist, numBins);
    const auto fileHandle = Poco::TemporaryFile();
    auto alg = createAlg(ws, fileHandle.path());

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void testEventWSThrows() {
    const int numBins(1);
    const int numHist(1);
    const bool isHist = false;
    const auto ws =
        WorkspaceCreationHelper::create2DWorkspace123(numHist, numBins, isHist);
    const auto fileHandle = Poco::TemporaryFile();
    auto alg = createAlg(ws, fileHandle.path());

    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void testFileMatchesExpectedFormat() {
    // Get a .nxs file as we need to make sure that the all the correct
    // log files save in the correct format which is non-trivial
    // to set up using the workspace creation helpers
    const std::string wsName = "nxsWorkspace";
    Load nxsLoader;
    nxsLoader.initialize();
    nxsLoader.setProperty("Filename", m_inputNexusFile);
    nxsLoader.setProperty("OutputWorkspace", wsName);
    nxsLoader.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(nxsLoader.execute());

    const auto ws = AnalysisDataService::Instance().retrieve(wsName);
    const auto inputWs = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);

    auto fileHandle = Poco::TemporaryFile();
    auto alg = createAlg(inputWs, fileHandle.path());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    AnalysisDataService::Instance().remove(wsName);

    const bool wasEqual = FileComparisonHelper::isEqualToReferenceFile(
        m_referenceFileName, fileHandle.path());
    // Delete output file then do the assertion so we always delete
    fileHandle.remove();
    TS_ASSERT(wasEqual);
  }

private:
  const std::string m_referenceFileName{
      "SaveOpenGenieAsciiEnginXReference.his"};
  const std::string m_outputFileName{"SaveOpenGenieAscii_output.his"};
  const std::string m_inputNexusFile{"SaveOpenGenieAsciiInput.nxs"};

  std::unique_ptr<SaveOpenGenieAscii>
  createAlg(MatrixWorkspace_sptr ws, const std::string &tempFilePath) {
    auto alg = std::make_unique<SaveOpenGenieAscii>();
    alg->initialize();

    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("Filename", tempFilePath);
    alg->setProperty("OpenGenieFormat", "ENGIN-X Format");
    alg->setRethrows(true);
    return alg;
  }
};

#endif /* SAVEOPENGENIEASCIITEST_H_ */
