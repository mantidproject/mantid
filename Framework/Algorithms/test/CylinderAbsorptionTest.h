// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CYLINDERABSORPTIONTEST_H_
#define CYLINDERABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/CylinderAbsorption.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::API::MatrixWorkspace_sptr;

class CylinderAbsorptionTest : public CxxTest::TestSuite {
public:
  void testNameAndVersion() {
    Mantid::Algorithms::CylinderAbsorption atten;
    TS_ASSERT_EQUALS(atten.name(), "CylinderAbsorption");
    TS_ASSERT_EQUALS(atten.version(), 1);
  }

  void testInit() {
    Mantid::Algorithms::CylinderAbsorption atten;
    TS_ASSERT_THROWS_NOTHING(atten.initialize());
    TS_ASSERT(atten.isInitialized());
  }

  void testExec() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    std::string outputWS("factors");
    Mantid::Algorithms::CylinderAbsorption atten;
    configureAbsCommon(atten, testWS, outputWS);
    configureAbsSample(atten);
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_DELTA(result->readY(0).front(), 0.7210, 0.0001);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.2052, 0.0001);
    TS_ASSERT_DELTA(result->readY(0)[8], 0.2356, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testWithoutSample() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    Mantid::Algorithms::CylinderAbsorption atten;
    // required to get the proper behavior of failed exec
    atten.setRethrows(true);

    // intentionally skip the sample information
    configureAbsCommon(atten, testWS, "factors");
    TS_ASSERT_THROWS(atten.execute(), std::invalid_argument);
    TS_ASSERT(!atten.isExecuted());
  }

  void testWithSetSample() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;
    using FloatArrayProperty = Mantid::Kernel::ArrayProperty<double>;

    // create the material
    auto material = boost::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("ChemicalFormula", "V"),
        "");
    material->declareProperty(Mantid::Kernel::make_unique<FloatProperty>(
                                  "SampleNumberDensity", 0.07192),
                              "");

    // create the geometry
    auto geometry = boost::make_shared<Mantid::Kernel::PropertyManager>();
    geometry->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "Cylinder"), "");
    geometry->declareProperty(
        Mantid::Kernel::make_unique<FloatProperty>("Height", 4), "");
    geometry->declareProperty(
        Mantid::Kernel::make_unique<FloatProperty>("Radius", 0.4), "");
    std::vector<double> center{0, 0, 0};
    geometry->declareProperty(Mantid::Kernel::make_unique<FloatArrayProperty>(
                                  "Center", std::move(center)),
                              "");

    // set the sample information
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", testWS);
    setsample.setProperty("Material", material);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();
    testWS = setsample.getProperty("InputWorkspace");

    // run the actual algorithm
    std::string outputWS("factors");
    Mantid::Algorithms::CylinderAbsorption atten;
    configureAbsCommon(atten, testWS, outputWS);

    // the geometry was set on the input workspace
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_DELTA(result->readY(0).front(), 0.7210, 0.0001);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.2052, 0.0001);
    TS_ASSERT_DELTA(result->readY(0)[8], 0.2356, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  // This is identical to testWithSetSample except the number of segments to split into is larger. The number of segments are taken from the WISH system test.
  void testWithSetSampleLotsOfSegments() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;
    using FloatArrayProperty = Mantid::Kernel::ArrayProperty<double>;

    // create the material
    auto material = boost::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("ChemicalFormula", "V"),
        "");
    material->declareProperty(Mantid::Kernel::make_unique<FloatProperty>(
                                  "SampleNumberDensity", 0.07192),
                              "");

    // create the geometry
    auto geometry = boost::make_shared<Mantid::Kernel::PropertyManager>();
    geometry->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "Cylinder"), "");
    geometry->declareProperty(
        Mantid::Kernel::make_unique<FloatProperty>("Height", 4), "");
    geometry->declareProperty(
        Mantid::Kernel::make_unique<FloatProperty>("Radius", 0.4), "");
    std::vector<double> center{0, 0, 0};
    geometry->declareProperty(Mantid::Kernel::make_unique<FloatArrayProperty>(
                                  "Center", std::move(center)),
                              "");

    // set the sample information
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setProperty("InputWorkspace", testWS);
    setsample.setProperty("Material", material);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();
    testWS = setsample.getProperty("InputWorkspace");

    // run the actual algorithm
    std::string outputWS("factors");
    Mantid::Algorithms::CylinderAbsorption atten;
    configureAbsCommon(atten, testWS, outputWS);
    // set the number of segments to be really small like WISH system test
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("NumberOfSlices",
                                                    "10"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("NumberOfAnnuli",
                                                    "10"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("NumberOfWavelengthPoints",
                                                    "25"));
    // the geometry was set on the input workspace
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));

    // these values are different than testWithSetSample because of the smaller segment sizes
    TS_ASSERT_DELTA(result->readY(0).front(), 0.7286, 0.0001);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.2213, 0.0001);
    TS_ASSERT_DELTA(result->readY(0)[8], 0.2517, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }


  void testInelastic() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    const std::string outputWS("factors");
    Mantid::Algorithms::CylinderAbsorption atten;
    configureAbsCommon(atten, testWS, outputWS);
    configureAbsSample(atten);
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("EMode", "Indirect"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("EFixed", "1.845"));
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_DELTA(result->readY(0).front(), 0.4796, 0.0001);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.2510, 0.0001);
    TS_ASSERT_DELTA(result->readY(0)[2], 0.4110, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  MatrixWorkspace_sptr createTestWorkspace() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    return testWS;
  }

  /// set what is used for all - intentionally skip the sample information
  void configureAbsCommon(Mantid::Algorithms::CylinderAbsorption &alg,
                          MatrixWorkspace_sptr &inputWS,
                          const std::string &outputWSname) {
    if (!alg.isInitialized())
      alg.initialize();

    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NumberOfSlices", "2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NumberOfAnnuli", "2"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NumberOfWavelengthPoints", "5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ExpMethod", "Normal"));
  }

  void configureAbsSample(Mantid::Algorithms::CylinderAbsorption &alg) {
    // sizes in cm
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("CylinderSampleHeight", "4"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("CylinderSampleRadius", "0.4"));
    // values for vanadium
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("AttenuationXSection", "5.08"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ScatteringXSection", "5.1"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("SampleNumberDensity", "0.07192"));
  }
};

#endif /*CYLINDERABSORPTIONTEST_H_*/
