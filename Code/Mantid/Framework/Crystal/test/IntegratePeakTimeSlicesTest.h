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
#include "MantidCurveFitting/Fit.h"
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
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Unit.h"
#include "MantidAPI/IPeak.h"

#include <map>
using namespace Mantid;
using namespace DataObjects;
using namespace Geometry;
using namespace API;
using namespace Mantid::Crystal;
using namespace std;

class IntegratePeakTimeSlicesTest: public CxxTest::TestSuite
{
public:
  void test_abc()
  {
    int NRC = 30;
    int NTimes = 40;
    int PeakRow = 12;
    int PeakCol = 17;
    int PeakChan = 15;
    double MaxPeakIntensity = 600;
    double MaxPeakRCSpan = 5;
    double MaxPeakTimeSpan = 4;

    Workspace2D_sptr wsPtr = create2DWorkspaceWithRectangularInstrument(1, NRC, .05, NTimes);

    wsPtr->getAxis(0)->setUnit("TOF");

    //Set times;
    MantidVecPtr x_vals;
    for (int i = 0; i < NTimes; i++)
      x_vals.access().push_back(18000.0 + i * 100);

    for (size_t k = 0; k < wsPtr->getNumberHistograms(); k++)
      wsPtr->setX(k, x_vals);

    Geometry::Instrument_const_sptr instP = wsPtr->getInstrument();
    IComponent_const_sptr bankC = instP->getComponentByName(std::string("bank1"));

    if (bankC->type().compare("RectangularDetector") != 0)
      throw std::runtime_error(" No Rect bank named bank 1");

    boost::shared_ptr<const Geometry::RectangularDetector> bankR = boost::dynamic_pointer_cast<const
        Geometry::RectangularDetector>(bankC);

    boost::shared_ptr<Geometry::Detector> pixelp = bankR->getAtXY(PeakCol, PeakRow);

    //Now get Peak.
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

    //Now set up data in workspace2D
    double dQ = 0;
    double Q0 = calcQ(bankR, instP, PeakRow, PeakCol, 1000.0 + 30.0 * 50);

    double TotIntensity = 0;
    for (int row = 0; row < NRC; row++)
      for (int col = 0; col < NRC; col++)
      {
        double MaxR = max<double> (0.0, MaxPeakIntensity * (1 - abs(row - PeakRow) / MaxPeakRCSpan));
        double MaxRC = max<double> (0.0, MaxR * (1 - abs(col - PeakCol) / MaxPeakRCSpan));
        MantidVecPtr dataY;
        MantidVecPtr dataE;

        for (int chan = 0; chan < NTimes; chan++)
        {
          double val = max<double> (0.0, MaxRC * (1 - abs(chan - PeakChan) / MaxPeakTimeSpan));
          TotIntensity += val;
          val += 1.4;

          dataY.access().push_back(val);
          dataE.access().push_back(1.0);
          if ((val - 1.4) > MaxPeakIntensity * .1)
          {
            double Q = calcQ(bankR, instP, row, col, 1000.0 + chan * 50);
            dQ = max<double> (dQ, fabs(Q - Q0));
          }
        }

        boost::shared_ptr<Detector> detP = bankR->getAtXY(col, row);
        detid2index_map * map = wsPtr->getDetectorIDToWorkspaceIndexMap(true);

        detid2index_map::iterator it = map->find(detP->getID());
        size_t wsIndex = (*it).second;

        wsPtr->setData(wsIndex, dataY, dataE);
      }

    PeaksWorkspace_sptr pks(new PeaksWorkspace());

    pks->addPeak(peak);

    IntegratePeakTimeSlices algP;
    wsPtr->setName("InputWorkspace");
    pks->setName("PeaksWorkspace");

    try
    {
      algP.initialize();
      algP.setProperty("PeakIndex", 0);
      algP.setProperty("PeakQspan", dQ);

      algP.setProperty<MatrixWorkspace_sptr> ("InputWorkspace", wsPtr);

      algP.setProperty<PeaksWorkspace_sptr> ("Peaks", pks);
      algP.setPropertyValue("OutputWorkspace", "aaa");
      algP.execute();
      algP.setPropertyValue("OutputWorkspace", "aaa");

      TableWorkspace_sptr Twk = algP.getProperty("OutputWorkspace");

      TS_ASSERT_LESS_THAN(fabs(pks->getPeak(0).getIntensity() - 59923.5), 100.0);
      TS_ASSERT_LESS_THAN(fabs(pks->getPeak(0).getSigmaIntensity() - 380.71), 1.0);
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("Time", 0) - 19250), 20);
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("Background", 1) - 1.4), .2);
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("Intensity", 2) - 11157.3), 20);
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("NCells", 3) -  225), 5);
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("ChiSqrOverDOF", 4) - 258.275), 1.5);
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("TotIntensity", 0) - 4065), 10);

      /*   std::vector<std::string> names = Twk->getColumnNames();
      std::cout<<"Intensitty="<<pks->getPeak(0).getIntensity()<<"   sigma="<<pks->getPeak(0).getSigmaIntensity()<<std::endl;
       for( int i=0; i<Twk->columnCount();i++)
       {
         std::cout<<std::setw(15)<<names[i];
       for( int j=0; j< Twk->rowCount();j++)
       std::cout<< setw(12)<<Twk->cell<double>(j,i);
       std::cout<<std::endl;

       }

        Intensitty=59923.5   sigma=380.71
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background     1.42022     1.44054      1.4735     1.48123      1.4735     1.44054     1.42022
      Intensity     3720.21      7440.3     11157.3     14880.4     11157.3      7440.3     3720.21
           Mcol          17          17          17          17          17          17          17
           Mrow          12          12          12          12          12          12          12
          SScol     3.98085     3.98281     3.97949     3.98244     3.97949     3.98281     3.98085
          SSrow     3.98219     3.98214     3.98291     3.98211     3.98291     3.98214     3.98219
           SSrc           0           0 0.000267033           0 0.000267033           0           0
         NCells         225         225         210         225         210         225         225
  ChiSqrOverDOF       26.77      106.92     258.275     427.785     258.275      106.92       26.77
   TotIntensity        4065        7815       11544       15315       11544        7815        4065
BackgroundError     0.39112    0.781681     1.27035     1.56355     1.27035    0.781681     0.39112
FitIntensityError     41.4984     82.9473      130.21      165.91      130.21     82.9473     41.4984
  ISAWIntensity     3745.45     7490.88     11234.6     14981.7     11234.6     7490.88     3745.45
ISAWIntensityError     98.0362     134.863      164.32     187.991      164.32     134.863     98.0362
      Start Row           5           5           5           5           5           5           5
        End Row          19          19          18          19          18          19          19
      Start Col          10          10          10          10          10          10          10
        End Col          24          24          24          24          24          24          24

       */
    } catch (char * s)
    {
      std::cout << "Error= " << s << std::endl;
    } catch (std::exception &es)
    {
      std::cout << "Error1=" << es.what() << std::endl;
    } catch (...)
    {
      std::cout << "Some Error Happened" << std::endl;
    }
  }


  /**
   *  Example program only. Not a test program
   */
  void SampleProgram()
  {

    boost::shared_ptr<Mantid::API::Algorithm> loadSNSNexus;
    boost::shared_ptr<Mantid::API::Algorithm> Rebin;
    try
    {

      loadSNSNexus = Mantid::API::AlgorithmFactory::Instance(). create(string("LoadEventNexus"), 1);

    } catch (std::runtime_error& e)
    {
      printf("ERRRRR  %s\n", e.what());
    }
    try
    {
      loadSNSNexus->initialize();
      loadSNSNexus->setProperty<std::string> (std::string("Filename"), std::string(
          "/home/ruth/Mantid/Test/AutoTestData/TOPAZ_3176_event.nxs"));

      Workspace2D_sptr WS2D;
      loadSNSNexus->setPropertyValue(std::string("BankName"), std::string("bank26"));
      loadSNSNexus->setPropertyValue(std::string("OutputWorkspace"), "aaa");

      loadSNSNexus->execute();

      EventWorkspace_sptr evwsP = boost::dynamic_pointer_cast<EventWorkspace>(
          AnalysisDataService::Instance().retrieve("aaa"));


      Rebin = Mantid::API::AlgorithmFactory::Instance(). create(std::string("Rebin"), 1);
      ;
      Rebin->initialize();

      Rebin->setProperty<MatrixWorkspace_sptr> ("InputWorkspace", boost::dynamic_pointer_cast<
          MatrixWorkspace>(evwsP));

      Rebin->setProperty<bool> ("PreserveEvents", false);

      Rebin->setPropertyValue(std::string("OutputWorkspace"), std::string("RebinResult"));
      Rebin->setPropertyValue(std::string("Params"), std::string("17258.2,-.004,33500"));

      Rebin->execute();

      Workspace2D_sptr wsPtr = boost::dynamic_pointer_cast<Workspace2D>(
          AnalysisDataService::Instance().retrieve("RebinResult"));

      Geometry::Instrument_const_sptr instP = wsPtr->getInstrument();

      IComponent_const_sptr bankC = instP->getComponentByName(std::string("bank26"));

      if (bankC->type().compare("RectangularDetector") != 0)
        throw std::runtime_error(" No Rect bank named bank 26");

      boost::shared_ptr<const Geometry::RectangularDetector> bankR = boost::dynamic_pointer_cast<
          const Geometry::RectangularDetector>(bankC);

      boost::shared_ptr<Geometry::Detector> pixelp = bankR->getAtXY(57, 214);

      //std::cout<<"pixel ID="<<pixelp->getID()<<std::endl;
      Peak peak(instP, pixelp->getID(), 6.955836);

      PeaksWorkspace_sptr pks(new PeaksWorkspace());
      pks->setName("Peaks3");
      pks->addPeak(peak);

      IntegratePeakTimeSlices algP;
      algP.initialize();
      algP.setProperty("PeakIndex", 0);
      algP.setProperty("PeakQspan", .003);
      algP.setPropertyValue("OutputWorkspace", "ccc");
      algP.setProperty<MatrixWorkspace_sptr> ("InputWorkspace", wsPtr);
      algP.setProperty<PeaksWorkspace_sptr> ("Peaks", pks);
      algP.execute();
      algP.setPropertyValue("OutputWorkspace", "ccc");
      TableWorkspace_sptr Table = (algP.getProperty("OutputWorkspace"));

      if (!Table)
      {
        Table = boost::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve(
            "ccc"));
        if (!Table)
          std::cout << "Could Not retrieve frome Analysys data service" << std::endl;
      }

      std::vector<std::string> names = Table->getColumnNames();
    } catch (std::exception &s)
    {
      std::cout << "error =" << s.what() << std::endl;
    }
  }



private:

  /**
   *   Calculates Q
   */
  double calcQ(RectangularDetector_const_sptr bankP, boost::shared_ptr<const Instrument> instPtr, int row, int col,
      double time)
  {
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
  Workspace2D_sptr create2DWorkspaceWithRectangularInstrument(int Npanels, int NRC, double sideLength,
      int NTimes)
  {
    // Workspace2D_sptr wsPtr = WorkspaceFactory::Instance().create("Workspace2D", NPanels;

    const size_t &NVectors = (size_t) (Npanels * NRC * NRC);
    const size_t &ntimes = (size_t) NTimes;
    const size_t &nvals = (size_t) NTimes;

    Workspace2D_sptr wsPtr = boost::dynamic_pointer_cast<Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", NVectors, ntimes, nvals));
    //wsPtr->initialize(NVectors, ntimes, nvals);

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(Npanels, NRC,
        sideLength);

    wsPtr->setInstrument(inst);

    wsPtr->rebuildSpectraMapping(false);

    return wsPtr;
  }

};
#endif /* INTEGRATEPEAKTIMESLICESTEST_H_ */
