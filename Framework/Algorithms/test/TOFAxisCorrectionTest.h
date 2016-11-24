#ifndef MANTID_ALGORITHMS_TOFAXISCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_TOFAXISCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/TOFAxisCorrection.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::TOFAxisCorrection;
using namespace Mantid::API;
using namespace WorkspaceCreationHelper;

class TOFAxisCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TOFAxisCorrectionTest *createSuite() { return new TOFAxisCorrectionTest(); }
  static void destroySuite( TOFAxisCorrectionTest *suite ) { delete suite; }

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
    auto alg = createTOFAxisCorrectionAlgorithm();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("ReferenceWorkspace", referenceWs) );
    TS_ASSERT_THROWS_NOTHING( alg->execute() );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT( outputWs );
    TS_ASSERT_EQUALS( outputWs->run().getPropertyAsSingleValue("EI"), referenceEi )
    TS_ASSERT_EQUALS( outputWs->run().getPropertyAsSingleValue("wavelength"), referenceWavelength )
    for (size_t i = 0; i < inputWs->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < blocksize; ++j) {
        TS_ASSERT_DELTA( outputWs->x(i)[j], referenceWs->x(i)[j], 1e-6 )
        TS_ASSERT_EQUALS( outputWs->y(i)[j], inputWs->y(i)[j] )
        TS_ASSERT_EQUALS( outputWs->e(i)[j], inputWs->e(i)[j] )
      }
      TS_ASSERT_DELTA( outputWs->x(i).back(), referenceWs->x(i).back(), 1e-6 )
    }
  }

  void test_CorrectionWithoutReferenceWorkspace()
  {
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
    const double velocity = length / (eppTOF * 1e-6);
    const double nominalEi = Mantid::PhysicalConstants::NeutronMass * velocity * velocity / 2 / Mantid::PhysicalConstants::meV;
    inputWs->mutableRun().addProperty("EI", nominalEi, true);
    const double nominalWavelength = Mantid::PhysicalConstants::h / velocity / Mantid::PhysicalConstants::NeutronMass * 1e10;
    inputWs->mutableRun().addProperty("wavelength", nominalWavelength, true);
    const double actualEi = 1.05 * nominalEi;
    const double actualElasticTOF = length / (std::sqrt(2 * actualEi * Mantid::PhysicalConstants::meV / Mantid::PhysicalConstants::NeutronMass)) / 1e-6;
    const double actualVelocity = length / (actualElasticTOF * 1e-6);
    const double actualWavelength = Mantid::PhysicalConstants::h / actualVelocity / Mantid::PhysicalConstants::NeutronMass * 1e10;
    const double TOFshift = actualElasticTOF - eppTOF;
    ITableWorkspace_sptr eppTable = createEPPTableWorkspace(eppRows);
    auto alg = createTOFAxisCorrectionAlgorithm();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("EPPTable", eppTable) );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("IndexType", "WorkspaceIndex") )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("ReferenceSpectra", "1-300") )
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("IncidentEnergy", actualEi) )
    TS_ASSERT_THROWS_NOTHING( alg->execute() );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT( outputWs );
    TS_ASSERT_EQUALS( outputWs->run().getPropertyAsSingleValue("EI"), actualEi );
    TS_ASSERT_EQUALS( outputWs->run().getPropertyAsSingleValue("wavelength"), actualWavelength )
    for (size_t i = 0; i < inputWs->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < blocksize; ++j) {
        TS_ASSERT_DELTA( outputWs->x(i)[j], inputWs->x(i)[j] + TOFshift, 1e-6 )
        TS_ASSERT_EQUALS( outputWs->y(i)[j], inputWs->y(i)[j] )
        TS_ASSERT_EQUALS( outputWs->e(i)[j], inputWs->e(i)[j] )
      }
      TS_ASSERT_DELTA( outputWs->x(i).back(), inputWs->x(i).back() + TOFshift, 1e-6 )
    }
  }

  void test_FailureIfNoInputPropertiesSet() {
    auto alg = createTOFAxisCorrectionAlgorithm();
    TS_ASSERT_THROWS_ANYTHING( alg->execute() );
    TS_ASSERT( !alg->isExecuted() );
  }

  void test_FailureIfOnlyInputAndOutputWorkspacesSet() {
    const size_t blocksize = 128;
    const double x0 = 1431;
    const double dx = 0.33;
    const double eppTOF = x0 + blocksize / 4 * dx + dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, eppTOF);
    auto alg = createTOFAxisCorrectionAlgorithm();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_ANYTHING( alg->execute() );
    TS_ASSERT( !alg->isExecuted() );
  }

  void test_FailureIfReferenceWorkspaceIncompatible() {
    const size_t blocksize = 16;
    const double x0 = 23.66;
    const double dx = 0.05;
    const double TOF = x0 + blocksize * dx / 2;
    auto inputWs = createInputWorkspace(blocksize, x0, dx, TOF);
    auto referenceWs = createInputWorkspace(blocksize - 1, x0, dx, TOF);
    auto alg = createTOFAxisCorrectionAlgorithm();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("ReferenceWorkspace", referenceWs) );
    TS_ASSERT_THROWS_ANYTHING( alg->execute() );
    TS_ASSERT( !alg->isExecuted() );
  }

  void test_FailureNoEiGivenAtAll() {
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
    auto alg = createTOFAxisCorrectionAlgorithm();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("EPPTable", eppTable) );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("IndexType", "WorkspaceIndex") )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("ReferenceSpectra", "1-300") )
    TS_ASSERT_THROWS_ANYTHING( alg->execute() );
    TS_ASSERT( !alg->isExecuted() );
  }

  void test_Init()
  {
    TOFAxisCorrection alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_UseEiFromSampleLogs()
  {
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
    const double TOFshift = actualElasticTOF - eppTOF;
    std::vector<EPPTableRow> eppRows(inputWs->getNumberHistograms());
    for (auto &row : eppRows) {
      row.peakCentre = eppTOF;
    }
    ITableWorkspace_sptr eppTable = createEPPTableWorkspace(eppRows);
    auto alg = createTOFAxisCorrectionAlgorithm();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("EPPTable", eppTable) );
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("IndexType", "WorkspaceIndex") )
    TS_ASSERT_THROWS_NOTHING( alg->setPropertyValue("ReferenceSpectra", "1-300") )
    TS_ASSERT_THROWS_NOTHING( alg->execute() );
    TS_ASSERT( alg->isExecuted() );

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");
    TS_ASSERT( outputWs );
    TS_ASSERT_EQUALS( outputWs->run().getPropertyAsSingleValue("EI"), actualEi );
    for (size_t i = 0; i < inputWs->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < blocksize; ++j) {
        TS_ASSERT_DELTA( outputWs->x(i)[j], inputWs->x(i)[j] + TOFshift, 1e-6 )
        TS_ASSERT_EQUALS( outputWs->y(i)[j], inputWs->y(i)[j] )
        TS_ASSERT_EQUALS( outputWs->e(i)[j], inputWs->e(i)[j] )
      }
      TS_ASSERT_DELTA( outputWs->x(i).back(), inputWs->x(i).back() + TOFshift, 1e-6 )
    }
  }

private:
  static std::unique_ptr<TOFAxisCorrection> createTOFAxisCorrectionAlgorithm() {
    std::unique_ptr<TOFAxisCorrection> alg(new TOFAxisCorrection);
    alg->setChild(true);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg->initialize() )
    TS_ASSERT( alg->isInitialized() )
    return alg;
  }

  static MatrixWorkspace_sptr createEmptyIN4Workspace(const std::string &wsName) {
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

  static MatrixWorkspace_sptr createInputWorkspace(const size_t blocksize, const double x0, const double dx, const double TOF) {
    auto inputWs = createEmptyIN4Workspace("_input_ws");
    const double sigma = 3 * dx;
    auto gaussianPeak = [TOF, sigma](const double x) {
      const double a = -(x - TOF) / sigma;
      return std::exp(-a * a / 2);
    };
    fillWorkspace(inputWs, blocksize, x0, dx, gaussianPeak);
    const double length = flightLengthIN4(inputWs);
    const double Ei = incidentEnergy(TOF, length);
    inputWs->mutableRun().addProperty("EI", Ei);
    const double lambda = wavelength(Ei, length);
    inputWs->mutableRun().addProperty("wavelength", lambda);
    return inputWs;
  }

  template <typename Function>
  static void fillWorkspace(MatrixWorkspace_sptr &ws, const size_t nBins, const double x0, const double dx, Function yFromX) {
    ws = WorkspaceFactory::Instance().create(ws, ws->getNumberHistograms(), nBins+1, nBins);
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < nBins; ++j) {
        const double x = x0 + j * dx;
        ws->mutableX(i)[j] = x;
        const double y = yFromX(x + dx / 2);
        ws->mutableY(i)[j] = y;
        ws->mutableE(i)[j] = std::sqrt(y);
      }
      ws->mutableX(i).back() = x0 + nBins * dx;
    }
  }

  static double flightLengthIN4(MatrixWorkspace_const_sptr ws) {
    const double l1 = ws->spectrumInfo().l1();
    const double l2 = ws->spectrumInfo().l2(1);
    return l1 + l2;
  }

  static double incidentEnergy(const double TOF, const double flightLenght) {
    const double velocity = flightLenght / (TOF * 1e-6);
    return Mantid::PhysicalConstants::NeutronMass * velocity * velocity / 2 / Mantid::PhysicalConstants::meV;
  }

  static double tof(const double Ei, const double flightLength) {
    return flightLength / std::sqrt(2 * Ei * Mantid::PhysicalConstants::meV / Mantid::PhysicalConstants::NeutronMass) / 1e-6;
  }

  static double wavelength(const double Ei, const double flightLength) {
    const double velocity = flightLength / tof(Ei, flightLength);
    return Mantid::PhysicalConstants::h / velocity / Mantid::PhysicalConstants::NeutronMass * 1e4;
  }
};


#endif /* MANTID_ALGORITHMS_TOFAXISCORRECTIONTEST_H_ */
