// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CalculateEfficiency2TEST_H_
#define CalculateEfficiency2TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/CalculateEfficiency2.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/SANSInstrumentCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class CalculateEfficiency2Test : public CxxTest::TestSuite {
public:
  /*
   * Generate fake data for which we know what the result should be
   */
  void setUpWorkspace() {
    inputWS = "sampledata";

    Mantid::DataObjects::Workspace2D_sptr ws =
        SANSInstrumentCreationHelper::createSANSInstrumentWorkspace(inputWS);

    // Set up the X bin for the monitor channels
    for (int i = 0; i < SANSInstrumentCreationHelper::nMonitors; i++) {
      auto &X = ws->mutableX(i);
      X[0] = 1;
      X[1] = 2;
    }

    for (int ix = 0; ix < SANSInstrumentCreationHelper::nBins; ix++) {
      for (int iy = 0; iy < SANSInstrumentCreationHelper::nBins; iy++) {
        int i = ix * SANSInstrumentCreationHelper::nBins + iy +
                SANSInstrumentCreationHelper::nMonitors;
        auto &X = ws->mutableX(i);
        auto &Y = ws->mutableY(i);
        auto &E = ws->mutableE(i);
        X[0] = 1;
        X[1] = 2;
        Y[0] = 2.0;
        E[0] = 1;
      }
    }
    // Change one of the bins so that it will be excluded for having a high
    // signal
    auto &Y = ws->mutableY(SANSInstrumentCreationHelper::nMonitors + 5);
    Y[0] = 202.0;
  }

  void testName() { TS_ASSERT_EQUALS(correction.name(), "CalculateEfficiency") }

  void testVersion() { TS_ASSERT_EQUALS(correction.version(), 2) }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(correction.initialize())
    TS_ASSERT(correction.isInitialized())
  }

  void testExecDefault() {
    setUpWorkspace();
    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("OutputWorkspace", outputWS))

    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(
        ws_out =
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_out =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    double tolerance(1e-03);
    TS_ASSERT_DELTA(ws2d_out->y(1 + SANSInstrumentCreationHelper::nMonitors)[0],
                    1.0, tolerance);
    TS_ASSERT_DELTA(
        ws2d_out->y(15 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0,
        tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(6 + SANSInstrumentCreationHelper::nMonitors)[0],
                    1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(1 + SANSInstrumentCreationHelper::nMonitors)[0],
                    0.5, tolerance);
    TS_ASSERT_DELTA(
        ws2d_out->e(15 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5,
        tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(6 + SANSInstrumentCreationHelper::nMonitors)[0],
                    0.5, tolerance);

    // Check that pixels that were out of range were masked
    const auto &oSpecInfo = ws2d_out->spectrumInfo();
    TS_ASSERT(!oSpecInfo.isMasked(5 + SANSInstrumentCreationHelper::nMonitors));
    TS_ASSERT(!oSpecInfo.isMasked(1 + SANSInstrumentCreationHelper::nMonitors));
  }

  void testExecWithPixelsExcluded() {
    // Repeat the calculation by excluding high/low pixels
    setUpWorkspace();
    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(
        correction.setProperty<double>("MinThreshold", 0.5))
    TS_ASSERT_THROWS_NOTHING(
        correction.setProperty<double>("MaxThreshold", 1.50))

    TS_ASSERT_THROWS_NOTHING(correction.execute())
    TS_ASSERT(correction.isExecuted())

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(
        ws_out =
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_out =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    double tolerance(1e-03);
    TS_ASSERT_DELTA(ws2d_out->x(1 + SANSInstrumentCreationHelper::nMonitors)[0],
                    1.0, tolerance);
    TS_ASSERT_DELTA(ws2d_out->x(1 + SANSInstrumentCreationHelper::nMonitors)[1],
                    2.0, tolerance);

    TS_ASSERT_DELTA(ws2d_out->y(1 + SANSInstrumentCreationHelper::nMonitors)[0],
                    1.0, tolerance);
    TS_ASSERT_DELTA(
        ws2d_out->y(15 + SANSInstrumentCreationHelper::nMonitors)[0], 1.0,
        tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(6 + SANSInstrumentCreationHelper::nMonitors)[0],
                    1.0, tolerance);

    TS_ASSERT_DELTA(ws2d_out->e(1 + SANSInstrumentCreationHelper::nMonitors)[0],
                    0.5, tolerance);
    TS_ASSERT_DELTA(
        ws2d_out->e(15 + SANSInstrumentCreationHelper::nMonitors)[0], 0.5,
        tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(6 + SANSInstrumentCreationHelper::nMonitors)[0],
                    0.5, tolerance);

    // Check that pixels that were out of range where EMPTY_DBL
    TS_ASSERT_DELTA(ws2d_out->y(5 + SANSInstrumentCreationHelper::nMonitors)[0],
                    EMPTY_DBL(), tolerance);

    const auto &oSpecInfo2 = ws2d_out->spectrumInfo();
    TS_ASSERT(
        !oSpecInfo2.isMasked(1 + SANSInstrumentCreationHelper::nMonitors));

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

  /*
   * Function that will validate results against known results found with
   * "standard" HFIR reduction package.
   */
  void validate() {
    Mantid::DataHandling::LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BioSANS_exp61_scan0004_0001.xml");
    const std::string inputWS("wav");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();

    Mantid::DataHandling::MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", "wav");
    mover.setPropertyValue("ComponentName", "detector1");
    // According to the instrument geometry, the center of the detector is
    // located at N_pixel / 2 + 0.5
    // X = (16-192.0/2.0+0.5)*5.15/1000.0 = -0.409425
    // Y = (95-192.0/2.0+0.5)*5.15/1000.0 = -0.002575
    mover.setPropertyValue("X", "0.409425");
    mover.setPropertyValue("Y", "0.002575");
    mover.setPropertyValue("Z", "6");
    mover.execute();

    if (!correction.isInitialized())
      correction.initialize();

    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        correction.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(
        correction.setProperty<double>("MinThreshold", 0.5))
    TS_ASSERT_THROWS_NOTHING(
        correction.setProperty<double>("MaxThreshold", 1.50))

    correction.execute();

    TS_ASSERT(correction.isExecuted())

    Mantid::API::MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 36866)

    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "Wavelength")

    Mantid::API::Workspace_sptr ws_in;
    TS_ASSERT_THROWS_NOTHING(
        ws_in = Mantid::API::AnalysisDataService::Instance().retrieve(inputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_in =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_in);

    Mantid::API::Workspace_sptr ws_out;
    TS_ASSERT_THROWS_NOTHING(
        ws_out =
            Mantid::API::AnalysisDataService::Instance().retrieve(outputWS));
    Mantid::DataObjects::Workspace2D_sptr ws2d_out =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(ws_out);

    // Number of monitors
    int nmon = Mantid::DataHandling::LoadSpice2D::nMonitors;
    // Get the coordinate of the detector pixel

    double tolerance(1e-03);
    TS_ASSERT_DELTA(ws2d_out->y(1 + nmon)[0], 0.980083, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(193 + nmon)[0], 1.23006, tolerance);
    TS_ASSERT_DELTA(ws2d_out->y(6 + nmon)[0], 1.10898, tolerance);

    TS_ASSERT_DELTA(ws2d_out->e(1 + nmon)[0], 0.0990047, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(193 + nmon)[0], 0.110913, tolerance);
    TS_ASSERT_DELTA(ws2d_out->e(6 + nmon)[0], 0.105261, tolerance);

    // Check that pixels that were out of range were masked
    const auto &oSpecInfo = ws2d_out->spectrumInfo();
    TS_ASSERT(oSpecInfo.isMasked(1826));
    TS_ASSERT(oSpecInfo.isMasked(2014));
    TS_ASSERT(oSpecInfo.isMasked(2015));

    Mantid::API::AnalysisDataService::Instance().remove(inputWS);
    Mantid::API::AnalysisDataService::Instance().remove(outputWS);
  }

private:
  Mantid::Algorithms::CalculateEfficiency2 correction;
  std::string inputWS;
};

#endif /*CalculateEfficiency2TEST_H_*/
