#ifndef MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_
#define MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTypes/Core/DateAndTime.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <numeric>
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidAlgorithms/CalculatePoleFigure.h"

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid;

namespace {

//-----------------------------------------------------
/** create a Bragg workspace containg 2 spectra.
 * Each has 100 data points containing a Gaussian peak
 * between d = 1.2 and 1.5
 * @brief createBraggWorkspace
 * @return
 */
Mantid::API::MatrixWorkspace_sptr createBraggWorkspace(const std::string &name) {

  // TODO/NOW - Refer to convertUnitsTest for how to build an instrument

  size_t n = 100;

  Mantid::API::FrameworkManager::Instance();
  Mantid::DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
          Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 2, n,
                                                           n));
  // create unit
  std::string unitlabel("dSpacing");
  ws->getAxis(0)->unit() =
      Mantid::Kernel::UnitFactory::Instance().create(unitlabel);

  // set instrument as reduced VULCAN east and west bank
  Instrument_sptr testInst(new Instrument);

  // Define a source component: [0,0,-43.754]
  Mantid::Geometry::ObjComponent *source =
      new Mantid::Geometry::ObjComponent("moderator", Object_sptr(), testInst.get());
  source->setPos(V3D(0, 0.0, -43.754));
  testInst->add(source);
  testInst->markAsSource(source);

  // Define a sample as a simple sphere
  Mantid::Geometry::ObjComponent *sample =
      new Mantid::Geometry::ObjComponent("samplePos", Object_sptr(), testInst.get());
  testInst->setPos(0.0, 0.0, 0.0);
  testInst->add(sample);
  testInst->markAsSamplePos(sample);

  // add two pixels
  // pixel 1:  [-2,0,-3.67394e-16]
  Mantid::Geometry::Detector *physicalPixel = new Detector("pixel", 1, testInst.get());
  physicalPixel->setPos(-2, 0, 0);
  testInst->add(physicalPixel);
  testInst->markAsDetector(physicalPixel);

  Mantid::Geometry::Detector *physicalPixel2 = new Detector("pixel", 2, testInst.get());
  physicalPixel2->setPos(2, 0, 0);
  testInst->add(physicalPixel2);
  testInst->markAsDetector(physicalPixel2);

  // set instrument
  ws->setInstrument(testInst);
  ws->getSpectrum(0).addDetectorID(physicalPixel->getID());
  ws->getSpectrum(1).addDetectorID(physicalPixel2->getID());

  // set data
  auto &X = ws->mutableX(0);
  auto &Y = ws->mutableY(0);
  auto &E = ws->mutableE(0);

  // create Gaussian peak
  double dx = 0.01;
  for (size_t i = 0; i < n; i++) {
    X[i] = double(i) * dx + 1.2;
    Y[i] = exp(-(X[i]-1.5)*(X[i]-1.5)/(0.02)) * (1.0+double(i));
    E[i] = sqrt(fabs(Y[i]));
  }

  // add properties
  // add HROT
  Kernel::TimeSeriesProperty<double> *hrotprop = new Kernel::TimeSeriesProperty<double>("HROT");
  Types::Core::DateAndTime time0(1000000);
  hrotprop->addValue(time0, -0.003857142);
  ws->mutableRun().addProperty(hrotprop);

  // add Omega
  Kernel::TimeSeriesProperty<double> *omegaprop = new Kernel::TimeSeriesProperty<double>("OMEGA");
  omegaprop->addValue(time0, 89.998);
  API::Run &run = ws->mutableRun();
  run.addProperty(omegaprop);

  // add to ADS
  Mantid::API::AnalysisDataService::Instance().add(name, ws);


  return ws;
}
}

class CalculatePoleFigureTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CalculatePoleFigure alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------------------
  /**
   * @brief test_Execute
   */
  void test_Execute() {

    API::Workspace_sptr ws =
        createBraggWorkspace("TwoSpecPoleFigure");

    CalculatePoleFigure pfcalculator;
    pfcalculator.initialize();

    // set properties
    pfcalculator.setProperty("InputWorkspace", ws);
    pfcalculator.setProperty("OutputWorkspace", "TwoSpecPoleFigure");
    pfcalculator.setProperty("MinD", 1.3);
    pfcalculator.setProperty("MaxD", 1.5);
    pfcalculator.execute();

    // run
    TS_ASSERT(pfcalculator.isExecuted());

    // check results
    TS_ASSERT(API::AnalysisDataService::Instance().doesExist("TwoSpecPoleFigure"));
    API::ITableWorkspace_sptr outws = boost::dynamic_pointer_cast<API::ITableWorkspace>(API::AnalysisDataService::Instance().retrieve("TwoSpecPoleFigure"));
    TS_ASSERT(outws);
    if (!outws)
        return;

    // shall have 2 rows
    TS_ASSERT_EQUALS(outws->rowCount(), 2);

    // row 0
    API::TableRow row0 = outws->getRow(0);
    int wsindex;
    double r_td, r_nd, intensity;
    row0 >> wsindex >> r_td >> r_nd >> intensity;

    // row 1
    API::TableRow row1 = outws->getRow(1);
    int wsindex1;
    double r_td1, r_nd1, intensity1;
    row1 >> wsindex1 >> r_td1 >> r_nd1 >> intensity1;

    TS_ASSERT_EQUALS(wsindex, 0);
    TS_ASSERT_EQUALS(r_td, r_td1);

    return;
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEPOLEFIGURE_H_ */
