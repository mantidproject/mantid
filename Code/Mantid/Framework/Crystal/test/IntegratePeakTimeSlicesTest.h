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
#include "MantidGeometry/IInstrument.h"
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
  IntegratePeakTimeSlicesTest()
  {

  }

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
    Workspace2D_sptr wsPtr = create2DWorkspaceWithRectangularInstrument(1, NRC, .05,  NTimes);
  
    wsPtr->getAxis(0)->setUnit("TOF");
  
    //Set times;
    MantidVecPtr x_vals;
    for (int i = 0; i < NTimes; i++)
      x_vals.access().push_back(18000.0 + i *100);

    for (size_t k = 0; k < wsPtr->getNumberHistograms(); k++)
      wsPtr->setX(k, x_vals);

    Geometry::IInstrument_sptr instP = wsPtr->getInstrument();
    boost::shared_ptr<Geometry::IComponent> bankC = instP->getComponentByName(std::string("bank1"));

    if (bankC->type().compare("RectangularDetector") != 0)
      throw std::runtime_error(" No Rect bank named bank 1");

    boost::shared_ptr<Geometry::RectangularDetector> bankR = boost::dynamic_pointer_cast<
                                                 Geometry::RectangularDetector>(bankC);
  
    boost::shared_ptr<Geometry::Detector> pixelp = bankR->getAtXY(PeakCol, PeakRow);

    //Now get Peak.
    double PeakTime = 18000+(PeakChan+.5)*100;
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

    //cout << "dQ=" << dQ<<", Tot Peak Int="<<TotIntensity << endl;
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
    algP.setPropertyValue("OutputWorkspace","aaa");
    algP.execute();
    algP.setPropertyValue("OutputWorkspace","aaa");
    //std::cout<<"Peak results="<< peak.getIntensity()<<","<<peak.getSigmaIntensity()<<std::endl;
    TableWorkspace_sptr Twk = algP.getProperty("OutputWorkspace");

     TS_ASSERT_LESS_THAN( fabs(pks->getPeak(0).getIntensity()-60000),100.0);
     TS_ASSERT_LESS_THAN( fabs(pks->getPeak(0).getSigmaIntensity()-375.5),1.0);
     TS_ASSERT_LESS_THAN( fabs(Twk->getRef<double>("Time",0) -19250),20);
     TS_ASSERT_LESS_THAN( fabs(Twk->getRef<double>("Background",1) -1.4),.2);
     TS_ASSERT_LESS_THAN( fabs(Twk->getRef<double>("Intensity",2) -11206),20);
     TS_ASSERT_LESS_THAN( fabs(Twk->getRef<double>("NCells",3) -420),5);
     TS_ASSERT_LESS_THAN( fabs(Twk->getRef<double>("ChiSqrOverDOF",4) -134.464),1.5);
     TS_ASSERT_LESS_THAN( fabs(Twk->getRef<double>("TotIntensity",0) -4338),10);
    //std::cout<< Pks->getPeak(0).getIntensity()<<","<< Pks->getPeak(0).getSigmaIntensity()<<std::endl;
    std::vector<std::string> names = Twk->getColumnNames();
    /*
    for( int i=0; i<Twk->columnCount();i++)
    {
       std::cout<<std::setw(15)<<names[i];
       for( int j=0; j< Twk->rowCount();j++)
           std::cout<< setw(12)<<Twk->cell<double>(j,i);
       std::cout<<std::endl;

     }
    
Intensitty=59985.5,  sigma= 375.476
slice Info
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background     1.40207     1.40485     1.40621     1.40828     1.40728     1.40414     1.40243
      Intensity     3735.37     7469.21     11206.2     14941.6     11203.8     7470.79     3734.57
           Mcol          17          17          17          17          17          17          17
           Mrow          12          12          12          12          12          12          12
          SScol     3.99188     3.99281     3.99224     3.99229     3.99239     3.99216     3.99223
          SSrow     3.99316     3.99376     3.99351     3.99343     3.99403     3.99369     3.99316
           SSrc           0-0.000266736           0           0 0.000124477           0-0.000106694
         NCells         420         399         420         420         399         420         399
  ChiSqrOverDOF     14.2131     59.7547      127.85     227.296     134.464     56.8184     14.9517
   TotIntensity        4338      8058.6       11838       15588     11808.6        8088      4308.6
BackgroundError     0.19604    0.413891    0.587967    0.783968    0.620871    0.391965    0.207033
FitIntensityError     28.4576     58.5656     85.3544     113.807     87.8525     56.9013     29.2931
  ISAWIntensity     3749.13     7498.06     11247.4     14996.5     11247.1     7498.26     3749.03
ISAWIntensityError     99.2641     133.555      160.85     184.072     160.865     133.653     98.9773
      Start Row           3           3           3           3           3           3           3
        End Row          22          21          22          22          21          22          21
      Start Col           7           7           7           7           7           7           7
        End Col          27          27          27          27          27    
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
    loadSNSNexus->setProperty<std::string>(std::string("Filename"), 
                           std::string("/home/ruth/Mantid/Test/AutoTestData/TOPAZ_3176_event.nxs"));

    Workspace2D_sptr WS2D;
    loadSNSNexus->setPropertyValue( std::string("BankName"), std::string("bank26"));
    loadSNSNexus->setPropertyValue( std::string("OutputWorkspace"),"aaa");


    loadSNSNexus->execute();

    EventWorkspace_sptr evwsP = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("aaa"));
           //loadSNSNexus->getProperty("OutputWorkspace");

    Rebin=Mantid::API::AlgorithmFactory::Instance(). create(std::string("Rebin"), 1);;
    Rebin->initialize();

    Rebin->setProperty<MatrixWorkspace_sptr>("InputWorkspace", 
                                        boost::dynamic_pointer_cast<MatrixWorkspace>(evwsP));
    Rebin->setProperty<bool>("PreserveEvents", false);

    Rebin->setPropertyValue(std::string("OutputWorkspace"),std::string("RebinResult"));
    Rebin->setPropertyValue(std::string("Params"),std::string("17258.2,-.004,33500"));

    Rebin->execute();


    Workspace2D_sptr wsPtr = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("RebinResult"));

    Geometry::IInstrument_sptr instP = wsPtr->getInstrument();

    boost::shared_ptr<Geometry::IComponent> bankC = instP->getComponentByName(std::string("bank26"));

    if (bankC->type().compare("RectangularDetector") != 0)
         throw std::runtime_error(" No Rect bank named bank 26");

   boost::shared_ptr<Geometry::RectangularDetector> bankR = boost::dynamic_pointer_cast<
                                          Geometry::RectangularDetector>(bankC);

   boost::shared_ptr<Geometry::Detector> pixelp = bankR->getAtXY(57,214);

   //std::cout<<"pixel ID="<<pixelp->getID()<<std::endl;
   Peak peak(instP, pixelp->getID(), 6.955836);
   
   PeaksWorkspace_sptr pks(new PeaksWorkspace());
   pks->setName("Peaks3");
   pks->addPeak(peak);
   IntegratePeakTimeSlices algP;
   algP.initialize();
   algP.setProperty("PeakIndex",0);
   algP.setProperty("PeakQspan",.003);
   algP.setPropertyValue("OutputWorkspace","ccc");
   algP.setProperty<MatrixWorkspace_sptr>("InputWorkspace",wsPtr); 
   algP.setProperty<PeaksWorkspace_sptr>("Peaks",pks);
   algP.execute();
   algP.setPropertyValue("OutputWorkspace","ccc");
   TableWorkspace_sptr Table=(algP.getProperty("OutputWorkspace"));
  
   if( !Table)
   {
        Table = boost::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve("ccc"));
        if( !Table)
           std::cout<<"Could Not retrieve frome Analysys data service"<<std::endl;
    }


    std::vector<std::string> names = Table->getColumnNames();
  }catch( std::exception &s)
  {
    std::cout<<"error ="<< s.what()<<std::endl;
  }
}

private:
  double calcQ(RectangularDetector_sptr bankP, boost::shared_ptr<IInstrument> instPtr, int row, int col,
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
    return x[0]/2/M_PI;

  }
  Workspace2D_sptr create2DWorkspaceWithRectangularInstrument(int Npanels, int NRC, double sideLength,
      int NTimes)
 {   
    // Workspace2D_sptr wsPtr = WorkspaceFactory::Instance().create("Workspace2D", NPanels;
    
    const size_t &NVectors = (size_t)(Npanels * NRC * NRC);
    const size_t &ntimes = (size_t) NTimes;
    const size_t &nvals = (size_t) NTimes;
 
    Workspace2D_sptr wsPtr = boost::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", NVectors, ntimes, nvals));
    //wsPtr->initialize(NVectors, ntimes, nvals);
  
    IInstrument_sptr inst= ComponentCreationHelper::createTestInstrumentRectangular2(Npanels, NRC, sideLength);
   
    wsPtr->setInstrument(boost::dynamic_pointer_cast<Geometry::IInstrument>(inst));
   
    wsPtr->rebuildSpectraMapping(false);
   
    return wsPtr;
  }

};
#endif /* INTEGRATEPEAKTIMESLICESTEST_H_ */
