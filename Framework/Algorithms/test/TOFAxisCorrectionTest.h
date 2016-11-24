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


  void test_Init()
  {
    TOFAxisCorrection alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_CorrectionUsingReferenceWorkspace() {
    const size_t blocksize = 16;
    const double x0 = 23.66;
    const double dx = 0.05;
    auto inputData = [](const double x) {
      return 1.97 * x;
    };
    auto inputWs = createEmptyIN4Workspace("ws");
    fillWorkspace(inputWs, blocksize, x0, dx, inputData);
    const double inputEi = 11.05;
    const double inputWavelength = 6.26;
    inputWs->mutableRun().addProperty("EI", inputEi, true);
    inputWs->mutableRun().addProperty("wavelength", inputWavelength, true);
    const double TOFshift = -1.69;
    MatrixWorkspace_sptr referenceWs = inputWs->clone();
    auto referenceData = [](const double x) {
      return 999.98 * x + 2.3;
    };
    fillWorkspace(referenceWs, blocksize, x0+TOFshift, dx, referenceData);
    const double referenceEi = 1.1 * inputEi;
    const double referenceWavelength = 0.77 * inputWavelength;
    referenceWs->mutableRun().addProperty("EI", referenceEi, true);
    referenceWs->mutableRun().addProperty("wavelength", referenceWavelength, true);
    TOFAxisCorrection alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("ReferenceWorkspace", referenceWs) );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr outputWs = alg.getProperty("OutputWorkspace");
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
    auto inputWs = createEmptyIN4Workspace("ws");
    const size_t blocksize = 512;
    const double x0 = 1402;
    const double dx = 0.23;
    const size_t eppIndex = blocksize / 3;
    const double eppTOF = x0 + static_cast<double>(eppIndex) * dx + dx / 2;
    const double sigma = 3 * dx;
    auto gaussianPeak = [x0, sigma](const double x) {
      const double a = -(x - x0) / sigma;
      return std::exp(-a * a / 2);
    };
    fillWorkspace(inputWs, blocksize, x0, dx, gaussianPeak);
    std::vector<EPPTableRow> eppRows(inputWs->getNumberHistograms());
    for (auto &row : eppRows) {
      row.peakCentre = eppTOF;
    }
    const double length = flightLengthIN4(inputWs);
    const double velocity = length / (eppTOF * 1e-6);
    const double nominalEi = Mantid::PhysicalConstants::NeutronMass * velocity * velocity / 2 / Mantid::PhysicalConstants::meV;
    const double actualEi = 1.05 * nominalEi;
    const double actualElasticTOF = length / (std::sqrt(2 * actualEi * Mantid::PhysicalConstants::meV / Mantid::PhysicalConstants::NeutronMass)) / 1e-6;
    const double TOFshift = actualElasticTOF - eppTOF;
    ITableWorkspace_sptr eppTable = createEPPTableWorkspace(eppRows);
    TOFAxisCorrection alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("EPPTable", eppTable) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("IndexType", "WorkspaceIndex") )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ReferenceSpectra", "1-300") )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("IncidentEnergy", actualEi) )
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr outputWs = alg.getProperty("OutputWorkspace");
    TS_ASSERT( outputWs );
    for (size_t i = 0; i < inputWs->getNumberHistograms(); ++i) {
      for (size_t j = 0; j < blocksize; ++j) {
        TS_ASSERT_DELTA( outputWs->x(i)[j], inputWs->x(i)[j] + TOFshift, 1e-6 )
        TS_ASSERT_EQUALS( outputWs->y(i)[j], inputWs->y(i)[j] )
        TS_ASSERT_EQUALS( outputWs->e(i)[j], inputWs->e(i)[j] )
      }
      TS_ASSERT_DELTA( outputWs->x(i).back(), inputWs->x(i).back() + TOFshift, 1e-6 )
    }
  }

  void test_UseEiFromSampleLogs()
  {
    auto inputWs = createEmptyIN4Workspace("ws");
    const size_t blocksize = 512;
    const double x0 = 1390.1;
    const double dx = 0.24;
    const size_t eppIndex = 2 * blocksize / 3;
    const double eppTOF = x0 + static_cast<double>(eppIndex) * dx + dx / 2;
    const double sigma = 4 * dx;
    auto gaussianPeak = [x0, sigma](const double x) {
      const double a = -(x - x0) / sigma;
      return std::exp(-a * a / 2);
    };
    fillWorkspace(inputWs, blocksize, x0, dx, gaussianPeak);
    std::vector<EPPTableRow> eppRows(inputWs->getNumberHistograms());
    for (auto &row : eppRows) {
      row.peakCentre = eppTOF;
    }
    const double length = flightLengthIN4(inputWs);
    const double velocity = length / (eppTOF * 1e-6);
    const double nominalEi = Mantid::PhysicalConstants::NeutronMass * velocity * velocity / 2 / Mantid::PhysicalConstants::meV;
    const double actualEi = 0.93 * nominalEi;
    inputWs->mutableRun().addProperty("EI", actualEi, true);
    const double actualElasticTOF = length / (std::sqrt(2 * actualEi * Mantid::PhysicalConstants::meV / Mantid::PhysicalConstants::NeutronMass)) / 1e-6;
    const double TOFshift = actualElasticTOF - eppTOF;
    ITableWorkspace_sptr eppTable = createEPPTableWorkspace(eppRows);
    TOFAxisCorrection alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWs) )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "_unused_for_child") )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("EPPTable", eppTable) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("IndexType", "WorkspaceIndex") )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ReferenceSpectra", "1-300") )
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr outputWs = alg.getProperty("OutputWorkspace");
    TS_ASSERT( outputWs );
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
  MatrixWorkspace_sptr createEmptyIN4Workspace(const std::string &wsName) const {
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

  template <typename Function>
  void fillWorkspace(MatrixWorkspace_sptr &ws, const size_t nBins, const double x0, const double dx, Function yFromX) const {
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

  double flightLengthIN4(MatrixWorkspace_const_sptr ws) {
    const double l1 = ws->spectrumInfo().l1();
    const double l2 = ws->spectrumInfo().l2(1);
    return l1 + l2;
  }
};


#endif /* MANTID_ALGORITHMS_TOFAXISCORRECTIONTEST_H_ */
