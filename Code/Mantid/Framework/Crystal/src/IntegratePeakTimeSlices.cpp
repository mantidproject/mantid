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
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionFactory.h"
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
//#include "MantidGeometry/Surfaces/Surface.h"
#include <boost/lexical_cast.hpp>
#include <vector>
#include "MantidAPI/Algorithm.h"
#include <algorithm>
#include <math.h>
#include <cstdio>
#include <stdio.h>
#include <time.h>

//#include <boost/random/poisson_distribution.hpp>
#include "MantidAPI/ISpectrum.h"
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace std;
namespace Mantid
{
  namespace Crystal
  {

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
#define IVarBoundary  19
#define NAttributes  20

    //Parameter indicies
#define IBACK   0
#define ITINTENS  1
#define IXMEAN  2
#define IYMEAN  3
#define IVXX  4
#define IVYY  5
#define IVXY  6
//TODO: Edge Peaks-Return NoPeak(Intensity=0,variance=0) if any center on any slice is out of bounds
//TODO: Calc ratio for edge peaks, should use the slant of bivariate normal instead of assuming
//        distribution axes are lined up with the row and col vectors
#define NParameters  7



    namespace{
      //           # std sigs  0    .25     .5     .75    1     1.25   1.5    2      2.5
      const double probs[9]={.5f,.5987f,.6915f,.7734f,.8413f,.8944f,.9322f,.9599f,.9772f};

      const int MinRowColSpan = 6;
      const int MaxRowColSpan = 36;
      const int MinTimeSpan =3;
      const double NeighborhoodRadiusDivPeakRadius=1.5;
      const double MaxNeighborhoodRadius = 10;
      const double NStdDevPeakSpan = 2;
      const double MaxGoodRatioFitvsExpIntenisites=2.5;
      const double MinGoodRatioFitvsExpIntenisites=.25;
      const double MinGoodIoverSigI = 3.0;
      const double MinVariationInXYvalues =.6;//Peak spans one pixel only
      const double MaxCorrCoeffinXY =.9; //otherwise all data on one line
    }

    IntegratePeakTimeSlices::IntegratePeakTimeSlices() :
      Algorithm(), R0(-1)
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
      AttributeNames[19] = "VarianceBoundary";

      ParameterNames[0] = "Background";
      ParameterNames[1] = "Intensity";
      ParameterNames[2] = "Mcol";
      ParameterNames[3] = "Mrow";
      ParameterNames[4] = "SScol";
      ParameterNames[5] = "SSrow";
      ParameterNames[6] = "SSrc";

     // for (int i = 0; i < NAttributes; i++)
     //   AttributeValues[i] = 0;

      for (int i = 0; i < NParameters; i++)
        ParameterValues[i] = 0;


    }

    double SQRT( double  v)
    {
      if( v < 0)
        return -1;
      return sqrt(v );
    }
    /// Destructor
    IntegratePeakTimeSlices::~IntegratePeakTimeSlices()
    {
      delete [] NeighborIDs;
    }


    void IntegratePeakTimeSlices::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace", "", Direction::Input),
          "A 2D workspace with X values of time of flight");

      declareProperty(new WorkspaceProperty<TableWorkspace> ("OutputWorkspace", "", Direction::Output),
          "Name of the output table workspace with Log info");

      declareProperty(new WorkspaceProperty<PeaksWorkspace> ("Peaks", "", Direction::Input),
          "Workspace of Peaks");

      declareProperty("PeakIndex", 0, "Index of peak in PeaksWorkspace to integrate");

      declareProperty("PeakQspan", .06, "Max magnitude of Q of Peak to Q of Peak Center, where mod(Q)=1/d");

      declareProperty("CalculateVariances", true ,"Calc (co)variances given parameter values versus fit (co)Variances ");

      declareProperty("Ties","","Tie parameters(Background,Intensity, Mrow,...) to values/formulas.");

      declareProperty("NBadEdgePixels", 0, "Number of  bad Edge Pixels");


      declareProperty("Intensity", 0.0, "Peak Integrated Intensity", Direction::Output);

      declareProperty("SigmaIntensity",0.0,"Peak Integrated Intensity Error", Direction::Output);


    }

   /**
    * Executes this algorithm
    *
    * Integrates one peak
    * -First attempts to find row/col extents, time extents to fully get most of the peak
    * -Integrate each time slice.
    * -Report Results
    */
    void IntegratePeakTimeSlices::exec()
    {
      time_t seconds1;

      seconds1 = time (NULL);

      double dQ= getProperty("PeakQspan");

      g_log.debug("------------------Start Peak Integrate-------------------");


      if (dQ <= 0)
      {
        g_log.error("Negative PeakQspans are not allowed. Use .17/G where G is the max unit cell length");
        throw  std::runtime_error("Negative PeakQspans are not allowed in IntegratePeakTimeSlices");
      }


      MatrixWorkspace_const_sptr inpWkSpace = getProperty("InputWorkspace");
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

      //------------------------------- Get Panel --------------------------------------
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

      FindPlane(  center,  xvec,  yvec, ROW, COL,NROWS,NCOLS, CellWidth, CellHeight, peak);


      g_log.debug() <<"   Peak Index "<<indx <<std::endl;

      double TotVariance = 0;
      double TotIntensity = 0;
      double lastRow = ROW;
      double Row0 = lastRow;
      double lastCol = COL;
      double Col0 = lastCol;
      string spec_idList="";

      // For quickly looking up workspace index from det id
      wi_to_detid_map = inpWkSpace->getDetectorIDToWorkspaceIndexMap();

      TableWorkspace_sptr TabWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(0));

      //----------------------------- get Peak extents ------------------------------
      try
      {

        // Find the workspace index for this detector ID
        detid2index_map::const_iterator it = wi_to_detid_map.find(detID);
        size_t wsIndx = (it->second);

        double R      = CalculatePositionSpan( peak, dQ )/2;

        R = min<double> (MaxRowColSpan*max<double>(CellWidth,CellHeight), R);
        R = max<double> (MinRowColSpan*max<double>(CellWidth,CellHeight), R);

        R=2*R;  //Gets a few more background cells.
        int Chan;

        const MantidVec & X = inpWkSpace->dataX(wsIndx);
        int dChan = CalculateTimeChannelSpan(peak, dQ, X, int(wsIndx), Chan);

        dChan = max<int> (dChan, MinTimeSpan);


        double Centy = Row0;
        double Centx = Col0;
        IDetector_const_sptr CenterDet =  peak.getDetector();


        double neighborRadius ;//last radius for finding neighbors

        neighborRadius = min<double>(MaxNeighborhoodRadius,NeighborhoodRadiusDivPeakRadius*R);
        int Nneighbors = (int)(neighborRadius*neighborRadius/CellWidth/CellHeight*4);


        Nneighbors = min<int>(Nneighbors,(int)inpWkSpace->getNumberHistograms()-2);
        delete [] NeighborIDs;

        NeighborIDs = new int[Nneighbors+2];
        NeighborIDs[0]=Nneighbors+2;
        NeighborIDs[1] = 2;
        Kernel::V3D Cent =(center+xvec*(Centx-COL)+yvec*(Centy-ROW));

        getNeighborPixIDs(panel,Cent,neighborRadius, NeighborIDs );

        if( NeighborIDs[1] <10)
        {
          g_log.error("Not enough neighboring pixels to fit ");
          throw std::runtime_error("Not enough neighboring pixels to fit ");
        }
        int NBadEdgeCells = getProperty("NBadEdgePixels");
        int MaxChan = -1;
        double MaxCounts = -1;

        //     --------------- Find Time Chan with max counts----------------
        for( int dir =1 ; dir >-2; dir -=2)
        {
          bool done = false;
          for( int t= 0; t < dChan && !done; t++)
          if( dir < 0 &&  t==0 )
           {
             Centy = Row0;
             Centx = Col0;
            }
           else if( Chan+dir*t <0 || Chan+dir*t >= (int)X.size())
              done = true;
           else
           {

             int NN = NeighborIDs[1];
              MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                         std::string("Workspace2D"), 3,NN,NN);

              Kernel::V3D CentPos = center+yvec*(Centy-ROW)*CellHeight + xvec*(Centx-COL)*CellWidth;

              boost::shared_ptr<DataModeHandler> XXX( new DataModeHandler(R,R,Centy,Centx,CellWidth,CellHeight,
                                                   getProperty("CalculateVariances"),NBadEdgeCells,NCOLS-NBadEdgeCells,
                                                   NBadEdgeCells,NROWS-NBadEdgeCells));
              AttributeValues = XXX;
              XXX->setCurrentRadius(R);

              SetUpData1(Data, inpWkSpace,  Chan+dir*t, Chan+dir*t,  R ,CenterDet->getPos(), spec_idList);

              if (AttributeValues->StatBaseVals(ISSIxx) > 0)
              {
                if (AttributeValues->StatBaseVals(IIntensities) > MaxCounts)
                {
                  MaxCounts = AttributeValues->StatBaseVals(IIntensities);
                  MaxChan = Chan + dir * t;
                }
                if (AttributeValues->StatBaseVals(IIntensities) > 0)
                {
                  Centx = AttributeValues->StatBaseVals(ISSIx) / AttributeValues->StatBaseVals(IIntensities);
                  Centy = AttributeValues->StatBaseVals(ISSIy) / AttributeValues->StatBaseVals(IIntensities);
                }
                else
                  done = true;
              }
              else
                done = true;
              
              if( t >= 3 &&  (AttributeValues->StatBaseVals(IIntensities) < MaxCounts/2.0) && MaxCounts >= 0)
                 done = true;

           }
        }
        if( MaxChan >0)
            Chan =  MaxChan ;


        g_log.debug()<<
            "   largest Channel,Radius,CellWidth,CellHeight = "<<Chan<<" "<< R<<" "<<CellWidth<<" "
                                                 << CellHeight<<std::endl;

        if (R < MinRowColSpan/2*max<double>(CellWidth, CellHeight) || dChan < MinTimeSpan)
        {
          g_log.error("Not enough rows and cols or time channels ");
          throw std::runtime_error("Not enough rows and cols or time channels ");
        }

        InitializeColumnNamesInTableWorkspace(TabWS);

        //------------------------------------- Start the Integrating -------------------------------
        double time;
        int ncells;

        Mantid::API::Progress prog(this, 0.0, 100.0, (int) dChan);

        // Set from attributes replace by R0
        R0= -1;
        int LastTableRow = -1;
        boost::shared_ptr<DataModeHandler>origAttributeList(new DataModeHandler());
        boost::shared_ptr<DataModeHandler>lastAttributeList( new DataModeHandler());

        for (int dir = 1; dir >= -1; dir -= 2)
        {
          bool done = false;

          for (int chan = 0; chan < dChan  && !done; chan++)
            if (dir < 0 && chan == 0)
            {
              lastRow = Row0;
              lastCol = Col0;
              lastAttributeList = origAttributeList;
              if( TabWS->rowCount() >0 )
                LastTableRow = 0;

            }
            else if( Chan+dir*chan <0 || Chan+dir*chan >= (int)X.size())
               done = true;
            else
            {

              int xchan = Chan + dir * chan;

              size_t topIndex = xchan + 1;
              if (topIndex >= X.size())
                topIndex = X.size() - 1;

              time = (X[xchan] + X[topIndex]) / 2.0;

              double Radius = R;

              if( R0 >0)
                 Radius = R0;

              int NN= NeighborIDs[1];

              MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                  std::string("Workspace2D"), 3, NN,NN);//neighbors.size(), neighbors.size());


              g_log.debug()<<" A:chan="<< xchan<<"  time="<<time<<"   Radius="<<Radius
                  <<"row= "<<lastRow<<"  col="<<lastCol<<std::endl;

              SetUpData(Data, inpWkSpace, panel, xchan,xchan, lastCol,lastRow,Cent,
                                                      neighborRadius, Radius, spec_idList);

              AttributeValues->setTime( time);

             // if( dir==1 && chan ==0)
            //    origAttributeList= AttributeValues;


              ncells = (int) (AttributeValues->StatBaseVals(ISS1));

              std::vector<double> params;
              std::vector<double> errs;
              std::vector<std::string> names;

              if (AttributeValues->StatBaseVals(ISSIxx) > 0 &&
                  AttributeValues->IsEnoughData( ParameterValues, g_log) &&
                  ParameterValues[ITINTENS] > 0)
              {
                double chisqOverDOF;

                Fit(Data,chisqOverDOF,done,  names,params,errs, lastRow, lastCol, neighborRadius);

                if(!done)//Bivariate error happened
                {

                  if (isGoodFit(params, errs, names, chisqOverDOF))
                  {
                    LastTableRow =UpdateOutputWS(TabWS, dir, xchan, params, errs, names, chisqOverDOF,
                          AttributeValues->time, spec_idList);

                    double TotSliceIntensity = AttributeValues->StatBaseVals(IIntensities);
                    double TotSliceVariance = AttributeValues->StatBaseVals(IVariance);

                    updatePeakInformation(    params,             errs,           names,
                                              TotVariance,       TotIntensity,
                                              TotSliceIntensity, TotSliceVariance, chisqOverDOF,
                                              ncells);

                    lastAttributeList= AttributeValues;

                    if( dir==1 && chan==0)
                      origAttributeList =lastAttributeList;
                  }
                  else

                    done = true;

                }

              }
              else //(!IsEnoughData() || ParameterValues[ITINTENS] <= 0
              {
                done = true;

              }

              if( done )//try to merge
              {     done = false;

                    int chanMin,chanMax;
                    if( ( dir ==1 && chan ==0 ) || lastAttributeList->CellHeight <= 0)
                    {
                      chanMin=xchan;
                      chanMax =xchan+1;
                      if( dir < 0)
                        chanMax++;
                      boost::shared_ptr<DataModeHandler>XXX(new DataModeHandler(*AttributeValues));
                      AttributeValues = XXX;
                      if( X.size() > 0)
                        AttributeValues->setTime((X[chanMax]+ X[chanMin])/2.0);

                    }else//lastAttributeList exists

                    {
                      chanMin = std::min<int>( xchan, xchan-dir);
                      chanMax = chanMin+1;
                      if( lastAttributeList->case4)
                         chanMax++;

                     boost::shared_ptr<DataModeHandler>XXX(new DataModeHandler(*lastAttributeList));
                     AttributeValues = XXX;

                     AttributeValues->setTime( (time+AttributeValues->time)/2.0);
                    }


                    if( updateNeighbors(panel, AttributeValues->getCurrentCenter(),
                                 Cent,AttributeValues->getCurrentRadius(),neighborRadius))
                         Cent = AttributeValues->getCurrentCenter();

                    int NN= NeighborIDs[1];
                    MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                                     std::string("Workspace2D"), 3, NN,NN);

                    SetUpData1(Data    , inpWkSpace, chanMin, chanMax,
                        AttributeValues->getCurrentRadius()            ,
                        AttributeValues->getCurrentCenter(), spec_idList);

                    double chisqOverDOF;

                    g_log.debug("Try Merge 2 time slices");
                    if( AttributeValues->StatBaseVals(ISSIxx)>=0 &&
                        AttributeValues->IsEnoughData(ParameterValues, g_log))

                       Fit(Data,chisqOverDOF,done, names, params,
                             errs, lastRow, lastCol, neighborRadius);
                    else
                      chisqOverDOF=-1;

                    if(!done && isGoodFit(params, errs, names, chisqOverDOF))
                    {

                      if( LastTableRow >=0 && LastTableRow < (int) TabWS->rowCount())
                        TabWS->removeRow(LastTableRow);
                      else
                        LastTableRow =-1;

                      LastTableRow =UpdateOutputWS(TabWS, dir, (chanMin+chanMax)/2.0, params, errs,
                                      names, chisqOverDOF,  AttributeValues->time, spec_idList);

                      if( lastAttributeList->lastISAWVariance > 0  && lastAttributeList->CellHeight >0 )
                      {
                        TotIntensity -=lastAttributeList->lastISAWIntensity;
                        TotVariance -= lastAttributeList->lastISAWVariance;
                      }

                      double TotSliceIntensity = AttributeValues->StatBaseVals(IIntensities);

                      double TotSliceVariance = AttributeValues->StatBaseVals(IVariance);


                      updatePeakInformation(    params,             errs,           names,
                                      TotVariance,       TotIntensity,
                                     TotSliceIntensity, TotSliceVariance, chisqOverDOF,
                                      (int) AttributeValues->StatBaseVals(ISS1));

                     // lastAttributeList= AttributeValues;

                      if( dir ==1 && (chan ==0||chan==1))
                      {
                        AttributeValues->case4 = true;
                        origAttributeList =AttributeValues;
                      }else
                        LastTableRow = -1;


                    }else
                    {
                      boost::shared_ptr<DataModeHandler>XXX( new DataModeHandler());
                      lastAttributeList =XXX;
                    }
                    done = true;

             }

              //Get ready for the next round
              Data.reset();

              if (!done)
              {

                //Now set up the center for this peak
                int i = find("Mrow", names);
                lastRow = (int)( params[i]+.5);
                i = find("Mcol", names);
                lastCol = (int)( params[i]+.5);
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

        setProperty("OutputWorkspace", TabWS);

        setProperty("Intensity", TotIntensity);
        setProperty("SigmaIntensity", SQRT(TotVariance));
        time_t seconds2;

        seconds2 = time (NULL);
        double   dif = difftime (seconds2,seconds1);
        g_log.debug() <<"Finished Integr peak number "<< indx <<" in "<< dif<<" seconds"<<std::endl;

      } catch (std::exception &ss)
      {

        std::cout << "Error occurred XX " << ss.what() << std::endl;
        throw std::runtime_error( ss.what());
      }

    }


    /**
       * Finds all neighbors within a given Radius of the Center on the given component.
       * @param comp -The component of interest
       * @param Center- the center of the neighbors
       * @param Radius - The radius from the center of neighbors to be included
       * @param ArryofID -The detector ID's of the neighbors. The id of the pixel at the center may be included.
       */
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
                return true;

            if( box.yMax() <=miny)
               return true;

            if( box.zMin()>=maxz)
                return true;

            if( box.zMax() <=minz)
               return true;;

            boost::shared_ptr<Geometry::Detector> det =   boost::dynamic_pointer_cast<Geometry::Detector>( comp);
           // if( comp->type().compare(0,8,"Detector")==0 || comp->type().compare("RectangularDetectorPixel")==0)
            if( det )
             {
                     V3D pos = det->getPos() -Center;
                     if( pos.X()*pos.X()+pos.Y()*pos.Y()+pos.Z()*pos.Z() <Radius*Radius)
                     {
                       ArryofID[N]=  det->getID();
                        N++;
                        ArryofID[1] = N;

                     }
                    return true;
              }

             boost::shared_ptr<const Geometry::ICompAssembly> Assembly =
                 boost::dynamic_pointer_cast<const Geometry::ICompAssembly>( comp);

             if( !Assembly)
                return true;

             bool b = true;

             for( int i=0; i< Assembly->nelements() && b; i++)
                b= getNeighborPixIDs( Assembly->getChild(i),Center,Radius,ArryofID);

             return b;
       }


      /**
       * Checks and updates if needed the list of NeighborIDs
       * @param comp  Component with the neighboring pixels
       * @param CentPos    new Center
       * @param oldCenter   old Center
       * @param NewRadius   new Radius
       * @param neighborRadius  old the new neighborhood radius
       */
       bool IntegratePeakTimeSlices::updateNeighbors( boost::shared_ptr< Geometry::IComponent> &comp,
           V3D CentPos,  V3D oldCenter,double NewRadius, double &neighborRadius)
       {
           double DD =(CentPos - oldCenter).norm();
           bool  changed = false;
           if(DD +NewRadius >neighborRadius)
           {
              int NN= int(NStdDevPeakSpan*NeighborhoodRadiusDivPeakRadius*NewRadius/CellWidth*NStdDevPeakSpan*NeighborhoodRadiusDivPeakRadius*NewRadius/CellHeight);
              if( NeighborIDs[0]< NN)
              {
                delete [] NeighborIDs;
                NeighborIDs = new int[NN+2];
                NeighborIDs[0]=NN+2;
              }
              NeighborIDs[1]=2;
              neighborRadius = NeighborhoodRadiusDivPeakRadius*NewRadius;

              getNeighborPixIDs(comp, CentPos, neighborRadius, NeighborIDs);
              changed=true;

           }else// big enough neighborhood so
             neighborRadius -= DD;

           return changed;
       }


    /**
     * Calculates the span in rows and columns needed to include all data within dQ of the
     * specified peak
     * @param peak  The peak of interest
     * @param dQ    The offset from the peak's Q value for the data of interest
     *
     * NOTE: differentials of Q =mv*sin(scatAng/2)/2 were used to calculate this
     *  Also s=r*theta was used to transfer d ScatAng to distance on a bank.
     */
    double IntegratePeakTimeSlices::CalculatePositionSpan(  API::IPeak const &peak,
                                                      const double                   dQ)
   {

     try
     {
       double Q = 0, ScatAngle = 0, dScatAngle = 0, DetSpan = 0;

       Q = peak.getQLabFrame().norm();
       Geometry::Instrument_const_sptr instr = peak.getInstrument();
       const Geometry::IComponent_const_sptr  sample = instr->getSample();
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


    /**
     * Calculates the span of channels needed to encompass all data around the peak with Q values within dQ
     * of this peak's Q value
     *
     * @param peak     The peak of interest
     * @param dQ       The offset of peak's Q value whose data is considered part of the peak
     * @param X        The list of time channel values.
     * @param specNum  The spectral number for the pixel(Not Currently Used)
     * @param Centerchan  The center time channel number( from X)
     *
     * @return The number of time channels around Centerchan to use
     */
    int IntegratePeakTimeSlices::CalculateTimeChannelSpan( API::IPeak     const & peak,
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


    /**
     * For NonFlat banks, this determines the data of a small planar region approximating the instrument
     * close to the peak
     *
     * @param center  The position of the center of this plane
     * @param xvec    The direction the column values increase
     * @param yvec    The direction the row values increase
     * @param ROW     The row for this peak( 0 if undefined)
     * @param COL     The col for this peak( 0 if undefined)
     * @param NROWS   The number? of rows for this bANK
     * @param NCOLS    The number of columns for this bank
     * @param pixWidthx   The width of a pixel
     * @param pixHeighty  The height of a pixel
     * @param peak
     */
     void IntegratePeakTimeSlices::FindPlane( V3D & center, V3D & xvec, V3D& yvec,
                                              double &ROW, double &COL,int &NROWS,
                                              int & NCOLS,double &pixWidthx, double&pixHeighty,
                                              API::IPeak const &peak) const
     {

       NROWS= NCOLS = -1;
       IDetector_const_sptr det = peak.getDetector();
       V3D detPos = det->getPos();

       center.setX( detPos.X());
       center.setY( detPos.Y());
       center.setZ( detPos.Z());

       boost::shared_ptr<const Detector> dett = boost::dynamic_pointer_cast<const Detector >(det);

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
        ROW=peak.getRow();
        COL=peak.getCol();
        Geometry::Instrument_const_sptr inst =peak.getInstrument();
        if( !inst)
           throw std::invalid_argument("No instrument for peak");
        boost::shared_ptr< const
              IComponent >  panel =inst->getComponentByName( peak.getBankName());

        boost::shared_ptr<const RectangularDetector>ddet = boost::dynamic_pointer_cast<const RectangularDetector>(panel);

        if( ddet )
        {
           std::pair<int,int> CR =ddet->getXYForDetectorID(det->getID());
           ROW = CR.second;
           COL=CR.first;
           pixWidthx = ddet->xstep();
           pixHeighty= ddet->ystep();

           NROWS = ddet->ypixels();
           NCOLS = ddet->xpixels();

           return;

        }
        //Get NROWS and NCOLS for other panels
       NROWS= NCOLS = -1;

        if( !inst)
          return;

        if( !panel)
          return;
        boost::shared_ptr<const Component>compPanel =
                         boost::dynamic_pointer_cast<const Component>(panel);
        boost::shared_ptr<IComponent> panel1( compPanel->base()->clone());
        BoundingBox B;

        Quat rot =panel1->getRotation();

        rot.inverse();

        panel1->rotate(rot);

        panel1->getBoundingBox(B);


        NROWS = (int)((B.yMax()-B.yMin())/pixHeighty+.5);
        NCOLS = (int)((B.xMax()-B.xMin())/pixWidthx+.5);


     }

    /**
     * Updates the cumulative statistics for the data being considered
     *
     * @param intensity -The experimental intensity at the given pixel
     * @param variance  -The square of the errors in the above intensity
     * @param row       -The row of the given pixel
     * @param col       -The column of the given pixel
     * @param StatBase  -The data accumulator
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


   /**
    * Finds and saves the initial values and modes( like isEdge) for this data.
    *
    * @param Varx  The Variance of the x(column) values
    * @param Vary  The Variance of the y(row)  values
    * @param b     The background
    *
    * NOTE:Used only in GetParams which is used only in PreFit which is not
    * currently used.
    */
   std::vector<double>  DataModeHandler::InitValues( double Varx,double Vary,double b)
    {
       std::vector<double> Res(7);

       Res[IVXX]= Varx;
       Res[IVYY] = Vary;
       Res[IVXY] =0;
       int nCells = (int)StatBase[ISS1];
       double Den = StatBase[IIntensities] - b * nCells;
       Res[IXMEAN] = ( StatBase[ISSIx] - b * StatBase[ISSx] )/Den;
       Res[IYMEAN] = ( StatBase[ISSIy] - b * StatBase[ISSy] )/Den;
       Res[IBACK] = b;
       Res[ITINTENS] =StatBase[IIntensities] - b * nCells;

       //---- Is Edge Cell ???-------
       double  NstdX = 4*( currentRadius/CellWidth - EdgeX )/sqrt( Varx );
       double  NstdY = 4*( currentRadius/CellHeight - EdgeY )/sqrt( Vary );
       double sigx=1;
       double sigy=1;
       if( NstdX < 0) sigx=-1;
       if( NstdY < 0) sigy=-1;

       double  x = 1;
       if( sigy*NstdY < 7 && sigy*NstdY >= 0) //is close to row edge
       {
         x =probs[ (int)(sigy*NstdY +.5) ];
         if( sigy < 0) x=1-x;
         double  My2= StatBase[IStartRow];
         if( Res[IYMEAN] -My2 > My2+StatBase[INRows] -Res[IYMEAN] )
           My2 +=StatBase[INRows];
         Res[IYMEAN] = Res[IYMEAN]*x +(1-x)*My2;

       }
       double  x1 = 1;
       if( sigx*NstdX < 7 && sigx*NstdX > 0)//is close to x edge
       {
         x1 =probs[ (int)(sigx*NstdX +.5) ];
         if( sigx < 0) x1=1-x1;
         double  Mx2= StatBase[IStartCol];
         if( Res[IXMEAN] -Mx2 > Mx2+StatBase[INCol] -Res[IXMEAN] )
             Mx2 +=StatBase[INCol];
         Res[IXMEAN] = Res[IXMEAN]*x1 +(1-x1)*Mx2;

        }
       Res[ITINTENS] /=x*x1;

       return  Res;
     }

   /**
    * Calculates the initial values of the parameters given background b.
    * Used only in the PreFit method that is not used.
    * @param b   the background
    *
    * @return The vector of parameter values.
    */
   std::vector<double> DataModeHandler::GetParams( double b )
   {

     int nCells = (int) (StatBase[ISS1]);
     double Den = StatBase[IIntensities] - b * nCells;
     double Varx,Vary;

     Varx =VarxHW;
     Vary =VaryHW;

     double Rx = lastRCRadius/CellWidth - EdgeX;
     double Ry = lastRCRadius/CellHeight - EdgeY;
     if(Varx <=0)
        Varx=HalfWidthAtHalfHeightRadius * HalfWidthAtHalfHeightRadius ;

     if( Vary <=0)
       Vary =HalfWidthAtHalfHeightRadius * HalfWidthAtHalfHeightRadius ;
          //Use
     if(Rx*Rx < 4*Varx || Ry*Ry < 4*Vary )
     {
       return InitValues(  Varx, Vary, b);
     }
     if( Den < 0)
       return std::vector<double>();

     double Mx = StatBase[ISSIx] - b * StatBase[ISSx];
     double My = StatBase[ISSIy] - b * StatBase[ISSy];

     double Sxx = (StatBase[ISSIxx] - b * StatBase[ISSxx] - Mx * Mx / Den) / Den;
     double Syy = (StatBase[ISSIyy] - b * StatBase[ISSyy] - My * My / Den) / Den;
     double Sxy = (StatBase[ISSIxy] - b * StatBase[ISSxy] - Mx * My / Den) / Den;

     double Intensity= StatBase[IIntensities] - b * nCells;
     double    col = Mx / Den;
     double    row = My / Den;
     std::vector<double> Result(7);
     Result[IBACK] = b;
     Result[ITINTENS] = Intensity;
     Result[IXMEAN] = col;
     Result[IYMEAN] = row;
     Result[IVXX] = Sxx;
     Result[IVYY] = Syy;
     Result[IVXY] = Sxy;

     return Result;

   }

   /**
    *  Sets the Accumulated data values into this class, then updates other information like initial values
    *
    *  @param StatBase  The accumulated data that is to be considered.
    */
    bool DataModeHandler::setStatBase(std::vector<double> const &StatBase)

    {
      double   TotBoundaryIntensities =StatBase[ITotBoundary ] ;
      int      nBoundaryCells = (int)StatBase[INBoundary];
      this->StatBase = StatBase;
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

      double Varx,Vary;
      Varx =StatBase[INCol]/7 ;//Range = 3.5 standard deviations
      Vary =StatBase[INRows]/7 ;
      Varx *= Varx;
      Vary *= Vary;

      double Rx = lastRCRadius/CellWidth - EdgeX;
      double Ry = lastRCRadius/CellHeight - EdgeY;
      if( CellWidth >0 && currentRadius >0 && lastCol >0 && lastRow > 0)
      if( Rx*Rx < 4*std::max<double>(Varx,VarxHW) || HalfWidthAtHalfHeightRadius < 0 ||
                 Ry*Ry < 4*std::max<double>(Vary,VaryHW) )// Edge peak so cannot use samples
      {
        Vx_calc= VarxHW;
        Vy_calc = VaryHW;
        Vxy_calc =0;
        col_calc = lastCol;
        row_calc = lastRow;
        back_calc = b;
        Intensity_calc =StatBase[IIntensities] - b * nCells;
        if( Vx_calc <=0 || Vy_calc<=0) //EdgePeak but not big enuf
          return true;

        double params[]={back_calc,Intensity_calc,col_calc,row_calc,Vx_calc,Vy_calc,Vxy_calc};
        double r=CalcSampleIntensityMultiplier( params );
        Intensity_calc *=r;
        return  true;
      }
      if( Den <=0)
        Den = 1;

      bool done = false;
      int ntimes = 0;
      double Mx, My, Sxx, Syy, Sxy;


      double RangeX= StatBase[INCol]/2;
      double RangeY = StatBase[INRows]/2;


      while (!done && ntimes < 29)
      {
        Mx = StatBase[ISSIx] - b * StatBase[ISSx];
        My = StatBase[ISSIy] - b * StatBase[ISSy];
        Sxx = (StatBase[ISSIxx] - b * StatBase[ISSxx] - Mx * Mx / Den) / Den;
        Syy = (StatBase[ISSIyy] - b * StatBase[ISSyy] - My * My / Den) / Den;
        Sxy = (StatBase[ISSIxy] - b * StatBase[ISSxy] - Mx * My / Den) / Den;
        ntimes++;
        done = false;

        if (Sxx <= RangeX/12 || Syy <= RangeY/12 || Sxy * Sxy / Sxx / Syy > .9)
        {
          b = b*.95;

          if (ntimes +1 == 29)
            b=0;

          Den = StatBase[IIntensities] - b * nCells;
          if (Den <= 1)
            Den = 1;

        }else
          done = true;

      }

     back_calc = b;
     Intensity_calc= StatBase[IIntensities] - b * nCells;
     col_calc = Mx / Den;
     row_calc = My / Den;
     Vx_calc = Sxx;
     Vy_calc = Syy;
     Vxy_calc = Sxy;
     return false;


    }

    /**
     * Calculates the new radius for neighborhoods so as to include almost all of a peak
     *
     * @return the new radius
     */
    double DataModeHandler::getNewRCRadius()
    {
      double Vx,Vy;
      Vx= VarxHW;
      Vy=VaryHW;
      if( Vx <0)
          Vx = HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;
      if( Vx<0)
        Vy=HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;

      double Rx = lastRCRadius/CellWidth - EdgeX;
      double Ry = lastRCRadius/CellHeight - EdgeY;
      double mult =1;
      if( Rx*Rx > 4*Vx )
          Vx = std::max<double>(VarxHW,  Vx_calc);
      else
        mult = 1.35;

      if( Ry*Ry > 4*Vy)
        Vy =std::max<double>(VaryHW, Vy_calc);
      else
        mult *=1.35;

      double  DD = max<double>( sqrt(Vy)*CellHeight, sqrt(Vx)*CellWidth);
      double NewRadius= 1.4* max<double>( MinRowColSpan*max<double>(CellWidth,CellHeight), 4.5*DD);
      NewRadius = mult*min<double>(baseRCRadius, NewRadius);
               //1.4 is needed to get more background cells. In rectangle the corners were background

       NewRadius = min<double>(MaxRowColSpan*max<double>(CellWidth,CellHeight),NewRadius);

       return NewRadius;
    }

    /**
     * For edge peaks, the sample standard deviations do not work. This attempts to estimate the peak
     * widths by using half width at half max idea
     *
     * @param xvals   The x(column) values of the data to be considered
     * @param yvals   The y(row) values of the data to be considered
     * @param counts  The intensity at the given row and column (and timeslice)
     */
    void DataModeHandler::setHeightHalfWidthInfo( Mantid::MantidVecPtr &xvals,
                                                 Mantid::MantidVecPtr &yvals,
                                                 Mantid::MantidVecPtr &counts)
    {
      double minCount,
             maxCount;
      MantidVec X = xvals.access();
      MantidVec Y = yvals.access();
      MantidVec C = counts.access();
      VarxHW = -1;
      VaryHW = -1;
      int N = (int)X.size();

      HalfWidthAtHalfHeightRadius = -2;

      if( N <= 2 )
        return ;

      minCount = maxCount = C[0];
      double MaxX = -1;
      double MaxY = -1;
      int nmax=0;
      double lowX,lowY,highX,highY;
      lowX=highX =X[0];
      lowY=highY =Y[0];

      for( int i = 1; i < N; i++ )
      {
        if( X[i]<lowX) lowX=X[i];
        else if (X[i]>highX)highX=X[i];


        if( Y[i]<lowY)        lowY=Y[i];
        else if (Y[i]>highY)  highY=Y[i];

        if( C[i] > maxCount )
        {
          maxCount = C[i];
          MaxX = X[i];
          MaxY = Y[i];
          nmax=1;
        }
        else if( C[i] < minCount )
        {
          minCount = C[i];

        }else if( C[i] == maxCount)//Get a tolerance on this
        {
          MaxX += X[i];
          MaxY += Y[i];
          nmax++;
        }
      }
      if( minCount == maxCount)
        return ;

      MaxX /=nmax;
      MaxY /=nmax;

      double dCount = std::max<double>(.51,(maxCount- minCount)/6.2);
      double CountUp = (maxCount+ minCount)/2 + dCount;
      double CountLow =( maxCount+ minCount)/2 - dCount;

      //Checking for weak peak not really reaching edge, so could use full peak work
      double d2Edge= min<double>(min<double>(MaxX-lowX,highX-MaxX),min<double>(MaxY-lowY,highY-MaxY));
      double dSpanx = (highX-lowX)/6.;
      double dSpany= (highY-lowY)/6.0;
      if( MaxX+d2Edge >= highX-.000001)
      {
        lowX = highX-(highX-MaxX)/4;
        if( (int)lowX ==(int)highX) lowX -=1;
        highY=highY-(highX-MaxX)/4;
        lowY=lowY + (highX-MaxX)/4;


      }else if( MaxX-d2Edge <= lowX+.000001)
      {
        highX= lowX+(MaxX-lowX)/4;
        if( (int) highX == (int)lowX)highX +=1.0;

        highY -=(MaxX-lowX)/4;
        lowY  +=(MaxX-lowX)/4;

      }else if(MaxY+d2Edge >= highY-.000001)
      {
        lowY = highY-(highY-MaxY)/4;
        if( (int)lowY ==(int)highY) lowY -=1;
        highX -=(highY-MaxY)/4;
        lowX += (highY-MaxY)/4;

      }else
      {
        highY = lowY+(MaxY-lowY)/4;
        if( (int)lowY ==(int)highY) highY -=1;
        highX -= (MaxY-lowY)/4;
        lowX  += (MaxY-lowY)/4;

      }


      int nMax = 0;
      int nMin = 0;
      double TotMax = 0;
      double TotMin = 0;
      double offset = std::max<double>(.2, (maxCount - minCount) / 20);
      double TotR_max =0;
      double TotR_min =0;
      int nedge1Cells=0;
      int nintCells =0;
      int nBoundEdge1Cells =0;
      int nBoundIntCells =0;
      double TotRx0=0;
      double TotRy0=0;
      double TotCx=0;
      double TotCy=0;
      double IntOffset = std::min<double>( highX-lowX , highY-lowY );
      for (int i = 0; i < N ; i++)
      {
        if ( C[i] > maxCount - offset)
        {
          TotMax += C[i];
          nMax++;
          TotR_max+= C[i]*sqrt( (X[i]-MaxX)*(X[i]-MaxX)+ (Y[i]-MaxY)*(Y[i]-MaxY));

        }
        if (C[i] < minCount + offset)

        {
          TotMin += C[i];
          nMin++;

          TotR_min+= C[i]*sqrt( (X[i]-MaxX)*(X[i]-MaxX)+ (Y[i]-MaxY)*(Y[i]-MaxY));
        }

        if( X[i] >= lowX && X[i] <= highX &&  Y[i]>=lowY && Y[i]<=highY)
        {
          nedge1Cells++;
          if( C[i]<minCount+offset) nBoundEdge1Cells++;
        }
        if( fabs(MaxX-X[i])<  IntOffset && fabs(MaxY-Y[i])< IntOffset)
        {
          nintCells++;
          if( C[i] < minCount+offset) nBoundIntCells++;

        }
        if(fabs(MaxY-Y[i])<1.2 &&  fabs(MaxX-X[i])>1.2 && C[i]>= CountLow && C[i] <= CountUp && fabs(MaxX-X[i])<dSpanx)
        {
          TotRx0 += (C[i]-minCount)*(X[i]-MaxX)*(X[i]-MaxX);
          TotCx += C[i]-minCount;
        }

        if(fabs(MaxX-X[i]) <1.2 && fabs(MaxY-Y[i])>1.2 &&  C[i]>= CountLow && C[i] <= CountUp && fabs(MaxY-Y[i])<dSpany )
        {
          TotRy0 += (C[i]-minCount)*(Y[i]-MaxY)*(Y[i]-MaxY);
          TotCy +=C[i]-minCount;
        }
      }

      if( nMax + nMin == N)      // all data are on two  levels essentially
      {
        if( TotMax <=0) TotMax =1;
        if (TotMin <=0)TotMin =1;
        double AvR = .5*(TotR_max/TotMax + TotR_min/TotMin);
        HalfWidthAtHalfHeightRadius = AvR/.8326;

        VarxHW = HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;
        VaryHW = HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;
        return ;
      }


      double TotR=0,nR=-1,nRx=-1,nRy=-1;
      double MidVal = ( TotMax/nMax + TotMin/nMin )/2.0;
      double TotRx=0,TotRy=0;
      while( (nR <= 0 || nRy <=0 || nRx <=0) && offset < MidVal )
      {
         TotR = 0;
         nR = 0;
         TotRx=0;
         TotRy=0;
         nRx=0;
         nRy=0;

        for( int i = 0; i < N; i++ )
        if( C[i] < MidVal + offset && C[i] > MidVal - offset )
        {
          double X1 = X[i] - MaxX;
          double Y1 = Y[i] - MaxY;
          TotR += sqrt( X1*X1 + Y1*Y1 );
          nR++;
          if((X1>=-1.2 && X1<=1.2)&& fabs(Y1)>1.2  &&  fabs(Y1)<dSpany)
          {
            nRy++;
            TotRy += abs(Y1);
          }
          if( (Y1>=-1.2 && Y1 <= 1.2)&& fabs(X1)>1.2 && fabs(X1)<dSpanx)
          {
            nRx++;
            TotRx += fabs(X1);
          }
        }
        offset *= 1.1;
      }

      double AvR = TotR/nR;
      HalfWidthAtHalfHeightRadius = AvR/.8326;

      if( nRx > 0)
        VarxHW= (TotRx/nRx)*(TotRx/nRx)/.8326/.8326;
      else if( TotCx >0)
        VarxHW = TotRx0*TotRx0/TotCx/TotCx/.8326/.8326;
      else if( HalfWidthAtHalfHeightRadius >0)
        VarxHW = HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;
      else
        VarxHW = -1;

      if( nRy > 0)
        VaryHW= (TotRy/nRy)*(TotRy/nRy)/.8326/.8326;
      else if( TotCy > 0)
        VaryHW = TotRy0*TotRy0/TotCy/TotCy/.8326/.8326;
      else if(HalfWidthAtHalfHeightRadius > 0)
        VaryHW = HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;
      else
        VaryHW = -1;

    }
    /**
     * Initial phase at converting Detector data to workspace data that will be sent to the Fit Function, BivariateNormal.
     * @param Data  -workspace that will be sent to the Fit Function, BivariateNormal
     * @param inpWkSpace -The MatrixWorkspace with all the experimental data
     * @param comp -The parent of all the pixels that are neighbors of the peak being considered
     * @param chanMin -The minimum channel to use
     * @param chanMax -The maximum channel to be collapsed to one channel
     * @param CentX -The current peak row
     * @param CentY -The current peak column
     * @param CentNghbr -The center of the current and next(if changed) neighbor pixels
     * @param neighborRadius -The radius of the current neighbor pixels
     * @param Radius -The current starting Radius to use for neighbors.
     * @param spec_idList -The list of spectral id's that are neighbors
     *
     */
    void IntegratePeakTimeSlices::SetUpData( MatrixWorkspace_sptr          & Data,
                                             MatrixWorkspace_const_sptr    const & inpWkSpace,
                                             boost::shared_ptr< Geometry::IComponent> comp,
                                             const int                       chanMin,
                                             const int                       chanMax,
                                             double                          CentX,
                                             double                          CentY,
                                             Kernel::V3D                     &CentNghbr,
                                             double                        &neighborRadius,//from CentDetspec
                                             double                         Radius,
                                             string                     &spec_idList)
    {

      Kernel::V3D CentPos1 = center + xvec*(CentX-COL)*CellWidth
                                   + yvec*(CentY-ROW)*CellHeight;

      int NBadEdgeCells = getProperty("NBadEdgePixels");

      boost::shared_ptr<DataModeHandler> X(new DataModeHandler(Radius,Radius,CentY,CentX,
          CellWidth,CellHeight,getProperty("CalculateVariances"),NBadEdgeCells,NCOLS-NBadEdgeCells,
          NBadEdgeCells,NROWS-NBadEdgeCells));

      AttributeValues = X;
      AttributeValues->setCurrentRadius( Radius );
      AttributeValues->setCurrentCenter( CentPos1);

      SetUpData1(Data             , inpWkSpace, chanMin, chanMax,
                 Radius            , CentPos1, spec_idList);

      if( AttributeValues->StatBaseVals(ISSIxx)< 0)// Not enough data
          return;


      double NewRadius = AttributeValues->getNewRCRadius();
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


      double DD =(CentPos - CentNghbr).norm();

          if(DD +NewRadius >neighborRadius)
          {
             int NN= int(NStdDevPeakSpan* NeighborhoodRadiusDivPeakRadius*NewRadius/CellWidth*
                 NStdDevPeakSpan* NeighborhoodRadiusDivPeakRadius*NewRadius/CellHeight);
             if( NeighborIDs[0]< NN)
             {
               delete [] NeighborIDs;
               NeighborIDs = new int[NN+2];
               NeighborIDs[0]=NN+2;
             }//else
              // NN= NeighborIDs[0]-2;
             NeighborIDs[1]=2;
             neighborRadius = NeighborhoodRadiusDivPeakRadius*NewRadius;
             CentNghbr = CentPos;
             getNeighborPixIDs(comp, CentPos, neighborRadius, NeighborIDs);


          }else// big enough neighborhood so
            neighborRadius -= DD;

      //if( changed) CentNghbr = CentPos.
      boost::shared_ptr<DataModeHandler> X1(new DataModeHandler(Radius,NewRadius,CentY,CentX,
          CellWidth,CellHeight, getProperty("CalculateVariances"),NBadEdgeCells,NCOLS-NBadEdgeCells,
          NBadEdgeCells,NROWS-NBadEdgeCells));

      AttributeValues = X1;
      AttributeValues->setCurrentRadius( NewRadius);
      AttributeValues->setCurrentCenter( CentPos);
      SetUpData1(Data, inpWkSpace, chanMin, chanMax,
                 NewRadius, CentPos, spec_idList );

    }

    /**
     * Prepares the data for futher analysis adding meta data and marking data on the edges of detectors
     * @param Data: Output workspace
     * @param inpWkSpace: Input workspace
     * @param chanMin: Minimum channel
     * @param chanMax: Maximum channel. Will be collapsed to one channel
     * @param Radius: The radius of detectors
     * @param CentPos: Center on plane
     * @param spec_idList: List of spectra id's
     */
    void  IntegratePeakTimeSlices:: SetUpData1(API::MatrixWorkspace_sptr              &Data,
                                               API::MatrixWorkspace_const_sptr        const &inpWkSpace,
                                               const int                               chanMin,
                                               const int                               chanMax,
                                               double                      Radius,
                                               Kernel::V3D                 CentPos,
                                               string                     &spec_idList
                                               )
    {
      UNUSED_ARG(g_log);
      std::vector<double> StatBase( NAttributes );
      if( NeighborIDs[1] < 10)
      {
         StatBase[ISSIxx]= -1;
        return;
      }
      boost::shared_ptr<Workspace2D> ws = boost::dynamic_pointer_cast<Workspace2D>(Data);

      int NBadEdges = getProperty("NBadEdgePixels");
      spec_idList.clear();

      for (int i = 0; i < NAttributes + 2; i++)
        StatBase.push_back(0);

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
      double TotBoundaryVariances =0;


      int N=0;
      double BoundaryRadius=min<double>(.90*Radius, Radius-1.5*max<double>(CellWidth,CellHeight));
      double minRow= 20000,
             maxRow =-1,
             minCol =20000,
             maxCol =-1;

      int jj=0;
      Mantid::MantidVecPtr pX;

      Mantid::MantidVec& xRef = pX.access();
      for (int i = 2; i < NeighborIDs[1]; i++)
      {
        int DetID = NeighborIDs[i];

        size_t workspaceIndex ;
        if( wi_to_detid_map.count(DetID)>0)
           workspaceIndex= wi_to_detid_map.find(DetID)->second;
        else
        {
          throw std::runtime_error("No workspaceIndex for detID="+boost::lexical_cast<std::string>(DetID));
        }


        IDetector_const_sptr Det = inpWkSpace->getDetector(workspaceIndex);
        V3D pixPos = Det->getPos();


        if( i > 2)
          spec_idList +=  "," ;


        V3D dist = pixPos - CentPos;
        if (dist.scalar_prod(dist) < Radius*Radius)

        { spec_idList +=  boost::lexical_cast<std::string>( inpWkSpace->getSpectrum( workspaceIndex )->getSpectrumNo());

          double R1 = dist.scalar_prod(yvec);
          double R1a = R1/CellHeight;

          double row = ROW + R1a;

          double C1 = dist.scalar_prod(xvec);
          double C1a = C1/CellWidth;

          double col = COL + C1a;

          if (row > NBadEdges && col > NBadEdges && (NROWS < 0 || row < NROWS - NBadEdges) && (NCOLS < 0
              || col < NCOLS - NBadEdges))
          {
            Mantid::MantidVec histogram = inpWkSpace->readY(workspaceIndex);

            Mantid::MantidVec histoerrs = inpWkSpace->readE(workspaceIndex);
            double intensity = 0;
            double variance = 0;
            for (int chan = chanMin; chan <= chanMax; chan++)
            {
              intensity += histogram[chan];
              variance += histoerrs[chan] * histoerrs[chan];
            }

            N++;
            yvalB.push_back(intensity);
            double sigma = 1;

            errB.push_back(sigma);
            xvalB.push_back((double) col);
            YvalB.push_back((double) row);

            xRef.push_back((double) jj);
            jj++;

            updateStats(intensity, variance, row, col, StatBase);

            if ((pixPos - CentPos).norm() > BoundaryRadius)
            {
              TotBoundaryIntensities += intensity;
              nBoundaryCells++;

              TotBoundaryVariances += variance;

            }

            if (row < minRow)
              minRow = row;
            if (col < minCol)
              minCol = col;
            if (row > maxRow)
              maxRow = row;
            if (col > maxCol)
              maxCol = col;

          }//if not bad edge


        }//peak within radius
        }//for each neighbor


      AttributeValues->EdgeY = max<double>(0.0,max<double>(-ROW+minRow+Radius/CellHeight,-maxRow + ROW + Radius/CellHeight));
      AttributeValues->EdgeX =max<double>(0.0, max<double>(-COL+minCol+Radius/CellWidth, -maxCol+COL+Radius/CellWidth));
      if( AttributeValues->EdgeY <=1)
        AttributeValues->EdgeY=0;
      if( AttributeValues->EdgeX <=1)
        AttributeValues->EdgeX=0;


      Data->setX(0, pX);
      Data->setX(1, pX);
      Data->setX(2, pX);

      ws->setData(0, yvals, errs);
      ws->setData(1, xvals);
      ws->setData(2, Yvals);
      AttributeValues->setHeightHalfWidthInfo(xvals,Yvals,yvals);

      StatBase[IStartRow] = minRow;
      StatBase[IStartCol] =minCol;
      StatBase[INRows] = maxRow-minRow+1;
      StatBase[INCol] = maxCol-minCol+1;


      StatBase[ITotBoundary ] = TotBoundaryIntensities;
      StatBase[INBoundary] = nBoundaryCells;
      StatBase[IVarBoundary]= TotBoundaryVariances;
      EdgePeak = AttributeValues->setStatBase( StatBase );

      ParameterValues[IBACK] = AttributeValues->getInitBackground();
      ParameterValues[ITINTENS] = AttributeValues->getInitIntensity();
      ParameterValues[IXMEAN] = AttributeValues->getInitCol();
      ParameterValues[IYMEAN] =AttributeValues->getInitRow();
      ParameterValues[IVXX] = AttributeValues->getInitVarx();
      ParameterValues[IVYY] = AttributeValues->getInitVary();
      ParameterValues[IVXY] = AttributeValues->getInitVarxy();
    }


    /**
     * Finds the time channel with the given time in
     * @param X    The vector of time values from a MatrixWorkspace
     * @param time  The desired time
     * @return the time channel
     */
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




    /**
     * Determines if a Peak is an edge peak
     * @param params   The list of parameter values
     * @param nparams  The number of parameters
     *
     * @return true if it is an edge peak, otherwise false.
     */
    bool DataModeHandler::isEdgePeak( const double* params, int nparams)
    {
      double Varx = HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;
      double Vary = Varx;
      if( nparams > 4)
      {
        Varx =params[IVXX];
        Vary = params[IVYY];
      }

      if( Varx <=0 || Vary <= 0 || HalfWidthAtHalfHeightRadius <=0 )
        return true;

      double Rx = lastRCRadius/CellWidth - EdgeX; // span from center in x direction
      double Ry = lastRCRadius/CellHeight - EdgeY;//span from center  in y direction

      if( Rx*Rx <  NStdDevPeakSpan*NStdDevPeakSpan*std::max<double>(Varx,VarxHW) ||
              Ry*Ry < NStdDevPeakSpan*NStdDevPeakSpan*std::max<double>(Vary, VaryHW) )
        return true;

      return false;
    }

    /**
     * Calculates the string for the Function Property of the Fit Algorithm
     *
     * @return the string for the Function Property of the Fit Algorithm
     */
    std::string IntegratePeakTimeSlices::CalculateFunctionProperty_Fit()
    {

      std::ostringstream fun_str;

      fun_str << "name=BivariateNormal,";

      if( AttributeValues->CalcVariances())
              fun_str<<"CalcVariances=1";
          else
              fun_str<<"CalcVariances=-1";


      int NN =  NParameters;
      if( AttributeValues->CalcVariances())
           NN -=3;

      for (int i = 0; i < NN; i++)
      {
        fun_str<<"," << ParameterNames[i] << "=" << ParameterValues[i];

      }



      return fun_str.str();
    }


    /**
     *  Utility to find a name in a vector of strings
     *
     *  @param oneName  The name to search for
     *  @param nameList  The list of names
     *
     *  @return the position in the vector of oneName or -1.
     */
    int IntegratePeakTimeSlices::find( std::string              const &oneName,
                                       std::vector<std::string> const &nameList)

    {
      for (size_t i = 0; i < nameList.size(); i++)
        if (oneName.compare(nameList[i]) == (int) 0)
          return (int) i;

      return -1;
    }

    /**
     * Constructor for class that holds mode and various settings
     *
     * @param handler   Copy(deep) constructor. The other handler to copy info from.
     */
        DataModeHandler::DataModeHandler(const DataModeHandler & handler)
        {

          this->baseRCRadius = handler.baseRCRadius;
          this->lastRCRadius = handler.lastRCRadius;
          this->HalfWidthAtHalfHeightRadius = handler.HalfWidthAtHalfHeightRadius;
          this->calcNewRCRadius = handler.calcNewRCRadius;
          this->lastRow = handler.lastRow;
          this->lastCol = handler.lastCol;
          this->time = handler.time;
          this->CellWidth = handler.CellWidth;
          this->CellHeight = handler.CellHeight;
          this->currentRadius = handler.currentRadius;
          this->currentPosition = handler.currentPosition;
          this->StatBase = handler.StatBase;
          this->EdgeX = handler.EdgeX;
          this->EdgeY = handler.EdgeY;
          this->CalcVariance = handler.CalcVariance;
          this->VarxHW = handler.VarxHW;
          this->VaryHW = handler.VaryHW;
          this->MaxRow = handler.MaxRow;
          this->MaxCol = handler.MaxCol;
          this->MinRow = handler.MinRow;
          this->MinCol = handler.MinCol;
          this->lastISAWIntensity=handler.lastISAWIntensity;
          this->lastISAWVariance = handler.lastISAWIntensity;
          this->back_calc = handler.back_calc;
          this->Intensity_calc = handler.Intensity_calc;
          this->row_calc = handler.row_calc;
          this->col_calc = handler.col_calc;
          this->Vx_calc = handler.Vx_calc;
          this->Vy_calc = handler.Vy_calc;
          this->Vxy_calc = handler.Vxy_calc;
          this->case4 = handler.case4;
        }
    /**
     * Utility method to calculate variances from data given background and means
     *
     * @param background - the background to use
     * @param meanx - the mean x( col) value to use
     * @param meany - the mean y( row) value to use
     * @param Varxx - The resultant variance in the x values around above mean with background removed
     * @param Varyy - The resultant variance in the y values around above mean with background removed
     * @param Varxy - The resultant covariance in the x and values around above mean with background removed
     * @param StatBase - The "data".
     */
        void DataModeHandler::CalcVariancesFromData( double background, double meanx, double meany,
                                                            double &Varxx, double &Varxy, double &Varyy,
                                                            std::vector<double>&StatBase)
        {

          double Den = StatBase[IIntensities]-background*StatBase[ISS1];
          Varxx =(StatBase[ISSIxx]-2*meanx*StatBase[ISSIx]+meanx*meanx*StatBase[IIntensities]-
                   background*(StatBase[ISSxx]-2*meanx*StatBase[ISSx]+meanx*meanx*StatBase[ISS1]))
                   /Den;

          Varyy=(StatBase[ISSIyy]-2*meany*StatBase[ISSIy]+meany*meany*StatBase[IIntensities]-
              background*(StatBase[ISSyy]-2*meany*StatBase[ISSy]+meany*meany*StatBase[ISS1]))
              /Den;

          Varxy =(StatBase[ISSIxy]-meanx*StatBase[ISSIy]-meany*StatBase[ISSIx]+
              meanx*meany*StatBase[IIntensities]-
              background*(StatBase[ISSxy]-meanx*StatBase[ISSy]-meany*StatBase[ISSx]+meanx*meany*StatBase[ISS1]))
              /Den;

          if( CalcVariances())
          {

            Varxx =  std::min<double>(Varxx, 1.21*getInitVarx());//copied from BiVariateNormal
            Varxx = std::max<double>(Varxx, .79*getInitVarx());
            Varyy =  std::min<double>(Varyy, 1.21*getInitVary());
            Varyy = std::max<double>(Varyy, .79*getInitVary());
          }

        }
    /**
     * Calculates the string form of the constraints to be sent to the Fit Algorithm and also saves it in a vector form
     * @param   Bounds  The vector form for the constraints
     * @param   CalcVariances The to trigger variance calculation
     */
        std::string DataModeHandler::CalcConstraints( std::vector< std::pair<double,double> > & Bounds,
               bool CalcVariances)
        {
          double TotIntensity = StatBase[IIntensities];
          double ncells       =StatBase[ISS1];
          double Variance = StatBase[IVariance];
          double TotBoundaryIntensities= StatBase[ITotBoundary ];
          double TotBoundaryVariances = StatBase[IVarBoundary];

          double nBoundaryCells=   StatBase[INBoundary];
          double back = TotBoundaryIntensities/nBoundaryCells;
          double backVar= std::max<double>(nBoundaryCells/50.0,TotBoundaryVariances)/nBoundaryCells/nBoundaryCells;
          double IntensVar = Variance +ncells*ncells*backVar;

          double relError = .25;

          if( back_calc != back )
            relError = .45;

          int N = NParameters;
          if( CalcVariances)
            N = N-3;

          double NSigs=NStdDevPeakSpan;
          if( back_calc >0)
             NSigs = std::max<double>( NStdDevPeakSpan,7-5*back_calc/back);//background too high
          ostringstream str;

          NSigs *=max<double>(1.0, Intensity_calc/(TotIntensity -ncells*back_calc));
          str<< max<double>(0.0, back_calc-NSigs*(1+relError)*sqrt(backVar))<<"<Background<"<<(back+NSigs*(1.8+relError)*sqrt(backVar))
             <<","<< max<double>(0.0, Intensity_calc-NSigs*(1+relError)*sqrt(IntensVar))<<
             "<Intensity<"<<Intensity_calc+NSigs*(1+relError)*sqrt(IntensVar);

          double min =max<double>(0.0, back_calc-NSigs*(1+relError)*sqrt(backVar));
          double maxx =back+NSigs*(1.8+relError)*sqrt(backVar);
          Bounds.push_back( pair<double,double>(min,maxx ));
          Bounds.push_back( pair<double,double>(max<double>(0.0, Intensity_calc-NSigs*(1+relError)*sqrt(IntensVar)),
                                            Intensity_calc+NSigs*(1+relError)*sqrt(IntensVar)));
          double relErr1= relError*.75;
          double val =col_calc;
          double minn= std::max<double>(MinCol-.5, (1-relErr1)*val);
          maxx =std::min<double>((1+relErr1)*val, MaxCol+.5);

          str<<","<<minn <<"<"<<"Mcol"<<"<"<<maxx;
          Bounds.push_back(pair<double,double>(minn,maxx));

          val = row_calc;

          minn= std::max<double>(MinRow-.5, (1-relErr1)*val);
          maxx =std::min<double>((1+relErr1)*val, MaxRow+.5);
          str<<","<<minn <<"<"<<"Mrow"<<"<"<<maxx;
          Bounds.push_back(pair<double,double>(minn,maxx));

          if( N >=5)
          {
            val = Vx_calc;
            double valmin= val;
            double valmax = val;
            if( VarxHW>0)
            {
              valmin = std::min<double>(val, VarxHW);
              valmax = std::max<double>(val, VarxHW);

            }


            relErr1 *=.6;//Edge peaks: need to restrict sigmas.
            str<<","<<(1-relErr1)*valmin <<"<"<<"SScol"<<"<"<<(1+relErr1)*valmax;
            Bounds.push_back(pair<double,double>((1-relErr1)*valmin,(1+relErr1)*valmax));

            val = Vy_calc;
            valmin= val;
            valmax = val;
            if( VaryHW>0)
            {
              valmin = std::min<double>(val, VaryHW);
              valmax = std::max<double>(val, VaryHW);

            }
            str<<","<<(1-relErr1)*valmin <<"<"<<"SSrow"<<"<"<<(1+relErr1)*valmax;
            Bounds.push_back(pair<double,double>((1-relErr1)*valmin,(1+relErr1)*valmax));


          }

          return str.str();
        }



       /**
        * Sets up data for the Fit Algorithm call and invokes it.
        * @param Data    The workspace with experimental results
        * @param chisqOverDOF  the chi squared over degrees of freedom result from the Fit Algorithm
        * @param done   Usually true except if there is not enough data or Fit Algorithm was not called
        * @param names  The parameter names
        * @param params  The parameter values from the Fit Algorithm
        * @param errs The parameter errorfrom the Fit Algorithm
        * @param lastRow  The previous row( for log info only)
        * @param lastCol  The previous col( for log info only)
        * @param neighborRadius The neighborhood radius( for log info only)
        */
       void IntegratePeakTimeSlices::Fit(MatrixWorkspace_sptr &Data,double &chisqOverDOF,
               bool &done,       std::vector<string>&names, std::vector<double>&params,
               std::vector<double>&errs
               ,double lastRow,double lastCol,double neighborRadius)
           {

              bool CalcVars = AttributeValues->CalcVariances();
              std::vector<std::pair<double, double> > Bounds;
              std::string Constraints = AttributeValues->CalcConstraints(Bounds, CalcVars);
              IAlgorithm_sptr fit_alg;
              fit_alg = createChildAlgorithm("Fit");
              std::string fun_str = CalculateFunctionProperty_Fit();

              std::string SSS("   Fit string ");
              SSS += fun_str;
              g_log.debug(SSS);
              g_log.debug()<<"      TotCount=" << AttributeValues->StatBase[IIntensities] << std::endl;

              fit_alg->setPropertyValue("Function", fun_str);

              fit_alg->setProperty("InputWorkspace", Data);
              fit_alg->setProperty("WorkspaceIndex", 0);
              fit_alg->setProperty("StartX", 0.0);
              fit_alg->setProperty("EndX", 0.0 + (double) NeighborIDs[1]);
              fit_alg->setProperty("MaxIterations", 5000);
              fit_alg->setProperty("CreateOutput", true);

              fit_alg->setProperty("Output", "out");

              fit_alg->setProperty("MaxIterations", 50);

              std::string tie = getProperty("Ties");
              if (tie.length() > (size_t) 0)
                fit_alg->setProperty("Ties", tie);
              if (Constraints.length() > (size_t) 0)
                fit_alg->setProperty("Constraints", Constraints);
              try
              {
                fit_alg->executeAsChildAlg();

                chisqOverDOF = fit_alg->getProperty("OutputChi2overDoF");
                std::string outputStatus = fit_alg->getProperty( "OutputStatus" );
                g_log.debug()<<"Chisq/OutputStatus="<< chisqOverDOF<< "/"<< outputStatus<< std::endl;

                names.clear(); params.clear();errs.clear();
                ITableWorkspace_sptr RRes = fit_alg->getProperty( "OutputParameters");
                for( int prm = 0; prm < (int)RRes->rowCount()-1; prm++ )
                {
                  names.push_back( RRes->getRef< string >( "Name", prm ) );
                  params.push_back( RRes->getRef< double >( "Value", prm ));
                  double error = RRes->getRef< double >( "Error",prm);
                  errs.push_back( error );
                }
                if( names.size()< 5)
                {
                  names.push_back( ParameterNames[IVXX] );
                  names.push_back( ParameterNames[IVYY] );
                  names.push_back( ParameterNames[IVXY] );
                  double Varxx, Varxy,Varyy;
                  AttributeValues->CalcVariancesFromData( params[IBACK], params[IXMEAN], params[IYMEAN],
                      Varxx, Varxy, Varyy, AttributeValues->StatBase);
                  params.push_back( Varxx );
                  params.push_back( Varyy );
                  params.push_back( Varxy );
                  errs.push_back( 0 );
                  errs.push_back( 0 );
                  errs.push_back( 0 );

                }

              } catch (std::exception &Ex1)//ties or something else went wrong in BivariateNormal
              {
                done = true;
                g_log.error()<<"Bivariate Error for PeakNum="<<(int)getProperty("PeakIndex")<<":" << std::string(Ex1.what())
                     <<std::endl;
              }catch(...)
              {
                done = true;
                g_log.error()<<"Bivariate Error A for peakNum="<<(int)getProperty("PeakIndex")<<std::endl;
              }
              if (!done)//Bivariate error happened
              {

               g_log.debug() << "   Thru Algorithm: chiSq=" << setw(7) << chisqOverDOF << endl;
               g_log.debug() << "  Row,Col Radius=" << lastRow << "," << lastCol << "," << neighborRadius << std::endl;

                double sqrtChisq = -1;
                if (chisqOverDOF >= 0)
                  sqrtChisq = (chisqOverDOF);

                sqrtChisq = max<double> (sqrtChisq, AttributeValues->StatBaseVals(IIntensities)
                    / AttributeValues->StatBaseVals(ISS1));
                sqrtChisq = SQRT(sqrtChisq);

                for (size_t kk = 0; kk < params.size(); kk++)
                {
                  g_log.debug() << "   Parameter " << setw(8) << names[kk] << " " << setw(8) << params[kk];
                  // if (names[kk].substr(0, 2) != string("SS"))
                  g_log.debug() << "(" << setw(8) << (errs[kk] * sqrtChisq) << ")";
                  if (Bounds.size() > kk)
                  {
                    pair<double, double> upLow = Bounds[kk];
                    g_log.debug() << " Bounds(" << upLow.first << "," << upLow.second << ")";
                  }
                  g_log.debug() << endl;
                }

                double intensity = AttributeValues->CalcISAWIntensity( params.data());
                g_log.debug() << "IsawIntensity= "<< intensity << std::endl;
              }

           }

       /**
        * Not used. Did 3 Fit Algorithm calls and returned the one with the smallest chiSqOverDOF.
        * Did not work well
         * @param Data    The workspace with experimental results
        * @param chisqOverDOF  the chi squared over degrees of freedom result from the Fit Algorithm
        * @param done   Usually true except if there is not enough data or Fit Algorithm was not called
        * @param names  The parameter names
        * @param params  The parameter values from the Fit Algorithm
        * @param errs The parameter errorfrom the Fit Algorithm
        * @param lastRow  The previous row( for log info only)
        * @param lastCol  The previous col( for log info only)
        * @param neighborRadius The neighborhood radius( for log info only)
        */
      void IntegratePeakTimeSlices::PreFit(MatrixWorkspace_sptr &Data,double &chisqOverDOF,
           bool &done,       std::vector<string>&names, std::vector<double>&params,
           std::vector<double>&errs,double lastRow,double lastCol,double neighborRadius)
       {

        double background = ParameterValues[IBACK];
        int N = 3;
        if( background <=0)
        {
          background =0;
          N=1;
        }
        bool CalcVars = AttributeValues->CalcVariances();
        int NParams =4;
        if( !CalcVars )NParams+=3;

        double minChiSqOverDOF = -1;
        double Bestparams[7];
        std::string Bestnames[7];
        for( int i=0; i < N;i++)
        {
          Fit(Data,chisqOverDOF,done,names, params,errs,lastRow,lastCol,neighborRadius);
          g_log.debug()<<"-----------------------"<<i<<"--------------------------"<<std::endl;
          if(( minChiSqOverDOF < 0 || chisqOverDOF <  minChiSqOverDOF )&& (chisqOverDOF > 0) && !done)
          {
            for(int j=0;j<NParams;j++)
              {
               Bestparams[j] = ParameterValues[j];
               Bestnames[j] = ParameterNames[j];
              }
            minChiSqOverDOF = chisqOverDOF;
          }

          //Next round, reduce background
          background = background/2;
          if( i+1 == N-1)
            background =0;

          std::vector<double> prms= AttributeValues->GetParams( background);

          for( int j=0; j< NParams ; j++)
            ParameterValues[j] = prms[j];
        }
        vector<std::string> ParNames( ParameterNames, ParameterNames +NParams);
        for( int i=0; i < NParams; i++)
        {
          int k= find( Bestnames[i] , ParNames);
          if( k >=0 && k < NParams)
            ParameterValues[k] = Bestparams[k];

        }

        Fit(Data,chisqOverDOF,done,names, params,errs,lastRow,lastCol,neighborRadius);

       }

    /**
     * Determines if the list of parameters and errors represent a "good" fit
     *
     * @param params    The list of parameter values
     * @param errs      The list of the errors in the parameter values
     * @param names     The names of the parameters
     * @param chisqOverDOF  The fitting error
     *
     * @return  true if the fit is good,otherwise false
     */
    bool IntegratePeakTimeSlices::isGoodFit(std::vector<double >              const & params,
                                            std::vector<double >              const & errs,
                                            std::vector<std::string >         const &names,
                                            double chisqOverDOF)
    {
      int Ibk = find("Background", names);
      int IIntensity = find("Intensity", names);

      if (chisqOverDOF < 0)
          {

              g_log.debug()<<"   Bad Slice- negative chiSq= "<<chisqOverDOF<<std::endl;;
             return false;
          }

      int NBadEdgeCells = getProperty("NBadEdgePixels");
      NBadEdgeCells = (int)(.6*NBadEdgeCells);
      if( params[IXMEAN] < NBadEdgeCells || params[IYMEAN] < NBadEdgeCells ||
          params[IXMEAN]> NCOLS-NBadEdgeCells || params[IYMEAN] > NROWS-NBadEdgeCells)
        return false;



      int ncells = (int) (AttributeValues->StatBaseVals(ISS1));

      if (AttributeValues->StatBaseVals(IIntensities) <= 0 || (AttributeValues->StatBaseVals(IIntensities) - params[Ibk] * ncells)
          <= 0)
      {

           g_log.debug()<<"   Bad Slice. Negative Counts= "<<AttributeValues->StatBaseVals(IIntensities) - params[Ibk] * ncells
               <<std::endl;;
        return false;
      }

      double x = params[IIntensity] / (AttributeValues->StatBaseVals(IIntensities) - params[Ibk] * ncells);

      if ((x < MinGoodRatioFitvsExpIntenisites || x > MaxGoodRatioFitvsExpIntenisites)&& !EdgePeak)// The fitted intensity should be close to tot intensity - background
      {
        g_log.debug()<< "   Bad Slice. Fitted Intensity & Observed Intensity(-back) too different. ratio="
                                               <<x<<std::endl;

        return false;
      }

      bool GoodNums = true;
      bool paramBad=false;
      size_t BadParamNum = static_cast<size_t>(-1);
      for (size_t i = 0; i < errs.size(); i++)
        if (errs[i] != errs[i])
        {
          GoodNums = false;
          paramBad=false;
          BadParamNum=i;
        }
        else if( errs[i] < 0)
        {
          GoodNums = false;
          paramBad=false;
          BadParamNum=i;
        }
        else if (params[i] != params[i])
        {
          GoodNums = false;
          paramBad=true;
          BadParamNum=i;
        }

      if (!GoodNums)
        {
          std::string obj=" parameter ";
          if( !paramBad )
            obj=" error ";
          g_log.debug()<<"   Bad Slice."<<obj<<BadParamNum<<
                    " is not a number"<< std::endl;
        }

      if( !GoodNums)
        return false;

      GoodNums = true;

      std::string Err("back ground is negative");
      if (params[Ibk] < -.002)
        GoodNums = false;

      if( GoodNums)Err ="Intensity is negative";
      if (params[IIntensity] < 0)
        GoodNums = false;


      double IsawIntensity= AttributeValues->CalcISAWIntensity( params.data());
      double IsawVariance = AttributeValues->CalcISAWIntensityVariance(params.data(), errs.data(),chisqOverDOF);
       if(GoodNums) Err ="Isaw Variance is negative";
      if (IsawVariance > 0)
        {
         if( GoodNums)Err= "I/sigI > 3";
         if (IsawIntensity*IsawIntensity/IsawVariance < MinGoodIoverSigI*MinGoodIoverSigI )
           GoodNums = false;
        }
      else
        GoodNums = false;


      if (!GoodNums)

      {   g_log.debug()<<  Err <<std::endl;

          return false;
      }


      //Check weak peak. Max theoretical height should be more than 3

      double maxPeakHeightTheoretical = params[ITINTENS] / 2 / M_PI / sqrt(params[IVXX] * params[IVYY]
          - params[IVXY] * params[IVXY]);

      double AvHeight = AttributeValues->StatBaseVals(IIntensities)/AttributeValues->StatBaseVals(ISS1)-params[IBACK];

      if (maxPeakHeightTheoretical < 2*AvHeight || AvHeight < 0 || maxPeakHeightTheoretical < 0)
      {

           g_log.debug()<<"   Bad Slice. Peak too small= "<<maxPeakHeightTheoretical<<
                  "/"<<AvHeight<<std::endl;
        return false;
      }

      double Nrows = std::max<double>(AttributeValues->StatBase[INRows], AttributeValues->StatBase[INCol]);
      if( maxPeakHeightTheoretical < 1 && (params[IVXX] > Nrows*Nrows/4 || params[IVYY] > Nrows*Nrows/4))
      {
        g_log.debug()<<"Peak is too flat "<<std::endl;
        return false;
      }

      //Exponential too steep, i.e. intensities at pixels 1 from center are <3* intensity center
      if( params[IVXX]+params[IVYY] >
               2.6*(params[IVXX]*params[IVYY]-params[IVXY]*params[IVXY]))
          {
              g_log.debug()<<"      Bad Slice. Too steep of an exponential"<< std::endl;
             return false;
          }

      return true;
    }


    /**
     * Calculates if there is enough data to for there to be a peak
     *
     * @param ParameterValues pointer to the parameter values
     * @return  true if there is enough data, otherwise false
     */
   bool DataModeHandler::IsEnoughData( const double *ParameterValues, Kernel::Logger&  )
    {
      // Check if flat
    double Varx,Vary, Cov;

      if( StatBase.size() <=0)
        return false;

      double ncells = (int)StatBase[IIntensities];
      if( ncells <=0)
        return false;

      double meanx = StatBase[ISSIx]/ncells;
      double meany= StatBase[ISSIy]/ncells;

      if(   !CalcVariances() )
      {
        meanx = ParameterValues[IXMEAN];
        meany = ParameterValues[IYMEAN];
        Varx =ParameterValues[IVXX];
        Vary = ParameterValues[IVYY];
        Cov  =ParameterValues[IVXY];

      }else
         CalcVariancesFromData( ParameterValues[0], meanx,meany, Varx,Cov,Vary, StatBase);

      if( Varx <MinVariationInXYvalues || Vary <MinVariationInXYvalues)  //All data essentially the same.
         return false;

      if( Cov*Cov > MaxCorrCoeffinXY*Varx*Vary)//All data on a obtuse line
        return false;




      return true;

    }


    /**
     * Calculates the error in integration closest to the latest ISAW calculations
     *
     * @param background    The background
     * @param backError    The error in this background value
     * @param ChiSqOverDOF  The fitting error
     * @param TotVariance    The Total square of the intensity errors in all the cells considered
     * @param ncells           The number of cells
     *
     * @return  The error in the integrated intensity
     */
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
       
      return SQRT(Variance);

    }


    /**
     * Initializes the column names in the output table workspace
     *
     * @param TabWS  The TableWorkspace
     */
    void IntegratePeakTimeSlices::InitializeColumnNamesInTableWorkspace(
                                                    TableWorkspace_sptr &TabWS)
    {
      //TabWS->setName("Log Table");
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
      TabWS->addColumn("double", "TotIntensityError");
      TabWS->addColumn("str", "SpecIDs");
    }




    /**
     * Updates the information in the output OutputWorkspace
     * @param TabWS  The OutputWorkspace
     * @param dir    The direction the time slices are going( >0 means increasing otherwise decreasing)
     * @param chan   The number of channels away from the center channel this data corresponds to
     * @param params  The parameter values
     * @param errs    The error in these parameter values
     * @param names   The names of the parameters
     * @param Chisq   The fitting error
     * @param time     The time for this channel
     * @param spec_idList  The list of spectral id's used to integrate this time channel
     *
     * @return The last row updated
     */
    int IntegratePeakTimeSlices::UpdateOutputWS( DataObjects::TableWorkspace_sptr         &TabWS,
                                                  const int                                  dir,
                                                  const double                                  chan,
                                                  std::vector<double >                 const &params,
                                                  std::vector<double >                 const &errs,
                                                  std::vector<std::string>             const &names,
                                                  const double                               Chisq,
                                                  const double time,
                                                  string                                  spec_idList)
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

      int ncells = (int) (AttributeValues->StatBaseVals(ISS1));
      double chisq= max<double>(Chisq, AttributeValues->StatBaseVals(IIntensities)/max<int>(ncells,1));

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

      TabWS->getRef<double> (std::string("TotIntensity"), TableRow) = AttributeValues->StatBaseVals(IIntensities);
      TabWS->getRef<double> (std::string("BackgroundError"), TableRow) = errs[Ibk] * SQRT(chisq);
      TabWS->getRef<double> (std::string("ISAWIntensity"), TableRow) = AttributeValues->CalcISAWIntensity(params.data());
          //AttributeValues->StatBaseVals(IIntensities)
          //                                                                   - params[Ibk] * ncells;

      TabWS->getRef<double> (std::string("ISAWIntensityError"), TableRow) = sqrt(AttributeValues->
          CalcISAWIntensityVariance(params.data(),errs.data(), Chisq));

          //CalculateIsawIntegrateError(
          //params[Ibk], errs[Ibk], chisq, AttributeValues->StatBaseVals(IVariance), ncells);
      
      TabWS->getRef<double> (std::string("Time"), TableRow) = time;

      TabWS->getRef<double>(std::string("TotalBoundary"), TableRow)= AttributeValues->StatBaseVals( ITotBoundary);
      TabWS->getRef<double>(std::string("NBoundaryCells"), TableRow)= AttributeValues->StatBaseVals( INBoundary);


      TabWS->getRef<double> (std::string("Start Row"), TableRow) = AttributeValues->StatBaseVals(IStartRow);
      TabWS->getRef<double> (std::string("End Row"), TableRow) = AttributeValues->StatBaseVals(IStartRow)
          + AttributeValues->StatBaseVals(INRows) - 1;

      TabWS->getRef<double> (std::string("Start Col"), TableRow) = AttributeValues->StatBaseVals(IStartCol);
      TabWS->getRef<double> (std::string("End Col"), TableRow) = AttributeValues->StatBaseVals(IStartCol)
          + AttributeValues->StatBaseVals(INCol) - 1;
      TabWS->getRef<double> (std::string("TotIntensityError"), TableRow) = SQRT(AttributeValues->StatBaseVals(IVariance));
      TabWS->getRef<string>( std::string("SpecIDs"), TableRow ) = spec_idList;

      return newRowIndex;
    }

    
    /**
     * Updates AttributeValues with this peak information from this time slice
     * @param params  The parameter values
     * @param errs    The error in these parameter values
     * @param names   The names of the parameters
     * @param TotVariance  The total of the squares of the intensity errors from the data
     * @param TotIntensity  The total of the intensities so far
     * @param  TotSliceIntensity   The total intensity for this slice
     * @param  TotSliceVariance   The total square of intensity errors for this time slice
     * @param chisqdivDOF   The fitting error
     * @param ncells       The number of cells
     */
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
      UNUSED_ARG( TotSliceIntensity);
      UNUSED_ARG( TotSliceVariance );
      UNUSED_ARG( names );
      UNUSED_ARG( ncells);

      double err = 0;
      double intensity = 0;

      err = AttributeValues->CalcISAWIntensityVariance(params.data(), errs.data(), chisqdivDOF);

      intensity = AttributeValues->CalcISAWIntensity(params.data());
      TotIntensity += intensity;

      TotVariance += err;
      g_log.debug() << "TotIntensity/TotVariance=" << TotIntensity << "/" << TotVariance << std::endl;

    }

    /**
     * Determines whether the Variances can be calculated. If specified yes and the peak is an edge peak then
     * the Variances cannot be calculated.
     *
     * @return true if the Variances desire to be and can be calculated, otherwise false
     */
    bool DataModeHandler::CalcVariances( )
    {
      if( !CalcVariance)
        return false;

      double param[7]={back_calc,Intensity_calc,col_calc,row_calc,Vx_calc,Vy_calc, Vxy_calc};
      return !isEdgePeak( param,7);

    }

   /**
    * Calculates the Intensity designed for Edge Peaks
    *
    * @param params The Fitted parameters
    *
    * @return this calculated intensity
    *
    * Also the variable lastISAWIntensity was updated.
    */
   double DataModeHandler::CalcISAWIntensity(const double* params)
   {

     double ExperimentalIntensity  = StatBase[IIntensities]-
                                         params[IBACK]*StatBase[ISS1];

     double r= CalcSampleIntensityMultiplier( params );

     double alpha =(float)( 0+.5*( r-1 ) );
     alpha = std::min<double>( 1.0f , alpha );

     lastISAWIntensity = ExperimentalIntensity*r;//*( 1-alpha )+ alpha * FitIntensity;
     return lastISAWIntensity;

   }
   /**
      * Calculates the Error in the Intensity designed for Edge Peaks
      *
      * @param params The fitted parameter values
      * @param errs    The error in these fitted parameters
      * @param chiSqOvDOF The fitting error
      *
      * @return this calculated error in the intensity

      */
   double DataModeHandler::CalcISAWIntensityVariance( const double* params,const double* errs, double chiSqOvDOF)
   {

     int ncells = (int)StatBase[ISS1];
     double B = StatBase[IVariance] / ncells;
      if( B < chiSqOvDOF)
         B = chiSqOvDOF;

     double ExperimVar = StatBase[IVariance];
     double IntensityBackError = errs[IBACK] * sqrt(B);


     ExperimVar += IntensityBackError * IntensityBackError * ncells*ncells + params[IBACK]*ncells;


     double r= CalcSampleIntensityMultiplier( params );
     double alpha =(float)( 0+.5*( r - 1 ));
      alpha = std::min<double>( 1.0f , alpha );

     lastISAWVariance =  ExperimVar*r*r;//*( 1 - alpha ) + alpha * FitVar;
     return lastISAWVariance;
   }


   /**
    * Calculates the multiplier of TotIntensity - background for edge peaks to correspond to the missing data.
    * @param params the parameter values
    *
    * @return  The multiplier the should adjust the TotIntensity-background for an EdgePeak
    */
   double DataModeHandler::CalcSampleIntensityMultiplier( const double* params)const
   {
           int MinRow = (int)StatBase[ IStartRow ];
           int MaxRow = MinRow+(int)StatBase[ INRows ]-1;
           int MinCol = (int)StatBase[ IStartCol ];
           int MaxCol= MinCol + (int)StatBase[ INCol ]-1;
           double r=1;

           if( params[IVXX] <=0 || params[IVYY]<=0)
             return 1.0;


           //  NstdX iand NstdY are the number of 1/4 standard deviations. Elements of probs are in
           //    1/4 standard deviations
           double NstdX = 4*min<double>(  params[ IXMEAN ]-MinCol , MaxCol-params[ IXMEAN ] )
                            /sqrt(params[ IVXX ]);

           double NstdY = 4*min<double>(  params[ IYMEAN ] - MinRow , MaxRow-params[ IYMEAN ] )
                         /sqrt(params[IVYY]);

           double sgn = 1;
           if( NstdX < 0 ){ sgn = -1; }
           double P=1;
           if( sgn*NstdX <9)
           {
             int xx= (int)(sgn*NstdX);
             double a=probs[xx];
             double b=1;
             if( xx+1<=8)
               b=probs[xx+1];
             P=a+(b-a)*(sgn*NstdX-xx);

           }
           if( NstdX >= 7.5 )r = 1.0;
           else if( sgn > 0 ) r = 1/P;
           else  r = 1/(1-P  );

          if( NstdY < 0 ){ sgn = -1; }
          P = 1;
          if (sgn * NstdY < 9)
          {
            int xx = (int) (sgn * NstdY);
            double a = probs[xx];
            double b = 1;
            if (xx + 1 <= 8)
               b = probs[xx + 1];
             P = a + (b - a) * (sgn * NstdY - xx);

          }
      if (NstdY >= 7.5)
        r *= 1.0;
          else if( sgn >0)r *= 1/P;
           else  r *= 1/(1-P);

           r = std::max<double>( r , 1.0 );
           return r;
   }

}//namespace Crystal
}//namespace Mantid
//Attr indicies
