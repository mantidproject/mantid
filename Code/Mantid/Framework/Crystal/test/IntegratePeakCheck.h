/*
 * IntegratePeakTimeSlicesTest.h
 *
 *  Created on: Jun 7, 2011
 *      Author: ruth
 */

#ifndef INTEGRATEPEAKCHECK_H_
#define INTEGRATEPEAKCHECK_H_


#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Objects/BoundingBox.h"
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
#include "MantidKernel/MersenneTwister.h"
#include "MantidAPI/IPeak.h"
#include <math.h>
#include <cstdlib>

#include <map>
#include "MantidKernel/Quat.h"
#include "MantidAPI/FrameworkManager.h"
using namespace Mantid;
using namespace DataObjects;
using namespace Geometry;
using namespace API;
using namespace Mantid::Crystal;
using namespace std;

class IntegratePeakCheck: public CxxTest::TestSuite
{
public:
  IntegratePeakCheck()
  {
    Mantid::API::FrameworkManager::Instance();
    usePoisson = false;
    m_randGen.setSeed(1234);
  }
/*
   int* ArryofIDs = new int[500];
    ArryofIDs[0]=500;
    ArryofIDs[1]=2;
    Kernel::V3D Center = pixelp->getPos();
    double Radius = .5;
    std::cout<<"IntegratePeakCheck G"<<std::endl;
    boost::shared_ptr< Geometry::RectangularDetector> comp1 =
            boost::const_pointer_cast< Geometry::RectangularDetector> (bankR);
    boost::shared_ptr<Geometry::IComponent>comp =
        boost::dynamic_pointer_cast<Geometry::IComponent>( comp1);
    std::cout<<"IntegratePeakCheck H"<<std::endl;
    std::cout<<"Neighbors="<<getNeighborPixIDs(comp, Center,Radius,ArryofIDs)<<","<<ArryofIDs[1]<<Center<<std::endl;
    std::cout<<"IntegratePeakCheck I"<<std::endl;
    for( int i=2; i<ArryofIDs[1];i++)
    {
      std::pair< int, int > res = bankR->getXYForDetectorID( ArryofIDs[i]);
      std::cout<<"("<<res.first<<","<<res.second<<")";

    }
    std::cout<<std::endl;
    delete ArryofIDs;
 */
  bool getNeighborPixIDs( boost::shared_ptr< Geometry::IComponent> comp,
                           Kernel::V3D &Center,
                           double &Radius,
                          int* &ArryofID)
   {

        int N = ArryofID[1];
        int MaxN= ArryofID[0];
        if( N >= MaxN)
          return false;
        Geometry::BoundingBox box;
        comp->getBoundingBox( box);

        double minx = Center.X() - Radius;
        double miny = Center.Y() - Radius;
        double minz = Center.Z() - Radius;
        double maxx = Center.X() + Radius;
        double maxy = Center.Y() + Radius;
        double maxz = Center.Z() + Radius;

        if( box.xMin()>=maxx)
            return true;
        if( box.xMax() <=minx)
           return true;;
        if( box.yMin()>=maxy)
            return true;;
        if( box.yMax() <=miny)
           return true;;
        if( box.zMin()>=maxz)
            return true;;
        if( box.zMax() <=minz)
           return true;;

        if( comp->type().compare("Detector")==0 || comp->type().compare("RectangularDetectorPixel")==0)
         {
                 boost::shared_ptr<Geometry::Detector> det =   boost::dynamic_pointer_cast<Geometry::Detector>( comp);
                 if( (det->getPos()-Center).norm() <Radius)
                 {
                   ArryofID[N]=  det->getID();
                   N++;
                    ArryofID[1] = N;
                 }
                return true;
          }

         boost::shared_ptr<const Geometry::CompAssembly> Assembly =
             boost::dynamic_pointer_cast<const Geometry::CompAssembly>( comp);

         if( !Assembly)
            return true;

         bool b = true;

         for( int i=0; i< Assembly->nelements() && b; i++)
            b= getNeighborPixIDs( Assembly->getChild(i),Center,Radius,ArryofID);

         return b;
   }

  void test_abc()
  {
    for( int i=0; i<1;i++)
      trest_abc();
  }
  void trest_abc()
  {
    int NRC = 80;//30;
    int NTimes = 40;
    int PeakRow =22;// 12;
    int PeakCol = 27;//17;
    int PeakChan = 15;
    double MaxPeakIntensity = 2600;
    double MaxPeakRCSpan = 10;//5;
    int MaxPeakTimeSpan = 4;

    double T[40]={0.0};
    Workspace2D_sptr wsPtr = create2DWorkspaceWithRectangularInstrument(1, NRC, .05, NTimes);

    wsPtr->getAxis(0)->setUnit("TOF");

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

// --------------- testing for nearest neighborhood -----------------------------------------
/*   std::cout<<"----------------------------------------------"<<std::endl;
    std::cout<< "        Neighbors  "<< std::endl;
    detid2index_map * detid_to_wi_map = wsPtr->getDetectorIDToWorkspaceIndexMap( false );
    Mantid::detid2index_map::iterator it;

    it = (*detid_to_wi_map).find( pixelp->getID() );
    size_t wsIndx = it->second;
    double CellHeight = bankR->ysize()/bankR->ypixels();
    double CellWidth = bankR->xsize()/bankR->xpixels();
    double neighborRadius = 1.5*MaxPeakRCSpan*max<double>(CellHeight,CellWidth);
    double Nneighbors =(1.5*MaxPeakRCSpan)*(1.5*MaxPeakRCSpan)*3.1415;//  *5 gets more

    specid_t CentDetspec = wsPtr->getSpectrum( wsIndx)->getSpectrumNo();
    wsPtr->buildNearestNeighbours(true);
    wsPtr->getNeighboursExact(CentDetspec,Nneighbors,true );
    std::map< specid_t, Kernel::V3D > neighbors  = wsPtr->getNeighbours( CentDetspec, neighborRadius, true);
    int kk=0;
    std::cout<< "nearest neighbors from (r,c)=(22,27). Radius-dist/pixels ="<<neighborRadius<<"/"<<
                 (1.5*MaxPeakRCSpan)<<std::endl;
    for( map< specid_t, Kernel::V3D>::iterator nn=neighbors.begin();nn!=neighbors.end();nn++)
    {
       Kernel::V3D pixPos = (*nn).second;
       pixPos +=pixelp->getPos();
       Kernel::V3D center = bankR->getPos();
       double ROW = bankR->ypixels()/2;
       double COL = bankR->xpixels()/2;
       Kernel::V3D xvec(1,0,0);
       Kernel::V3D yvec(0,1,0);
       Kernel::Quat Rot = bankR->getRotation();
       Rot.rotate(xvec);
       Rot.rotate(yvec);
       double row = ROW + (pixPos - center).scalar_prod(yvec) / CellHeight;
       double col = COL + (pixPos - center).scalar_prod(xvec) / CellWidth;

       kk++;
       std::cout<<"("<<row<<","<<col<<")";;
       if( kk++%10==0)std::cout<<std::endl;
    }

    std::cout<<std::endl<<"----------------------------------------------"<<std::endl;
    delete detid_to_wi_map;

*/


//-------------------- end testing for nearest neighborhood ---------------------------

    //Now set up data in workspace2D
    double dQ = 0;
    double Q0 = calcQ(bankR, instP, PeakRow, PeakCol, 1000.0 + 30.0 * 50);


    double TotIntensity = 0;
    double Background =1.4;
    double corr = 0;//.5*MaxPeakRCSpan/2;
    std::cout<<"Starting setting up data"<<std::endl;
    for (int row = 0; row < NRC; row++)
      for (int col = 0; col < NRC; col++)
      {


        MantidVecPtr dataY;
        MantidVecPtr dataE;
        double vv= CalcDataRect( row, col,MaxPeakIntensity,Background,NTimes,PeakCol,PeakRow, MaxPeakRCSpan,MaxPeakRCSpan,
            corr, PeakChan,MaxPeakTimeSpan,dataY,  dataE,Q0, 1000.0, 50.0,dQ,  T, instP, bankR);

        TotIntensity+= vv;

        boost::shared_ptr<Detector> detP = bankR->getAtXY(col, row);
        detid2index_map * map = wsPtr->getDetectorIDToWorkspaceIndexMap(true);

        detid2index_map::iterator it = map->find(detP->getID());
        size_t wsIndex = (*it).second;

        wsPtr->setData(wsIndex, dataY, dataE);
      }
      std::cout<<"Start get Neighbors"<<std::endl;
       double Radius = 40*.05;
      int* ArryofIDs = new int[6500];
       ArryofIDs[0]=6500;
       ArryofIDs[1]=2;
       Kernel::V3D Center = pixelp->getPos();

       boost::shared_ptr< Geometry::RectangularDetector> comp1 =
               boost::const_pointer_cast< Geometry::RectangularDetector> (bankR);
       boost::shared_ptr<Geometry::IComponent>comp =
           boost::dynamic_pointer_cast<Geometry::IComponent>( comp1);

       for( int i=2; i<ArryofIDs[1];i++)
       {
         std::pair< int, int > res = bankR->getXYForDetectorID( ArryofIDs[i]);

       }

       delete ArryofIDs;
    PeaksWorkspace_sptr pks(new PeaksWorkspace());

    std::cout<<"Ending setting up data"<<std::endl;
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
      algP.setProperty("CalculateVariances", false);
      //algP.setProperty("Ties","Background=1.4");

      algP.execute();

      algP.setPropertyValue("OutputWorkspace", "aaa");

       double intensity = algP.getProperty("Intensity");
       double sigma = algP.getProperty("SigmaIntensity");
       TableWorkspace_sptr Twk = algP.getProperty("OutputWorkspace");

      std::vector<std::string> names = Twk->getColumnNames();

      std::cout<<"Intensitty="<<intensity<<"   sigma="<<sigma <<
          "  Theoret intensity="<<TotIntensity<<std::endl;

      std::cout<<std::setw(12)<<"Act Int";
      for( int i=0; i<Twk->columnCount();i++)
           std::cout<<std::setw(10)<<std::string(names[i]).substr(0,10);
      std::cout<<std::endl;
      for( int j=0; j< Twk->rowCount(); j++)
      {
         std::cout<< setw(10)<<T[j+12];
         for( int i=0;i+1< Twk->columnCount(); i++)
            std::cout<< setw(10)<<Twk->cell<double>(j,i);
         std::cout<<std::endl;
      }

  /*    std::cout<<std::setw(15)<<"Act Int";
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
 Normal totIntensity =2600. Poisson, Vxy=0,CalcVariances = false. Center rc=(22,27),Var=2.78. Latest fixes
Intensitty=10683.2   sigma=188.557  Theoret intensity=10088
        Act Int         733        1316        2028        2425        2174        1487         639
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background     1.48965     1.33345      1.3675      1.3236     1.33323     1.32687     1.42502
      Intensity     670.542     1310.63     1969.33     2703.18     1986.71     1376.21     666.567
           Mcol     27.1986     27.0639     26.9593     27.0644     26.8834     27.0129     26.9705
           Mrow      21.993     22.0217     21.9996     22.0288     22.0238     22.0335      22.052
          SScol     2.76821      2.6502     2.66979     2.92978     2.73129     2.72863     2.89939
          SSrow     2.86457     3.18223     2.77575     2.89194     2.96681     3.07288     2.84758
           SSrc    0.254323   -0.116972   -0.238217   0.0674513   -0.170581    0.132619    0.142739
         NCells         342         361         361         361         361         342         361
  ChiSqrOverDOF     3.35854     5.70275      4.8726     4.41752     5.00384     4.25035     3.44579
   TotIntensity        1180        1792        2463        3181        2468        1830        1181
BackgroundError    0.111218    0.140699    0.128992    0.123878    0.131451     0.12563    0.109217
FitIntensityError     17.2677     22.8292     20.2332     20.1288     21.1062     19.8094     17.6234
  ISAWIntensity     670.541     1310.63     1969.33     2703.18     1986.71     1376.21     666.567
ISAWIntensityError      56.354     69.6652     77.4138     87.4528     77.6268     67.8808     57.0083
  TotalBoundary         110          90         103          92          96          83         111
 NBoundaryCells          70          72          72          72          72          70          72
      Start Row          12          13          12          12          13          13          13
        End Row          30          31          30          30          31          31          31
      Start Col          18          18          17          18          17          18          17
        End Col          35          36          35          36          35          35          35

=================================================================================
 Normal totIntensity =2600. no Poisson, Vxy=0,CalcVariances = false
 Intensitty=10400   sigma=129.864  Theoret intensity=9892
        Act Int     641.752      1283.5     1925.26     2567.01     1925.26      1283.5     641.752
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background         1.4         1.4         1.4         1.4         1.4         1.4         1.4
      Intensity         650        1300        1950        2600        1950        1300         650
           Mcol          17          17          17          17          17          17          17
           Mrow          12          12          12          12          12          12          12
          SScol          25          25          25          25          25          25          25
          SSrow          25          25          25          25          25          25          25
           SSrc 3.17327e-17-6.24401e-17-6.92601e-16-2.44512e-16-6.92601e-16-6.24401e-17 3.17327e-17
         NCells         702         702         702         702         702         702         702
  ChiSqrOverDOF 1.19819e-31 3.38955e-31  1.3749e-30 6.24846e-31  1.3749e-30 3.38955e-31 1.19819e-31
   TotIntensity     1613.29     2243.78     2874.28     3504.77     2874.28     2243.78     1613.29
BackgroundError 2.51693e-17 4.23331e-17 8.52599e-17 5.74771e-17 8.52599e-17 4.23331e-17 2.51693e-17
FitIntensityError 1.69903e-14 2.85766e-14  5.7554e-14 3.87995e-14  5.7554e-14 2.85766e-14 1.69903e-14
  ISAWIntensity     630.492     1260.98     1891.48     2521.97     1891.48     1260.98     630.492
ISAWIntensityError     92.6493     107.492      120.52     132.271      120.52     107.492     92.6493
  TotalBoundary     155.338     167.875     180.413      192.95     180.413     167.875     155.338
 NBoundaryCells         102         102         102         102         102         102         102
      Start Row           2           2           2           2           2           2           2
        End Row          27          27          27          27          27          27          27
      Start Col           2           2           2           2           2           2           2
        End Col          28          28          28          28          28          28          28

 ===========================================================
  Normal totIntensity =2600. Poisson, Vxy=0,CalcVariances = false
  Intensitty=9367.52   sigma=117.334  Theoret intensity=9928
        Act Int         682        1297        1954        2499        1955
           Time       19350       19450       19550       19650       19750
        Channel          13          14          15          16          17
     Background     1.16368     1.56151     1.20608     1.43547     1.41426
      Intensity     1561.83     1844.88     2680.26     1971.74     1308.81
           Mcol     16.6725     16.8946     17.0803     16.8705     16.8182
           Mrow     11.9658      11.944      12.078     12.1852     12.0658
          SScol     35.1846     21.9897     24.9791     23.0044     21.6356
          SSrow      29.777     24.7186     27.3536     23.5459     24.0853
           SSrc    -1.19386    0.605025    0.634657    -1.52383   -0.312735
         NCells         702         702         702         702         702
  ChiSqrOverDOF     3.01364     3.66889     5.02992     4.63504     3.32007
   TotIntensity        2290        2894        3434        2935        2272
BackgroundError    0.158827    0.132047     0.16842    0.149575    0.124544
FitIntensityError     124.071     85.8388     116.607     96.8152     79.9521
  ISAWIntensity      1473.1     1797.82     2587.33      1927.3     1279.19
ISAWIntensityError     128.699     116.813     135.127     122.344     104.445
  TotalBoundary         177         186         179         158         157
 NBoundaryCells         102         102         102         102         102
      Start Row           2           2           2           2           2
        End Row          27          27          27          27          27
      Start Col           2           2           2           2           2
        End Col          28          28          28          28          28
        =================================================
        theoretical normal mean 2600, center (22,27)  CalcVarieance no Poisson

        Intensitty=10400   sigma=192.088  Theoret intensity=10327
        Act Int         650        1300        1950        2600        1950        1300         650
           Time       19250       19350       19450       19550       19650       19750       19850
        Channel          12          13          14          15          16          17          18
     Background         1.4         1.4         1.4         1.4         1.4         1.4         1.4
      Intensity         650        1300        1950        2600        1950        1300         650
           Mcol          27          27          27          27          27          27          27
           Mrow          22          22          22          22          22          22          22
          SScol     2.77785     2.77777      2.7778     2.77777      2.7778     2.77777     2.77785
          SSrow     2.77785     2.77777      2.7778     2.77777      2.7778     2.77777     2.77785
           SSrc  1.0746e-12-5.37302e-13-9.55205e-13 8.05954e-13-9.55205e-13-5.37302e-13  1.0746e-12
         NCells         509         509         509         509         509         509         509
  ChiSqrOverDOF     2.67701     3.95403     5.23104     6.50806     5.23104     3.95403     2.67701
   TotIntensity      1362.6      2012.6      2662.6      3312.6      2662.6      2012.6      1362.6
BackgroundError   0.0751439   0.0913246    0.105042    0.117164    0.105042   0.0913246   0.0751439
FitIntensityError     10.0164     12.1731     14.0016     15.6173     14.0016     12.1731     10.0164
  ISAWIntensity         650        1300        1950        2600        1950        1300         650
ISAWIntensityError     59.4822     69.8998     78.9547      87.073     78.9547     69.8998     59.4822
  TotalBoundary       151.2       151.2       151.2       151.2       151.2       151.2       151.2
 NBoundaryCells         108         108         108         108         108         108         108
      Start Row          10          10          10          10          10          10          10
        End Row          34          34          34          34          34          34          34
      Start Col          15          15          15          15          15          15          15


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




private:
  bool usePoisson;
  Mantid::Kernel::MersenneTwister m_randGen;
  double  Poisson( double mean)
  {
    double T = exp(-mean);
    double P = m_randGen.nextValue();
    int N=0;
    double S =T;
   // std::cout<< "Poisson "<< mean<<","<<P<<","<<T;
     while( P > S && N < 100000 )
     {
       N++;
       T *= mean/N;
       S +=T;

     }
     //std::cout<<","<<S<<","<<N<< std::endl;
     return N;

  }

  double CalcDataRect( double row, double col,double MaxIntensity,double Background,int nTimes,
                double Mx0,double My0, double Spanx,double Spany, double Vxy, double Chan0,int SpanChan,
                MantidVecPtr &dataY, MantidVecPtr &dataE,double Q0,double time0, double TperChan,double &dQ, double* TotI,
                boost::shared_ptr<const Instrument> instP,RectangularDetector_const_sptr bankR)
  {
           UNUSED_ARG( Vxy);
           double MaxR = max<double> (0.0, MaxIntensity * (1 - abs(row - My0) / Spany));
           double MaxRC = max<double> (0.0, MaxR * (1 - abs(col - Mx0) /Spanx));
           double TT=0;
           for (int chan = 0; chan < nTimes; chan++)
           {
             double val = max<double> (0.0, MaxRC * (1 - abs(chan - Chan0) / SpanChan));
             if ( usePoisson )
               val = Poisson( val);

             TT+=val;
             TotI[chan]+=val;
             val += Background;

             dataY.access().push_back(val);
             dataE.access().push_back( sqrt(val));

             if ((val - Background) > MaxIntensity * .1)
             {// std::cout<<"("<<row<<","<<col<<","<<val<<",";
               double Q = calcQ(bankR, instP, row, col, time0+chan*TperChan);
               dQ = max<double> (dQ, fabs(Q - Q0));
               //std::cout<<","<<dQ<<")";
             }
           }
        return TT;
  }

  double NormVal( double Background, double Intensity, double Mcol, double Mrow, double Vx,
                   double Vy, double Vxy, double row, double col)
   {

     double uu = Vx* Vy - Vxy * Vxy;

     double coefNorm = .5 / M_PI / sqrt(uu);

     double expCoeffx2 = -Vy / 2 / uu;
     double expCoeffxy = Vxy / uu;
     double expCoeffy2 = -Vx / 2 / uu;
     double dx = col - Mcol;
     double dy = row - Mrow;
     return  Background + coefNorm * Intensity *
                  exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy + expCoeffy2 * dy * dy);

   }

  double CalcDataNorm( double row, double col,double MaxIntensity,double Background,int nTimes,
                double Mx0,double My0, double Spanx,double Spany, double Vxy, double Chan0,int SpanChan,
                MantidVecPtr &dataX, MantidVecPtr &dataE, double Q0,double time0, double TperChan,double &dQ, double* T,
                boost::shared_ptr<const Instrument> instP,RectangularDetector_const_sptr bankR)
  {
     double TotI=0;
     double Sx= Spanx/2/3;
     double Sy= Spany/2/3;
     double uu = Sx*Sx* Sy*Sy - Vxy * Vxy;

     double coefNorm = .5 / M_PI / sqrt(uu);
     double PeakMaxIntensity = coefNorm* MaxIntensity;

     for( int chan=0;chan<nTimes; chan++)
     {
       double Intensity =  max<double> (0.0, MaxIntensity * (1 - abs( chan- Chan0) /SpanChan));

       double val =NormVal( Background,Intensity, Mx0, My0,
           Sx*Sx,Sy*Sy, Vxy, row,col);
       double Tval =val-Background;


       if ( usePoisson )
         val = Poisson( val);

       dataX.access().push_back( val );
       dataE.access().push_back( sqrt(val) );
       T[chan]+= val - Background;
       TotI += val- Background;

       if ( Tval > .1*PeakMaxIntensity)
       {
         double Q = calcQ(bankR, instP, row, col, time0 +TperChan*chan);
         dQ = max<double> (dQ, fabs(Q - Q0));
       }

     }
     return TotI;
  }
  /**
   *   Calculates Q
   */
  double calcQ(RectangularDetector_const_sptr bankP, boost::shared_ptr<const Instrument> instPtr, double row, double col,
      double time)
  {
    boost::shared_ptr<Detector> detP = bankP->getAtXY((int)(.5+col),(int)(.5+ row));

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
#endif /* INTEGRATEPEAKCHECK_H_ */
