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
      algP.setProperty("CalculateVariances", true);
      //algP.setProperty("Ties","Background=1.4");
      algP.execute();

      algP.setPropertyValue("OutputWorkspace", "aaa");

   
       double intensity = algP.getProperty("Intensity");
       double sigma = algP.getProperty("SigmaIntensity");
       TableWorkspace_sptr Twk = algP.getProperty("OutputWorkspace");
  

       TS_ASSERT_LESS_THAN(fabs(intensity -59982.9), 100.0);
      //Not sure why this reduced the error so much in the test
      TS_ASSERT_LESS_THAN(fabs(sigma -606.577), 1.0);

      TS_ASSERT_EQUALS( Twk->rowCount(), 7);
      
      if( Twk->rowCount() <5)
          return;

      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> (std::string("Time"), 0) - 19250), 20);  
    
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> (std::string("Background"), 1) - 1.40532), .2);
   
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("Intensity", 2) - 11201.5), 20);
      
   
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("NCells", 3) -  401), 5);
    
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("ChiSqrOverDOF", 4) -   133.909), 1.5);
    
      TS_ASSERT_LESS_THAN(fabs(Twk->getRef<double> ("TotIntensity", 0) - 4311.4), 10);
      
  

     /*   std::vector<std::string> names = Twk->getColumnNames();
      std::cout<<"Intensitty="<<pks->getPeak(0).getIntensity()<<"   sigma="<<pks->getPeak(0).getSigmaIntensity()<<
               "  Theoret intensity="<<TotIntensity<<std::endl;
      std::cout<<std::setw(15)<<"Act Int";
      for( int j=12; j< 12+Twk->rowCount();j++)
             std::cout<< setw(12)<<T[j];
      std::cout<<std::endl;
       for( int i=0; i<Twk->columnCount();i++)
       {
         std::cout<<std::setw(15)<<names[i];
       for( int j=0; j< Twk->rowCount();j++)
       std::cout<< setw(12)<<Twk->cell<double>(j,i);
       std::cout<<std::endl;

       }
       */
       /*
  Intensitty=59870.7   sigma=665.098
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background     1.42777     1.52378      1.5442     1.59245      1.5442     1.52378     1.42777
      Intensity     3728.05        7427     11136.6     14848.6     11136.6        7427     3728.05
           Mcol     17.0001          17          17          17          17          17     17.0001
           Mrow     12.0001     12.0001          12          12          12     12.0001     12.0001
          SScol     3.98569     3.97392     3.97892     3.97874     3.97892     3.97392     3.98569
          SSrow     3.98622     3.97946     3.97847      3.9784     3.97847     3.97946     3.98622
           SSrc -0.00115238 5.38493e-05 0.000837369 0.000628028 0.000837369 5.38493e-05 -0.00115238
         NCells         169         156         169         169         169         156         169
  ChiSqrOverDOF     35.7813     155.375     321.268     571.226     321.268     155.375     35.7813
   TotIntensity      3986.6      7718.4     11486.6     15236.6     11486.6      7718.4      3986.6
BackgroundError    0.547322     1.20823     1.64171     2.18909     1.64171     1.20823    0.547322
FitIntensityError     50.3565     106.679     150.909     201.222     150.909     106.679     50.3565
  ISAWIntensity     3745.31     7480.69     11225.6     14967.5     11225.6     7480.69     3745.31
ISAWIntensityError     113.065     208.524     297.868      390.35     297.868     208.524     113.065
      Start Row           5           6           6           6           6           6           5
        End Row          17          17          18          18          18          17          17
      Start Col          10          11          11          11          11          11          10
        End Col          22          23          23          23          23          23          22
------------------------------------------------------------------------------------
better-Calc new Variances instead of using attrib sent in  4 std dev
Intensitty=59953.3   sigma=621.308  Theoret intensity=60000
        Act Int        3750        7500       11250       15000       11250        7500        3750
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background     1.40659     1.42122     1.43184     1.44245     1.43184     1.42122     1.40659
      Intensity     3728.63     7453.29     11179.9     14906.6     11179.9     7453.29     3728.63
           Mcol          17          17          17          17          17          17          17
           Mrow          12          12          12          12          12          12          12
          SScol     3.98984     3.98363     3.98363     3.98363     3.98363     3.98363     3.98984
          SSrow     3.98933     3.98363     3.98363     3.98363     3.98363     3.98363     3.98933
           SSrc-6.21198e-14-1.64668e-12-1.28421e-12-9.32082e-13-1.28421e-12-1.64668e-12-6.21198e-14
         NCells         289         289         289         289         289         289         289
  ChiSqrOverDOF      20.689     83.3321     187.497     333.329     187.497     83.3321      20.689
   TotIntensity      4154.6      7904.6     11654.6     15404.6     11654.6      7904.6      4154.6
BackgroundError    0.294296    0.590553    0.885829     1.18111    0.885829    0.590553    0.294296
FitIntensityError     35.4244     71.0317     106.547     142.063     106.547     71.0317     35.4244
  ISAWIntensity      3748.1     7493.87     11240.8     14987.7     11240.8     7493.87      3748.1
ISAWIntensityError     108.604     193.503      278.58     363.777      278.58     193.503     108.604
  TotalBoundary        89.6        89.6        89.6        89.6        89.6        89.6        89.6
 NBoundaryCells          64          64          64          64          64          64          64
      Start Row           3           4           4           4           4           4           3
        End Row          19          20          20          20          20          20          19
      Start Col           9           9           9           9           9           9           9
        End Col          25          25          25          25          25          25          25

========================================================================================
Circular regions
Intensitty=59982.9   sigma=606.577  Theoret intensity=60000
        Act Int        3750        7500       11250       15000       11250        7500        3750
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background     1.40266     1.40532     1.40798     1.41064     1.40798     1.40532     1.40266
      Intensity     3733.82     7467.64     11201.5     14935.3     11201.5     7467.64     3733.82
           Mcol          27          27          27          27          27          27          27
           Mrow          22          22          22          22          22          22          22
          SScol     3.99206     3.99206     3.99206     3.99206     3.99206     3.99206     3.99206
          SSrow     3.99206     3.99206     3.99206     3.99206     3.99206     3.99206     3.99206
           SSrc-5.34109e-12-5.46531e-12-4.38881e-12-2.48423e-12-4.38881e-12-5.46531e-12-5.34109e-12
         NCells         401         401         401         401         401         401         401
  ChiSqrOverDOF     14.8788     59.5151     133.909     238.061     133.909     59.5151     14.8788
   TotIntensity      4311.4      8061.4     11811.4     15561.4     11811.4      8061.4      4311.4
BackgroundError    0.205936    0.411872    0.617809    0.823745    0.617809    0.411872    0.205936
FitIntensityError     29.2084     58.4169     87.6253     116.834     87.6253     58.4169     29.2084
  ISAWIntensity     3748.93     7497.87     11246.8     14995.7     11246.8     7497.87     3748.93
ISAWIntensityError     108.136     189.481     271.573     353.892     271.573     189.481     108.136
  TotalBoundary       151.2       151.2       151.2       151.2       151.2       151.2       151.2
 NBoundaryCells         108         108         108         108         108         108         108
      Start Row          11          11          11          11          11          11          11
        End Row          33          33          33          33          33          33          33
      Start Col          16          16          16          16          16          16          16
        End Col          38          38          38          38          38          38          38

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
