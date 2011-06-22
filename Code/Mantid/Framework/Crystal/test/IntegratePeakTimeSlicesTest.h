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

#include <map>
using namespace Mantid;
using namespace DataObjects;
using namespace Geometry;
using namespace API;

using namespace std;

class IntegratePeakTimeSlicesTest: public CxxTest::TestSuite
{
public:
  IntegratePeakTimeSlicesTest()
  {
  }

  void est_abc()
  {
    int NRC = 30;
    int NTimes = 40;
    int PeakRow = 12;
    int PeakCol = 17;
    int PeakChan = 15;
    double MaxPeakIntensity = 600;
    double MaxPeakRCSpan = 5;
    double MaxPeakTimeSpan = 8;

    Workspace2D_sptr wsPtr;// create2DWorkspaceWithRectangularInstrument(1, NRC,        NTimes);
    // wsPtr->getAxis(0)->setUnit("TOF");
    //Set times;
    MantidVecPtr x_vals;
    for (int i = 0; i < NTimes; i++)
      x_vals.access().push_back(1000.0 + i * 50);

    for (int k = 0; k < wsPtr->getNumberHistograms(); k++)
      wsPtr->setX(k, x_vals);

    Geometry::IInstrument_sptr instP = wsPtr->getInstrument();
    boost::shared_ptr<Geometry::IComponent> bankC = instP->getComponentByName(std::string("bank1"));

    if (bankC->type().compare("RectangularDetector") != 0)
      throw std::runtime_error(" No Rect bank named bank 1");

    boost::shared_ptr<Geometry::RectangularDetector> bankR = boost::dynamic_pointer_cast<
                                                 Geometry::RectangularDetector>(bankC);
    try
    {
      Kernel::V3D pp(2.5, 0.0, 3);
      bankR->setPos(pp);
      Kernel::Quat Q;
      Kernel::V3D pq(0.0, 1.0, 0.0);
      Q.setAngleAxis(20.0, pq);

    } catch (std::exception &ss1)
    {
      std::cout << "Cannot move/rot detector err=" << ss1.what() << std::endl;
    }

    boost::shared_ptr<Geometry::Detector> pixelp = bankR->getAtXY(PeakCol, PeakRow);

    //Now get Peak.
    double PeakTime = 2500;
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
          dataE.access().push_back(sqrt(val));
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

    cout << "dQ=" << dQ << endl;
    PeaksWorkspace_sptr pks(new PeaksWorkspace());

    pks->addPeak(peak);

    boost::shared_ptr<Mantid::API::Algorithm> algP =
          Mantid::API::AlgorithmFactory::Instance().create("IntegratePeakTimeSlices");

    algP->initialize();
    algP->setProperty("PeakIndex", 0);
    algP->setProperty("PeakQspan", dQ);

    algP->setProperty<MatrixWorkspace_sptr> ("InputWorkspace", wsPtr);

    algP->setProperty<PeaksWorkspace_sptr> ("Peaks", pks);

    try
    {
      //TableWorkspace_sptr twks(new Mantid::DataObjects::TableWorkspace());
      //algP.setProperty<TableWorkspace_sptr>("OutputWorkspace", twks);

      algP->execute();

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
  void test_abc()
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
    Rebin->setPropertyValue(std::string("Params"),std::string("17200,-.004,33500"));

    Rebin->execute();


    Workspace2D_sptr wsPtr = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve("RebinResult"));

    Geometry::IInstrument_sptr instP = wsPtr->getInstrument();

    boost::shared_ptr<Geometry::IComponent> bankC = instP->getComponentByName(std::string("bank26"));

    if (bankC->type().compare("RectangularDetector") != 0)
         throw std::runtime_error(" No Rect bank named bank 1");

   boost::shared_ptr<Geometry::RectangularDetector> bankR = boost::dynamic_pointer_cast<
                                          Geometry::RectangularDetector>(bankC);

   boost::shared_ptr<Geometry::Detector> pixelp = bankR->getAtXY(55,216);


   Peak peak(instP, 1718232, 6.947);

   PeaksWorkspace_sptr pks(new PeaksWorkspace());

   pks->addPeak(peak);
   IntegratePeakTimeSlices algP;
   algP.initialize();
   algP.setProperty("PeakIndex",0);
   algP.setProperty("PeakQspan",.003);
   algP.setProperty<MatrixWorkspace_sptr>("InputWorkspace",wsPtr);
   algP.setPropertyValue("OutputWorkspace","table");
   algP.setProperty<PeaksWorkspace_sptr>("Peaks",pks);
   algP.execute();
   
   
  }catch( std::exception &s)
  {
    std::cout<<"error ="<< s.what()<<std::endl;
  }
    std::cout<<"F"<<std::endl;

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
    double ScatAng = acos(pos.Y() / pos.norm());
    Q.fromTOF(x, x, L1, L2, ScatAng, 0, 0, 0.0);
    return x[0];

  }
  Workspace2D_sptr create2DWorkspaceWithRectangularInstrument(int Npanels, int NRC, double sideLength,
      int NTimes)
  {
    Workspace2D_sptr wsPtr;
    size_t NVectors = (size_t)(Npanels * NRC * NRC);
    size_t ntimes = (size_t) NTimes;
    size_t nvals = (size_t) NTimes;
    wsPtr->initialize(NVectors, ntimes, nvals);
    boost::shared_ptr<RectangularDetector> RectDet;
    boost::shared_ptr<Object> oo;
    RectDet->initialize(oo, NRC, -sideLength / 2, sideLength / NRC, NRC, -sideLength / 2, sideLength
        / NRC, 20, true, NRC, 1);
    RectDet->setPos(3, 0, 3);

    Kernel::Quat Q;

    Q.setAngleAxis(20.0, Kernel::V3D(0.0, 1.0, 0.0));
    RectDet->rotate(Q);
    boost::shared_ptr<Instrument> inst;
    boost::shared_ptr<IComponent> RCompP = boost::dynamic_pointer_cast<IComponent>(RectDet);

    inst->add(&(*RCompP));
    for (int r = 0; r < NRC; r++)
      for (int c = 0; c < NRC; c++)
      {
        boost::shared_ptr<Detector> detP = RectDet->getAtXY(c, r);
        boost::shared_ptr<IComponent> detPC = boost::dynamic_pointer_cast<IComponent>(detP);
        //NO don't do this ---inst->add(&(*detPC));
        inst->markAsDetector(&(*detP));
      }
    //linking problem  Why?? Cannot recast??
    /*boost::shared_ptr<Detector> sample("Sample", 2, inst);
    boost::shared_ptr<Detector> source("Source", 1, inst);
    sample->setPos(0, 0, 0);
    source->setPos(-10, 0, 0);
    inst->add(&(*sample));
    inst->markAsSamplePos(&(*sample));
    inst->markAsSource(&(*source));
    */
    wsPtr->setInstrument(boost::dynamic_pointer_cast<Geometry::IInstrument>(inst));
    wsPtr->rebuildSpectraMapping(false);
    return wsPtr;
  }

};
#endif /* INTEGRATEPEAKTIMESLICESTEST_H_ */
