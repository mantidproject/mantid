// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_MDTRANSFQ3D_H_
#define MANTID_MDALGORITHMS_MDTRANSFQ3D_H_

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidMDAlgorithms/MDTransfQ3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

using Mantid::coord_t;

namespace {

class MDTransfQ3DTestHelper : public MDTransfQ3D {
public:
  bool getLorentzCorr() const { return m_isLorentzCorrected; }
  double const *getSinThetaArray() const { return m_SinThetaSqArray; }
  double getCurSinThetaSq() const { return m_SinThetaSq; }
};

std::tuple<MDTransfQ3DTestHelper, MDWSDescription>
createTestTransform(const double efixed, const DeltaEMode::Type emode) {
  auto indirectInelasticWS =
      WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(
          4, 10, true);
  // Set efixed. Test helpers buildPreprocessedDetectorsWorkspace sets efixed
  // column to the value of Ei in the log
  indirectInelasticWS->mutableRun().addProperty("Ei", efixed, "meV", true);
  const int ndimensions{4};
  MDWSDescription wsDescription(ndimensions);
  MDTransfQ3DTestHelper q3dTransform;
  // set wide limits to catch everything
  wsDescription.setMinMax({-100, -100, -100, -100}, {100, 100, 100, 100});
  wsDescription.buildFromMatrixWS(indirectInelasticWS, q3dTransform.transfID(),
                                  DeltaEMode::asString(emode), {});
  wsDescription.m_PreprDetTable =
      WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(
          indirectInelasticWS);
  q3dTransform.initialize(wsDescription);

  return std::make_tuple(q3dTransform, wsDescription);
}

} // namespace

class MDTransfQ3DTest : public CxxTest::TestSuite {
public:
  static MDTransfQ3DTest *createSuite() { return new MDTransfQ3DTest(); }
  static void destroySuite(MDTransfQ3DTest *suite) { delete suite; }

  void testWSDescriptionPart() {

    MDTransfQ3D Q3DTransformer;
    TS_ASSERT_EQUALS("Q3D", Q3DTransformer.transfID())

    TS_ASSERT_EQUALS(4, Q3DTransformer.getNMatrixDimensions(DeltaEMode::Direct))
    TS_ASSERT_EQUALS(3,
                     Q3DTransformer.getNMatrixDimensions(DeltaEMode::Elastic))
    TS_ASSERT_EQUALS(4,
                     Q3DTransformer.getNMatrixDimensions(DeltaEMode::Indirect))
  }

  void testWSDescrUnitsPart() {
    MDTransfQ3D Q3DTransformer;
    std::vector<std::string> outputDimUnits;

    TS_ASSERT_THROWS_NOTHING(
        outputDimUnits = Q3DTransformer.outputUnitID(DeltaEMode::Direct))
    TS_ASSERT_EQUALS(4, outputDimUnits.size())
    TS_ASSERT_EQUALS("MomentumTransfer", outputDimUnits[0])
    TS_ASSERT_EQUALS("MomentumTransfer", outputDimUnits[1])
    TS_ASSERT_EQUALS("MomentumTransfer", outputDimUnits[2])
    TS_ASSERT_EQUALS("DeltaE", outputDimUnits[3])

    TS_ASSERT_THROWS_NOTHING(
        outputDimUnits = Q3DTransformer.outputUnitID(DeltaEMode::Elastic))
    TS_ASSERT_EQUALS(3, outputDimUnits.size())
  }

  void testWSDescrIDPart() {
    MDTransfQ3D Q3DTransformer;
    std::vector<std::string> outputDimID;

    TS_ASSERT_THROWS_NOTHING(
        outputDimID = Q3DTransformer.getDefaultDimID(DeltaEMode::Direct))
    TS_ASSERT_EQUALS(4, outputDimID.size())
    TS_ASSERT_EQUALS("Q1", outputDimID[0])
    TS_ASSERT_EQUALS("Q2", outputDimID[1])
    TS_ASSERT_EQUALS("Q3", outputDimID[2])
    TS_ASSERT_EQUALS("DeltaE", outputDimID[3])

    TS_ASSERT_THROWS_NOTHING(
        outputDimID = Q3DTransformer.getDefaultDimID(DeltaEMode::Elastic))
    TS_ASSERT_EQUALS(3, outputDimID.size())
    TS_ASSERT_EQUALS("Q1", outputDimID[0])
    TS_ASSERT_EQUALS("Q2", outputDimID[1])
    TS_ASSERT_EQUALS("Q3", outputDimID[2])
  }

  void testWSDescrInputUnitID() {
    MDTransfQ3D Q3DTransformer;
    std::string inputUnitID;

    TS_ASSERT_THROWS_NOTHING(inputUnitID =
                                 Q3DTransformer.inputUnitID(DeltaEMode::Direct))
    TS_ASSERT_EQUALS("DeltaE", inputUnitID)

    TS_ASSERT_THROWS_NOTHING(
        inputUnitID = Q3DTransformer.inputUnitID(DeltaEMode::Indirect))
    TS_ASSERT_EQUALS("DeltaE", inputUnitID)

    TS_ASSERT_THROWS_NOTHING(
        inputUnitID = Q3DTransformer.inputUnitID(DeltaEMode::Elastic))
    TS_ASSERT_EQUALS("Momentum", inputUnitID)
  }

  void testElasticModeWithLorentzCorrection() {
    MDTransfQ3DTestHelper Q3DTransf;
    TSM_ASSERT("Should not be lorentz corrected by default ",
               !Q3DTransf.getLorentzCorr())

    const auto QMode = Q3DTransf.transfID();
    const auto dEMode = DeltaEMode::asString(DeltaEMode::Elastic);
    const std::vector<std::string> dimPropNames{"T", "Ei"};
    auto elasticTestWS = WorkspaceCreationHelper::
        createProcessedWorkspaceWithCylComplexInstrument(4, 10, true);
    // rotate the crystal by twenty degrees back;
    elasticTestWS->mutableRun().mutableGoniometer().setRotationAngle(0, 20);
    // add workspace energy
    elasticTestWS->mutableRun().addProperty("Ei", 13., "meV", true);
    elasticTestWS->mutableRun().addProperty("T", 70., "K", true);
    MDWSDescription wsDescription(5);
    wsDescription.buildFromMatrixWS(elasticTestWS, QMode, dEMode, dimPropNames);

    TSM_ASSERT_THROWS(
        "No detectors yet defined, so should throw run time error: ",
        Q3DTransf.initialize(wsDescription), const std::runtime_error &)

    // let's preprocess detectors positions to go any further
    wsDescription.m_PreprDetTable =
        WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(
            elasticTestWS);
    // let's set 2Theta=0 for simplicity and violate const correctness for
    // testing purposes here
    auto &TwoTheta = const_cast<std::vector<double> &>(
        wsDescription.m_PreprDetTable->getColVector<double>("TwoTheta"));
    for (double &i : TwoTheta) {
      i = 0;
    }

    TSM_ASSERT_THROWS_NOTHING("should initialize properly: ",
                              Q3DTransf.initialize(wsDescription))
    TSM_ASSERT("Should not be lorentz corrected by default ",
               !Q3DTransf.getLorentzCorr())

    wsDescription.setLorentsCorr(true);
    TSM_ASSERT_THROWS_NOTHING("should initialize properly: ",
                              Q3DTransf.initialize(wsDescription))
    TSM_ASSERT("Lorentz corrections should be now set ",
               Q3DTransf.getLorentzCorr())

    TSM_ASSERT("Array of sin(Theta)^2 should be defined: ",
               Q3DTransf.getSinThetaArray())

    // specify a 5D vector to accept MD coordinates
    std::vector<coord_t> coord(5);
    TSM_ASSERT("Generic coordinates should be in range, so should be true ",
               Q3DTransf.calcGenericVariables(coord, 5))
    TSM_ASSERT_DELTA("4th Generic coordinates should be temperature ", 70,
                     coord[3], 2.e-8)
    TSM_ASSERT_DELTA("5th Generic coordinates should be Ei ", 13, coord[4],
                     2.e-8)

    TSM_ASSERT(
        " Y-dependent coordinates should be in range so it should be true: ",
        Q3DTransf.calcYDepCoordinates(coord, 0))
    TSM_ASSERT_DELTA("Sin(theta)^2 should be set to 0 by previous command ", 0,
                     Q3DTransf.getCurSinThetaSq(), 2.e-8)

    double signal(1), errorSq(1);
    TSM_ASSERT(
        " Matrix coordinates should be in range so function return true: ",
        Q3DTransf.calcMatrixCoord(10, coord, signal, errorSq))

    TSM_ASSERT_DELTA(" But lorentz corrections for the detector placed on the "
                     "beam path should set signal to 0 ",
                     0, signal, 2.e-8)
    TSM_ASSERT_DELTA(" But lorentz corrections for the detector placed on the "
                     "beam path should set  error to 0 ",
                     0, errorSq, 2.e-8)
  }

  void testDirectInelasticMode() {
    // setup
    const double efixed{13.};
    MDTransfQ3DTestHelper q3dTransform;
    MDWSDescription wsDescription;
    std::tie(q3dTransform, wsDescription) =
        createTestTransform(efixed, DeltaEMode::Direct);

    // convert
    const double deltaE{1.2};
    std::vector<coord_t> qOmega{0.0, 0.0, 0.0, 0.0};
    q3dTransform.calcYDepCoordinates(qOmega, 0);
    double signal{1.0}, error{1.0};
    q3dTransform.calcMatrixCoord(deltaE, qOmega, signal, error);

    // assert
    const auto detDir{
        wsDescription.m_PreprDetTable->getColVector<V3D>("DetDirections")[0]};
    const double kfinal{
        sqrt((efixed - deltaE) /
             Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq)};
    TS_ASSERT_DELTA(-detDir[0] * kfinal, qOmega[0], 1e-05)
    TS_ASSERT_DELTA(-detDir[1] * kfinal, qOmega[1], 1e-05)
    const double kfixed{
        sqrt(efixed / Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq)};
    TS_ASSERT_DELTA(kfixed - detDir[2] * kfinal, qOmega[2], 1e-05)
    TS_ASSERT_DELTA(deltaE, qOmega[3], 1e-05)
    TS_ASSERT_DELTA(1.0, signal, 1e-05)
    TS_ASSERT_DELTA(1.0, error, 1e-05)
  }

  void testIndirectInelasticMode() {
    // setup
    const double efixed{2.45};
    MDTransfQ3DTestHelper q3dTransform;
    MDWSDescription wsDescription;
    std::tie(q3dTransform, wsDescription) =
        createTestTransform(efixed, DeltaEMode::Indirect);

    // convert
    const double deltaE{0.8};
    std::vector<coord_t> qOmega{0.0, 0.0, 0.0, 0.0};
    q3dTransform.calcYDepCoordinates(qOmega, 0);
    double signal{1.0}, error{1.0};
    q3dTransform.calcMatrixCoord(deltaE, qOmega, signal, error);

    // assert
    const auto detDir{
        wsDescription.m_PreprDetTable->getColVector<V3D>("DetDirections")[0]};
    const double kfixed{
        sqrt(efixed / Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq)};
    TS_ASSERT_DELTA(-detDir[0] * kfixed, qOmega[0], 1e-05)
    TS_ASSERT_DELTA(-detDir[1] * kfixed, qOmega[1], 1e-05)
    const double ki{
        sqrt((efixed + deltaE) /
             Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq)};
    TS_ASSERT_DELTA(ki - detDir[2] * kfixed, qOmega[2], 1e-05)
    TS_ASSERT_DELTA(deltaE, qOmega[3], 1e-05)
    TS_ASSERT_DELTA(1.0, signal, 1e-05)
    TS_ASSERT_DELTA(1.0, error, 1e-05)
  }
};

#endif
