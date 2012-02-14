/*WIKI* 


This algorithm fits a bivariate normal distribution( plus background) to the 
data on each time slice. This algorithm only works for peaks on a Rectangular  
Detector.  The rectangular area used for the fitting is calculated based on 
the dQ parameter.  A good value for dQ is .1667/largest unit cell length.

The table workspace is also a result. Each line contains information on the fit
for each good time slice.  The column names( and information) in the table are:
 Time, Channel, Background, Intensity,Mcol,Mrow,SScol,SSrow,SSrc,NCells,
 ChiSqrOverDOF,TotIntensity,BackgroundError,FitIntensityError,ISAWIntensity,
 ISAWIntensityError,TotalBoundary,NBoundaryCells,Start Row,End Row,Start Col,End Col


The final Peak intensity is the sum of the IsawIntensity for each time slice.
The error is the square root of the sum of squares of the IsawIntensityError values.

The columns whose names are  Background, Intensity, Mcol, Mrow, SScol, SSrow, and SSrc
are the parameters for the BivariateNormal curve fitting function.  

This algorithm has been carefully tweaked to give good results for interior peaks only. 
Peaks close to the edge of the detector may not give good results.

This Algorithm is also used by the PeakIntegration algorithm when the Fit tag is selected.
*WIKI*/
/*
 * IntegratePeakTimeSlices.cpp
 *
 *  Created on: May 5, 2011
 *      Author: ruth
 *
 *
 *
 */

#include "MantidCrystal/IntegratePeakTimeSlices.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidGeometry/IComponent.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Surfaces/Surface.h"
//
#include <vector>
#include "MantidAPI/Algorithm.h"
#include <algorithm>
#include <math.h>
#include <cstdio>
#include <boost/random/poisson_distribution.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace std;
namespace Mantid
{
  namespace Crystal
  {

    Kernel::Logger& IntegratePeakTimeSlices::g_log = Kernel::Logger::get("IntegratePeakTimeSlices");

    DECLARE_ALGORITHM(IntegratePeakTimeSlices)

    //Attr, AttributeValues, and StatBase indicies
#define IStartRow 0
#define IStartCol 1
#define INRows 2
#define INCol 3
#define ISSIxx 4
#define ISSIyy 5
#define ISSIxy 6
#define ISSxx 7
#define ISSyy 8
#define ISSxy 9
#define ISSIx 10
#define ISSIy 11
#define ISSx  12
#define ISSy  13
#define IIntensities 14
#define ISS1         15
#define IVariance    16
#define ITotBoundary  17
#define INBoundary    18
#define NAttributes  19

    //Parameter indicies
#define IBACK   0
#define ITINTENS  1
#define IXMEAN  2
#define IYMEAN  3
#define IVXX  4
#define IVYY  5
#define IVXY  6

#define NParameters  7

//TODO  Adjust for edge peaks. Identify, if so do not calculate (co)variances, do NOT say bad slice when
   //Fit and IsawIntensity too far apart. Also, using std devn's to determine size of rectangle has to be completely redone.


    IntegratePeakTimeSlices::IntegratePeakTimeSlices() :
      Algorithm(), wi_to_detid_map(NULL), R0(-1)
    {
      debug = false;

      if (debug)
        g_log.setLevel(7);
      EdgePeak = false;
      NeighborIDs = new int[3];
      NeighborIDs[0]=3;
      NeighborIDs[1]=2;
      AttributeNames[0] = "StartRow";
      AttributeNames[1] = "StartCol";
      AttributeNames[2] = "NRows";
      AttributeNames[3] = "NCols";
      AttributeNames[4] = "SSIxx";
      AttributeNames[5] = "SSIyy";
      AttributeNames[6] = "SSIxy";
      AttributeNames[7] = "SSxx";
      AttributeNames[8] = "SSyy";
      AttributeNames[9] = "SSxy";
      AttributeNames[10] = "SSIx";
      AttributeNames[11] = "SSIy";
      AttributeNames[12] = "SSx";
      AttributeNames[13] = "SSy";
      AttributeNames[14] = "Intensities";
      AttributeNames[15] =" SS1";
      AttributeNames[16] = "Variance";
      AttributeNames[17] = "TotBoundary";
      AttributeNames[18] = "NBoundary";

      ParameterNames[0] = "Background";
      ParameterNames[1] = "Intensity";
      ParameterNames[2] = "Mcol";
      ParameterNames[3] = "Mrow";
      ParameterNames[4] = "SScol";
      ParameterNames[5] = "SSrow";
      ParameterNames[6] = "SSrc";

      for (int i = 0; i < NAttributes; i++)
        AttributeValues[i] = 0;

      for (int i = 0; i < NParameters; i++)
        ParameterValues[i] = 0;
    }

    /// Destructor
    IntegratePeakTimeSlices::~IntegratePeakTimeSlices()
    {
      delete wi_to_detid_map;
      delete [] NeighborIDs;
    }

    void IntegratePeakTimeSlices::initDocs()
    {
      this->setWikiSummary("Integrates each time slice around a peak adding the results to the peak object");
      this->setOptionalMessage("The algorithm uses CurveFitting::BivariateNormal for fitting a time slice");
    }


    void IntegratePeakTimeSlices::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace", "", Direction::Input),
          "A 2D workspace with X values of time of flight");

      declareProperty(new WorkspaceProperty<TableWorkspace> ("OutputWorkspace", "", Direction::Output),
          "Name of the output workspace with Log info");

      declareProperty(new WorkspaceProperty<PeaksWorkspace> ("Peaks", "", Direction::Input),
          "Workspace of Peaks");

      declareProperty("PeakIndex", 0, "Index of peak in PeaksWorkspace to integrate");

      declareProperty("PeakQspan", .03, "Max magnitude of Q of Peak to Q of Peak Center, where |Q|=1/d");

      declareProperty("CalculateVariances",true,"Calc (co)variances given parameter values versus fit (co)Variances ");

      declareProperty("Ties","","Tie parameters(Background,Intensity, Mrow,...) to values/formulas.");

      declareProperty("Intensity", 0.0, "Peak Integrated Intensity", Direction::Output);

      declareProperty("SigmaIntensity",0.0,"Peak Integrated Intensity Error", Direction::Output);


    }

    bool IntegratePeakTimeSlices::getNeighborPixIDs( boost::shared_ptr< Geometry::IComponent> comp,
                                                     Kernel::V3D                           &Center,
                                                     double                                &Radius,
                                                     int*                                  &ArryofID)
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

    double IntegratePeakTimeSlices::CalculatePositionSpan(  DataObjects::Peak const &peak,
                                                      const double                   dQ)
   {

     double Q = 0, ScatAngle = 0, dScatAngle = 0, DetSpan = 0;

     try
     {
       Q = peak.getQLabFrame().norm();
       Geometry::Instrument_const_sptr instr = peak.getInstrument();
       const Geometry::IObjComponent_const_sptr  sample = instr->getSample();
       V3D pos = peak.getDetPos()-sample->getPos();

       ScatAngle = acos(pos.Z() / pos.norm());


       dScatAngle = 2 * dQ / Q * tan(ScatAngle/2);


       DetSpan = pos.norm() * dScatAngle; //s=r*theta

       DetSpan = fabs(DetSpan);

      // IDetector_sptr det = peak.getDetector();

       return DetSpan;

     } catch (std::exception &s)
     {
       std::cout << "err in getNRowsCols, reason=" << s.what() << std::endl;
       return 0;
     }

   }


    void IntegratePeakTimeSlices::exec()
    {

      char logInfo[200];

      double dQ = getProperty("PeakQspan");

      g_log.debug("------------------Start Peak Integrate-------------------");


      if (dQ <= 0)
      {
        g_log.error("Negative PeakQspans are not allowed. Use .17/G where G is the max unit cell length");
        throw  std::runtime_error("Negative PeakQspans are not allowed in IntegratePeakTimeSlices");
      }


      MatrixWorkspace_sptr inpWkSpace = getProperty("InputWorkspace");
      if (!inpWkSpace)
      {
        g_log.error("Improper Input Workspace");
        throw std::runtime_error("Improper Input Workspace in IntegratePeakTimeSlices");
      }


      PeaksWorkspace_sptr peaksW;
      peaksW = getProperty("Peaks");
      if (!peaksW)
      {
        g_log.error("Improper Peaks Input");
        throw std::runtime_error("Improper Peaks Input");
      }

      int indx = getProperty("PeakIndex");

      IPeak &peak = peaksW->getPeak(indx);

      boost::shared_ptr<const Geometry::IComponent> panel_const= peak.getInstrument()->getComponentByName( peak.getBankName());

      boost::shared_ptr< Geometry::IComponent>panel =
              boost::const_pointer_cast< Geometry::IComponent>( panel_const);
      if( !panel  || !panel_const)
      {
        g_log.error( "Cannot get panel for a peak");
        throw std::runtime_error("Cannot get panel for a peak");
      }
      BoundingBox box;
      panel->getBoundingBox( box);

      int detID = peak.getDetectorID();
      if( !box.isPointInside( peak.getDetPos()))
          {
              g_log.error("Detector pixel is NOT inside the Peaks Bank");
              throw std::runtime_error("Detector pixel is NOT inside the Peaks Bank");
          }
      FindPlane(  center,  xvec,  yvec, ROW, COL, CellWidth, CellHeight, peak);

      sprintf(logInfo, std::string("   Peak Index %5d\n").c_str(), indx);
      g_log.debug(std::string(logInfo));

      double TotVariance = 0;
      double TotIntensity = 0;
      double lastRow = ROW;
      double Row0 = lastRow;
      double lastCol = COL;
      double Col0 = lastCol;

      // For quickly looking up workspace index from det id
      wi_to_detid_map = inpWkSpace->getDetectorIDToWorkspaceIndexMap( false );

      TableWorkspace_sptr TabWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(0));



      try
      {

        Mantid::detid2index_map::iterator it1= wi_to_detid_map->begin();


        // Find the workspace index for this detector ID
        Mantid::detid2index_map::iterator it;
        it = (*wi_to_detid_map).find(detID);
        size_t wsIndx = (it->second);

        double R      = CalculatePositionSpan( peak, dQ )/2;

        R = min<double> (36*max<double>(CellWidth,CellHeight), R);
        R = max<double> (6*max<double>(CellWidth,CellHeight), R);
        R=1.1*R;//Gets a few more background cells.
        int Chan;

        Mantid::MantidVec X = inpWkSpace->dataX(wsIndx);
        int dChan = CalculateTimeChannelSpan(peak, dQ, X, int(wsIndx), Chan);

        dChan = max<int> (dChan, 3);


        int MaxChan = -1;
        double MaxCounts = -1;
        double Centy = Row0;
        double Centx = Col0;
        IDetector_const_sptr CenterDet =  peak.getDetector();
        //std::map< specid_t, V3D > neighbors;


        bool done = false;
        //specid_t   CentDetspec;//last for finding neighbors
        double neighborRadius ;//last radius for finding neighbors
        //CentDetspec = spec;
        neighborRadius = min<double>(10,1.5*R);
        int Nneighbors = (int)(neighborRadius*neighborRadius/CellWidth/CellHeight*4);


        Nneighbors = min<int>(Nneighbors,(int)inpWkSpace->getNumberHistograms()-2);
        //inpWkSpace->buildNearestNeighbours( true);
        //neighbors = inpWkSpace->getNeighboursExact( spec, Nneighbors ,true);

        //neighbors  = inpWkSpace->getNeighbours( CentDetspec, neighborRadius, true);
        //neighbors[CentDetspec]=Kernel::V3D(0.0,0.0,0.0);
        delete [] NeighborIDs;

        NeighborIDs = new int[Nneighbors+2];
        NeighborIDs[0]=Nneighbors+2;
        NeighborIDs[1] = 2;
        Kernel::V3D Cent =(center+xvec*(Centx-COL)+yvec*(Centy-ROW));

        getNeighborPixIDs(panel,Cent,neighborRadius, NeighborIDs );
       // std::cout<<"  R,#neighbors,neighborRadius="<<R<<","<<NeighborIDs[1]<<","<<neighborRadius<<std::endl;
       // std::cout<<"Chan and dChan,Cell dims="<< Chan<<","<<dChan<<","<<CellWidth<<","<<CellHeight<<std::endl;
        if( NeighborIDs[1] <10)
        {
          g_log.error("Not enough neighboring pixels to fit ");
          throw std::runtime_error("Not enough neighboring pixels to fit ");
        }
        for( int dir =1 ; dir >-2; dir -=2)
        for( int t= 0; t < dChan && !done; t++)
          if( dir < 0 &&  t==0 )
           {
             Centy = Row0;
             Centx = Col0;
             done = false;
            }
           else if( Chan+dir*t <0 || Chan+dir*t >= (int)X.size())
              done = true;
           else
           {

             int NN = NeighborIDs[1];
              MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                         std::string("Workspace2D"), 3,NN,NN);

              Kernel::V3D CentPos = center+yvec*(Centy-ROW)*CellHeight + xvec*(Centx-COL)*CellWidth;
              SetUpData1(Data, inpWkSpace,  Chan+dir*t,  R ,CenterDet->getPos());
              if (AttributeValues[ISSIxx] > 0)
              {
                if (AttributeValues[IIntensities] > MaxCounts)
                {
                  MaxCounts = AttributeValues[IIntensities];
                  MaxChan = Chan + dir * t;
                }
                if (AttributeValues[IIntensities] > 0)
                {
                  Centx = AttributeValues[ISSIx] / AttributeValues[IIntensities];
                  Centy = AttributeValues[ISSIy] / AttributeValues[IIntensities];
                }
                else
                  done = true;
              }
              else
                done = true;
              
           }


        Chan = max<int>( Chan , MaxChan );


        sprintf(logInfo, std::string("   largest Channel,Radius,CellWidth,CellHeight = %d  %7.3f  %7.3f  %7.3f\n").c_str(),
                           Chan, R, CellWidth, CellHeight);
        g_log.debug(std::string(logInfo));

        if (R < 2*max<double>(CellWidth, CellHeight) || dChan < 3)
        {
          g_log.error("Not enough rows and cols or time channels ");
          throw std::runtime_error("Not enough rows and cols or time channels ");
        }

        InitializeColumnNamesInTableWorkspace(TabWS);


        IAlgorithm_sptr fit_alg;

        double time;
        int ncells;

        Mantid::API::Progress prog(this, 0.0, 100.0, (int) dChan);

        // Set from attributes replace by R0
        R0= -1;

        for (int dir = 1; dir >= -1; dir -= 2)
        {
          bool done = false;

          for (int chan = 0; chan < dChan  && !done; chan++)
            if (dir < 0 && chan == 0)
            {
              lastRow = Row0;
              lastCol = Col0;
            }
            else if( Chan+dir*chan <0 || Chan+dir*chan >= (int)X.size())
               done = true;
            else
            {


              int nchan = chan;
              int xchan = Chan + dir * chan;
              if (nchan <= 1)
                nchan = 1;

              size_t topIndex = xchan + 1;
              if (topIndex >= X.size())
                topIndex = X.size() - 1;

              time = (X[xchan] + X[topIndex]) / 2.0;

              double Radius = 2*R;

              if( R0 >0)
              {
                 Radius = R0;

              }
              int NN= NeighborIDs[1];
              MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                  std::string("Workspace2D"), 3, NN,NN);//neighbors.size(), neighbors.size());

              sprintf(logInfo, string(
                              " A:chan= %d  time=%7.2f  Radius=%7.3f  row= %5.2f  col=%5.2f \n").c_str(),
                                 xchan, time, Radius, lastRow, lastCol);
              g_log.debug(std::string(logInfo));


              SetUpData(Data, inpWkSpace, panel, xchan, lastCol,lastRow,Cent,//CentDetspec,//NeighborIDs,
                                                      neighborRadius, Radius);

              ncells = (int) (AttributeValues[ISS1]);

              std::vector<double> params;
              std::vector<double> errs;
              std::vector<std::string> names;
            /*std::cout<<"Attributes="<<std::endl;
              for( int kk=0; kk<NAttributes;kk++)
              {
                std::cout<<"("<<kk<<")="<<AttributeValues[kk];
              }

              std::cout<<std::endl;
              */
               if (IsEnoughData() && ParameterValues[ITINTENS] > 0)
              {

                fit_alg = createSubAlgorithm("Fit");

                fit_alg->setProperty("InputWorkspace", Data);
                fit_alg->setProperty("WorkspaceIndex", 0);
                fit_alg->setProperty("StartX", 0.0);
                fit_alg->setProperty("EndX", 0.0 + (double)NeighborIDs[1]);
                fit_alg->setProperty("MaxIterations", 5000);

                std::string fun_str = CalculateFunctionProperty_Fit();

                std::string SSS("   Fit string ");
                SSS += fun_str;
                g_log.debug(SSS);

                fit_alg->setPropertyValue("Function", fun_str);

                std::string tie = getProperty("Ties");
                if( tie.length() > (size_t)0)
                  fit_alg->setProperty("Ties", tie);

                try
                {
                  fit_alg->executeAsSubAlg();

                  double chisq = fit_alg->getProperty("OutputChi2overDoF");

                  params = fit_alg->getProperty("Parameters");
                  errs = fit_alg->getProperty("Errors");
                  names = fit_alg->getProperty("ParameterNames");

                  ostringstream res;
                  res << "   Thru Algorithm: chiSq=" << setw(7) << chisq << endl;
                  res<<"  Row,Col Radius="<<lastRow<<","<<lastCol<<","<<neighborRadius<<std::endl;

                  double sqrtChisq = -1;
                  if (chisq >= 0)
                    sqrtChisq = sqrt(chisq);

                  for (size_t kk = 0; kk < params.size(); kk++)
                  {
                    res << "   Parameter " << setw(8) << names[kk] << " " << setw(8) << params[kk];
                    if (names[kk].substr(0, 2) != string("SS"))
                      res << "(" << setw(8) << (errs[kk] * sqrtChisq) << ")";

                    res << endl;
                  }
                  g_log.debug(res.str());

                  if (isGoodFit(params, errs, names, chisq))
                  {
                    UpdateOutputWS(TabWS, dir, xchan, params, errs, names, chisq, time);

                    double TotSliceIntensity = AttributeValues[IIntensities];
                    double TotSliceVariance = AttributeValues[IVariance];

                    updatePeakInformation(    params,             errs,           names,
                                              TotVariance,       TotIntensity,
                                              TotSliceIntensity, TotSliceVariance, chisq,
                                              ncells);

                  }
                  else

                    done = true;

                }catch( std::exception &Ex1)//ties or something else went wrong in BivariateNormal
                {
                     done = true;
                     g_log.debug("Bivariate Error :"+std::string(Ex1.what()));
                }

              }
              else //(!IsEnoughData() || ParameterValues[ITINTENS] <= 0
              {
                done = true;

              }

              //Get ready for the next round
              Data.reset();
              if (fit_alg)
                fit_alg.reset();

              if (!done)
              {

                //Now set up the center for this peak
                int i = find("Mrow", names);
                lastRow = (int) params[i];
                i = find("Mcol", names);
                lastCol = (int) params[i];
                prog.report();


              }
              else
                if (dir > 0)
                  prog.report(dChan / 2);
                else
                  prog.report(dChan);

              params.clear();
              errs.clear();
              names.clear();


            }
        }

      } catch (std::exception &EE1)
      {
        std::cout << "Error in main reason=" << EE1.what() << std::endl;

        throw std::runtime_error(" Error IntegratePeakTimeSlices:"+std::string(EE1.what()));
      }catch( std::string &mess)
      {
        throw std::runtime_error("Error IntegratePeakTimeSlices:"+mess);

      }catch(...)
      {
        throw std::runtime_error("Error IntegratePeakTimeSlices:");
      }

      try
      {
        //std::cout<<std::endl<<"Setting Peak and Intensity ,sigma="<<TotIntensity<<","<<
        //       sqrt(TotVariance)<<std::endl;
       // peak.setIntensity(TotIntensity);
       // peak.setSigmaIntensity(sqrt(TotVariance));

        setProperty("OutputWorkspace", TabWS);

        setProperty("Intensity", TotIntensity);
        setProperty("SigmaIntensity", sqrt(TotVariance));

      } catch (std::exception &ss)
      {

        std::cout << "Error occurred XX " << ss.what() << std::endl;
        throw std::runtime_error( ss.what());
      }

    }


    //Finds an item in a MantidVec of time bin boundaries
    int IntegratePeakTimeSlices::find(Mantid::MantidVec const & X, const double time)
    {
      int sgn = 1;

      if (X[0] > X[1])
        sgn = -1;
      
      if (sgn * (X[0] - time) >= 0)
        return 0;

      if (sgn * (time - X[X.size() - 1]) >= 0)
        return (int) X.size() - 1;

      for (size_t i = 0; i < (size_t)X.size() - (size_t) 1; i++)
      {
        if (sgn * (time - X[i]) >= 0 && sgn * (X[i + (size_t) 1] - time) >= 0)
          return (int) i;
      }

      return -1;
    }


    int IntegratePeakTimeSlices::CalculateTimeChannelSpan( DataObjects::Peak     const & peak,
                                                          const double                  dQ,
                                                          Mantid::MantidVec      const& X,
                                                          const int                     specNum,
                                                          int & Centerchan)
    {
      UNUSED_ARG( specNum);
      double Q = peak.getQLabFrame().norm();//getQ( peak)/2/M_PI;

      V3D pos = peak.getDetPos();
      double time = peak.getTOF();
      double dtime = dQ / Q * time;
      int chanCenter = find(X, time);
       
      Centerchan = chanCenter;
      int chanLeft = find(X, time - dtime);
      int chanRight = find(X, time + dtime);
      int dchan = abs(chanCenter - chanLeft);

      if (abs(chanRight - chanCenter) > dchan)
        dchan = abs(chanRight - chanCenter);


      dchan = max<int> (3,  dchan );

      return  dchan + 5;//heuristic should be a lot more
    }




    void IntegratePeakTimeSlices::InitializeColumnNamesInTableWorkspace(
                                                    TableWorkspace_sptr &TabWS)
    {
      TabWS->setName("Log Table");
      TabWS->addColumn("double", "Time");
      TabWS->addColumn("double", "Channel");
      TabWS->addColumn("double", "Background");
      TabWS->addColumn("double", "Intensity");
      TabWS->addColumn("double", "Mcol");
      TabWS->addColumn("double", "Mrow");
      TabWS->addColumn("double", "SScol");
      TabWS->addColumn("double", "SSrow");
      TabWS->addColumn("double", "SSrc");
      TabWS->addColumn("double", "NCells");
      TabWS->addColumn("double", "ChiSqrOverDOF");
      TabWS->addColumn("double", "TotIntensity");
      TabWS->addColumn("double", "BackgroundError");
      TabWS->addColumn("double", "FitIntensityError");
      TabWS->addColumn("double", "ISAWIntensity");
      TabWS->addColumn("double", "ISAWIntensityError");
      TabWS->addColumn("double", "TotalBoundary");
      TabWS->addColumn("double", "NBoundaryCells");
      TabWS->addColumn("double", "Start Row");
      TabWS->addColumn("double", "End Row");
      TabWS->addColumn("double", "Start Col");
      TabWS->addColumn("double", "End Col");
    }



 /* //For Edge Peaks
    void UpdateStats( const double          intensity,
                                               const double          variance,
                                               const int             Row,
                                               const int             Col,
                                               std::vector<double> & StatBase)
    {
      for( int i=StatBase.size(); i< 8*NAttributes-7*4+16//for octant max's
                                            ; i++)
        StatBase.push_back(0);
      int Oct=0;
      if( row <(int)(.5+ROW)) Oct=4;
      if( col < (int)(.5+COL)) Oct+=2;
      if( abs( row -(int)(.5+ROW)) <abs(col-COL))Oct++;

      row = row-ROW;
      col = col-COL;
      //  use row = row -ROW and col =col-COL ???????
      // To determine which octants are vacant keep  max(min) row/col fo each octant.
      StatBase[8*ISSIxx-7*ISSIxx+Oct] += col * col * intensity;
      StatBase[8*ISSIyy-7*ISSIxx+Oct] += intensity * row * row;
      StatBase[8*ISSIxy-7*ISSIxx+Oct] += intensity * row * col;
      StatBase[8*ISSxx-7*ISSIxx+Oct] += col * col;
      StatBase[8*ISSyy-7*ISSIxx+Oct] += row * row;
      StatBase[8*ISSxy-7*ISSIxx+Oct] += row * col;
      StatBase[8*ISSIx-7*ISSIxx+Oct] += intensity * col;
      StatBase[8*ISSIy-7*ISSIxx+Oct] += intensity * row;
      StatBase[8*ISSx-7*ISSIxx+Oct] += col;
      StatBase[8*ISSy-7*ISSIxx+Oct] += row;
      StatBase[8*IIntensities-7*ISSIxx+Oct] += intensity;
      StatBase[8*IVariance-7*ISSIxx+Oct] += variance;
      int StartOcts =8*IVariance-7*ISSIxx+8;
      if( oct >= 4 && row < StatBase[ StartOcts+2*Oct])
        StatBase[StartOcts+2*Oct]=row;
      else if( Oct<4 && row > StatBase[ StartOcts+2*Oct])
        StatBase[StartOcts+2*Oct]=row;
      if( (Oct/2) %2 ==1 && col < StatBase[StartOcts+2*Oct + 1])
           col =StatBase[StartOcts+2*Oct + 1];
      else if((Oct/2) %2 ==0 && col >StatBase[StartOcts+2*Oct + 1])
          col =StatBase[StartOcts+2*Oct + 1];
    }

    //Reflect then sum octants
   std::vector<double>  CollapseAndReflect( std::vector<double> StatBase, std::vector<int> Attr)
    {

     int StartOcts =8*IVariance-7*ISSIxx+8;
     int StartRow = Attr[IStarRow];
     int EndRow   = StartRow + Attr[INRows]+1;
     int StartCol = Attr[IStarCol];
     int EndCol  = StartCol + Attr[INCol]+1;

     //return StatBase;
    }
*/
    void IntegratePeakTimeSlices::updateStats( const double          intensity,
                                               const double          variance,
                                               const double          row,
                                               const double          col,
                                               std::vector<double> & StatBase)
    {

      StatBase[ISSIxx] += col * col * intensity;
      StatBase[ISSIyy] += intensity * row * row;
      StatBase[ISSIxy] += intensity * row * col;
      StatBase[ISSxx] += col * col;
      StatBase[ISSyy] += row * row;
      StatBase[ISSxy] += row * col;
      StatBase[ISSIx] += intensity * col;
      StatBase[ISSIy] += intensity * row;
      StatBase[ISSx] += col;
      StatBase[ISSy] += row;
      StatBase[IIntensities] += intensity;
      StatBase[IVariance] += variance;
      StatBase[ISS1] +=1;

    }



    void IntegratePeakTimeSlices::getInitParamValues(std::vector<double> const &StatBase,
                                                     const double               TotBoundaryIntensities,
                                                     const int                  nBoundaryCells)
    {
      double b = 0;
      if (nBoundaryCells > 0)
        b = TotBoundaryIntensities / nBoundaryCells;

      int nCells = (int) (StatBase[ISS1]);
      double Den = StatBase[IIntensities] - b * nCells;
      int k=0;
      while (Den <= 0 && b!=0)
      {
        b = b*.7;
        Den = StatBase[IIntensities] - b * nCells;

        if( k <8)
          k++;
        else
          b=0;

      }

      if( Den <=0)
        Den = 1;

      bool done = false;
      int ntimes = 0;
      double Mx, My, Sxx, Syy, Sxy;

      //Variances at background =0. Should be used to nail down initial parameters.
      double Sxx0 = (StatBase[ISSIxx]-StatBase[ISSIx]*StatBase[ISSIx]/ StatBase[IIntensities])
                                     / StatBase[IIntensities];

      double Syy0 = (StatBase[ISSIyy]-StatBase[ISSIy]*StatBase[ISSIy]/ StatBase[IIntensities])
                                 / StatBase[IIntensities];
      while (!done && ntimes<8)
      {
        Mx = StatBase[ISSIx] - b * StatBase[ISSx];
        My = StatBase[ISSIy] - b * StatBase[ISSy];
        Sxx = (StatBase[ISSIxx] - b * StatBase[ISSxx] - Mx * Mx / Den) / Den;
        Syy = (StatBase[ISSIyy] - b * StatBase[ISSyy] - My * My / Den) / Den;
        Sxy = (StatBase[ISSIxy] - b * StatBase[ISSxy] - Mx * My / Den) / Den;
        ntimes++;
        done = false;

        if (Sxx <= 0 || Syy <= 0 || Sxy * Sxy / Sxx / Syy > .8)
        {
          b = b*.6;

          if (ntimes +1 ==8)
            b=0;

          Den = StatBase[IIntensities] - b * nCells;
          if (Den <= 1)
            Den = 1;

        }else
          done = true;

      }

      ParameterValues[IBACK] = b;
      ParameterValues[ITINTENS] = StatBase[IIntensities] - b * nCells;
      ParameterValues[IXMEAN] = Mx / Den;
      ParameterValues[IYMEAN] = My / Den;
      ParameterValues[IVXX] = Sxx;
      ParameterValues[IVYY] = Syy;
      ParameterValues[IVXY] = Sxy;

    }



    //Data is the slice subdate, inpWkSpace is ALl the data,
    void IntegratePeakTimeSlices::SetUpData( MatrixWorkspace_sptr          & Data,
                                             MatrixWorkspace_sptr    const & inpWkSpace,
                                             boost::shared_ptr< Geometry::IComponent> comp,
                                             const int                       chan,
                                             double                          CentX,
                                             double                          CentY,
                                             Kernel::V3D                     &CentNghbr,
                                             //specid_t                      &CentDetspec,
                                             //int*                            nghbrs,//Not used
                                            // std::map< specid_t, V3D >     &neighbors,
                                             double                        &neighborRadius,//from CentDetspec
                                             double                         Radius)
    {
     // size_t wsIndx = inpWkSpace->getIndexFromSpectrumNumber(CentDetspec);

     // Geometry::IDetector_const_sptr CentDett= inpWkSpace->getDetector( wsIndx);

      Kernel::V3D CentPos1 = center + xvec*(CentX-COL)*CellWidth
                                   + yvec*(CentY-ROW)*CellHeight;

      SetUpData1(Data             , inpWkSpace, chan,
                 Radius            , CentPos1);

      if( AttributeValues[ISSIxx]< 0)// Not enough data
          return;

      double  DD = max<double>( sqrt(ParameterValues[IVYY])*CellHeight, sqrt(ParameterValues[IVXX])*CellWidth);
      double NewRadius= 1.1* max<double>( 5*max<double>(CellWidth,CellHeight), 4*DD);
      //1.4 is needed to get more background cells. In rectangle the corners were background

      NewRadius = min<double>(30*max<double>(CellWidth,CellHeight),NewRadius);

      if( R0 > 0)
      {
         NewRadius = R0;
      } else
      {
        R0 = NewRadius;
      }

      CentX= ParameterValues[IXMEAN];
      CentY=ParameterValues[IYMEAN];
      Kernel::V3D CentPos = center + xvec*(CentX-COL)*CellWidth + yvec*(CentY-ROW)*CellHeight;


      DD =(CentPos - CentPos1).norm();
     // Kernel::V3D CentDetPos = CentDett->getPos();

      if(DD +NewRadius >neighborRadius)
      {
         int NN= int(2*NewRadius*2*NewRadius*2.25/CellWidth/CellHeight);
         if( NeighborIDs[0]< NN)
         {
           delete [] NeighborIDs;
           NeighborIDs = new int[NN+2];
           NeighborIDs[0]=NN+2;
         }else
           NN= NeighborIDs[0]-2;
         NeighborIDs[1]=2;
         neighborRadius = 1.5*NewRadius;
         CentNghbr = CentPos;
         getNeighborPixIDs(comp, CentPos, neighborRadius, NeighborIDs);


      }else// big enough neighborhood so
        neighborRadius -= DD;

      SetUpData1(Data, inpWkSpace, chan,
                 NewRadius, CentPos );

    }



    void  IntegratePeakTimeSlices:: SetUpData1(API::MatrixWorkspace_sptr              &Data,
                                               API::MatrixWorkspace_sptr        const &inpWkSpace,
                                               const int                               chan,

                                               double                      Radius,
                                               Kernel::V3D                 CentPos
                                               )
    {
      UNUSED_ARG(g_log);
      //std::cout<<"Start SetUpData1="<<Radius<<","<<CentPos<<","<<NeighborIDs[1]<< std::endl;
      if( NeighborIDs[1] < 10)
      {
        AttributeValues[ISSIxx]= -1;
        return;
      }
      boost::shared_ptr<Workspace2D> ws = boost::shared_dynamic_cast<Workspace2D>(Data);

      std::vector<double> StatBase;


      for (int i = 0; i < NAttributes + 2; i++)
        StatBase.push_back(0);

      Mantid::MantidVecPtr pX;

      Mantid::MantidVec& xRef = pX.access();
      for (int j = 0; j < NeighborIDs[1]; j++)
      {
        xRef.push_back((double)j);
      }

      Data->setX(0, pX);
      Data->setX(1, pX);
      Data->setX(2, pX);

      Mantid::MantidVecPtr yvals;
      Mantid::MantidVecPtr errs;
      Mantid::MantidVecPtr xvals;
      Mantid::MantidVecPtr Yvals;

      Mantid::MantidVec &yvalB = yvals.access();
      Mantid::MantidVec &errB = errs.access();
      Mantid::MantidVec &xvalB = xvals.access();
      Mantid::MantidVec &YvalB = Yvals.access();

      double TotBoundaryIntensities = 0;
      int nBoundaryCells = 0;
     // std::map<specid_t, V3D >::iterator it;
     // std::map<detid_t, size_t>::iterator det2wsInxIterator;

      int N=0;
      double BoundaryRadius=min<double>(.90*Radius, Radius-1.5*max<double>(CellWidth,CellHeight));
      double minRow= 20000,
             maxRow =-1,
             minCol =20000,
             maxCol =-1;
      //for( it = neighbors.begin(); it != neighbors.end(); it++)
     //   if((CentDetPos+ (*it).second -CentPos).norm()>Radius)
      //  {

     //   }else
      for (int i = 2; i < NeighborIDs[1]; i++)
      {
        int DetID = NeighborIDs[i];

        size_t workspaceIndex ;
        if( wi_to_detid_map->count(DetID)>0)
           workspaceIndex= wi_to_detid_map->find(DetID)->second;//inpWkSpace->getIndexFromSpectrumNumber(spec );
        else
        {
         g_log.error("No workspaceIndex for detID="+DetID);
         throw  std::runtime_error("No workspaceIndex for detID="+DetID);
        }


        IDetector_const_sptr Det = inpWkSpace->getDetector(workspaceIndex);

        V3D pixPos = Det->getPos();

        if ((pixPos - CentPos).norm() < Radius)

        {

          double row = ROW + (pixPos - center).scalar_prod(yvec) / CellHeight;

          double col = COL + (pixPos - center).scalar_prod(xvec) / CellWidth;

          Mantid::MantidVec histogram = inpWkSpace->readY(workspaceIndex);

          Mantid::MantidVec histoerrs = inpWkSpace->readE(workspaceIndex);

          double intensity = histogram[chan];
          double variance = histoerrs[chan] * histoerrs[chan];

          N++;
          yvalB.push_back(intensity);
          double sigma = 1;


          errB.push_back(sigma);
          xvalB.push_back((double) col);
          YvalB.push_back((double) row);

          //std::cout<<"("<<row<<","<<col<<","<<intensity<<")";


          updateStats(intensity, variance, row, col, StatBase);


          if ((pixPos - CentPos).norm() > BoundaryRadius)
          {
            TotBoundaryIntensities += intensity;
            nBoundaryCells++;
           //  std::cout<<"*";
          }

          if (row < minRow)
            minRow = row;
          if (col < minCol)
            minCol = col;
          if (row > maxRow)
            maxRow = row;
          if (col > maxCol)
            maxCol = col;


        }
        }

     // std::cout<<std::endl<<"N elts="<<N<< ","<<TotBoundaryIntensities<<","<<nBoundaryCells<<std::endl;
      ws->setData(0, yvals, errs);
      ws->setData(1, xvals);
      ws->setData(2, Yvals);


      StatBase[IStartRow] = minRow;
      StatBase[IStartCol] =minCol;
      StatBase[INRows] = maxRow-minRow+1;
      StatBase[INCol] = maxCol-minCol+1;

      getInitParamValues(StatBase, TotBoundaryIntensities, nBoundaryCells);

      for (int i = 0; i < NAttributes; i++)
        AttributeValues[i] = StatBase[i];

      AttributeValues[ITotBoundary ] = TotBoundaryIntensities;
      AttributeValues[INBoundary] = nBoundaryCells;

    }

    std::string IntegratePeakTimeSlices::CalculateFunctionProperty_Fit()
    {

      std::ostringstream fun_str;

      fun_str << "name=BivariateNormal,";

      for (int i = 0; i < NParameters; i++)
      {
        fun_str << ParameterNames[i] << "=" << ParameterValues[i]<<",";

      }
      if( getProperty( "CalculateVariances") && !EdgePeak)
          fun_str<<"CalcVariances=1";
      else
          fun_str<<"CalcVariances = -1";

      return fun_str.str();
    }



    int IntegratePeakTimeSlices::find( std::string              const &oneName,
                                       std::vector<std::string> const &nameList)

    {
      for (size_t i = 0; i < nameList.size(); i++)
        if (oneName.compare(nameList[i]) == (int) 0)
          return (int) i;

      return -1;
    }


    bool IntegratePeakTimeSlices::isGoodFit(std::vector<double >              const & params,
                                            std::vector<double >              const & errs,
                                            std::vector<std::string >         const &names,
                                            double chisq)
    {
      int Ibk = find("Background", names);
      int IIntensity = find("Intensity", names);

      char logInfo[200];

      if (chisq < 0)
      {

        sprintf(logInfo, std::string("   Bad Slice- negative chiSq= %7.2f\n").c_str(), chisq);
        g_log.debug(std::string(logInfo));
        return false;
      }

      int ncells = (int) (AttributeValues[ISS1]);

      if (AttributeValues[IIntensities] <= 0 || (AttributeValues[IIntensities] - params[Ibk] * ncells)
          <= 0)
      {

        sprintf(logInfo, std::string("   Bad Slice. Negative Counts= %7.2f\n").c_str(),
            AttributeValues[IIntensities] - params[Ibk] * ncells);
        g_log.debug(std::string(logInfo));
        return false;
      }

      double x = params[IIntensity] / (AttributeValues[IIntensities] - params[Ibk] * ncells);

      if ((x < .8 || x > 1.25)&& !EdgePeak)// The fitted intensity should be close to tot intensity - background
      {

        sprintf(
            logInfo,
            std::string(
                "   Bad Slice. Fitted Intensity & Observed Intensity(-back) too different. ratio= %7.2f\n").c_str(),
            x);
        g_log.debug(std::string(logInfo));
        return false;
      }
      bool GoodNums = true;

      for (size_t i = 0; i < errs.size(); i++) //NaN's
        if (errs[i] != errs[i])
          GoodNums = false;
        else if (params[i] != params[i])
          GoodNums = false;

      if (!GoodNums)
        g_log.debug("   Bad Slice.Some params and errs are not numbers");

      GoodNums = true;

      if (params[Ibk] < 0)
        GoodNums = false;

      if (params[IIntensity] < 0)
        GoodNums = false;

      double sqrChi = sqrt(chisq);

      if (AttributeValues[IIntensities] > 0)
        if (sqrChi * errs[IIntensity] / AttributeValues[IIntensities] > .2)
          GoodNums = false;

      if (!GoodNums)
        g_log.debug(
            "   Bad Slice.Some params and errs are out of bounds or relative errors are too high");

      if (params[IIntensity] > 0)
        if (errs[IIntensity] * sqrt(chisq) / params[IIntensity] > .1)
        {

          sprintf(logInfo, std::string("   Bad Slice. rel errors too large= %7.2f\n").c_str(),
              errs[ITINTENS] * sqrt(chisq));
          g_log.debug(std::string(logInfo));
          return false;
        }

      //Check weak peak. Max theoretical height should be more than 3

      double maxPeakHeightTheoretical = params[ITINTENS] / 2 / M_PI / sqrt(params[IVXX] * params[IVYY]
          - params[IVXY] * params[IVXY]);

      if (maxPeakHeightTheoretical < 1.5)
      {

        sprintf(logInfo, std::string("   Bad Slice. Peak too small= %7.2f\n").c_str(),
            maxPeakHeightTheoretical);
        g_log.debug(std::string(logInfo));
        return false;
      }

      return true;
    }



    //Calculate error used by ISAW. It "doubles" the background error.
    double IntegratePeakTimeSlices::CalculateIsawIntegrateError(const double background,
                                                                const double backError,
                                                                const double ChiSqOverDOF,
                                                                const double TotVariance,
                                                                const int ncells)
    {
     
      double B = TotVariance / ncells;
      if( B < ChiSqOverDOF)
         B = ChiSqOverDOF;

      double Variance = TotVariance + (backError * backError * B) * ncells * ncells
          + background * ncells;
       
      return sqrt(Variance);

    }


    void IntegratePeakTimeSlices::UpdateOutputWS( DataObjects::TableWorkspace_sptr         &TabWS,
                                                  const int                                  dir,
                                                  const int                                  chan,
                                                  std::vector<double >                 const &params,
                                                  std::vector<double >                 const &errs,
                                                  std::vector<std::string>             const &names,
                                                  const double                               Chisq,
                                                  const double time)
    {
      int Ibk = find("Background", names);
      int IIntensity = find("Intensity", names);
      int IVx = find("SScol", names);
      int IVy = find("SSrow", names);
      int IVxy = find("SSrc", names);
      int Irow = find("Mrow", names);
      int Icol = find("Mcol", names);

      int newRowIndex = 0;

      if (dir > 0)
        newRowIndex = static_cast<int>(TabWS->rowCount());

      int TableRow = static_cast<int>(TabWS->insertRow(newRowIndex));

      int ncells = (int) (AttributeValues[ISS1]);
      double chisq= max<double>(Chisq, AttributeValues[IIntensities]/max<int>(ncells,1));
      TabWS->getRef<double> (std::string("Background"), TableRow) = params[Ibk];
      TabWS->getRef<double> (std::string("Channel"), TableRow) = chan;

      TabWS->getRef<double> (std::string("Intensity"), TableRow) = params[IIntensity];
      TabWS->getRef<double> (std::string("FitIntensityError"), TableRow) = errs[IIntensity]
          * sqrt(chisq);
      TabWS->getRef<double> (std::string("Mcol"), TableRow) = params[Icol];
      TabWS->getRef<double> (std::string("Mrow"), TableRow) = params[Irow];

      TabWS->getRef<double> (std::string("SScol"), TableRow) = params[IVx];
      TabWS->getRef<double> (std::string("SSrow"), TableRow) = params[IVy];

      TabWS->getRef<double> (std::string("SSrc"), TableRow) = params[IVxy];
      TabWS->getRef<double> (std::string("NCells"), TableRow) = ncells;
      TabWS->getRef<double> (std::string("ChiSqrOverDOF"), TableRow) = chisq;

      TabWS->getRef<double> (std::string("TotIntensity"), TableRow) = AttributeValues[IIntensities];
      TabWS->getRef<double> (std::string("BackgroundError"), TableRow) = errs[Ibk] * sqrt(chisq);
      TabWS->getRef<double> (std::string("ISAWIntensity"), TableRow) = AttributeValues[IIntensities]
                                                                             - params[Ibk] * ncells;

      TabWS->getRef<double> (std::string("ISAWIntensityError"), TableRow) = CalculateIsawIntegrateError(
          params[Ibk], errs[Ibk], chisq, AttributeValues[IVariance], ncells);
      
      TabWS->getRef<double> (std::string("Time"), TableRow) = time;

      TabWS->getRef<double>(std::string("TotalBoundary"), TableRow)= AttributeValues[ ITotBoundary];
      TabWS->getRef<double>(std::string("NBoundaryCells"), TableRow)= AttributeValues[ INBoundary];


      TabWS->getRef<double> (std::string("Start Row"), TableRow) = AttributeValues[IStartRow];
      TabWS->getRef<double> (std::string("End Row"), TableRow) = AttributeValues[IStartRow]
          + AttributeValues[INRows] - 1;

      TabWS->getRef<double> (std::string("Start Col"), TableRow) = AttributeValues[IStartCol];
      TabWS->getRef<double> (std::string("End Col"), TableRow) = AttributeValues[IStartCol]
          + AttributeValues[INCol] - 1;

    }

    
    //TODO: If EdgePeak use fit intensity and error(*2?)
    void IntegratePeakTimeSlices::updatePeakInformation( std::vector<double >     const &params,
                                                         std::vector<double >     const &errs,
                                                         std::vector<std::string >const &names,
                                                         double                        &TotVariance,
                                                         double                        &TotIntensity,
                                                         double const                   TotSliceIntensity,
                                                         double const                   TotSliceVariance,
                                                         double const                   chisqdivDOF,
                                                         const int ncells)
    {
      int Ibk = find("Background", names);
      double err =0;
      if( !EdgePeak  )
      {
        err = CalculateIsawIntegrateError(params[Ibk], errs[Ibk], chisqdivDOF, TotSliceVariance, ncells);

        TotIntensity += TotSliceIntensity - params[IBACK] * ncells;

        TotVariance += err * err;

      }else
      {
       int IerrInt = find("Intensity", names);
       TotIntensity += params[IerrInt];
       err = errs[IerrInt];
       TotVariance += err * err;

      }

    }


    bool IntegratePeakTimeSlices::IsEnoughData()
    {

      char logInfo[200];

      double VIx0_num = AttributeValues[ISSIxx] - 2 * ParameterValues[IXMEAN] * AttributeValues[ISSIx]
          + ParameterValues[IXMEAN] * ParameterValues[IXMEAN] * AttributeValues[IIntensities];

      double VIy0_num = AttributeValues[ISSIyy] - 2 * ParameterValues[IYMEAN] * AttributeValues[ISSIy]
          + ParameterValues[IYMEAN] * ParameterValues[IYMEAN] * AttributeValues[IIntensities];

      double VIxy0_num = AttributeValues[ISSIxy] - ParameterValues[IXMEAN] * AttributeValues[ISSIy]
          - ParameterValues[IYMEAN] * AttributeValues[ISSIx] + ParameterValues[IYMEAN]
          * ParameterValues[IXMEAN] * AttributeValues[IIntensities];

      double Vx0_num = AttributeValues[ISSxx] - 2 * ParameterValues[IXMEAN] * AttributeValues[ISSx]
          + ParameterValues[IXMEAN] * ParameterValues[IXMEAN] * AttributeValues[INRows]
              * AttributeValues[INCol];

      double Vy0_num = AttributeValues[ISSyy] - 2 * ParameterValues[IYMEAN] * AttributeValues[ISSy]
          + ParameterValues[IYMEAN] * ParameterValues[IYMEAN] * AttributeValues[INRows]
              * AttributeValues[INCol];

      double Vxy0_num = AttributeValues[ISSIxy] - ParameterValues[IXMEAN] * AttributeValues[ISSIy]
          - ParameterValues[IYMEAN] * AttributeValues[ISSIx] + ParameterValues[IYMEAN]
          * ParameterValues[IXMEAN] * AttributeValues[INRows] * AttributeValues[INCol];

      double Denominator = AttributeValues[IIntensities] - ParameterValues[IBACK]
          * AttributeValues[INRows] * AttributeValues[INCol];

      double Vx = (VIx0_num - ParameterValues[IBACK] * Vx0_num) / Denominator;
      double Vy = (VIy0_num - ParameterValues[IBACK] * Vy0_num) / Denominator;
      double Vxy = (VIxy0_num - ParameterValues[IBACK] * Vxy0_num) / Denominator;

      double Z = 4 * M_PI * M_PI * (Vx * Vy - Vxy * Vxy);

      if (fabs(Z) < .10) //Not high enough of a peak
      {

        sprintf(logInfo, std::string("   Not Enough Data, Peak Height(%7.2f) is too low\n").c_str(), Z);
        g_log.debug(std::string(logInfo));
        return false;
      }

      // if (Vx <= 0 || Vy <= 0 || Vxy * Vxy / Vx / Vy > .8)// All points close to one line
      //    return false;

      return true;

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

      bool IntegratePeakTimeSlices:: getNeighborPixIDs( boost::shared_ptr< Geometry::IComponent> comp,
                               Kernel::V3D &Center,
                               double &Radius,
                              int* &ArryofID)
       {
            std::cout<< comp->type()<<std::endl;
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
             {       std::cout<<"Detector case"<<std::endl;
                     boost::shared_ptr<Geometry::Detector> det =   boost::dynamic_pointer_cast<Geometry::Detector>( comp);
                     ArryofID[N]=  det->getID();
                     N++;
                     ArryofID[1] = N;
                    return true;
              }

             boost::shared_ptr<const Geometry::CompAssembly> Assembly =
                 boost::dynamic_pointer_cast<const Geometry::CompAssembly>( comp);

             if( !Assembly)
                return true;

             bool b = true;
             std::cout<<"#kids="<<Assembly->nelements()<<std::endl;
             for( int i=0; i< Assembly->nelements() && b; i++)
                b= getNeighborPixIDs( Assembly->getChild(i),Center,Radius,ArryofID);

             return b;
       }
   */
    void IntegratePeakTimeSlices::FindPlane( V3D & center, V3D & xvec, V3D& yvec,
                                             double &ROW, double &COL,
                                             double &pixWidthx, double&pixHeighty,
                                             DataObjects::Peak const &peak) const
    {

      IDetector_const_sptr det = peak.getDetector();
      V3D detPos = det->getPos();

      center.setX( detPos.X());
      center.setY( detPos.Y());
      center.setZ( detPos.Z());

      boost::shared_ptr<const Mantid::Geometry::IComponent> parent = det->getParent();
      /*  //Will not use shape() then surface normals. Only works for rectangular detectors.
    // RectangleDetectors also work but no other Composite classes

      std::cout<<"    C"<< parent->type()<<std::endl;
      boost::shared_ptr<const Mantid::Geometry::ObjComponent> ObjCompParent =
          boost::dynamic_pointer_cast<const Mantid::Geometry::ObjComponent >
            ( det);//parent);
      std::cout<<"    D"<<std::endl;
      while ( !ObjCompParent && parent)
      {
        std::cout<<"    D "<< parent->type()<<std::endl;
        parent = parent->getParent();
        ObjCompParent =
                  boost::dynamic_pointer_cast<const Mantid::Geometry::ObjComponent >
                    ( parent);
      }
      if( !ObjCompParent)
      {
        std::cout<<"detector parent is NOT an ObjComponent"<<std::endl;
        return;
      }
      Object_const_sptr shape = ObjCompParent->shape();
      const std::vector<const Surface*> surfaces=shape->getSurfacePtr  ( ) ;
      V3D surfaceNormal(0,0,0);
      V3D badSurfaceNormal(0,0,0);
      double minDistance = 1000000.00;
      std::cout<<"    nsurfaces for "<< det->getName()<<"="<<surfaces.size()<<std::endl;
      for(size_t i =0; i< surfaces.size() ; i++)
      {
        std::cout<<"    dist of det from surface "<<i<<" is "<<surfaces[i]->distance( det->getPos())<<std::endl;
        std::cout<<"       surf normal="<<surfaces[i]->surfaceNormal( det->getPos())<<std::endl;
        if( surfaces[i]->distance( det->getPos())< minDistance)
        {
          surfaceNormal = surfaces[i]->surfaceNormal( det->getPos());
          minDistance= surfaces[i]->distance( det->getPos());
        }
      }
      std::cout<<"    surface normal is "<<surfaceNormal<<std::endl;
      if( surfaceNormal == badSurfaceNormal || minDistance >100)
      {
        std::cout<<"Could not find a close surface"<<std::endl;
        return;
      }
     */
      boost::shared_ptr<const Detector> dett = boost::dynamic_pointer_cast<const Detector >(det);

       BoundingBox B;
       dett->getBoundingBox( B );

       pixWidthx  = dett->getWidth();
       pixHeighty = dett->getHeight();

       Kernel::Quat Qt = dett->getRotation();
       V3D yaxis0(0.0,1.0,0.0);

       Qt.rotate(yaxis0);
       yaxis0.normalize();

       V3D xaxis0(1,0,0);
       Qt.rotate( xaxis0);
       xaxis0.normalize();


       xvec.setX( xaxis0.X());
       xvec.setY( xaxis0.Y());
       xvec.setZ( xaxis0.Z());
       yvec.setX( yaxis0.X());
       yvec.setY( yaxis0.Y());
       yvec.setZ( yaxis0.Z());
       ROW=0;
       COL=0;
       boost::shared_ptr<const RectangularDetector>ddet = boost::dynamic_pointer_cast<const RectangularDetector>(parent);
       if( !ddet)
         ddet = boost::dynamic_pointer_cast<const RectangularDetector>(parent->getParent());
       if( ddet )
       {
          std::pair<int,int> CR =ddet->getXYForDetectorID(det->getID());
          ROW = CR.second;
          COL=CR.first;
          pixWidthx = ddet->xstep();
          pixHeighty= ddet->ystep();

       }

  // std::cout<<"Center,ROW,COL="<<center<<","<<ROW<<","<<COL<<","<<CellWidth<<","<<CellHeight<<std::endl;
    }
}//namespace Crystal
}//namespace Mantid
//Attr indicies
