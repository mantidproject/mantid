// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FLATPLATEABSORPTIONTEST_H_
#define FLATPLATEABSORPTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/FlatPlateAbsorption.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::API::MatrixWorkspace_sptr;

class FlatPlateAbsorptionTest : public CxxTest::TestSuite {
public:
  void testNameAndVersion() {
    Mantid::Algorithms::FlatPlateAbsorption atten;
    TS_ASSERT_EQUALS(atten.name(), "FlatPlateAbsorption");
    TS_ASSERT_EQUALS(atten.version(), 1);
  }

  void testInit() {
    Mantid::Algorithms::FlatPlateAbsorption atten;
    TS_ASSERT_THROWS_NOTHING(atten.initialize());
    TS_ASSERT(atten.isInitialized());
  }

  void testExec() {
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    std::string outputWS("factors");
    Mantid::Algorithms::FlatPlateAbsorption atten;
    configureAbsCommon(atten, testWS, outputWS);
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleHeight", "2.3"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleWidth", "1.8"));
    TS_ASSERT_THROWS_NOTHING(atten.setPropertyValue("SampleThickness", "1.5"));
    // it is not clear what material this is
    TS_ASSERT_THROWS_NOTHING(
        atten.setPropertyValue("AttenuationXSection", "6.52"));
    TS_ASSERT_THROWS_NOTHING(
        atten.setPropertyValue("ScatteringXSection", "19.876"));
    TS_ASSERT_THROWS_NOTHING(
        atten.setPropertyValue("SampleNumberDensity", "0.0093"));
    TS_ASSERT_THROWS_NOTHING(atten.execute());
    TS_ASSERT(atten.isExecuted());

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)));
    TS_ASSERT_DELTA(result->readY(0).front(), 0.7389, 0.0001);
    TS_ASSERT_DELTA(result->readY(0)[1], 0.7042, 0.0001);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.4687, 0.0001);
    TS_ASSERT_DELTA(result->readY(1).front(), 0.7389, 0.0001);
    TS_ASSERT_DELTA(result->readY(1)[5], 0.5752, 0.0001);
    TS_ASSERT_DELTA(result->readY(1).back(), 0.4686, 0.0001);

    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  void testWithoutSample() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS = createTestWorkspace();

    Mantid::Algorithms::FlatPlateAbsorption atten;

    // intentionally skip the sample information
    configureAbsCommon(atten, testWS, "factors");
    TS_ASSERT(!atten.isExecuted());
  }

private:
  MatrixWorkspace_sptr createTestWorkspace() {
    // Create a small test workspace
    MatrixWorkspace_sptr testWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    return testWS;
  }

  /// set what is used for all - intentionally skip the sample information
  void configureAbsCommon(Mantid::Algorithms::FlatPlateAbsorption &alg,
                          MatrixWorkspace_sptr &inputWS,
                          const std::string &outputWSname) {
    if (!alg.isInitialized())
      alg.initialize();
    alg.setRethrows(true); // required to get the proper behavior of failed exec

    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty<MatrixWorkspace_sptr>("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputWSname));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("NumberOfWavelengthPoints", "3"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ExpMethod", "Normal"));
  }
};

#endif /*FLATPLATEABSORPTIONTEST_H_*/
