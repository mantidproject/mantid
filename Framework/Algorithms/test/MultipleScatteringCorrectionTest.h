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
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/MultipleScatteringCorrection.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::Algorithms::MultipleScatteringCorrection;
using Mantid::Kernel::Logger;

namespace {
/// static logger
Logger g_log("MultipleScatteringCorrectionTest");
} // namespace

class MultipleScatteringCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultipleScatteringCorrectionTest *createSuite() {
    Mantid::API::FrameworkManager::Instance();
    return new MultipleScatteringCorrectionTest();
  }
  static void destroySuite(MultipleScatteringCorrectionTest *suite) { delete suite; }

  void test_single() {
    // Create a workspace with vanadium data
    const std::string ws_name = "ws_vanadium";
    MakeSampleWorkspaceVanadium(ws_name);
    // note: use MakeSampleWorkspaceDiamond() is decided to use diamond, keep in mind that
    //       CarpenterSampleCorrection does not work on diamond workspace.

    // correct using Mayer correction
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setPropertyValue("InputWorkspace", ws_name);
    unitsAlg->setProperty("Target", "TOF");
    unitsAlg->setPropertyValue("OutputWorkspace", "ws_TOF");
    unitsAlg->execute();
    //
    auto mayerAlg = Mantid::API::AlgorithmManager::Instance().create("MayersSampleCorrection");
    mayerAlg->initialize();
    mayerAlg->setProperty("InputWorkspace", "ws_TOF");
    mayerAlg->setProperty("MultipleScattering", true);
    mayerAlg->setPropertyValue("OutputWorkspace", "rst_mayer");
    mayerAlg->execute();

    // correct using Carpenter correction
    unitsAlg->setPropertyValue("InputWorkspace", ws_name);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->setPropertyValue("OutputWorkspace", "ws_wavelength");
    unitsAlg->execute();
    //
    auto carrAlg = Mantid::API::AlgorithmManager::Instance().create("CarpenterSampleCorrection");
    carrAlg->initialize();
    carrAlg->setPropertyValue("InputWorkspace", "ws_wavelength");
    carrAlg->setPropertyValue("OutputWorkspace", "rst_carpenter");
    carrAlg->execute();

    // correct using multiple scattering correction
    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    // Wavelength target
    msAlg.setPropertyValue("InputWorkspace", "ws_wavelength");
    msAlg.setPropertyValue("OutputWorkspace", "rst_ms");
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());

    // Compare the results
    // auto checker = Mantid::API::AlgorithmManager::Instance().create("CompareWorkspaces");
    // checker->initialize();
    // checker->setPropertyValue("Workspace1", "rst_mayer");
    // checker->setPropertyValue("Workspace2", "rst_ms");
    // checker->setPropertyValue("Tolerance", "1e-4");
    // checker->setProperty("CheckInstrument", false);
    // checker->setProperty("CheckMasking", false);
    // checker->execute();

    // std::string msg = checker->getProperty("Messages");

    // g_log.notice() << msg << '\n';
  }

private:
  /**
   * @brief generate a workspace and register in ADS with given name
   *
   * @param name
   */
  void MakeSampleWorkspace(std::string const &name) {
    // Create a fake workspace with TOF data
    auto sampleAlg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    sampleAlg->initialize();
    sampleAlg->setProperty("Function", "Powder Diffraction");
    sampleAlg->setProperty("NumBanks", 4);
    sampleAlg->setProperty("BankPixelWidth", 1);
    sampleAlg->setProperty("XUnit", "TOF");
    sampleAlg->setProperty("XMin", 1000.0);
    sampleAlg->setProperty("XMax", 10000.0);
    sampleAlg->setPropertyValue("OutputWorkspace", name);
    sampleAlg->execute();

    // edit the instrument geometry
    auto editAlg = Mantid::API::AlgorithmManager::Instance().create("EditInstrumentGeometry");
    editAlg->initialize();
    editAlg->setPropertyValue("Workspace", name);
    editAlg->setProperty("PrimaryFlightPath", 5.0);
    editAlg->setProperty("SpectrumIDs", "1,2,3,4");
    editAlg->setProperty("L2", "2.0,2.0,2.0,2.0");
    editAlg->setProperty("Polar", "10.0,90.0,180.0,90.0");
    editAlg->setProperty("Azimuthal", "0.0,0.0,0.0,45.0");
    editAlg->setProperty("DetectorIDs", "1,2,3,4");
    editAlg->setProperty("InstrumentName", "Instrument");
    editAlg->execute();
  }

  /**
   * @brief make a sample workspace with V
   *
   * @param name
   */
  void MakeSampleWorkspaceVanadium(std::string const &name) {
    // make the workspace with given name
    MakeSampleWorkspace(name);

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
    setsample.setPropertyValue("InputWorkspace", name);
    setsample.setProperty("Material", material);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();
  }

  void MakeSampleWorkspaceDiamond(std::string const &name) {
    MakeSampleWorkspace(name);

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
    setsample.setPropertyValue("InputWorkspace", name);
    setsample.setProperty("Material", material);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();
  }
};
