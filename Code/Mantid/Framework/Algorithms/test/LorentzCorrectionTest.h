#ifndef MANTID_ALGORITHMS_LORENTZCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_LORENTZCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/LorentzCorrection.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <cmath>

using Mantid::Algorithms::LorentzCorrection;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace WorkspaceCreationHelper;

class LorentzCorrectionTest: public CxxTest::TestSuite
{

private:

  /*
   * Calculate what the weight should be.
   */
  double calculate_weight_at(MatrixWorkspace_sptr& ws, const int bin_index)
  {
    const Mantid::MantidVec& xData = ws->readX(0);

    auto detector = ws->getDetector(0);
    double twotheta = ws->detectorTwoTheta(detector);
    double lam = (xData[bin_index] + xData[bin_index + 1]) / 2;
    double weight = std::sin(twotheta / 2);
    weight = weight * weight;
    weight = weight / (lam * lam * lam * lam);
    return weight;
  }

  /**
   Create a workspace in wavelength with a simple instrument defined with a single detector.
   */
  MatrixWorkspace_sptr create_workspace(const int nBins)
  {
    Instrument_sptr instrument = boost::make_shared<Instrument>();
    instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, 0));
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *sample = new ObjComponent("some-surface-holder");
    source->setPos(V3D(15, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector* det = new Detector("my-detector", 1, NULL);
    det->setPos(20, (20 - sample->getPos().X()), 0);
    instrument->add(det);
    instrument->markAsDetector(det);

    const int nSpectra = 1;
    const double deltaX = 10;
    const double startX = 0;
    auto workspace = Create2DWorkspaceBinned(nSpectra, nBins, startX, deltaX); // Creates histogram data

    auto & ydata = workspace->dataY(0);
    auto & edata = workspace->dataE(0);
    for (size_t i = 0; i < ydata.size(); ++i)
    {
      ydata[i] = 1;
      edata[i] = 1;
    }

    workspace->getAxis(0)->setUnit("Wavelength");
    workspace->setYUnit("Counts");
    workspace->setInstrument(instrument);
    workspace->getSpectrum(0)->addDetectorID(det->getID());
    return workspace;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LorentzCorrectionTest *createSuite()
  {
    return new LorentzCorrectionTest();
  }
  static void destroySuite(LorentzCorrectionTest *suite)
  {
    delete suite;
  }

  void test_init()
  {
    LorentzCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_check_input_units()
  {
    const int nHisto = 1;
    const int nBins = 1;
    auto ws_tof = create2DWorkspaceWithFullInstrument(nHisto, nBins);

    LorentzCorrection alg;
    alg.setChild(true);
    alg.initialize();
    TSM_ASSERT_THROWS("Workspace must be in units of wavelength",
        alg.setProperty("InputWorkspace", ws_tof), std::invalid_argument&);
  }

  void test_throws_if_wavelength_zero()
  {
    auto ws_lam = this->create_workspace(2 /*nBins*/);
    ws_lam->dataX(0)[0] = 0; // Make wavelength zero
    ws_lam->dataX(0)[1] = 0; // Make wavelength zero
    LorentzCorrection alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", ws_lam);
    alg.setPropertyValue("OutputWorkspace", "temp");
    TSM_ASSERT_THROWS("Should throw with zero wavelength values.", alg.execute(), std::runtime_error&);
  }

  void test_execute()
  {
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

    const Mantid::MantidVec& yData = out_ws->readY(0);
    const Mantid::MantidVec& eData = out_ws->readE(0);

    int index = 0;
    double weight = calculate_weight_at(out_ws, index /*y index*/);
    TS_ASSERT_EQUALS(yData[index], weight);
    TS_ASSERT_EQUALS(eData[index], weight);

    index++; // go to 1
    weight = calculate_weight_at(out_ws, index /*y index*/);
    TS_ASSERT_EQUALS(yData[index], weight);
    TS_ASSERT_EQUALS(eData[index], weight);
  }

};

#endif /* MANTID_ALGORITHMS_LORENTZCORRECTIONTEST_H_ */
