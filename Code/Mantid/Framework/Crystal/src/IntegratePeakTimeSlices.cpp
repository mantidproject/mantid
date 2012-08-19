/*WIKI* 


This algorithm fits a bivariate normal distribution( plus background) to the data on each time slice.  The Fit program uses [[BivariateNormal]] for the Fit Function.

The area used for the fitting is calculated based on the dQ parameter.  A good value for dQ is 1/largest unit cell length. This parameter dictates the size of the area used to approximate the intensity of the peak. The estimate .1667/ max(a,b,c) assumes |Q|=1/d.

The result is returned in this algorithm's output "Intensity" and "SigmaIntensity" properties. The peak object is NOT CHANGED.


The table workspace is also a result. Each line contains information on the fit for each good time slice.  The column names( and information) in the table are:
  Time, Channel, Background, Intensity, Mcol, Mrow, SScol,SSrow, SSrc, NCells,
  ChiSqrOverDOF, TotIntensity, BackgroundError, FitIntensityError, ISAWIntensity,
   ISAWIntensityError,TotalBoundary, NBoundaryCells, Start Row, End Row,Start Col, and End Col.
   The last column has a comma separated List of sepctral ID's used in the time slice.


The final Peak intensity is the sum of the IsawIntensity for each time slice. The error is the square root of the sum of squares of the IsawIntensityError values.

The columns whose names are  Background, Intensity, Mcol, Mrow, SScol, SSrow, and SSrc correspond to the parameters for the BivariateNormal curve fitting function.

This algorithm has been carefully tweaked to give good results for interior peaks only. Peaks close to the edge of the detector may not give good results.

This Algorithm is also used by the [[PeakIntegration]] algorithm when the Fit tag is selected.

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
#include "MantidAPI/IFunction.h"
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
#include <boost/lexical_cast.hpp>
#include <vector>
#include "MantidAPI/Algorithm.h"
#include <algorithm>
#include <math.h>
#include <cstdio>
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

#define NParameters  7



    double probs[9]={.5f,.5987f,.6915f,.7734f,.8413f,.8944f,.9322f,.9599f,.9772f};
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
      delete wi_to_detid_map;
      delete [] NeighborIDs;
    }

    void IntegratePeakTimeSlices::initDocs()
    {
      this->setWikiSummary("Integrates each time slice around a peak ");
      this->setOptionalMessage("The algorithm uses CurveFitting::BivariateNormal for fitting a time slice");
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

      declareProperty("PeakQspan", .06, "Max magnitude of Q of Peak to Q of Peak Center, where |Q|=1/d");

      declareProperty("CalculateVariances", true ,"Calc (co)variances given parameter values versus fit (co)Variances ");

      declareProperty("Ties","","Tie parameters(Background,Intensity, Mrow,...) to values/formulas.");

      declareProperty("NBadEdgePixels", 0, "Number of  bad Edge Pixels");


      declareProperty("Intensity", 0.0, "Peak Integrated Intensity", Direction::Output);

      declareProperty("SigmaIntensity",0.0,"Peak Integrated Intensity Error", Direction::Output);


    }

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

      this->back_calc = handler.back_calc;
      this->Intensity_calc = handler.Intensity_calc;
      this->row_calc = handler.row_calc;
      this->col_calc = handler.col_calc;
      this->Vx_calc = handler.Vx_calc;
      this->Vy_calc = handler.Vy_calc;
      this->Vxy_calc = handler.Vxy_calc;
      this->case4 = handler.case4;
    }
    void DataModeHandler::CalcVariancesFromData( double background, double meanx, double meany,
                                                        double &Varxx, double &Varxy, double &Varyy,
                                                        std::vector<double>&ParameterValues)
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
      if( ParameterValues.size() >IVXX)
      {
        Varxx =  std::min<double>(Varxx, 1.21*ParameterValues[IVXX]);//copied from BiVariateNormal
        Varxx = std::max<double>(Varxx, .79*ParameterValues[IVXX]);
        Varyy =  std::min<double>(Varyy, 1.21*ParameterValues[IVYY]);
        Varyy = std::max<double>(Varyy, .79*ParameterValues[IVYY]);
      }

    };

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
      double backVar= TotBoundaryVariances/nBoundaryCells/nBoundaryCells;
      double IntensVar = Variance +ncells*ncells*backVar;

      double relError = .15;

      if( back_calc != back )
        relError = .35;

      int N = NParameters;
      if( CalcVariances)
        N = N-3;


      ostringstream str;
      str<< max<double>(0.0, back_calc-(1+2*relError)*sqrt(backVar))<<"<Background<"<<(back+(1+2*relError)*sqrt(backVar))
         <<","<< max<double>(0.0, Intensity_calc-(1+2*relError)*sqrt(IntensVar))<<
         "<Intensity<"<<TotIntensity-ncells*back+(1+2*relError)*sqrt(IntensVar);
      double min =max<double>(0.0, back_calc-(1+2*relError)*sqrt(backVar));
      double maxx =back+(1+2*relError)*sqrt(backVar);
      Bounds.push_back( pair<double,double>(min,maxx ));
      Bounds.push_back( pair<double,double>(max<double>(0.0, Intensity_calc-(1+2*relError)*sqrt(IntensVar)),TotIntensity-ncells*back+(1+2*relError)*sqrt(IntensVar)));
     /* for( int i=2; i<N; i++ )
      {
        if( i>0)
          str<<",";


        double val = ParameterValues[i];

        double relErr1=.75*relError;


        str<<(1-relErr1)*val <<"<"<<ParameterNames[i]<<"<"<<(1+relErr1)*val;
        Bounds.push_back(pair<double,double>((1-relErr1)*val,(1+relErr1)*val));
      }
     */
      double val =col_calc;

      double relErr1=.75*relError;


      str<<","<<(1-relErr1)*val <<"<"<<"Mcol"<<"<"<<(1+relErr1)*val;
      Bounds.push_back(pair<double,double>((1-relErr1)*val,(1+relErr1)*val));

      val = row_calc;
      str<<","<<(1-relErr1)*val <<"<"<<"Mrow"<<"<"<<(1+relErr1)*val;
      Bounds.push_back(pair<double,double>((1-relErr1)*val,(1+relErr1)*val));

      if( N >=5)
      {val = Vx_calc;
      str<<","<<(1-relErr1)*val <<"<"<<"SScol"<<"<"<<(1+relErr1)*val;
      Bounds.push_back(pair<double,double>((1-relErr1)*val,(1+relErr1)*val));

      val = Vy_calc;
      str<<","<<(1-relErr1)*val <<"<"<<"SSrow"<<"<"<<(1+relErr1)*val;
      Bounds.push_back(pair<double,double>((1-relErr1)*val,(1+relErr1)*val));


      val = Vxy_calc;
      str<<","<<(1-relErr1)*val <<"<"<<"SSrc"<<"<"<<(1+relErr1)*val;
      Bounds.push_back(pair<double,double>((1-relErr1)*val,(1+relErr1)*val));

      }

      return str.str();
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
   void IntegratePeakTimeSlices::Fit(MatrixWorkspace_sptr &Data,double &chisq,
       bool &done,       std::vector<string>&names, std::vector<double>&params,
       std::vector<double>&errs,double lastRow,double lastCol,double neighborRadius)
   {

     std::vector<std::pair<double, double> > Bounds;
      IAlgorithm_sptr fit_alg;
      fit_alg = createSubAlgorithm("Fit");
      std::string fun_str = CalculateFunctionProperty_Fit();

      std::string SSS("   Fit string ");
      SSS += fun_str;
      g_log.debug(SSS);

      bool CalcVars = AttributeValues->CalcVariances();
      std::string Constraints = AttributeValues->CalcConstraints(Bounds, CalcVars);
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
        fit_alg->executeAsSubAlg();

        chisq = fit_alg->getProperty("OutputChi2overDoF");

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
              Varxx, Varxy, Varyy, params);
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
        g_log.debug("Bivariate Error :" + std::string(Ex1.what()));
      }
      if (!done)//Bivariate error happened
      {

       g_log.debug() << "   Thru Algorithm: chiSq=" << setw(7) << chisq << endl;
       g_log.debug() << "  Row,Col Radius=" << lastRow << "," << lastCol << "," << neighborRadius << std::endl;

        double sqrtChisq = -1;
        if (chisq >= 0)
          sqrtChisq = (chisq);

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


      }

   }

   bool IntegratePeakTimeSlices::updateNeighbors( boost::shared_ptr< Geometry::IComponent> &comp,
       V3D CentPos,  V3D oldCenter,double NewRadius, double &neighborRadius)
   {
       double DD =(CentPos - oldCenter).norm();
       bool  changed = false;
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
          //CentNghbr = CentPos;
          getNeighborPixIDs(comp, CentPos, neighborRadius, NeighborIDs);
          changed=true;

       }else// big enough neighborhood so
         neighborRadius -= DD;

       return changed;
   }
    void IntegratePeakTimeSlices::exec()
    {

      double dQ= getProperty("PeakQspan");

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

      FindPlane(  center,  xvec,  yvec, ROW, COL,NROWS,NCOLS, CellWidth, CellHeight, peak);

     // sprintf(logInfo, std::string("   Peak Index %5d\n").c_str(), indx);
      g_log.debug() <<"   Peak Index "<<indx <<std::endl;

      double TotVariance = 0;
      double TotIntensity = 0;
      double lastRow = ROW;
      double Row0 = lastRow;
      double lastCol = COL;
      double Col0 = lastCol;
      string spec_idList="";

      // For quickly looking up workspace index from det id
      wi_to_detid_map = inpWkSpace->getDetectorIDToWorkspaceIndexMap( false );

      TableWorkspace_sptr TabWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(0));



      try
      {

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


        double Centy = Row0;
        double Centx = Col0;
        IDetector_const_sptr CenterDet =  peak.getDetector();
        //std::map< specid_t, V3D > neighbors;


        bool done = false;

        double neighborRadius ;//last radius for finding neighbors

        neighborRadius = min<double>(10,1.5*R);
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

        int MaxChan = -1;
        double MaxCounts = -1;

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

              boost::shared_ptr<DataModeHandler> XXX( new DataModeHandler(R,R,Centx,Centy,CellWidth,CellHeight,
                                                   getProperty("CalculateVariances")));
              AttributeValues = XXX;

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

        if( MaxChan >0)
            Chan =  MaxChan ;

      //  sprintf(logInfo, std::string("   largest Channel,Radius,CellWidth,CellHeight = %d  %7.3f  %7.3f  %7.3f\n").c_str(),
      //                     Chan, R, CellWidth, CellHeight);
        g_log.debug()<<
            "   largest Channel,Radius,CellWidth,CellHeight = "<<Chan<<" "<< R<<" "<<CellWidth<<" "
                                                 << CellHeight<<std::endl;

        if (R < 2*max<double>(CellWidth, CellHeight) || dChan < 3)
        {
          g_log.error("Not enough rows and cols or time channels ");
          throw std::runtime_error("Not enough rows and cols or time channels ");
        }

        InitializeColumnNamesInTableWorkspace(TabWS);


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

              double Radius = 2*R;

              if( R0 >0)
                 Radius = R0;

              int NN= NeighborIDs[1];

              MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                  std::string("Workspace2D"), 3, NN,NN);//neighbors.size(), neighbors.size());

             // sprintf(logInfo, string(
             //                 " A:chan= %d  time=%7.2f  Radius=%7.3f  row= %5.2f  col=%5.2f \n").c_str(),
              //                   xchan, time, Radius, lastRow, lastCol);//std::string(logInfo)
              g_log.debug()<<" A:chan="<< xchan<<"  time="<<time<<"   Radius="<<Radius
                  <<"row= "<<lastRow<<"  col="<<lastCol<<std::endl;

              SetUpData(Data, inpWkSpace, panel, xchan,xchan, lastCol,lastRow,Cent,
                                                      neighborRadius, Radius, spec_idList);

              AttributeValues->setTime( time);

              if( dir==1 && chan ==0)
                origAttributeList= AttributeValues;


              ncells = (int) (AttributeValues->StatBaseVals(ISS1));

              std::vector<double> params;
              std::vector<double> errs;
              std::vector<std::string> names;
              vector<pair<double, double> > Bounds;

              if (AttributeValues->IsEnoughData( ParameterValues, g_log) && ParameterValues[ITINTENS] > 0)
              {
                double chisq;

                Fit(Data,chisq,done,  names,params,errs, lastRow, lastCol, neighborRadius);

                if(!done)//Bivariate error happened
                {

                  if (isGoodFit(params, errs, names, chisq))
                  {
                    LastTableRow =UpdateOutputWS(TabWS, dir, xchan, params, errs, names, chisq,
                          AttributeValues->time, spec_idList);

                    double TotSliceIntensity = AttributeValues->StatBaseVals(IIntensities);
                    double TotSliceVariance = AttributeValues->StatBaseVals(IVariance);

                    updatePeakInformation(    params,             errs,           names,
                                              TotVariance,       TotIntensity,
                                              TotSliceIntensity, TotSliceVariance, chisq,
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
                    //std::cout<<"TRY TO MERGE indx/xchan="<<indx<<"/"<<xchan<<std::endl;
                    int chanMin,chanMax;
                    if( ( dir ==1 && chan ==0 ) || lastAttributeList->CellHeight < 0)
                    {
                      chanMin=xchan;
                      chanMax =xchan+1;
                      if( dir < 0)
                        chanMax++;
                      boost::shared_ptr<DataModeHandler>XXX(new DataModeHandler(*AttributeValues));
                      AttributeValues = XXX;
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

                    double chisq;

                    g_log.debug("Try Merge 2 time slices");
                    if( AttributeValues->IsEnoughData(ParameterValues, g_log))

                       Fit(Data,chisq,done, names, params,
                             errs, lastRow, lastCol, neighborRadius);
                    else
                      chisq=-1;

                    if(!done && isGoodFit(params, errs, names, chisq))
                    {

                      if( LastTableRow >=0 && LastTableRow < (int) TabWS->rowCount())
                        TabWS->removeRow(LastTableRow);
                      else
                        LastTableRow =-1;

                      LastTableRow =UpdateOutputWS(TabWS, dir, (chanMin+chanMax)/2.0, params, errs,
                                      names, chisq,  AttributeValues->time, spec_idList);

                      if( lastAttributeList->CellHeight > 0)
                      {
                        TotIntensity -=lastAttributeList->StatBaseVals(IIntensities);
                        TotVariance -= lastAttributeList->StatBaseVals(IVariance);
                      }

                      double TotSliceIntensity = AttributeValues->StatBaseVals(IIntensities);

                      double TotSliceVariance = AttributeValues->StatBaseVals(IVariance);


                      updatePeakInformation(    params,             errs,           names,
                                      TotVariance,       TotIntensity,
                                     TotSliceIntensity, TotSliceVariance, chisq,
                                      (int) AttributeValues->StatBaseVals(ISS1));

                     // lastAttributeList= AttributeValues;

                      if( dir ==1 && (chan ==0||chan==1))
                      {
                        lastAttributeList->case4 = true;
                        origAttributeList =lastAttributeList;
                      }else
                        LastTableRow = -1;


                    }else
                    {
                      boost::shared_ptr<DataModeHandler>XXX( new DataModeHandler());
                      lastAttributeList =XXX;
                    }
                    done = true;
                   // std::cout<<"END TRY TO MERGE indx="<<indx<<"/"<<xchan<<std::endl;
             }

              //Get ready for the next round
              Data.reset();
             // if (fit_alg)
             //   fit_alg.reset();

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

        setProperty("OutputWorkspace", TabWS);

        setProperty("Intensity", TotIntensity);
        setProperty("SigmaIntensity", SQRT(TotVariance));


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
      TabWS->addColumn("double", "TotIntensityError");
      TabWS->addColumn("str", "SpecIDs");
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



    void DataModeHandler::setStatBase(std::vector<double> const &StatBase)

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

        if (Sxx <= RangeX/12 || Syy <= RangeY/12 || Sxy * Sxy / Sxx / Syy > .8)
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



    }

    double DataModeHandler::getNewRCRadius()
    {
      double Vx,Vy;
      Vx = Vy =HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius;

      if( EdgeX < 0)
      Vx = std::max<double>(HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius,
              Vx_calc);

      if( EdgeY < 0)
        Vy =std::max<double>(HalfWidthAtHalfHeightRadius*HalfWidthAtHalfHeightRadius,
          Vy_calc);

      double  DD = max<double>( sqrt(Vy)*CellHeight, sqrt(Vx)*CellWidth);
      double NewRadius= 1.4* max<double>( 5*max<double>(CellWidth,CellHeight), 4*DD);
      NewRadius = min<double>(baseRCRadius, NewRadius);
               //1.4 is needed to get more background cells. In rectangle the corners were background

       NewRadius = min<double>(35*max<double>(CellWidth,CellHeight),NewRadius);

       return NewRadius;
    }

    void DataModeHandler::setHeighHalfWidthInfo( Mantid::MantidVecPtr &xvals,
                                                 Mantid::MantidVecPtr &yvals,
                                                 Mantid::MantidVecPtr &counts,
                                                 double ROW,
                                                 double COL)
    {
      double minCount,
             maxCount;
      MantidVec X = xvals.access();
      MantidVec Y = yvals.access();
      MantidVec C = counts.access();
      int N = (int)X.size();

      if( N <= 2 )
      {
        HalfWidthAtHalfHeightRadius = -1;
        return;
      }

      minCount = maxCount = C[0];
      for( int i = 1; i < N; i++ )
        if( C[i] > maxCount )
          maxCount = C[i];
        else if( C[i] < minCount )
          minCount = C[i];

      int N2Av = std::min<int>( 10,( int )( N/10 ) );
      int nMax = 0;
      int nMin = 0;
      double TotMax = 0;
      double TotMin = 0;
      double offset = std::max<double>( 1,( maxCount - minCount )/20 );

      for( int i = 0; i < N; i++ )
        if( nMax < N2Av && C[i]>maxCount - offset )
        {
          TotMax += C[i];
          nMax++;
        }else if( nMin < N2Av && C[i] < minCount + offset )
        {
          TotMin += C[i];
          nMin++;
        }
      double TotR = 0;
      int nR = 0;
      double MidVal = ( TotMax/nMax + TotMin/nMin )/2.0;

      while( nR <= 0 )
      {
        for( int i = 0; i < N; i++ )
        if( C[i] < MidVal + offset && C[i] > MidVal - offset )
        {
          double X1 = X[i] - COL;
          double Y1 = Y[i] - ROW;
          TotR += sqrt( X1*X1 + Y1*Y1 );
          nR++;
        }
        offset *= 1.1;
      }
      double AvR = TotR/nR;
      HalfWidthAtHalfHeightRadius = AvR/.8326;
    }
    //Data is the slice subdate, inpWkSpace is ALl the data,
    void IntegratePeakTimeSlices::SetUpData( MatrixWorkspace_sptr          & Data,
                                             MatrixWorkspace_sptr    const & inpWkSpace,
                                             boost::shared_ptr< Geometry::IComponent> comp,
                                             const int                       chanMin,
                                             const int                       chanMax,
                                             double                          CentX,
                                             double                          CentY,
                                             Kernel::V3D                     &CentNghbr,
                                             //specid_t                      &CentDetspec,
                                             //int*                            nghbrs,//Not used
                                            // std::map< specid_t, V3D >     &neighbors,
                                             double                        &neighborRadius,//from CentDetspec
                                             double                         Radius,
                                             string                     &spec_idList)
    {

      Kernel::V3D CentPos1 = center + xvec*(CentX-COL)*CellWidth
                                   + yvec*(CentY-ROW)*CellHeight;

      boost::shared_ptr<DataModeHandler> X(new DataModeHandler(Radius,Radius,CentX,CentY,
          CellWidth,CellHeight,getProperty("CalculateVariances")));

      AttributeValues = X;

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

      //if( changed) CentNghbr = CentPos.
      boost::shared_ptr<DataModeHandler> X1(new DataModeHandler(Radius,NewRadius,CentX,CentY,
          CellWidth,CellHeight, getProperty("CalculateVariances")));

      AttributeValues = X1;
      AttributeValues->setCurrentRadius( NewRadius);
      AttributeValues->setCurrentCenter( CentPos);
      SetUpData1(Data, inpWkSpace, chanMin, chanMax,
                 NewRadius, CentPos, spec_idList );

    }



    void  IntegratePeakTimeSlices:: SetUpData1(API::MatrixWorkspace_sptr              &Data,
                                               API::MatrixWorkspace_sptr        const &inpWkSpace,
                                               const int                               chanMin,
                                               const int                               chanMax,
                                               double                      Radius,
                                               Kernel::V3D                 CentPos,
                                               string                     &spec_idList
                                               )
    {
      UNUSED_ARG(g_log);
      std::vector<double> StatBase(NAttributes);
      if( NeighborIDs[1] < 10)
      {
         StatBase[ISSIxx]= -1;
        return;
      }
      boost::shared_ptr<Workspace2D> ws = boost::shared_dynamic_cast<Workspace2D>(Data);

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
        if( wi_to_detid_map->count(DetID)>0)
           workspaceIndex= wi_to_detid_map->find(DetID)->second;
        else
        {
         g_log.error("No workspaceIndex for detID="+DetID);
         throw  std::runtime_error("No workspaceIndex for detID="+DetID);
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

          double row = ROW + R1a;//(pixPos - center).scalar_prod(yvec) / CellHeight;

          double C1 = dist.scalar_prod(xvec);
          double C1a = C1/CellWidth;

          double col = COL + C1a;//(pixPos - center).scalar_prod(xvec) / CellWidth;

         // std::cout<<R1<<","<<R1a<<","<<ROW<<","<<row<<","<<C1<<","<<C1a<<","<<COL<<","<<col<<std::endl;
          if( row >NBadEdges && col >NBadEdges &&
              (NROWS<0 || row<NROWS-NBadEdges) &&
              ( NCOLS <0 || col<NCOLS-NBadEdges))
          {
          Mantid::MantidVec histogram = inpWkSpace->readY(workspaceIndex);

          Mantid::MantidVec histoerrs = inpWkSpace->readE(workspaceIndex);

          double intensity = 0;
          double variance =0;
          for(int chan=chanMin;chan <= chanMax;chan++)
          {
            intensity +=histogram[chan];
            variance +=histoerrs[chan] * histoerrs[chan];
          }

          N++;
          yvalB.push_back(intensity);
          double sigma = 1;


          errB.push_back(sigma);
          xvalB.push_back((double) col);
          YvalB.push_back((double) row);

          xRef.push_back((double)jj);jj++;

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
          else
          {

            AttributeValues->updateEdgeYsize(abs(ROW-row));
            AttributeValues->updateEdgeXsize(abs(COL-col));
          }
        }
        }

      Data->setX(0, pX);
      Data->setX(1, pX);
      Data->setX(2, pX);

      ws->setData(0, yvals, errs);
      ws->setData(1, xvals);
      ws->setData(2, Yvals);
      AttributeValues->setHeighHalfWidthInfo(xvals,Yvals,yvals,ROW,COL);

      ws->setName("index0");
      StatBase[IStartRow] = minRow;
      StatBase[IStartCol] =minCol;
      StatBase[INRows] = maxRow-minRow+1;
      StatBase[INCol] = maxCol-minCol+1;


      StatBase[ITotBoundary ] = TotBoundaryIntensities;
      StatBase[INBoundary] = nBoundaryCells;
      StatBase[IVarBoundary]= TotBoundaryVariances;
      AttributeValues->setStatBase( StatBase );

      ParameterValues[IBACK] = AttributeValues->getInitBackground();
      ParameterValues[ITINTENS] = AttributeValues->getInitIntensity();
      ParameterValues[IXMEAN] = AttributeValues->getInitCol();
      ParameterValues[IYMEAN] =AttributeValues->getInitRow();
      ParameterValues[IVXX] = AttributeValues->getInitVarx();
      ParameterValues[IVYY] = AttributeValues->getInitVary();
      ParameterValues[IVXY] = AttributeValues->getInitVarxy();
    }

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



      if (chisq < 0)
      {

          g_log.debug()<<"   Bad Slice- negative chiSq= "<<chisq<<std::endl;;
         return false;
      }

      int ncells = (int) (AttributeValues->StatBaseVals(ISS1));

      if (AttributeValues->StatBaseVals(IIntensities) <= 0 || (AttributeValues->StatBaseVals(IIntensities) - params[Ibk] * ncells)
          <= 0)
      {

           g_log.debug()<<"   Bad Slice. Negative Counts= "<<AttributeValues->StatBaseVals(IIntensities) - params[Ibk] * ncells
               <<std::endl;;
        return false;
      }

      double x = params[IIntensity] / (AttributeValues->StatBaseVals(IIntensities) - params[Ibk] * ncells);

      if ((x < .8 || x > 1.25)&& !EdgePeak)// The fitted intensity should be close to tot intensity - background
      {
        g_log.debug()<< "   Bad Slice. Fitted Intensity & Observed Intensity(-back) too different. ratio="
                                               <<x<<std::endl;

        return false;
      }

      bool GoodNums = true;

      for (size_t i = 0; i < errs.size(); i++)
        if (errs[i] != errs[i])
          GoodNums = false;
        else if( errs[i] < 0)
          GoodNums = false;
        else if (params[i] != params[i])
          GoodNums = false;

      if (!GoodNums)
        g_log.debug("   Bad Slice.Some params and errs are not numbers");

      if( !GoodNums)
        return false;

      GoodNums = true;

     // std::cout<<"xxxx"<<params[Ibk]<<","<<(params[Ibk] < -.001)<<","<<(params[IIntensity]<0)<<std::endl;
      if (params[Ibk] < -.001)
        GoodNums = false;

      if (params[IIntensity] < 0)
        GoodNums = false;

      double sqrChi = SQRT(chisq);

      if (AttributeValues->StatBaseVals(IIntensities) > 0)
        if (sqrChi * errs[IIntensity] / AttributeValues->StatBaseVals(IIntensities) > .2)
          GoodNums = false;

//TODO limit rel errors on row and col's. Weak peaks have row.col values migrate a lot


      if (!GoodNums)

      {   g_log.debug(
            "   Bad Slice.Some params and errs are out of bounds or relative errors are too high");
          return false;
      }

      if (params[IIntensity] > 0)
        if (errs[IIntensity] * SQRT(chisq) / params[IIntensity] > .35)
        {
            g_log.debug()<<"   Bad Slice. rel errors too large= "<< errs[ITINTENS] * sqrt(chisq)<<std::endl;;
          return false;
        }

      //Check weak peak. Max theoretical height should be more than 3

      double maxPeakHeightTheoretical = params[ITINTENS] / 2 / M_PI / sqrt(params[IVXX] * params[IVYY]
          - params[IVXY] * params[IVXY]);

      double AvHeight = AttributeValues->StatBaseVals(IIntensities)/AttributeValues->StatBaseVals(ISS1)-params[IBACK];

      if (maxPeakHeightTheoretical < 5*AvHeight || AvHeight < 0 || maxPeakHeightTheoretical < 0)
      {

       // sprintf(logInfo, std::string("   Bad Slice. Peak too small= %7.2f\n").c_str(),
       //     maxPeakHeightTheoretical);

           g_log.debug()<<"   Bad Slice. Peak too small= "<<maxPeakHeightTheoretical<<
                  "/"<<AvHeight<<std::endl;
        return false;
      }

      //Exponential too steep, i.e. intensities at pixels 1 from center are <3* intensity center
      if( params[IVXX]+params[IVYY] >
               2.6*(params[IVXX]*params[IVYY]-params[IVXY]*params[IVXY]))
          {
        g_log.debug()<<"      Bad Slice. Too steep exponential"<< std::endl;
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
       
      return SQRT(Variance);

    }

    //returns row where info was last added
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
      TabWS->getRef<double> (std::string("ISAWIntensity"), TableRow) = AttributeValues->StatBaseVals(IIntensities)
                                                                             - params[Ibk] * ncells;

      TabWS->getRef<double> (std::string("ISAWIntensityError"), TableRow) = CalculateIsawIntegrateError(
          params[Ibk], errs[Ibk], chisq, AttributeValues->StatBaseVals(IVariance), ncells);
      
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
      double intensty=0;
      if( !EdgePeak  )
      {
        err = CalculateIsawIntegrateError(params[Ibk], errs[Ibk], chisqdivDOF, TotSliceVariance, ncells);
        intensty =TotSliceIntensity - params[IBACK] * ncells;
        TotIntensity += TotSliceIntensity - params[IBACK] * ncells;

        TotVariance += err * err;

      }else
      {
       int IInt = find("Intensity", names);
       intensty = params[IInt];
       TotIntensity += params[IInt];
       err = errs[IInt];
       TotVariance += err * err;

      }
     // std::cout<<EdgePeak<<"("<<intensty<<","<<AttributeValues->CalcISAWIntensity(params.data())<<")("
     //       <<err*err<<","<< AttributeValues->CalcISAWIntensityVariance(params.data(),errs.data(), chisqdivDOF)<<")"<<std::endl;

    }

    bool DataModeHandler::CalcVariances( )
    {
      if( !CalcVariance)
        return false;

      double param[7]={back_calc,Intensity_calc,col_calc,row_calc,Vx_calc,Vy_calc, Vxy_calc};

      double r = CalcSampleIntensityMultiplier(param);

      if( r <= 1.0)
        return CalcVariance;
      else
        return false;
    }

   bool DataModeHandler::IsEnoughData( double *ParameterValues, Kernel::Logger& g_log )
    {


      double VIx0_num = StatBase[ISSIxx] - 2 * ParameterValues[IXMEAN] * StatBase[ISSIx]
          + ParameterValues[IXMEAN] * ParameterValues[IXMEAN] * StatBase[IIntensities];

      double VIy0_num = StatBase[ISSIyy] - 2 * ParameterValues[IYMEAN] * StatBase[ISSIy]
          + ParameterValues[IYMEAN] * ParameterValues[IYMEAN] * StatBase[IIntensities];

      double VIxy0_num = StatBase[ISSIxy] - ParameterValues[IXMEAN] * StatBase[ISSIy]
          - ParameterValues[IYMEAN] * StatBase[ISSIx] + ParameterValues[IYMEAN]
          * ParameterValues[IXMEAN] * StatBase[IIntensities];

      double Vx0_num = StatBase[ISSxx] - 2 * ParameterValues[IXMEAN] * StatBase[ISSx]
          + ParameterValues[IXMEAN] * ParameterValues[IXMEAN] * StatBase[INRows]
              * StatBase[INCol];

      double Vy0_num = StatBase[ISSyy] - 2 * ParameterValues[IYMEAN] * StatBase[ISSy]
          + ParameterValues[IYMEAN] * ParameterValues[IYMEAN] * StatBase[INRows]
              * StatBase[INCol];

      double Vxy0_num = StatBase[ISSIxy] - ParameterValues[IXMEAN] * StatBase[ISSIy]
          - ParameterValues[IYMEAN] * StatBase[ISSIx] + ParameterValues[IYMEAN]
          * ParameterValues[IXMEAN] * StatBase[INRows] * StatBase[INCol];

      double Denominator = StatBase[IIntensities] - ParameterValues[IBACK]
          * StatBase[INRows] * StatBase[INCol];

      if( Denominator <=0)
      {
        g_log.debug()<<" No counts after background removed"<<std::endl;
        return false;
      }
      double Vx = ( VIx0_num - ParameterValues[IBACK] * Vx0_num ) / Denominator;
      double Vy = ( VIy0_num - ParameterValues[IBACK] * Vy0_num ) / Denominator;
      double Vxy = ( VIxy0_num - ParameterValues[IBACK] * Vxy0_num ) / Denominator;

      double Z = 4 * M_PI * M_PI * (Vx * Vy - Vxy * Vxy);

      if ( fabs(Z) < .10 ) //Not high enough of a peak
      {
        g_log.debug()<<"   Not Enough Data, Peak Heigh "<<Z <<" is too low"<<std::endl;
        return false;
      }

      return true;

    }

   double DataModeHandler::CalcISAWIntensity(const double* params)const
   {

     double ExperimentalIntensity  = StatBase[IIntensities]-
                                         params[IBACK]*StatBase[ISS1];

     double FitIntensity =  params[ITINTENS];
     double r= CalcSampleIntensityMultiplier( params );

     double alpha =(float)( 0+.5*( r-1 ) );
     alpha = std::min<double>( 1.0f , alpha );

     return ExperimentalIntensity*r*( 1-alpha )+ alpha * FitIntensity;

   }

   double DataModeHandler::CalcISAWIntensityVariance( const double* params,const double* errs, double chiSqOvDOF)const
   {

     int ncells = (int)StatBase[ISS1];
     double B = StatBase[IVariance] / ncells;
      if( B < chiSqOvDOF)
         B = chiSqOvDOF;

     double ExperimVar = StatBase[IVariance];
     double IntensityBackError = errs[IBACK] * sqrt(B);


     ExperimVar += IntensityBackError * IntensityBackError * ncells*ncells + params[IBACK]*ncells;

     double FitVar= errs[ITINTENS]* errs[ITINTENS]* B;

     double r= CalcSampleIntensityMultiplier( params );
     double alpha =(float)( 0+.5*( r - 1 ));
      alpha = std::min<double>( 1.0f , alpha );

      return ExperimVar*r*r*( 1 - alpha ) + alpha * FitVar;
   }

   double DataModeHandler::CalcSampleIntensityMultiplier( const double* params)const
   {
           int MinRow = (int)StatBase[ IStartRow ];
           int MaxRow = MinRow+(int)StatBase[ INRows ]-1;
           int MinCol = (int)StatBase[ IStartCol ];
           int MaxCol= MinCol + (int)StatBase[ INCol ]-1;
           double r=1;

           if( params[IVXX] <=0 || params[IVYY]<=0)
             return 1.0;

           double NstdX = 4*min<double>(  params[ IXMEAN ]-MinCol , MaxCol-params[ IXMEAN ] )
                            /sqrt(params[ IVXX ]);

           double NstdY = 4*min<double>(  params[ IYMEAN ] - MinRow , MaxRow-params[ IYMEAN ] )
                         /sqrt(params[IVYY]);
           double sgn = 1;
           if( NstdX < 0 ){ sgn = -1; }

           if( NstdX >= 7.5 )r = 1.0;
           else if( sgn > 0 ) r = 1/probs[(int)( NstdX+.5 )];
           else  r = 1/(1-probs[(int)( -NstdX+.5)]  );

          if( NstdY < 0 ){ sgn = -1; }

           if( NstdY >= 7.5 )r *= 1.0;
           else if( sgn >0)r *= 1/probs[(int)( NstdY+.5 )];
           else  r *= 1/(1-probs[(int)(-NstdY+.5) ]);

           r = std::max<double>( r , 1.0 );
           return r;
   }

    void IntegratePeakTimeSlices::FindPlane( V3D & center, V3D & xvec, V3D& yvec,
                                             double &ROW, double &COL,int &NROWS,
                                             int & NCOLS,double &pixWidthx, double&pixHeighty,
                                             DataObjects::Peak const &peak) const
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
}//namespace Crystal
}//namespace Mantid
//Attr indicies
