// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/MultipleScatteringCorrection.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/UnitFactory.h"
#include "SampleFrameEquivalence.h"

#include <array>

using Mantid::Algorithms::MultipleScatteringCorrection;
using Mantid::API::AnalysisDataService;
using Mantid::Kernel::Logger;

namespace {
/// static logger
Logger g_log("MultipleScatteringCorrectionTest");
} // namespace

class MultipleScatteringCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultipleScatteringCorrectionTest *createSuite() { return new MultipleScatteringCorrectionTest(); }
  static void destroySuite(MultipleScatteringCorrectionTest *suite) { delete suite; }

  void test_single() {
    // Create a workspace with vanadium data
    const std::string ws_name = "ws_vanadium";
    MakeSampleWorkspaceVanadium(ws_name);

    // to wavelength
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setPropertyValue("InputWorkspace", ws_name);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->setPropertyValue("OutputWorkspace", "ws_wavelength");
    unitsAlg->execute();

    // correct using multiple scattering correction
    // NOTE:
    // using smaller element size will dramatically increase the computing time, and it might lead to a
    // memory allocation error from std::vector
    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    // Wavelength target
    msAlg.setPropertyValue("InputWorkspace", "ws_wavelength");
    msAlg.setPropertyValue("Method", "SampleOnly");
    msAlg.setPropertyValue("OutputWorkspace", "rst_ms");
    // msAlg.setProperty("ElementSize", 0.4); // mm
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());

    // Get the results from multiple scattering correction
    Mantid::API::MatrixWorkspace_sptr rst_ms =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_sampleOnly");

    // Given the current condition, we can only verify with some static values calculate using the current version
    // of multiple scattering correction.
    // This is mostly to make sure other changes that impacting multiple scattering correction can be caught early,
    // and the reference values here are by no means physically correct
    TS_ASSERT_DELTA(rst_ms->y(0)[0], 0.184945, 1e-3);
    TS_ASSERT_DELTA(rst_ms->y(0)[1], 0.182756, 1e-3);
    TS_ASSERT_DELTA(rst_ms->y(1)[0], 0.184469, 1e-3);
    TS_ASSERT_DELTA(rst_ms->y(1)[1], 0.182175, 1e-3);
  }

  void test_both_ways_of_orienting_the_sample_agree() {
    // A sample in its own frame with the goniometer on the run, and the same sample already rotated
    // into the lab frame, describe the same experiment and must correct identically.
    const auto rotation = SampleFrameEquivalence::rotationY(30.0);
    const auto ownFrame = runPlateCorrection("ms_own", rotation, false);
    const auto labFrame = runPlateCorrection("ms_lab", rotation, true);

    TS_ASSERT_DELTA(ownFrame[0], labFrame[0], 1e-9);
    TS_ASSERT_DELTA(ownFrame[1], labFrame[1], 1e-9);
    // and the rotation actually mattered - otherwise the assertions above prove nothing
    const auto unrotated = runPlateCorrection("ms_flat", Mantid::Kernel::Matrix<double>(3, 3, true), false);
    TS_ASSERT(std::abs(ownFrame[0] - unrotated[0]) > 1e-6);
  }

  void test_both_ways_of_orienting_the_sample_agree_with_a_container() {
    // Same invariant as above, for the other Method. The container is not goniometer-rotated by
    // anything in Mantid, so only the sample moves - but it must move, and by the same amount
    // whichever frame it arrived in.
    const auto rotation = SampleFrameEquivalence::rotationX(60.0);
    const auto ownFrame = runSampleAndContainerCorrection("msc_own", rotation, false);
    const auto labFrame = runSampleAndContainerCorrection("msc_lab", rotation, true);

    TS_ASSERT_DELTA(ownFrame[0], labFrame[0], 1e-9);
    TS_ASSERT_DELTA(ownFrame[1], labFrame[1], 1e-9);
    // and the rotation actually mattered - otherwise the assertions above prove nothing
    const auto unrotated =
        runSampleAndContainerCorrection("msc_flat", Mantid::Kernel::Matrix<double>(3, 3, true), false);
    TS_ASSERT(std::abs(ownFrame[0] - unrotated[0]) > 1e-6);
  }

  void test_sampleAndContainer() {
    // Create a workspace with vanadium data
    const std::string ws_name = "mstest";
    MakeSampleWorkspaceWithContainer(ws_name);

    // to wavelength
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setPropertyValue("InputWorkspace", ws_name);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->setPropertyValue("OutputWorkspace", ws_name);
    unitsAlg->execute();

    // correct using multiple scattering correction
    // NOTE:
    // using smaller element size will dramatically increase the computing time, and it might lead to a
    // memory allocation error from std::vector
    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    // Wavelength target
    msAlg.setPropertyValue("InputWorkspace", ws_name);
    msAlg.setPropertyValue("Method", "SampleOnly");
    msAlg.setPropertyValue("OutputWorkspace", "rst_ms");
    msAlg.setProperty("ElementSize", 0.5); // mm
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());
    Mantid::API::MatrixWorkspace_sptr rst_ms_sampleOnly =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_sampleOnly");

    //
    msAlg.initialize();
    msAlg.setPropertyValue("InputWorkspace", ws_name);
    msAlg.setPropertyValue("Method", "SampleAndContainer");
    msAlg.setProperty("ElementSize", 0.5); // mm
    msAlg.setPropertyValue("OutputWorkspace", "rst_ms");
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());
    Mantid::API::MatrixWorkspace_sptr rst_ms_containerOnly =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_containerOnly");
    Mantid::API::MatrixWorkspace_sptr rst_ms_sampleAndContainer =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_sampleAndContainer");

    TS_ASSERT_DELTA(rst_ms_sampleOnly->y(0)[0], 0.0923619, 1e-3);
    TS_ASSERT_DELTA(rst_ms_containerOnly->y(0)[0], 0.223564, 1e-3);
    TS_ASSERT_DELTA(rst_ms_sampleAndContainer->y(0)[0], 0.109557, 1e-3);
  }

private:
  /// Correct a plate sample held in the given frame and return the two spectra values. A plate is
  /// used rather than the vanadium cylinder because tilting a plate about y changes how much
  /// material the beam crosses, so the sample's orientation is visible in the answer.
  std::array<double, 2> runPlateCorrection(std::string const &name, const Mantid::Kernel::Matrix<double> &rotation,
                                           const bool baked) {
    MakeSampleWorkspace(name);
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setPropertyValue("InputWorkspace", name);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->setPropertyValue("OutputWorkspace", name + "_wl");
    unitsAlg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(name + "_wl");
    if (baked) {
      SampleFrameEquivalence::setSampleInLabFrame(*ws, rotation);
    } else {
      SampleFrameEquivalence::setSampleInOwnFrame(*ws, rotation);
    }

    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    msAlg.setPropertyValue("InputWorkspace", name + "_wl");
    msAlg.setPropertyValue("Method", "SampleOnly");
    // mm. Must be comfortably smaller than the 4 mm plate thickness or the unrotated case dices to
    // nothing, while the rotated one still fits elements into its larger bounding box.
    msAlg.setProperty("ElementSize", 2.0);
    msAlg.setPropertyValue("OutputWorkspace", name + "_rst");
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());

    auto rst = AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(name + "_rst_sampleOnly");
    return {rst->y(0)[0], rst->y(1)[0]};
  }

  /// Correct a rod sample sitting inside a hollow cylinder can, with the sample held in the given
  /// frame, and return the two spectra values. A rod standing along y is used because tilting it
  /// about x turns it towards the beam and changes how much material the beam crosses; it is small
  /// enough that a 60 degree tilt still leaves it inside the can's bore.
  std::array<double, 2> runSampleAndContainerCorrection(std::string const &name,
                                                        const Mantid::Kernel::Matrix<double> &rotation,
                                                        const bool baked) {
    MakeSampleWorkspace(name);

    auto setSampleAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SetSample");
    setSampleAlg->setRethrows(true);
    setSampleAlg->initialize();
    setSampleAlg->setPropertyValue("InputWorkspace", name);
    setSampleAlg->setPropertyValue("Material", R"({"ChemicalFormula":"V", "SampleNumberDensity": 0.0721})");
    setSampleAlg->setPropertyValue("Geometry",
                                   R"({"Shape": "Cylinder", "Height": 0.6, "Radius": 0.15, "Center": [0., 0., 0.]})");
    setSampleAlg->setPropertyValue("ContainerMaterial", R"({"ChemicalFormula":"V", "SampleNumberDensity": 0.0721})");
    setSampleAlg->setPropertyValue(
        "ContainerGeometry",
        R"({"Shape": "HollowCylinder", "Height": 0.8, "InnerRadius": 0.4, "OuterRadius": 0.5, "Center": [0., 0., 0.]})");
    TS_ASSERT_THROWS_NOTHING(setSampleAlg->execute());

    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setPropertyValue("InputWorkspace", name);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->setPropertyValue("OutputWorkspace", name + "_wl");
    unitsAlg->execute();

    // Replace only the sample shape, leaving the environment SetSample built in place. The rod is
    // given in metres, matching the 0.6 cm height and 0.15 cm radius set above.
    const std::string rodXML = "<cylinder id=\"rod\">"
                               "<centre-of-bottom-base x=\"0.0\" y=\"-0.003\" z=\"0.0\"/>"
                               "<axis x=\"0.0\" y=\"1.0\" z=\"0.0\"/>"
                               "<radius val=\"0.0015\"/>"
                               "<height val=\"0.006\"/>"
                               "</cylinder>";
    auto ws = AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(name + "_wl");
    const auto material = ws->sample().getShape().material();
    if (baked) {
      SampleFrameEquivalence::setSampleInLabFrame(*ws, rotation, material, rodXML);
    } else {
      SampleFrameEquivalence::setSampleInOwnFrame(*ws, rotation, material, rodXML);
    }

    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    msAlg.setPropertyValue("InputWorkspace", name + "_wl");
    msAlg.setPropertyValue("Method", "SampleAndContainer");
    msAlg.setProperty("ElementSize", 0.5);          // mm, comfortably below the 3 mm rod diameter
    msAlg.setProperty("ContainerElementSize", 1.5); // mm, coarser - the can only has to be present
    msAlg.setPropertyValue("OutputWorkspace", name + "_rst");
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());

    auto rst =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(name + "_rst_sampleAndContainer");
    return {rst->y(0)[0], rst->y(1)[0]};
  }

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
    sampleAlg->setProperty("NumBanks", 2);
    sampleAlg->setProperty("BankPixelWidth", 1);
    sampleAlg->setProperty("XUnit", "TOF");
    sampleAlg->setProperty("XMin", 1000.0);
    sampleAlg->setProperty("XMax", 1500.0);
    sampleAlg->setPropertyValue("OutputWorkspace", name);
    sampleAlg->execute();

    // edit the instrument geometry
    auto editAlg = Mantid::API::AlgorithmManager::Instance().create("EditInstrumentGeometry");
    editAlg->initialize();
    editAlg->setPropertyValue("Workspace", name);
    editAlg->setProperty("PrimaryFlightPath", 5.0);
    editAlg->setProperty("SpectrumIDs", "1,2");
    editAlg->setProperty("L2", "2.0,2.0");
    editAlg->setProperty("Polar", "10.0,90.0");
    editAlg->setProperty("Azimuthal", "0.0,45.0");
    editAlg->setProperty("DetectorIDs", "1,2");
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
    const double center_bottom_base_x = 0.0;
    const double center_bottom_base_y = -0.0284;
    const double center_bottom_base_z = 0.0;
    const double height = 2.95;  // cm
    const double radius = 0.568; // cm

    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;
    using FloatArrayProperty = Mantid::Kernel::ArrayProperty<double>;
    // material
    auto material = std::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(std::make_unique<StringProperty>("ChemicalFormula", chemical_formula), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleNumberDensity", number_density), "");
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

  void MakeSampleWorkspaceWithContainer(std::string const &name) {
    // make the workspace with given name
    MakeSampleWorkspace(name);

    auto setSampleAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SetSample");
    setSampleAlg->setRethrows(true);
    setSampleAlg->initialize();
    setSampleAlg->setPropertyValue("InputWorkspace", name);
    setSampleAlg->setPropertyValue("Material",
                                   R"({"ChemicalFormula": "La-(B11)5.94-(B10)0.06", "SampleNumberDensity": 0.1})");
    setSampleAlg->setPropertyValue("Geometry",
                                   R"({"Shape": "Cylinder", "Height": 1.0, "Radius": 0.2, "Center": [0., 0., 0.]})");
    setSampleAlg->setPropertyValue("ContainerMaterial", R"({"ChemicalFormula":"V", "SampleNumberDensity": 0.0721})");
    setSampleAlg->setPropertyValue(
        "ContainerGeometry",
        R"({"Shape": "HollowCylinder", "Height": 1.0, "InnerRadius": 0.2, "OuterRadius": 0.3, "Center": [0., 0., 0.]})");
    TS_ASSERT_THROWS_NOTHING(setSampleAlg->execute());
  }
};
