#ifndef MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_
#define MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;

class PhaseQuadMuonTest : public CxxTest::TestSuite {
public:
  void testTheBasics() {
    IAlgorithm_sptr phaseQuad =
        AlgorithmManager::Instance().create("PhaseQuad");
    TS_ASSERT_EQUALS(phaseQuad->name(), "PhaseQuad");
    TS_ASSERT_EQUALS(phaseQuad->category(), "Muon");
    TS_ASSERT_THROWS_NOTHING(phaseQuad->initialize());
    TS_ASSERT(phaseQuad->isInitialized());
  }

  void testExecPhaseTable() {
    // Load a muon dataset
    IAlgorithm_sptr loader = AlgorithmManager::Instance().create("Load");
    loader->setChild(true);
    loader->initialize();
    loader->setProperty("Filename", "emu00006473.nxs");
    loader->setPropertyValue("OutputWorkspace", "out_ws");
    loader->execute();
    Workspace_sptr temp = loader->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr inputWs =
        boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

    // Create and populate a detector table
    boost::shared_ptr<ITableWorkspace> phaseTable(
        new Mantid::DataObjects::TableWorkspace);
    populatePhaseTable(phaseTable);

    // Run PhaseQuad
    IAlgorithm_sptr phaseQuad =
        AlgorithmManager::Instance().create("PhaseQuad");
    phaseQuad->setChild(true);
    phaseQuad->initialize();
    phaseQuad->setProperty("InputWorkspace", inputWs);
    phaseQuad->setProperty("PhaseTable", phaseTable);
    phaseQuad->setPropertyValue("OutputWorkspace", "out_ws");
    TS_ASSERT_THROWS_NOTHING(phaseQuad->execute());
    TS_ASSERT(phaseQuad->isExecuted());

    // Get the output ws
    MatrixWorkspace_sptr outputWs = phaseQuad->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(
        outputWs->getSpectrum(0)->readX(),
        inputWs->getSpectrum(0)->readX()); // Check outputWs X values
    TS_ASSERT_EQUALS(outputWs->getSpectrum(1)->readX(),
                     inputWs->getSpectrum(1)->readX());

    auto specReY = outputWs->getSpectrum(0)->readY();
    auto specReE = outputWs->getSpectrum(0)->readE();
    auto specImY = outputWs->getSpectrum(1)->readY();
    auto specImE = outputWs->getSpectrum(1)->readE();
    // Check real Y values
    TS_ASSERT_DELTA(specReY[0], -0.9982, 0.0001);
    TS_ASSERT_DELTA(specReY[20], -0.0252, 0.0001);
    TS_ASSERT_DELTA(specReY[50], 0.0264, 0.0001);
    // Check real E values
    TS_ASSERT_DELTA(specReE[0], 0.0010, 0.0001);
    TS_ASSERT_DELTA(specReE[20], 0.0021, 0.0001);
    TS_ASSERT_DELTA(specReE[50], 0.0024, 0.0001);
    // Check imaginary Y values
    TS_ASSERT_DELTA(specImY[0], -0.9974, 0.0001);
    TS_ASSERT_DELTA(specImY[20], 0.0115, 0.0001);
    TS_ASSERT_DELTA(specImY[50], 0.0316, 0.0001);
    // Check imaginary E values
    TS_ASSERT_DELTA(specImE[0], 0.0029, 0.0001);
    TS_ASSERT_DELTA(specImE[20], 0.0031, 0.0001);
    TS_ASSERT_DELTA(specImE[50], 0.0035, 0.0001);
  }

  void populatePhaseTable(ITableWorkspace_sptr phaseTable) {
    phaseTable->addColumn("int", "DetectorID");
    phaseTable->addColumn("double", "DetectorAsymmetry");
    phaseTable->addColumn("double", "DetectorPhase");
    for (int i = 0; i < 16; i++) {
      TableRow phaseRow1 = phaseTable->appendRow();
      phaseRow1 << i << 1. << 0.;
      TableRow phaseRow2 = phaseTable->appendRow();
      phaseRow2 << i << 1. << 1.57;
    }
  }
};

#endif /* MANTID_ALGORITHMS_PHASEQUADMUONTEST_H_ */