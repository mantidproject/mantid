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
#include "MantidAPI/IPeak.h"
#include <math.h>

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
  IntegratePeakTimeSlicesTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }
  void test_abc()
  {
    int NRC =60;// 30;
    int NTimes = 40;
    int PeakRow = 22;// 12;
    int PeakCol = 27;// 17;
    int PeakChan = 15;
    double MaxPeakIntensity = 600;
    double MaxPeakRCSpan = 5;
    double MaxPeakTimeSpan = 4;

    double T[40]={0.0};
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

    /*for( int r=PeakRow-3; r<=PeakRow+3;r++)
      for( int c=PeakCol-3;c<=PeakCol+3;c++)
        std::cout<<"("<<r<<","<<c<<")pos="<<bankR->getAtXY(c,r)->getPos()<<std::endl;
    */
    Geometry::IDetector_const_sptr pix= wsPtr->getDetector(522);

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

    detid2index_map * map = wsPtr->getDetectorIDToWorkspaceIndexMap(true);

    for (int row = 0; row < NRC; row++)
      for (int col = 0; col < NRC; col++)
      {

        boost::shared_ptr<Detector> detP = bankR->getAtXY(col, row);

        detid2index_map::iterator it = map->find(detP->getID());
        size_t wsIndex = (*it).second;

        double MaxR = max<double> (0.0, MaxPeakIntensity * (1 - abs(row - PeakRow) / MaxPeakRCSpan));
        double MaxRC = max<double> (0.0, MaxR * (1 - abs(col - PeakCol) / MaxPeakRCSpan));
        MantidVecPtr dataY;
        MantidVecPtr dataE;


        for (int chan = 0; chan < NTimes; chan++)
        {
          double val = max<double> (0.0, MaxRC * (1 - abs(chan - PeakChan) / MaxPeakTimeSpan));
          TotIntensity += val;
          T[chan]+=val;
          val += 1.4;


          dataY.access().push_back(val);
          dataE.access().push_back(sqrt(val));
          if ((val - 1.4) > MaxPeakIntensity * .1)
          {
            double Q = calcQ(bankR, instP, row, col, 1000.0 + chan * 50);
            dQ = max<double> (dQ, fabs(Q - Q0));
          }
        }


        wsPtr->setData(wsIndex, dataY, dataE);

      }
    delete map;

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

      algP.setProperty("CalculateVariances",false);

      //algP.setProperty("Ties","Background=1.4");
      algP.execute();

      algP.setPropertyValue("OutputWorkspace", "aaa");

   
       double intensity = algP.getProperty("Intensity");
       double sigma = algP.getProperty("SigmaIntensity");
       TableWorkspace_sptr Twk = algP.getProperty("OutputWorkspace");
  

       TS_ASSERT_LESS_THAN(fabs(intensity -60000), 1500.0);
      //Not sure why this reduced the error so much in the test
      TS_ASSERT_LESS_THAN(fabs(sigma -539), 21.0);


      TS_ASSERT_EQUALS( Twk->rowCount(), 7);
      
      if( Twk->rowCount() <5)
          return;

      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> (std::string("Time"), 0) - 19200), 20);
    
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> (std::string("Background"), 1) -  1.2619  ), .5);
   
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("Intensity", 2) -  11309.8 ), 120);
      
   
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("NCells", 3) -  553), 5);
    

      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("ChiSqrOverDOF", 4) -   60.4183), 3.5);

    
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("TotIntensity", 0) -  5298.4  ), 10);
      
  

      /*
          std::vector<std::string> names = Twk->getColumnNames();

      std::cout<<"Intensitty="<<intensity<<"   sigma="<<sigma<<
               "  Theoret intensity="<<TotIntensity<<std::endl;
      std::cout<<std::setw(15)<<"Act Int";
      for( int j=12; j< 12+(int)Twk->rowCount();j++)
             std::cout<< setw(12)<<T[j];
      std::cout<<std::endl;

       for( int i=0; i+1<(int)Twk->columnCount();i++)
       {
         std::cout<<std::setw(15)<<names[i];
       for( int j=0; j< (int)Twk->rowCount();j++)
       {  std::cout<< setw(12);
         if( i+1< (int)Twk->columnCount())
           std::cout <<Twk->cell<double>(j,i);
         else
           std::cout <<Twk->cell<string>(j,i)<<"::";

       }

       std::cout<<std::endl;

       }



Intensity=58989.5   sigma=539.266  Theoret intensity=60000
        Act Int        3750        7500       11250       15000       11250        7500        3750
           Time       19200       19350       19450       19550       19650       19750       19900
        Channel        11.5          13          14          15          16          17        18.5
     Background     2.78004      1.2619     1.26187     1.26226     1.26187      1.2619     2.78004
      Intensity     3751.16     7556.68     11309.8       15063     11309.8     7556.68     3751.16
           Mcol          27          27          27          27          27          27          27
           Mrow     22.0001     22.0001     22.0001     22.0001     22.0001     22.0001     22.0001
          SScol     4.45014     4.45002     4.45012     4.45013     4.45012     4.45002     4.45014
          SSrow      4.4498     4.45009     4.45004     4.45006     4.45004     4.45009      4.4498
           SSrc 0.000203427 0.000317585 0.000485281 0.000653647 0.000485281 0.000317585 0.000203427
         NCells         553         553         553         553         553         553         553
  ChiSqrOverDOF     9.58119     26.8175     60.4183     107.505     60.4183     26.8175     9.58119
   TotIntensity      5298.4      8274.2     12024.2     15774.2     12024.2      8274.2      5298.4
BackgroundError    0.144835    0.242935    0.367252    0.491398    0.367252    0.242935    0.144835
FitIntensityError     33.4156      56.727     88.5118     119.975     88.5118      56.727     33.4156
  ISAWIntensity     3761.04     7576.37     11326.4     15076.2     11326.4     7576.37     3761.04
ISAWIntensityError     115.112     164.378     232.309     300.527     232.309     164.378     115.112
  TotalBoundary       347.2       173.6       173.6       173.6       173.6       173.6       347.2
 NBoundaryCells         124         124         124         124         124         124         124
      Start Row           9           9           9           9           9           9           9
        End Row          35          35          35          35          35          35          35
      Start Col          14          14          14          14          14          14          14
        End Col          40          40          40          40          40          40          40
TotIntensityError     72.7901     90.9626     109.655     125.595     109.655     90.9626     72.7901



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
        Table = AnalysisDataService::Instance().retrieveWS<TableWorkspace>(
            "ccc");
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
