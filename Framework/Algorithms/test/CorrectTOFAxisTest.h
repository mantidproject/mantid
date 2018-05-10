#ifndef MANTID_ALGORITHMS_CORRECTTOFAXISTEST_H_
#define MANTID_ALGORITHMS_CORRECTTOFAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/CorrectTOFAxis.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::CorrectTOFAxis;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

class CorrectTOFAxisTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CorrectTOFAxisTest *createSuite() { return new CorrectTOFAxisTest(); }
  static void destroySuite(CorrectTOFAxisTest *suite) { delete suite; }

  void test_CorrectionUsingReferenceWorkspace() {
    const size_t blocksize = 16;
    const double x0 = 23.66;
    const double dx = 0.05;
    const double TOF = x0 + dx * 3 * blocksize / 4;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, TOF);
    const double referenceTOF = 1.06 * TOF;
    const double length = flightLengthIN4(inputWs);
    const double referenceEi = incidentEnergy(referenceTOF, length);
    const double referenceWavelength = wavelength(referenceEi, length);
    auto referenceWs = createInputWorkspace(blocksize, x0, dx, referenceTOF);
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ReferenceWorkspace", referenceWs));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outputWs);
    TS_ASSERT_EQUALS(outputWs->run().getPropertyAsSingleValue("EI"),
                     referenceEi)
    TS_ASSERT_EQUALS(outputWs->run().getPropertyAsSingleValue("wavelength"),
                     referenceWavelength)
    for (size_t i = 0; i < inputWs->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < blocksize; ++j) {
        TS_ASSERT_DELTA(outputWs->x(i)[j], referenceWs->x(i)[j], 1e-6)
        TS_ASSERT_EQUALS(outputWs->y(i)[j], inputWs->y(i)[j])
        TS_ASSERT_EQUALS(outputWs->e(i)[j], inputWs->e(i)[j])
      }
      TS_ASSERT_DELTA(outputWs->x(i).back(), referenceWs->x(i).back(), 1e-6)
    }
  }

  void test_CorrectionUsingEPPTable() {
    const size_t blocksize = 512;
    const double x0 = 1402;
    const double dx = 0.23;
    const size_t eppIndex = blocksize / 3;
    const double eppTOF = x0 + static_cast<double>(eppIndex) * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    std::vector<EPPTableRow> eppRows(inputWs->getNumberHistograms());
    for (auto &row : eppRows) {
      row.peakCentre = eppTOF;
    }
    const double length = flightLengthIN4(inputWs);
    const double nominalEi = incidentEnergy(eppTOF, length);
    inputWs->mutableRun().addProperty("EI", nominalEi, true);
    const double nominalWavelength = wavelength(nominalEi, length);
    inputWs->mutableRun().addProperty("wavelength", nominalWavelength, true);
    const double actualEi = 1.05 * nominalEi;
    const double actualElasticTOF = tof(actualEi, length);
    const double actualWavelength = wavelength(actualEi, length);
    const double TOFShift = actualElasticTOF - eppTOF;
    ITableWorkspace_sptr eppTable = createEPPTableWorkspace(eppRows);
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("EPPTable", eppTable));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("IndexType", "Workspace Index"))
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("ReferenceSpectra", "1-300"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("EFixed", actualEi))
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    assertTOFShift(outputWs, inputWs, actualEi, actualWavelength, TOFShift);
  }

  void test_CorrectionUsingElasticBinIndexAndL2() {
    const size_t blocksize = 512;
    const double x0 = 1402;
    const double dx = 0.23;
    const size_t eppIndex = blocksize / 3;
    const double eppTOF = x0 + static_cast<double>(eppIndex) * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    const double length = flightLengthIN4(inputWs);
    const double nominalEi = incidentEnergy(eppTOF, length);
    inputWs->mutableRun().addProperty("EI", nominalEi, true);
    const double nominalWavelength = wavelength(nominalEi, length);
    inputWs->mutableRun().addProperty("wavelength", nominalWavelength, true);
    const double actualEi = 1.05 * nominalEi;
    const double actualElasticTOF = tof(actualEi, length);
    const double actualWavelength = wavelength(actualEi, length);
    const double TOFShift = actualElasticTOF - eppTOF;
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ElasticBinIndex", static_cast<int>(eppIndex)))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("EFixed", actualEi))
    const double l2 = inputWs->spectrumInfo().l2(13);
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("L2", l2))
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    assertTOFShift(outputWs, inputWs, actualEi, actualWavelength, TOFShift);
  }

  void test_CorrectionUsingElasticBinIndexAndReferenceSpectra() {
    const size_t blocksize = 512;
    const double x0 = 1402;
    const double dx = 0.23;
    const size_t eppIndex = blocksize / 3;
    const double eppTOF = x0 + static_cast<double>(eppIndex) * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    const double length = flightLengthIN4(inputWs);
    const double nominalEi = incidentEnergy(eppTOF, length);
    inputWs->mutableRun().addProperty("EI", nominalEi, true);
    const double nominalWavelength = wavelength(nominalEi, length);
    inputWs->mutableRun().addProperty("wavelength", nominalWavelength, true);
    const double actualEi = 1.05 * nominalEi;
    const double actualElasticTOF = tof(actualEi, length);
    const double actualWavelength = wavelength(actualEi, length);
    const double TOFShift = actualElasticTOF - eppTOF;
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("IndexType", "Workspace Index"))
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("ReferenceSpectra", "1-300"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ElasticBinIndex", static_cast<int>(eppIndex)))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("EFixed", actualEi))
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    assertTOFShift(outputWs, inputWs, actualEi, actualWavelength, TOFShift);
  }

  void test_FailureIfNoInputPropertiesSet() {
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
    TS_ASSERT(!alg->isExecuted());
  }

  void test_FailureIfOnlyInputAndOutputWorkspacesSet() {
    const size_t blocksize = 128;
    const double x0 = 1431;
    const double dx = 0.33;
    const double eppTOF = x0 + blocksize / 4 * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
    TS_ASSERT(!alg->isExecuted());
  }

  void test_FailureIfReferenceWorkspaceIncompatible() {
    const size_t blocksize = 16;
    const double x0 = 23.66;
    const double dx = 0.05;
    const double TOF = x0 + blocksize * dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, TOF);
    auto referenceWs = createInputWorkspace(blocksize - 1, x0, dx, TOF);
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ReferenceWorkspace", referenceWs));
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
    TS_ASSERT(!alg->isExecuted());
  }

  void test_FailureNoEiGivenAtAllWithElasticBinIndex() {
    const size_t blocksize = 512;
    const double x0 = 1390.1;
    const double dx = 0.24;
    const size_t elasticBin = blocksize / 3;
    const double eppTOF = x0 + static_cast<double>(elasticBin) * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    inputWs->mutableRun().removeProperty("EI");
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("IndexType", "Workspace Index"))
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("ReferenceSpectra", "1-300"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ElasticBinIndex", static_cast<int>(elasticBin)))
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
    TS_ASSERT(!alg->isExecuted());
  }

  void test_FailureNoEiGivenAtAllWithEPPTable() {
    const size_t blocksize = 512;
    const double x0 = 1390.1;
    const double dx = 0.24;
    const double eppTOF = x0 + blocksize / 3 * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    inputWs->mutableRun().removeProperty("EI");
    std::vector<EPPTableRow> eppRows(inputWs->getNumberHistograms());
    for (auto &row : eppRows) {
      row.peakCentre = eppTOF;
    }
    ITableWorkspace_sptr eppTable = createEPPTableWorkspace(eppRows);
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("EPPTable", eppTable));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("IndexType", "Workspace Index"))
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("ReferenceSpectra", "1-300"))
    TS_ASSERT_THROWS_ANYTHING(alg->execute());
    TS_ASSERT(!alg->isExecuted());
  }

  void test_Init() {
    CorrectTOFAxis alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_SampleLogsMissingInReferenceWorkspace() {
    const size_t blocksize = 16;
    const double x0 = 23.66;
    const double dx = 0.05;
    const double TOF = x0 + dx * 3 * blocksize / 4;
    auto inputWs =
        createInputWorkspaceWithoutSampleLogs(blocksize, x0, dx, TOF);
    const double referenceTOF = 1.06 * TOF;
    auto referenceWs = createInputWorkspace(blocksize, x0, dx, referenceTOF);
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg->setProperty("ReferenceWorkspace", referenceWs));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outputWs);
    TS_ASSERT(!outputWs->run().hasProperty("Ei"))
    TS_ASSERT(!outputWs->run().hasProperty("wavelength"))
    for (size_t i = 0; i < inputWs->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < blocksize; ++j) {
        TS_ASSERT_DELTA(outputWs->x(i)[j], referenceWs->x(i)[j], 1e-6)
        TS_ASSERT_EQUALS(outputWs->y(i)[j], inputWs->y(i)[j])
        TS_ASSERT_EQUALS(outputWs->e(i)[j], inputWs->e(i)[j])
      }
      TS_ASSERT_DELTA(outputWs->x(i).back(), referenceWs->x(i).back(), 1e-6)
    }
  }

  void test_UseEiFromSampleLogs() {
    const size_t blocksize = 512;
    const double x0 = 1390.1;
    const double dx = 0.24;
    const double eppTOF = x0 + blocksize / 3 * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    const double length = flightLengthIN4(inputWs);
    const double nominalEi = incidentEnergy(eppTOF, length);
    const double actualEi = 0.93 * nominalEi;
    inputWs->mutableRun().addProperty("EI", actualEi, true);
    const double actualElasticTOF = tof(actualEi, length);
    const double TOFShift = actualElasticTOF - eppTOF;
    // In this case the algorithm doesn't update the wavelength in
    // the samplelogs since also Ei will not be updated.
    const double originalWavelength = wavelength(nominalEi, length);
    std::vector<EPPTableRow> eppRows(inputWs->getNumberHistograms());
    for (auto &row : eppRows) {
      row.peakCentre = eppTOF;
    }
    ITableWorkspace_sptr eppTable = createEPPTableWorkspace(eppRows);
    auto alg = createCorrectTOFAxisAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", inputWs))
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("EPPTable", eppTable));
    TS_ASSERT_THROWS_NOTHING(
        alg->setPropertyValue("IndexType", "Workspace Index"))
    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("ReferenceSpectra", "1-300"))
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    assertTOFShift(outputWs, inputWs, actualEi, originalWavelength, TOFShift);
  }

private:
  static void addSampleLogs(MatrixWorkspace_sptr ws, const double TOF) {
    const double length = flightLengthIN4(ws);
    const double Ei = incidentEnergy(TOF, length);
    ws->mutableRun().addProperty("EI", Ei);
    const double lambda = wavelength(Ei, length);
    ws->mutableRun().addProperty("wavelength", lambda);
  }

  static void assertTOFShift(MatrixWorkspace_sptr shiftedWs,
                             MatrixWorkspace_sptr ws, const double ei,
                             const double wavelength, const double shift) {
    TS_ASSERT(shiftedWs);
    TS_ASSERT_EQUALS(shiftedWs->run().getPropertyAsSingleValue("EI"), ei);
    TS_ASSERT_DELTA(shiftedWs->run().getPropertyAsSingleValue("wavelength"),
                    wavelength, 1e-8)
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      const size_t numBins = ws->blocksize();
      for (size_t j = 0; j < numBins; ++j) {
        TS_ASSERT_DELTA(shiftedWs->x(i)[j], ws->x(i)[j] + shift, 1e-6)
        TS_ASSERT_EQUALS(shiftedWs->y(i)[j], ws->y(i)[j])
        TS_ASSERT_EQUALS(shiftedWs->e(i)[j], ws->e(i)[j])
      }
      TS_ASSERT_DELTA(shiftedWs->x(i).back(), ws->x(i).back() + shift, 1e-6)
    }
  }

  static std::unique_ptr<CorrectTOFAxis> createCorrectTOFAxisAlgorithm() {
    std::unique_ptr<CorrectTOFAxis> alg(new CorrectTOFAxis);
    alg->setChild(true);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize())
    TS_ASSERT(alg->isInitialized())
    return alg;
  }

  static MatrixWorkspace_sptr
  createEmptyIN4Workspace(const std::string &wsName) {
    Mantid::DataHandling::LoadEmptyInstrument loadInstrument;
    loadInstrument.setChild(true);
    loadInstrument.initialize();
    loadInstrument.setProperty("InstrumentName", "IN4");
    loadInstrument.setProperty("OutputWorkspace", wsName);
    loadInstrument.execute();
    MatrixWorkspace_sptr ws = loadInstrument.getProperty("OutputWorkspace");
    auto xAxis = ws->getAxis(0);
    xAxis->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    return ws;
  }

  static MatrixWorkspace_sptr createInputWorkspace(const size_t blocksize,
                                                   const double x0,
                                                   const double dx,
                                                   const double TOF) {
    auto inputWs =
        createInputWorkspaceWithoutSampleLogs(blocksize, x0, dx, TOF);
    addSampleLogs(inputWs, TOF);
    return inputWs;
  }

  static MatrixWorkspace_sptr
  createInputWorkspaceWithoutSampleLogs(const size_t blocksize, const double x0,
                                        const double dx, const double TOF) {
    auto inputWs = createEmptyIN4Workspace("_input_ws");
    const double sigma = 3 * dx;
    auto gaussianPeak = [TOF, sigma](const double x) {
      const double a = -(x - TOF) / sigma;
      return std::exp(-a * a / 2);
    };
    fillWorkspace(inputWs, blocksize, x0, dx, gaussianPeak);
    return inputWs;
  }

  template <typename Function>
  static void fillWorkspace(MatrixWorkspace_sptr &ws, const size_t nBins,
                            const double x0, const double dx, Function yFromX) {
    ws = WorkspaceFactory::Instance().create(ws, ws->getNumberHistograms(),
                                             nBins + 1, nBins);
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < nBins; ++j) {
        const double x = x0 + static_cast<double>(j) * dx;
        ws->mutableX(i)[j] = x;
        const double y = yFromX(x + dx / 2);
        ws->mutableY(i)[j] = y;
        ws->mutableE(i)[j] = std::sqrt(y);
      }
      ws->mutableX(i).back() = x0 + static_cast<double>(nBins) * dx;
    }
  }

  static double flightLengthIN4(MatrixWorkspace_const_sptr ws) {
    const double l1 = ws->spectrumInfo().l1();
    const double l2 = ws->spectrumInfo().l2(1);
    return l1 + l2;
  }

  static double incidentEnergy(const double TOF, const double flightLenght) {
    const double velocity = flightLenght / (TOF * 1e-6);
    return Mantid::PhysicalConstants::NeutronMass * velocity * velocity / 2 /
           Mantid::PhysicalConstants::meV;
  }

  static double tof(const double Ei, const double flightLength) {
    return flightLength /
           std::sqrt(2 * Ei * Mantid::PhysicalConstants::meV /
                     Mantid::PhysicalConstants::NeutronMass) /
           1e-6;
  }

  static double wavelength(const double Ei, const double flightLength) {
    const double velocity = flightLength / tof(Ei, flightLength);
    return Mantid::PhysicalConstants::h / velocity /
           Mantid::PhysicalConstants::NeutronMass * 1e4;
  }
};

#endif /* MANTID_ALGORITHMS_CORRECTTOFAXISTEST_H_ */
