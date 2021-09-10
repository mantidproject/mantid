// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/PaalmanPingsAbsorptionCorrection.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::PaalmanPingsAbsorptionCorrection;
using Mantid::API::AlgorithmManager;
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace_sptr;

class PaalmanPingsAbsorptionCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PaalmanPingsAbsorptionCorrectionTest *createSuite() { return new PaalmanPingsAbsorptionCorrectionTest(); }
  static void destroySuite(PaalmanPingsAbsorptionCorrectionTest *suite) { delete suite; }

  void test_missing_container() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    PaalmanPingsAbsorptionCorrection alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    std::string outWSgroup("absorption");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSgroup));
    alg.setRethrows(true);
    // Error missing container definition
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_LaB6() {
    // create the input workspace

    auto testWS = WorkspaceCreationHelper::create2DWorkspaceBinned(4, 1, 1.7981, 0.0002);

    auto testInst = ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(
        {2., 2., 2., 2.}, {10. * M_PI / 180, 90. * M_PI / 180, 170. * M_PI / 180, 90 * M_PI / 180},
        {0., 0., 0., 45 * M_PI / 180});
    testWS->setInstrument(testInst);
    testWS->rebuildSpectraMapping();
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    AnalysisDataService::Instance().addOrReplace("PaalmanPingsAbsorptionCorrectionTest", testWS);

    auto setSampleAlg = AlgorithmManager::Instance().createUnmanaged("SetSample");
    setSampleAlg->setRethrows(true);
    setSampleAlg->initialize();
    setSampleAlg->setPropertyValue("InputWorkspace", "PaalmanPingsAbsorptionCorrectionTest");
    setSampleAlg->setPropertyValue("Material",
                                   R"({"ChemicalFormula": "La-(B11)5.94-(B10)0.06", "SampleNumberDensity": 0.1})");
    setSampleAlg->setPropertyValue("Geometry",
                                   R"({"Shape": "Cylinder", "Height": 5.68, "Radius": 0.295, "Center": [0., 0., 0.]})");
    setSampleAlg->setPropertyValue("ContainerMaterial", R"({"ChemicalFormula":"V", "SampleNumberDensity": 0.0721})");
    setSampleAlg->setPropertyValue(
        "ContainerGeometry",
        R"({"Shape": "HollowCylinder", "Height": 5.68, "InnerRadius": 0.295, "OuterRadius": 0.315, "Center": [0., 0., 0.]})");
    TS_ASSERT_THROWS_NOTHING(setSampleAlg->execute());

    PaalmanPingsAbsorptionCorrection alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "PaalmanPingsAbsorptionCorrectionTest"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ElementSize", 0.1));
    std::string outWSgroup("absorption");
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSgroup));
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Mantid::API::MatrixWorkspace_sptr ass;
    TS_ASSERT_THROWS_NOTHING(ass = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outWSgroup + "_ass")));
    TS_ASSERT_DELTA(ass->readY(0)[0], 0.1466219, 1e-7);
    TS_ASSERT_DELTA(ass->readY(1)[0], 0.1977505, 1e-7);
    TS_ASSERT_DELTA(ass->readY(2)[0], 0.2517314, 1e-7);
    TS_ASSERT_DELTA(ass->readY(3)[0], 0.1622336, 1e-7);

    Mantid::API::MatrixWorkspace_sptr assc;
    TS_ASSERT_THROWS_NOTHING(assc = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outWSgroup + "_assc")));
    TS_ASSERT_DELTA(assc->readY(0)[0], 0.1406871, 1e-7);
    TS_ASSERT_DELTA(assc->readY(1)[0], 0.1903367, 1e-7);
    TS_ASSERT_DELTA(assc->readY(2)[0], 0.2422601, 1e-7);
    TS_ASSERT_DELTA(assc->readY(3)[0], 0.1550374, 1e-7);

    Mantid::API::MatrixWorkspace_sptr acc;
    TS_ASSERT_THROWS_NOTHING(acc = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outWSgroup + "_acc")));
    TS_ASSERT_DELTA(acc->readY(0)[0], 0.9489914, 1e-7);
    TS_ASSERT_DELTA(acc->readY(1)[0], 0.9471159, 1e-7);
    TS_ASSERT_DELTA(acc->readY(2)[0], 0.9493414, 1e-7);
    TS_ASSERT_DELTA(acc->readY(3)[0], 0.9365069, 1e-7);

    Mantid::API::MatrixWorkspace_sptr acsc;
    TS_ASSERT_THROWS_NOTHING(acsc = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outWSgroup + "_acsc")));
    TS_ASSERT_DELTA(acsc->readY(0)[0], 0.2919232, 1e-7);
    TS_ASSERT_DELTA(acsc->readY(1)[0], 0.4162723, 1e-7);
    TS_ASSERT_DELTA(acsc->readY(2)[0], 0.5561810, 1e-7);
    TS_ASSERT_DELTA(acsc->readY(3)[0], 0.3984180, 1e-7);

    // Compare to the AbsorptionCorrection algorithm, the A_s,s should
    // match the ScatterFrom='Sample', and A_c,c should match
    // ScatterFrom='Container'

    // first compare A_s,s
    auto absorptionCorrectionAlg = AlgorithmManager::Instance().createUnmanaged("AbsorptionCorrection");
    absorptionCorrectionAlg->setRethrows(true);
    absorptionCorrectionAlg->initialize();
    absorptionCorrectionAlg->setPropertyValue("InputWorkspace", "PaalmanPingsAbsorptionCorrectionTest");
    absorptionCorrectionAlg->setProperty("ElementSize", 0.1);
    absorptionCorrectionAlg->setPropertyValue("OutputWorkspace", "absorptionCorrection_ass");
    TS_ASSERT_THROWS_NOTHING(absorptionCorrectionAlg->execute());

    Mantid::API::MatrixWorkspace_sptr absorptionCorrection_ass;
    TS_ASSERT_THROWS_NOTHING(absorptionCorrection_ass = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("absorptionCorrection_ass")));

    TS_ASSERT_DELTA(ass->readY(0)[0], absorptionCorrection_ass->readY(0)[0], 1e-14);
    TS_ASSERT_DELTA(ass->readY(1)[0], absorptionCorrection_ass->readY(1)[0], 1e-14);
    TS_ASSERT_DELTA(ass->readY(2)[0], absorptionCorrection_ass->readY(2)[0], 1e-14);
    TS_ASSERT_DELTA(ass->readY(3)[0], absorptionCorrection_ass->readY(3)[0], 1e-14);

    // now compare for A_c,c
    absorptionCorrectionAlg->setPropertyValue("ScatterFrom", "Container");
    absorptionCorrectionAlg->setPropertyValue("OutputWorkspace", "absorptionCorrection_acc");
    TS_ASSERT_THROWS_NOTHING(absorptionCorrectionAlg->execute());
    Mantid::API::MatrixWorkspace_sptr absorptionCorrection_acc;
    TS_ASSERT_THROWS_NOTHING(absorptionCorrection_acc = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("absorptionCorrection_acc")));

    TS_ASSERT_DELTA(acc->readY(0)[0], absorptionCorrection_acc->readY(0)[0], 1e-14);
    TS_ASSERT_DELTA(acc->readY(1)[0], absorptionCorrection_acc->readY(1)[0], 1e-14);
    TS_ASSERT_DELTA(acc->readY(2)[0], absorptionCorrection_acc->readY(2)[0], 1e-14);
    TS_ASSERT_DELTA(acc->readY(3)[0], absorptionCorrection_acc->readY(3)[0], 1e-14);
  }
};
