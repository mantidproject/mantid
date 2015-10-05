/*
 * IntegratePeakTimeSlicesTest.h
 *
 *  Created on: Jun 7, 2011
 *      Author: ruth
 */

#ifndef INTEGRATEPEAKTIMESLICESTEST_H_
#define INTEGRATEPEAKTIMESLICESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCrystal/IntegratePeakTimeSlices.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Quat.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Unit.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include <math.h>

#include <map>
#include "MantidAPI/ITableWorkspace.h"
using namespace Mantid;
using namespace DataObjects;
using namespace Geometry;
using namespace API;
using namespace Mantid::Crystal;
using namespace std;

class IntegratePeakTimeSlicesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeakTimeSlicesTest *createSuite() {
    return new IntegratePeakTimeSlicesTest();
  }
  static void destroySuite(IntegratePeakTimeSlicesTest *suite) { delete suite; }

  IntegratePeakTimeSlicesTest() { Mantid::API::FrameworkManager::Instance(); }

  void test_abc() {
    int NRC = 60; // 30;
    int NTimes = 40;
    int PeakRow = 22; // 12;
    int PeakCol = 27; // 17;
    int PeakChan = 15;
    double MaxPeakIntensity = 600;
    double MaxPeakRCSpan = 5;
    double MaxPeakTimeSpan = 4;

    double T[40] = {0.0};
    Workspace2D_sptr wsPtr =
        create2DWorkspaceWithRectangularInstrument(1, NRC, .05, NTimes);

    wsPtr->getAxis(0)->setUnit("TOF");

    // Set times;
    MantidVecPtr x_vals;
    for (int i = 0; i < NTimes; i++)
      x_vals.access().push_back(18000.0 + i * 100);

    for (size_t k = 0; k < wsPtr->getNumberHistograms(); k++)
      wsPtr->setX(k, x_vals);

    Geometry::Instrument_const_sptr instP = wsPtr->getInstrument();
    IComponent_const_sptr bankC =
        instP->getComponentByName(std::string("bank1"));

    if (bankC->type().compare("RectangularDetector") != 0)
      throw std::runtime_error(" No Rect bank named bank 1");

    boost::shared_ptr<const Geometry::RectangularDetector> bankR =
        boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(bankC);

    boost::shared_ptr<Geometry::Detector> pixelp =
        bankR->getAtXY(PeakCol, PeakRow);

    Geometry::IDetector_const_sptr pix = wsPtr->getDetector(522);

    // Now get Peak.
    double PeakTime = 18000 + (PeakChan + .5) * 100;

    Mantid::Kernel::Units::Wavelength wl;
    Kernel::V3D pos = Kernel::V3D(instP->getSource()->getPos());
    pos -= instP->getSample()->getPos();
    double L1 = pos.norm();
    Kernel::V3D pos1 = pixelp->getPos();
    pos1 -= instP->getSample()->getPos();
    double L2 = pos1.norm();
    double dummy, phi;
    pos1.getSpherical(dummy, phi, dummy);
    double ScatAng = phi / 180 * M_PI;
    std::vector<double> x;
    x.push_back(PeakTime);

    wl.fromTOF(x, x, L1, L2, ScatAng, 0, 0, 0);
    double wavelength = x[0];

    Peak peak(instP, pixelp->getID(), wavelength);

    // Now set up data in workspace2D
    double dQ = 0;
    double Q0 = calcQ(bankR, instP, PeakRow, PeakCol, 1000.0 + 30.0 * 50);

    double TotIntensity = 0;

    const detid2index_map map = wsPtr->getDetectorIDToWorkspaceIndexMap(true);

    for (int row = 0; row < NRC; row++)
      for (int col = 0; col < NRC; col++) {

        boost::shared_ptr<Detector> detP = bankR->getAtXY(col, row);

        detid2index_map::const_iterator it = map.find(detP->getID());
        size_t wsIndex = (*it).second;

        double MaxR = max<double>(
            0.0, MaxPeakIntensity * (1 - abs(row - PeakRow) / MaxPeakRCSpan));
        double MaxRC =
            max<double>(0.0, MaxR * (1 - abs(col - PeakCol) / MaxPeakRCSpan));
        MantidVecPtr dataY;
        MantidVecPtr dataE;

        for (int chan = 0; chan < NTimes; chan++) {
          double val = max<double>(
              0.0, MaxRC * (1 - abs(chan - PeakChan) / MaxPeakTimeSpan));
          TotIntensity += val;
          T[chan] += val;
          val += 1.4;

          dataY.access().push_back(val);
          dataE.access().push_back(sqrt(val));
          if ((val - 1.4) > MaxPeakIntensity * .1) {
            double Q = calcQ(bankR, instP, row, col, 1000.0 + chan * 50);
            dQ = max<double>(dQ, fabs(Q - Q0));
          }
        }

        wsPtr->setData(wsIndex, dataY, dataE);
      }

    PeaksWorkspace_sptr pks(new PeaksWorkspace());

    pks->addPeak(peak);

    IntegratePeakTimeSlices algP;
    // wsPtr->setName("InputWorkspace");
    // pks->setName("PeaksWorkspace");
    try {
      algP.initialize();
      algP.setProperty("PeakIndex", 0);
      algP.setProperty("PeakQspan", dQ);

      algP.setProperty<MatrixWorkspace_sptr>("InputWorkspace", wsPtr);

      algP.setProperty<PeaksWorkspace_sptr>("Peaks", pks);
      algP.setPropertyValue("OutputWorkspace", "aaa");

      algP.setProperty("CalculateVariances", false);

      // algP.setProperty("Ties","Background=1.4");
      algP.execute();

      algP.setPropertyValue("OutputWorkspace", "aaa");

      double intensity = algP.getProperty("Intensity");
      double sigma = algP.getProperty("SigmaIntensity");
      boost::shared_ptr<TableWorkspace> Twk =
          algP.getProperty("OutputWorkspace");

      TS_ASSERT_LESS_THAN(fabs(intensity - 60300), 1500.0);
      // Not sure why this reduced the error so much in the test
      TS_ASSERT_LESS_THAN(fabs(sigma - 457.0), 21.0);

      TS_ASSERT_EQUALS(Twk->rowCount(), 7);

      if (Twk->rowCount() < 5)
        return;

      TS_ASSERT_LESS_THAN(
          fabs(Twk->getRef<double>(std::string("Time"), 0) - 19200), 20);

      TS_ASSERT_LESS_THAN(
          fabs(Twk->getRef<double>(std::string("Background"), 1) - 1.08824),
          .5);

      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double>("Intensity", 2) - 11460),
                          120);

      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double>("NCells", 3) - 439), 5);

      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double>("ChiSqrOverDOF", 4) - 72),
                          3.5);

      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double>("TotIntensity", 0) - 4979),
                          10);

    } catch (char *s) {
      std::cout << "Error= " << s << std::endl;
    } catch (std::exception &es) {
      std::cout << "Error1=" << es.what() << std::endl;
    } catch (...) {
      std::cout << "Some Error Happened" << std::endl;
    }
  }

private:
  /**
   *   Calculates Q
   */
  double calcQ(RectangularDetector_const_sptr bankP,
               boost::shared_ptr<const Instrument> instPtr, int row, int col,
               double time) {
    boost::shared_ptr<Detector> detP = bankP->getAtXY(col, row);

    double L2 = detP->getDistance(*(instPtr->getSample()));

    Kernel::Units::MomentumTransfer Q;
    std::vector<double> x;
    x.push_back(time);
    double L1 = instPtr->getSample()->getDistance(*(instPtr->getSource()));
    Kernel::V3D pos = detP->getPos();
    double ScatAng = fabs(asin(pos.Z() / pos.norm()));

    Q.fromTOF(x, x, L1, L2, ScatAng, 0, 0, 0.0);

    return x[0] / 2 / M_PI;
  }

  /**
   * Creates a 2D workspace for testing purposes
   */
  Workspace2D_sptr create2DWorkspaceWithRectangularInstrument(int Npanels,
                                                              int NRC,
                                                              double sideLength,
                                                              int NTimes) {
    // Workspace2D_sptr wsPtr =
    // WorkspaceFactory::Instance().create("Workspace2D", NPanels;

    const size_t &NVectors = (size_t)(Npanels * NRC * NRC);
    const size_t &ntimes = (size_t)NTimes;
    const size_t &nvals = (size_t)NTimes;

    Workspace2D_sptr wsPtr = boost::dynamic_pointer_cast<Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", NVectors, ntimes,
                                            nvals));
    // wsPtr->initialize(NVectors, ntimes, nvals);

    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(Npanels, NRC,
                                                                  sideLength);

    wsPtr->setInstrument(inst);

    wsPtr->rebuildSpectraMapping(false);

    return wsPtr;
  }
};
#endif /* INTEGRATEPEAKTIMESLICESTEST_H_ */
