// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_BIN2DPOWDERDIFFRACTIONTEST_H_
#define MANTID_ALGORITHMS_BIN2DPOWDERDIFFRACTIONTEST_H_

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/Bin2DPowderDiffraction.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cstdio>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::Algorithms::Bin2DPowderDiffraction;
using Mantid::Types::Event::TofEvent;

namespace {
Logger logger("Bin2DPowder");
}

class Bin2DPowderDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static Bin2DPowderDiffractionTest *createSuite() {
    return new Bin2DPowderDiffractionTest();
  }
  static void destroySuite(Bin2DPowderDiffractionTest *suite) { delete suite; }

  //-------------------- Test success --------------------------------------
  void test_Init() {
    Bin2DPowderDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_Binning() {
    using Mantid::API::IAlgorithm;
    using Mantid::DataHandling::MoveInstrumentComponent;
    using Mantid::Kernel::Unit;

    std::size_t numbins = 1;
    EventWorkspace_sptr eventWS = this->createInputWorkspace(numbins);

    int numSpectra = static_cast<int>(eventWS->getNumberHistograms());

    Bin2DPowderDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_bin2d_test1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("dSpaceBinning", "2,2,6"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("dPerpendicularBinning", "1,2,5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizeByBinArea", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // get and check output workspace
    boost::shared_ptr<MatrixWorkspace> outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "_bin2d_test1");
    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->getAxis(0)->length(), 3);
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "dSpacing");
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->length(), 3);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->unit()->unitID(),
                     "dSpacingPerpendicular");

    TS_ASSERT_DELTA((*(outputWS->getAxis(0)))(0), 2.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(0)))(1), 4.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(0)))(2), 6.0, 0.0001);

    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(0), 1.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(1), 3.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(2), 5.0, 0.0001);

    // all events must come into one bin
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->y(0)[1], 0);
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 0);
    TS_ASSERT_EQUALS(outputWS->y(1)[1], numbins * numSpectra);
  }

  void test_NormBinArea() {
    using Mantid::API::IAlgorithm;
    using Mantid::DataHandling::MoveInstrumentComponent;
    using Mantid::Kernel::Unit;

    std::size_t numbins = 1;
    EventWorkspace_sptr eventWS = this->createInputWorkspace(numbins);

    Bin2DPowderDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_bin2d_test1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("dSpaceBinning", "2,2,6"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("dPerpendicularBinning", "1,2,5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizeByBinArea", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // get and check output workspace
    boost::shared_ptr<MatrixWorkspace> outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "_bin2d_test1");
    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->getAxis(0)->length(), 3);
    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "dSpacing");
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->length(), 3);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->unit()->unitID(),
                     "dSpacingPerpendicular");

    TS_ASSERT_DELTA((*(outputWS->getAxis(0)))(0), 2.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(0)))(1), 4.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(0)))(2), 6.0, 0.0001);

    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(0), 1.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(1), 3.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(2), 5.0, 0.0001);

    // all events must come into one bin
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->y(0)[1], 0);
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 0);
    TS_ASSERT_DELTA(outputWS->y(1)[1], 6.25, 0.0001);
  }

  void test_BinningFromFile() {
    using Mantid::API::IAlgorithm;
    using Mantid::DataHandling::MoveInstrumentComponent;
    using Mantid::Kernel::Unit;

    std::size_t numbins = 1;
    EventWorkspace_sptr eventWS = this->createInputWorkspace(numbins);
    std::string binFileName = "bin2dpd_test.txt";

    this->createBinFile(binFileName);

    Bin2DPowderDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_bin2d_test1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BinEdgesFile", binFileName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizeByBinArea", "0"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // get and check output workspace
    boost::shared_ptr<MatrixWorkspace> outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "_bin2d_test1");
    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), "dSpacing");
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->length(), 3);
    TS_ASSERT_EQUALS(outputWS->getAxis(1)->unit()->unitID(),
                     "dSpacingPerpendicular");

    // bins vary, test x values
    TS_ASSERT_DELTA(outputWS->x(0)[0], 1.0, 0.0001);
    TS_ASSERT_DELTA(outputWS->x(0)[1], 3.0, 0.0001);
    TS_ASSERT_DELTA(outputWS->x(0)[2], 6.0, 0.0001);
    TS_ASSERT_DELTA(outputWS->x(0)[3], 6.0, 0.0001); // unify bins
    TS_ASSERT_DELTA(outputWS->x(1)[0], 2.0, 0.0001);
    TS_ASSERT_DELTA(outputWS->x(1)[1], 4.0, 0.0001);
    TS_ASSERT_DELTA(outputWS->x(1)[2], 5.15, 0.0001);
    TS_ASSERT_DELTA(outputWS->x(1)[3], 6.0, 0.0001);

    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(0), 3.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(1), 4.0, 0.0001);
    TS_ASSERT_DELTA((*(outputWS->getAxis(1)))(2), 4.5, 0.0001);

    // 25 events: 5 in one bin and 20 in the other one
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 0);
    TS_ASSERT_EQUALS(outputWS->y(0)[1], 0);
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 0);
    TS_ASSERT_EQUALS(outputWS->y(1)[1], 5);
    TS_ASSERT_EQUALS(outputWS->y(1)[2], 20);

    std::remove(binFileName.c_str()); // delete file
  }

  //-------------------- Test failure --------------------------------------

  void test_Zero2theta() {
    using Mantid::API::IAlgorithm;
    using Mantid::DataHandling::MoveInstrumentComponent;
    using Mantid::Kernel::Unit;

    EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 5,
                                                                        true);
    eventWS->getAxis(0)->setUnit("Wavelength");

    Bin2DPowderDiffraction alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_bin2d_test3"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("dSpaceBinning", "2,2,6"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("dPerpendicularBinning", "1,2,5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormalizeByBinArea", "0"));
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
  }

  //-------------------- Helpers --------------------------------------
private:
  EventWorkspace_sptr createInputWorkspace(std::size_t numbins) {
    using Mantid::API::IAlgorithm;
    using Mantid::DataHandling::MoveInstrumentComponent;
    using Mantid::Kernel::Unit;

    EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 5,
                                                                        true);
    // Set the X axes
    const auto &xVals = eventWS->x(0);
    const size_t xSize = xVals.size();
    auto ax0 = new NumericAxis(xSize);
    logger.information() << "xSize = " << xSize << std::endl;
    // X-axis is 1 <= wavelength <= 6 Angstrom with step of 0.05
    ax0->setUnit("Wavelength");
    for (size_t i = 0; i < xSize; i++) {
      ax0->setValue(i, 1.0 + 0.05 * xVals[i]);
    }
    eventWS->replaceAxis(0, ax0);
    // detector angles
    auto algc = boost::make_shared<MoveInstrumentComponent>();
    algc->initialize();
    algc->setProperty<EventWorkspace_sptr>("Workspace", eventWS);
    algc->setPropertyValue("ComponentName", "bank1");
    algc->setProperty("X", 1.0);
    algc->setProperty("Y", 0.0);
    algc->setProperty("Z", 1.0);
    algc->setPropertyValue("RelativePosition", "0");
    algc->execute();

    int numSpectra = static_cast<int>(eventWS->getNumberHistograms());

    // add events
    // Make up some data for each pixels
    for (int i = 0; i < numSpectra; i++) {
      // Create one event for each bin
      EventList &events = eventWS->getSpectrum(i);
      for (std::size_t ie = 0; ie < numbins; ie++) {
        // Create a list of events, randomize
        events += TofEvent(4.0);
      }
      events.addDetectorID(Mantid::detid_t(i));
    }
    logger.information() << "Number of events: " << numbins * numSpectra
                         << std::endl;

    return eventWS;
  }

  // creates test file with bins description
  void createBinFile(std::string fname) {
    std::ofstream binfile;
    binfile.open(fname);
    binfile << "#dp_min #dp_max\n";
    binfile << "#d_bins\n";
    binfile << "dp = 3.0  4.0\n";
    binfile << "  1.0  3.0  6.0\n\n";
    binfile << "dp = 4.0  4.5\n";
    binfile << "  2.0  4.0  5.15  6.0\n\n";
    binfile.close();
  }
};

#endif /* MANTID_ALGORITHMS_BIN2DPOWDERDIFFRACTIONTEST_H_ */
