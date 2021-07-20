// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/MultipleScatteringCorrection.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::Algorithms::MultipleScatteringCorrection;

class MultipleScatteringCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultipleScatteringCorrectionTest *createSuite() {
    Mantid::API::FrameworkManager::Instance();
    return new MultipleScatteringCorrectionTest();
  }
  static void destroySuite(MultipleScatteringCorrectionTest *suite) { delete suite; }

  void test_vanadium() {
    // Create a workspace with vanadium data
    Mantid::API::MatrixWorkspace_sptr ws = MakeSampleWorkspaceVanadium();

    // correct using Mayer correction
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setChild(true);
    unitsAlg->setProperty("InputWorkspace", ws);
    unitsAlg->setProperty("Target", "TOF");
    unitsAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_TOF = unitsAlg->getProperty("OutputWorkspace");
    //
    auto mayerAlg = Mantid::API::AlgorithmManager::Instance().create("MayersSampleCorrection");
    mayerAlg->setChild(true);
    mayerAlg->initialize();
    mayerAlg->setProperty("InputWorkspace", ws_TOF);
    mayerAlg->setProperty("MultipleScattering", true);
    mayerAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_mayer = mayerAlg->getProperty("OutputWorkspace");

    // correct using Carpenter correction
    unitsAlg->setProperty("InputWorkspace", ws);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_wavelength = unitsAlg->getProperty("OutputWorkspace");
    //
    auto carrAlg = Mantid::API::AlgorithmManager::Instance().create("CarpenterSampleCorrection");
    carrAlg->setChild(true);
    carrAlg->initialize();
    carrAlg->setProperty("InputWorkspace", ws_wavelength);
    carrAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_carr = carrAlg->getProperty("OutputWorkspace");

    // correct using multiple scattering correction
    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    // TOF target
    msAlg.setProperty("InputWorkspace", ws);
    msAlg.setProperty("Target", "TOF");
    msAlg.execute();
    Mantid::API::MatrixWorkspace_sptr ws_ms_TOF = msAlg.getProperty("OutputWorkspace");
    // Wavelength target
    msAlg.setProperty("InputWorkspace", ws);
    msAlg.setProperty("Target", "Wavelength");
    msAlg.execute();
    Mantid::API::MatrixWorkspace_sptr ws_ms_wavelength = msAlg.getProperty("OutputWorkspace");

    // Compare the results
    // TODO: Compare the results
  }

  void test_diamond() {
    // Create a workspace with diamond data
    Mantid::API::MatrixWorkspace_sptr ws = MakeSampleWorkspaceDiamond();

    // correct using Mayer correction
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setChild(true);
    unitsAlg->setProperty("InputWorkspace", ws);
    unitsAlg->setProperty("Target", "TOF");
    unitsAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_TOF = unitsAlg->getProperty("OutputWorkspace");
    //
    auto mayerAlg = Mantid::API::AlgorithmManager::Instance().create("MayersSampleCorrection");
    mayerAlg->setChild(true);
    mayerAlg->initialize();
    mayerAlg->setProperty("InputWorkspace", ws_TOF);
    mayerAlg->setProperty("MultipleScattering", true);
    mayerAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_mayer = mayerAlg->getProperty("OutputWorkspace");

    // correct using multiple scattering correction
    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    // TOF target
    msAlg.setProperty("InputWorkspace", ws);
    msAlg.setProperty("Target", "TOF");
    msAlg.execute();
    Mantid::API::MatrixWorkspace_sptr ws_ms_TOF = msAlg.getProperty("OutputWorkspace");

    // cast both to wavelength
    unitsAlg->setProperty("InputWorkspace", ws_mayer);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_mayer_wavelength = unitsAlg->getProperty("OutputWorkspace");
    unitsAlg->setProperty("InputWorkspace", ws_ms_TOF);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws_ms_wavelength = unitsAlg->getProperty("OutputWorkspace");

    // Compare the results
  }

private:
  /**
   * @brief create a sample workspace with a mocked geometry
   *
   * @return Mantid::API::MatrixWorkspace
   */
  static Mantid::API::MatrixWorkspace_sptr MakeSampleWorkspace() {
    // Create a fake workspace with TOF data
    auto sampleAlg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    sampleAlg->initialize();
    sampleAlg->setChild(true);
    sampleAlg->setProperty("Function", "Powder Diffraction");
    sampleAlg->setProperty("NumBanks", 4);
    sampleAlg->setProperty("XUnit", "TOF");
    sampleAlg->setProperty("XMin", 1000.0);
    sampleAlg->setProperty("XMax", 10000.0);
    sampleAlg->setPropertyValue("OutputWorkspace", "sample_ws");

    sampleAlg->execute();
    Mantid::API::MatrixWorkspace_sptr ws = sampleAlg->getProperty("OutputWorkspace");

    // edit the instrument geometry
    auto editAlg = Mantid::API::AlgorithmManager::Instance().create("EditInstrumentGeometry");
    editAlg->initialize();
    editAlg->setChild(true);
    editAlg->setProperty("Workspace", ws);
    editAlg->setProperty("PrimaryFlightPath", 5.0);
    editAlg->setProperty("SpectrumIDs", "1,2,3,4");
    editAlg->setProperty("L2", "2.0,2.0,2.0,2.0");
    editAlg->setProperty("Polar", "10.0,90.0,180.0,90.0");
    editAlg->setProperty("Azimuthal", "0.0,0.0,0.0,45.0");
    editAlg->setProperty("DetectorIDs", "1,2,3,4");
    editAlg->setProperty("InstrumentName", "Instrument");

    editAlg->execute();

    return ws;
  }

  /**
   * @brief Vanadium sample workspace
   *
   * @return Mantid::API::Workspace_sptr
   */
  static Mantid::API::MatrixWorkspace_sptr MakeSampleWorkspaceVanadium() {
    Mantid::API::MatrixWorkspace_sptr ws = MakeSampleWorkspace();

    // vanadium
    const std::string chemical_formula = "V";
    const double number_density = 0.07261;
    const double mass_density = 6.11;
    const double center_bottom_base_x = 0.0;
    const double center_bottom_base_y = -0.0284;
    const double center_bottom_base_z = 0.0;
    const double height = 0.00295;
    const double radius = 0.0568;

    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;
    using FloatArrayProperty = Mantid::Kernel::ArrayProperty<double>;
    // material
    auto material = std::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(std::make_unique<StringProperty>("ChemicalFormula", chemical_formula), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleNumberDensity", number_density), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleMassDensity", mass_density), "");
    // geometry
    auto geometry = std::make_shared<Mantid::Kernel::PropertyManager>();
    geometry->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    geometry->declareProperty(std::make_unique<FloatProperty>("Height", height), "");
    geometry->declareProperty(std::make_unique<FloatProperty>("Radius", radius), "");
    std::vector<double> center{center_bottom_base_x, center_bottom_base_y, center_bottom_base_z};
    geometry->declareProperty(std::make_unique<FloatArrayProperty>("Center", std::move(center)), "");
    std::vector<double> cylinderAxis{0, 1, 0};
    geometry->declareProperty(std::make_unique<FloatArrayProperty>("Axis", cylinderAxis), "");
    // set sample
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", ws);
    setsample.setProperty("Material", material);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();

    return ws;
  }

  static Mantid::API::MatrixWorkspace_sptr MakeSampleWorkspaceDiamond() {
    Mantid::API::MatrixWorkspace_sptr ws = MakeSampleWorkspace();

    // diamond
    const std::string chemical_formula = "C";
    const double number_density = 176.2; // 10^27/m^3 (https://en.wikipedia.org/wiki/Number_density)
    const double mass_density = 3.515;   // g/cm^3 (https://en.wikipedia.org/wiki/Carbon)
    const double center_bottom_base_x = 0.0;
    const double center_bottom_base_y = -0.0284;
    const double center_bottom_base_z = 0.0;
    const double height = 0.00295;
    const double radius = 0.0568;

    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;
    using FloatArrayProperty = Mantid::Kernel::ArrayProperty<double>;
    // material
    auto material = std::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(std::make_unique<StringProperty>("ChemicalFormula", chemical_formula), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleNumberDensity", number_density), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleMassDensity", mass_density), "");
    // geometry
    auto geometry = std::make_shared<Mantid::Kernel::PropertyManager>();
    geometry->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    geometry->declareProperty(std::make_unique<FloatProperty>("Height", height), "");
    geometry->declareProperty(std::make_unique<FloatProperty>("Radius", radius), "");
    std::vector<double> center{center_bottom_base_x, center_bottom_base_y, center_bottom_base_z};
    geometry->declareProperty(std::make_unique<FloatArrayProperty>("Center", std::move(center)), "");
    std::vector<double> cylinderAxis{0, 1, 0};
    geometry->declareProperty(std::make_unique<FloatArrayProperty>("Axis", cylinderAxis), "");
    // set sample
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", ws);
    setsample.setProperty("Material", material);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();

    return ws;
  }
};
