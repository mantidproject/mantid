#ifndef MANTID_ALGORITHMS_LORENTZCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_LORENTZCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/LorentzCorrection.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>

using Mantid::Algorithms::LorentzCorrection;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace WorkspaceCreationHelper;

class LorentzCorrectionTest : public CxxTest::TestSuite {

private:
  /*
   * Calculate what the weight should be.
   */
  double calculate_weight_at(double xMin, double xMax, double twotheta) {

    double lam = 0.5 * (xMin + xMax);
    double weight = std::sin(0.5 * twotheta);
    weight = weight * weight;
    weight = weight / (lam * lam * lam * lam);
    return weight;
  }

  /**
   Create a workspace in wavelength with a simple instrument defined with a
   single detector.
   */
  MatrixWorkspace_sptr create_workspace(const int nBins) {
    Instrument_sptr instrument = boost::make_shared<Instrument>();
    instrument->setReferenceFrame(
        boost::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 0));
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *sample = new ObjComponent("some-surface-holder");
    source->setPos(V3D(15, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector *det = new Detector("my-detector", 1, NULL);
    det->setPos(20, (20 - sample->getPos().X()), 0);
    instrument->add(det);
    instrument->markAsDetector(det);

    const int nSpectra = 1;
    const double deltaX = 10;
    const double startX = 0;
    auto workspace = create2DWorkspaceBinned(nSpectra, nBins, startX,
                                             deltaX); // Creates histogram data
    workspace->mutableY(0) = 1.0;
    workspace->mutableE(0) = 1.0;

    workspace->getAxis(0)->setUnit("Wavelength");
    workspace->setYUnit("Counts");
    workspace->setInstrument(instrument);
    workspace->getSpectrum(0).addDetectorID(det->getID());
    return workspace;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LorentzCorrectionTest *createSuite() {
    return new LorentzCorrectionTest();
  }
  static void destroySuite(LorentzCorrectionTest *suite) { delete suite; }

  void test_init() {
    LorentzCorrection alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_check_input_units() {
    const int nHisto = 1;
    const int nBins = 1;
    auto ws_tof = create2DWorkspaceWithFullInstrument(nHisto, nBins);

    LorentzCorrection alg;
    alg.setChild(true);
    alg.initialize();
    TSM_ASSERT_THROWS("Workspace must be in units of wavelength",
                      alg.setProperty("InputWorkspace", ws_tof),
                      std::invalid_argument &);
  }

  void test_throws_if_wavelength_zero() {
    auto ws_lam = this->create_workspace(2 /*nBins*/);
    ws_lam->mutableX(0) = 0; // Make wavelength zero
    LorentzCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws_lam);
    alg.setPropertyValue("OutputWorkspace", "temp");
    TSM_ASSERT_THROWS("Should throw with zero wavelength values.",
                      alg.execute(), std::runtime_error &);
  }

  void test_execute() {
    auto ws_lam = this->create_workspace(2 /*nBins*/);

    LorentzCorrection alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws_lam);
    alg.setPropertyValue("OutputWorkspace", "temp");
    alg.execute();
    MatrixWorkspace_sptr out_ws = alg.getProperty("OutputWorkspace");
    TS_ASSERT(out_ws != NULL);

    const std::string unitID = out_ws->getAxis(0)->unit()->unitID();
    TS_ASSERT_EQUALS(unitID, "Wavelength");

    const auto &xData = out_ws->x(0);
    const auto &yData = out_ws->y(0);
    const auto &eData = out_ws->e(0);
    const auto &spectrumInfo = out_ws->spectrumInfo();

    int index = 0;
    double weight = calculate_weight_at(xData[index], xData[index + 1],
                                        spectrumInfo.twoTheta(0));
    TS_ASSERT_EQUALS(yData[index], weight);
    TS_ASSERT_EQUALS(eData[index], weight);

    index++; // go to 1
    weight = calculate_weight_at(xData[index], xData[index + 1],
                                 spectrumInfo.twoTheta(0));
    TS_ASSERT_EQUALS(yData[index], weight);
    TS_ASSERT_EQUALS(eData[index], weight);
  }
};

#endif /* MANTID_ALGORITHMS_LORENTZCORRECTIONTEST_H_ */
