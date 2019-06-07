// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SetSampleMaterialTEST_H_
#define SetSampleMaterialTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/SetSampleMaterial.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;
using namespace Mantid::PhysicalConstants;

using Mantid::API::MatrixWorkspace_sptr;

class SetSampleMaterialTest : public CxxTest::TestSuite {
public:
  void testName() {
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    TS_ASSERT_EQUALS(setmat->name(), "SetSampleMaterial");
  }

  void testVersion() {
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    TS_ASSERT_EQUALS(setmat->version(), 1);
  }

  void testInit() {
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    TS_ASSERT_THROWS_NOTHING(setmat->initialize());
    TS_ASSERT(setmat->isInitialized());
  }

  void testExecAl2O3() {
    std::string wsName = "SetSampleMaterialTestWS";
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    if (!setmat->isInitialized())
      setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);

    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("ChemicalFormula", "Al2-O3"));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("SampleNumberDensity", "0.1183245"));
    TS_ASSERT_THROWS_NOTHING(setmat->execute());
    TS_ASSERT(setmat->isExecuted());

    // can get away with holding pointer as it is an inout ws property
    const auto sampleMaterial = testWS->sample().getMaterial();
    TS_ASSERT_DELTA(sampleMaterial.numberDensity(), 0.1183245, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda),
        3.1404, 0.0001);
    TS_ASSERT_DELTA(sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda),
                    0.0925, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.cohScatterLength(NeutronAtom::ReferenceLambda), 4.8614,
        0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterLengthSqrd(NeutronAtom::ReferenceLambda),
        24.9905, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecAl2O3overrides() {
    std::string wsName = "SetSampleMaterialTestWS";
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    if (!setmat->isInitialized())
      setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);

    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("ChemicalFormula", "Al2-O3"));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("SampleNumberDensity", "0.1183245"));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("ScatteringXSection", "3.1404"));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("AttenuationXSection", "0.0925"));
    TS_ASSERT_THROWS_NOTHING(setmat->execute());
    TS_ASSERT(setmat->isExecuted());

    // can get away with holding pointer as it is an inout ws property
    const auto sampleMaterial = testWS->sample().getMaterial();
    TS_ASSERT_DELTA(sampleMaterial.numberDensity(), 0.1183245, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda),
        3.1404, 0.0001);
    TS_ASSERT_DELTA(sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda),
                    0.0925, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.cohScatterLength(NeutronAtom::ReferenceLambda), 4.8614,
        0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterLengthSqrd(NeutronAtom::ReferenceLambda),
        24.9905, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecBaTiO3() {
    std::string wsName = "SetSampleMaterialTestWS";
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    if (!setmat->isInitialized())
      setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);

    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("ChemicalFormula", "Ba Ti O3"));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("SampleNumberDensity", "0.1183245")); // TODO
    TS_ASSERT_THROWS_NOTHING(setmat->execute());
    TS_ASSERT(setmat->isExecuted());

    // can get away with holding pointer as it is an inout ws property
    const auto sampleMaterial = testWS->sample().getMaterial();
    TS_ASSERT_DELTA(sampleMaterial.numberDensity(), 0.1183245, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda),
        4.0852, 0.0001);
    TS_ASSERT_DELTA(sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda),
                    1.4381, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.cohScatterLength(NeutronAtom::ReferenceLambda), 3.8082,
        0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterLengthSqrd(NeutronAtom::ReferenceLambda),
        32.5090, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecMat_Formula() {

    std::string wsName = "SetSampleMaterialTestWS_formula";
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    if (!setmat->isInitialized())
      setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);

    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("ChemicalFormula", "Al2-O3"));
    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("UnitCellVolume", "253.54"));
    TS_ASSERT_THROWS_NOTHING(setmat->setPropertyValue("ZParameter", "6"));
    TS_ASSERT_THROWS_NOTHING(setmat->execute());
    TS_ASSERT(setmat->isExecuted());

    const auto sampleMaterial = testWS->sample().getMaterial();
    TS_ASSERT_DELTA(sampleMaterial.numberDensity(), 0.1183245, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda),
        3.1404, 0.0001);
    TS_ASSERT_DELTA(sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda),
                    0.0925, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.cohScatterLength(NeutronAtom::ReferenceLambda), 4.8614,
        0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterLengthSqrd(NeutronAtom::ReferenceLambda),
        24.9905, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testExecMat_OneAtom() {

    std::string wsName = "SetSampleMaterialTestWS_oneatom";
    IAlgorithm *setmat =
        Mantid::API::FrameworkManager::Instance().createAlgorithm(
            "SetSampleMaterial");
    if (!setmat->isInitialized())
      setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);

    TS_ASSERT_THROWS_NOTHING(
        setmat->setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(setmat->setPropertyValue("ChemicalFormula", "Ni"));
    TS_ASSERT_THROWS_NOTHING(setmat->execute());
    TS_ASSERT(setmat->isExecuted());

    const auto sampleMaterial = testWS->sample().getMaterial();
    TS_ASSERT_DELTA(sampleMaterial.numberDensity(), 0.0913375, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda), 18.5,
        0.0001);
    TS_ASSERT_DELTA(sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda),
                    4.49, 0.0001);
    TS_ASSERT_DELTA(
        sampleMaterial.cohScatterLength(NeutronAtom::ReferenceLambda), 10.3,
        0.0001);
    const double totScattLength =
        sampleMaterial.totalScatterLength(NeutronAtom::ReferenceLambda);
    TS_ASSERT_DELTA(
        sampleMaterial.totalScatterLengthSqrd(NeutronAtom::ReferenceLambda),
        totScattLength * totScattLength, 0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testNumberDensity_FormulaUnits() {
    SetSampleMaterial setMaterial;
    setMaterial.initialize();
    setMaterial.setChild(true);
    setMaterial.setRethrows(true);
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    TS_ASSERT_THROWS_NOTHING(setMaterial.setProperty("InputWorkspace", ws))
    TS_ASSERT_THROWS_NOTHING(
        setMaterial.setProperty("ChemicalFormula", "Al2 O3"))
    TS_ASSERT_THROWS_NOTHING(
        setMaterial.setProperty("SampleNumberDensity", 0.23))
    TS_ASSERT_THROWS_NOTHING(
        setMaterial.setProperty("NumberDensityUnit", "Formula Units"))
    TS_ASSERT_THROWS_NOTHING(setMaterial.execute())
    TS_ASSERT(setMaterial.isExecuted())
    const Material &material{ws->sample().getMaterial()};
    TS_ASSERT_DELTA(material.numberDensity(), 0.23 * (2. + 3.), 1e-12)
  }
};

#endif /*SetSampleMaterialTEST_H_*/
