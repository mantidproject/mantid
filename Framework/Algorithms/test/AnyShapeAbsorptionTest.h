// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/AnyShapeAbsorption.h"
#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidAlgorithms/FlatPlateAbsorption.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::API::AlgorithmManager;
using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace_sptr;

class AnyShapeAbsorptionTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(atten.name(), "AbsorptionCorrection"); }

  void testVersion() { TS_ASSERT_EQUALS(atten.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(atten.initialize());
    TS_ASSERT(atten.isInitialized());
  }

  void testAgainstFlatPlate() {
    if (!atten.isInitialized())
      atten.initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::FlatPlateAbsorption flat;
    flat.initialize();
    TS_ASSERT_THROWS_NOTHING(flat.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    std::string flatWS("flat");
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("OutputWorkspace", flatWS));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleNumberDensity", "0.07192"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleHeight", "2.3"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleWidth", "1.8"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleThickness", "1.5"));
    TS_ASSERT_THROWS_NOTHING(flat.execute());
    TS_ASSERT(flat.isExecuted());

    // Using the output of the FlatPlateAbsorption algorithm is convenient
    // because
    // it adds the sample object to the workspace
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("InputWorkspace", flatWS));
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleNumberDensity", "0.07192"));
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    Mantid::API::MatrixWorkspace_sptr flatws;
    TS_ASSERT_THROWS_NOTHING(flatws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(flatWS)));
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outputWS)));
    // These should be extremely close to one another (a fraction of a %)
    TS_ASSERT_DELTA(result->readY(0).front(), flatws->readY(0).front(), 0.00001);
    TS_ASSERT_DELTA(result->readY(0).back(), flatws->readY(0).back(), 0.00001);
    TS_ASSERT_DELTA(result->readY(0)[8], flatws->readY(0)[8], 0.00001);
    // Check a few actual numbers as well
    TS_ASSERT_DELTA(result->readY(0).front(), 0.4953, 0.0001);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.0318, 0.0001);
    TS_ASSERT_DELTA(result->readY(0)[4], 0.1463, 0.0001);

    AnalysisDataService::Instance().remove(flatWS);
    AnalysisDataService::Instance().remove(outputWS);
  }

  void testAgainstCylinder() {
    Mantid::Algorithms::AnyShapeAbsorption atten2;
    atten2.initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::CylinderAbsorption cyl;
    cyl.initialize();
    TS_ASSERT_THROWS_NOTHING(cyl.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    std::string cylWS("cyl");
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("OutputWorkspace", cylWS));
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("SampleNumberDensity", "0.07192"));
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("CylinderSampleHeight", "4"));
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("CylinderSampleRadius", "0.4"));
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("NumberOfSlices", "10"));
    TS_ASSERT_THROWS_NOTHING(cyl.setPropertyValue("NumberOfAnnuli", "6"));
    TS_ASSERT_THROWS_NOTHING(cyl.execute());
    TS_ASSERT(cyl.isExecuted());

    // Using the output of the CylinderAbsorption algorithm is convenient
    // because it adds the sample object to the workspace
    TS_ASSERT_THROWS_NOTHING(atten2.setPropertyValue("InputWorkspace", cylWS));
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING(atten2.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(atten2.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(atten2.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(atten2.setPropertyValue("SampleNumberDensity", "0.07192"));
    TS_ASSERT_THROWS_NOTHING(atten2.execute());
    TS_ASSERT(atten2.isExecuted());

    Mantid::API::MatrixWorkspace_sptr cylws;
    TS_ASSERT_THROWS_NOTHING(cylws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(cylWS)));
    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outputWS)));
    // These should be somewhat close to one another (within a couple of %)
    Mantid::MantidVec y0 = result->readY(0);
    TS_ASSERT_DELTA(y0.front() / cylws->readY(0).front(), 1.0, 0.02);
    TS_ASSERT_DELTA(y0[4] / cylws->readY(0)[4], 1.0, 0.02);
    TS_ASSERT_DELTA(y0[7] / cylws->readY(0)[7], 1.0, 0.02);
    // Check a few actual numbers as well
    TS_ASSERT_DELTA(y0.front(), 0.7266, 0.0001);
    TS_ASSERT_DELTA(y0.back(), 0.2164, 0.0001);
    TS_ASSERT_DELTA(y0[5], 0.3680, 0.0001);

    // Now test with a gauge volume used.
    // Create a small cylinder to be the gauge volume
    std::string cylinder = "<cylinder id=\"shape\"> ";
    cylinder += R"(<centre-of-bottom-base x="0.0" y="-0.01" z="0.0" /> )";
    cylinder += R"(<axis x="0.0" y="0.0" z="1" /> )";
    cylinder += "<radius val=\"0.1\" /> ";
    cylinder += "<height val=\"0.02\" /> ";
    cylinder += "</cylinder>";

    cylws->mutableRun().addProperty("GaugeVolume", cylinder);

    // Re-run the algorithm
    Mantid::Algorithms::AnyShapeAbsorption atten3;
    atten3.initialize();
    TS_ASSERT_THROWS_NOTHING(atten3.setPropertyValue("InputWorkspace", cylWS));
    TS_ASSERT_THROWS_NOTHING(atten3.setPropertyValue("OutputWorkspace", "gauge"));
    TS_ASSERT_THROWS_NOTHING(atten3.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(atten3.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(atten3.setPropertyValue("SampleNumberDensity", "0.07192"));
    TS_ASSERT_THROWS_NOTHING(atten3.execute());
    TS_ASSERT(atten3.isExecuted());

    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve("gauge")));
    TS_ASSERT_LESS_THAN(result->readY(0).front(), y0.front());
    TS_ASSERT_LESS_THAN(result->readY(0).back(), y0.back());
    TS_ASSERT_LESS_THAN(result->readY(0)[1], y0[1]);
    TS_ASSERT_LESS_THAN(result->readY(0).back(), result->readY(0).front());

    AnalysisDataService::Instance().remove(cylWS);
    AnalysisDataService::Instance().remove(outputWS);
    AnalysisDataService::Instance().remove("gauge");
  }

  void testTinyVolume() {
    if (!atten.isInitialized())
      atten.initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    Mantid::Algorithms::FlatPlateAbsorption flat;
    flat.initialize();
    TS_ASSERT_THROWS_NOTHING(flat.setProperty<MatrixWorkspace_sptr>("InputWorkspace", testWS));
    std::string flatWS("flat");
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("OutputWorkspace", flatWS));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleNumberDensity", "0.07192"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleHeight", "2.3"));
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleWidth", "1.8"));
    // too thin to work in any shapes gauge volume creation
    TS_ASSERT_THROWS_NOTHING(flat.setPropertyValue("SampleThickness", ".1"));
    TS_ASSERT_THROWS_NOTHING(flat.execute());
    TS_ASSERT(flat.isExecuted());

    // Using the output of the FlatPlateAbsorption algorithm is convenient
    // because it adds the sample object to the workspace
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("InputWorkspace", flatWS));
    std::string outputWS("factors");
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleNumberDensity", "0.07192"));
    atten.setRethrows(true); // needed for the next check to work
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    AnalysisDataService::Instance().remove(flatWS);
  }

  void testScatterBy() {
    // these numbers are the default wl settings for NOMAD
    constexpr double WL_MIN = .1;
    constexpr size_t NUM_VALS = 10; // arbitrary
    constexpr double WL_DELTA = (2.9 - WL_MIN) / static_cast<double>(NUM_VALS);

    // create the input workspace
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, NUM_VALS, WL_MIN, WL_DELTA);
    auto testInst = ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions({2.}, {90.}, {0.});
    testInst->setName("ISIS_Histogram");
    inputWS->setInstrument(testInst);
    inputWS->rebuildSpectraMapping();
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    AnalysisDataService::Instance().addOrReplace("bobby", inputWS); // TODO rename this nonsense

    // set the sample/can geometry
    auto setSampleAlg = AlgorithmManager::Instance().createUnmanaged("SetSample");
    setSampleAlg->setRethrows(true);
    setSampleAlg->initialize();
    setSampleAlg->setPropertyValue("InputWorkspace", "bobby");
    setSampleAlg->setPropertyValue("Environment", R"({"Name": "CRYO-01", "Container": "8mm"})");
    setSampleAlg->setPropertyValue("Material",
                                   R"({"ChemicalFormula": "(Li7)2-C-H4-N-Cl6", "SampleNumberDensity": 0.1})");

    TS_ASSERT_THROWS_NOTHING(setSampleAlg->execute());

    Mantid::Algorithms::AnyShapeAbsorption absAlg;
    absAlg.setRethrows(true);
    absAlg.initialize();

    std::cout << "SAMPLE" << std::endl;
    // run the actual algorithm on the sample
    const std::string SAM_WS{"AbsorptionCorrection_Sample"};
    absAlg.setProperty("InputWorkspace", inputWS);
    absAlg.setPropertyValue("OutputWorkspace", SAM_WS);
    absAlg.setProperty("ScatterFrom", "Sample");
    absAlg.setProperty("EMode", "Elastic");
    TS_ASSERT_THROWS_NOTHING(absAlg.execute());
    TS_ASSERT(absAlg.isExecuted());

    std::cout << "CONTAINER" << std::endl;
    // run the algorithm on the container
    const std::string CAN_WS{"AbsorptionCorrection_Container"};
    absAlg.setProperty("InputWorkspace", inputWS);
    absAlg.setPropertyValue("OutputWorkspace", CAN_WS);
    absAlg.setProperty("ScatterFrom", "Container");
    absAlg.setProperty("EMode", "Elastic");
    TS_ASSERT_THROWS_NOTHING(absAlg.execute());
    TS_ASSERT(absAlg.isExecuted()); // REMOVE

    std::cout << "ENVIRONMENT" << std::endl;
    // run the algorithm on the environment - which doesn't exist in the xml
    const std::string ENV_WS{"AbsorptionCorrection_Environment"};
    absAlg.setProperty("InputWorkspace", inputWS);
    absAlg.setPropertyValue("OutputWorkspace", ENV_WS);
    absAlg.setProperty("ScatterFrom", "Environment");
    absAlg.setProperty("EMode", "Elastic");
    TS_ASSERT_THROWS(absAlg.execute(), const std::runtime_error &);

    // verify the sample term is bigger than the container term because the
    // material contains Li7
    Mantid::API::MatrixWorkspace_sptr samWS;
    TS_ASSERT_THROWS_NOTHING(samWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(SAM_WS)));
    const auto &samValues = samWS->readY(0);
    Mantid::API::MatrixWorkspace_sptr canWS;
    TS_ASSERT_THROWS_NOTHING(canWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(CAN_WS)));
    const auto &canValues = canWS->readY(0);
    TS_ASSERT_EQUALS(samValues.size(), canValues.size());
    // the actual compare - sample should absorb more
    for (size_t i = 0; i < NUM_VALS; ++i) {
      std::cout << "values[" << i << "] " << samWS->readX(0)[i] << " : " << samValues[i] << " and " << canValues[i]
                << '\n';
      TS_ASSERT(samValues[i] < canValues[i])
    }

    // cleanup - ENV_WS should never have been created
    AnalysisDataService::Instance().remove(SAM_WS);
    AnalysisDataService::Instance().remove(CAN_WS);
  }

private:
  Mantid::Algorithms::AnyShapeAbsorption atten;
};
