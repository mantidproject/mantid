/*WIKI* 


This algorithm fits a bivariate normal distribution( plus background) to the 
data on each time slice. This algorithm only works for peaks on a Rectangular  
Detector.  The rectangular area used for the fitting is calculated based on 
the dQ parameter.  A good value for dQ is .1667/largest unit cell length.

The table workspace is also a result. Each line contains information on the fit
for each good time slice.  The column names( and information) in the table are:
 Time, Channel, Background, Intensity,Mcol,Mrow,SScol,SSrow,SSrc,NCells,;
 ChiSqrOverDOF,TotIntensity,BackgroundError,FitIntensityError,ISAWIntensity,
 ISAWIntensityError,Start Row,End Row,Start Col,End Col

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
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/IPropertyManager.h"
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

//
#include <vector>
#include "MantidAPI/Algorithm.h"
#include <algorithm>
#include <math.h>
#include <cstdio>

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

    //Attr indicies
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
#define IVariance    15
#define NAttributes  16

    //Parameter indicies
#define IBACK   0
#define ITINTENS  1
#define IXMEAN  2
#define IYMEAN  3
#define IVXX  4
#define IVYY  5
#define IVXY  6

#define NParameters  7
    IntegratePeakTimeSlices::IntegratePeakTimeSlices() :
      Algorithm(), RectWidth(-1), RectHeight(-1)
    {
      debug = false;
      wi_to_detid_map = 0;
      if (debug)
        g_log.setLevel(7);

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
      AttributeNames[15] = "Variance";

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

      if (wi_to_detid_map)
      {

        delete wi_to_detid_map;
        wi_to_detid_map = 0;

      }
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

      declareProperty("Intensity", 0.0, "Peak Integrated Intensity", Direction::Output);

      declareProperty("SigmaIntensity", 0.0, "Peak Integrated Intensity Error", Direction::Output);

    }

    void IntegratePeakTimeSlices::exec()
    {
      int nPanelRows, nPanelCols;

      double CellHeight, CellWidth;

      char logInfo[200];

      double dQ = getProperty("PeakQspan");

      g_log.debug("------------------Start Peak Integrate-------------------");

      if (dQ <= 0)
      {
        g_log.error("Negative PeakQspans are not allowed. Use .17/G where G is the max unit cell length");
        throw;
      }

      MatrixWorkspace_sptr inpWkSpace = getProperty("InputWorkspace");
      if (!inpWkSpace)
      {
        g_log.error("Improper Input Workspace");
        throw;
      }
      //Check if detectors are RectangularDetectors
      Instrument_const_sptr inst = inpWkSpace->getInstrument();
      boost::shared_ptr<RectangularDetector> det;
      for (int i=0; i < inst->nelements(); i++)
      {
        det = boost::dynamic_pointer_cast<RectangularDetector>( (*inst)[i] );
        if (det) break;
      }
      if (!det)
        throw std::runtime_error("PeakIntegration only works for instruments with Rectangular Detectors.");

      PeaksWorkspace_sptr peaksW;
      peaksW = getProperty("Peaks");
      if (!peaksW)
      {
        g_log.error("Improper Peaks Input");
        throw;
      }

      int indx = getProperty("PeakIndex");

      IPeak &peak = peaksW->getPeak(indx);

      sprintf(logInfo, std::string("   Peak Index %5d\n").c_str(), indx);
      g_log.debug(std::string(logInfo));

      double TotVariance = 0;
      double TotIntensity = 0;
      int lastRow = peak.getRow();
      int Row0 = lastRow;
      int lastCol = peak.getCol();
      int Col0 = lastCol;

      // For quickly looking up workspace index from det id
      wi_to_detid_map = inpWkSpace->getDetectorIDToWorkspaceIndexMap(true);

      TableWorkspace_sptr TabWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(0));

      try
      {

        int detID = peak.getDetectorID();

        // Find the workspace index for this detector ID
        Mantid::detid2index_map::iterator it;
        it = (*wi_to_detid_map).find(detID);
        size_t specNum = (it->second);

        findNumRowsColsinPanel(peak, nPanelRows, nPanelCols, CellHeight, CellWidth, g_log);

        sprintf(logInfo,
            string("   Panel Rows,Cols Cell Height, width %5d %5d %6.3f  %6.3f \n").c_str(), nPanelRows,
            nPanelCols, CellHeight, CellWidth);
        g_log.debug(std::string(logInfo));

        double dRow = CalculatePanelRowColSpan(peak, dQ, CellHeight, CellWidth);
        //double MaxDrow = max<double>(nPanelRows,nPanelCols)*.15;

        dRow = min<double> (36, dRow);//Should use MaxDrow in place of 36, but test case does not workwell
        dRow = max<double> (6, dRow);
        int Chan;

        Mantid::MantidVec X = inpWkSpace->dataX(specNum);
        int dChan = CalculateTimeChannelSpan(peak, dQ, X, int(specNum), Chan);

        dChan = max<int> (dChan, 3);
      
        boost::shared_ptr<const RectangularDetector> panel = getPanel(peak);

        int MaxChan = -1;
        double MaxCounts = -1;
        double Centy = Row0;
        double Centx = Col0;
        bool done = false;
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
              std::vector<int> Attr= CalculateStart_and_NRowsCols( Centy,
                                                                   Centx,
                                                                    (int)(.5+dRow),
                                                                   (int)(.5+dRow),
                                                                   nPanelRows,
                                                                   nPanelCols);
             
              MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                         std::string("Workspace2D"), 2, Attr[2] * Attr[3] + 1, Attr[2] * Attr[3]);

              SetUpData1(Data, inpWkSpace, panel, Chan+dir*t, Attr, wi_to_detid_map, g_log);
              
              if( AttributeValues[IIntensities] > MaxCounts)
              {
                 MaxCounts = AttributeValues[IIntensities];
                 MaxChan = Chan+dir*t;
               }
               if( AttributeValues[IIntensities] >0)
               {
                  Centx = AttributeValues[ ISSIx]/AttributeValues[IIntensities];
                  Centy = AttributeValues[ ISSIy]/AttributeValues[IIntensities];
                }else
                   done = true;
              
           }
        
        Chan = max<int>( Chan , MaxChan );

        sprintf(logInfo, std::string("   Start/largest Channel = %d \n").c_str(), dRow, dChan);
        g_log.debug(std::string(logInfo));

        if (dRow < 2 || dChan < 3)
        {
          g_log.error("Not enough rows and cols or time channels ");
          throw;
        }

        InitializeColumnNamesInTableWorkspace(TabWS);

        

        IAlgorithm_sptr fit_alg;

        double time;
        int ncells;

        Mantid::API::Progress prog(this, 0.0, 100.0, (int) dChan);

        RectWidth = RectHeight = -1;
   
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

              int Pixelx = 2*(int)dRow;
              int Pixely = 2*(int)dRow;
              if( RectWidth >0 && RectHeight >0)
              {
                 Pixelx = (int)(.5+RectWidth);
                 Pixely = (int)(.5+RectHeight);
              }
              std::vector<int> Attr = CalculateStart_and_NRowsCols(  lastRow,
                                                                     lastCol,
                                                                     Pixely,
                                                                     Pixelx,            
                                                                     nPanelRows,
                                                                     nPanelCols);



              MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                  std::string("Workspace2D"), 2, Attr[2] * Attr[3] + 1, Attr[2] * Attr[3]);

              sprintf(logInfo, string(
                              " A:chan= %d  time=%7.2f  startRow=%d  startCol= %d  Nrows=%d  Ncols=%d\n").c_str(),
                                 xchan, time, Attr[0], Attr[1], Attr[2], Attr[3]);
              g_log.debug(std::string(logInfo));

              SetUpData(Data, inpWkSpace, panel, xchan, Attr);

              ncells = (int) (Attr[INRows] * Attr[INCol]);

              std::vector<double> params;
              std::vector<double> errs;
              std::vector<std::string> names;

              if (IsEnoughData() && ParameterValues[ITINTENS] > 0)
              {

                fit_alg = createSubAlgorithm("Fit");

                fit_alg->setProperty("InputWorkspace", Data);
                fit_alg->setProperty("WorkspaceIndex", 0);
                fit_alg->setProperty("StartX", 0.0);
                fit_alg->setProperty("EndX", 0.0 + Attr[2] * Attr[3]);
                fit_alg->setProperty("MaxIterations", 5000);
                //fit_alg->setProperty("Output", "fit");

                std::string fun_str = CalculateFunctionProperty_Fit();

                std::string SSS("   Fit string ");
                SSS += fun_str;
                g_log.debug(SSS);

                fit_alg->setProperty("Function", fun_str);

                fit_alg->executeAsSubAlg();

                double chisq = fit_alg->getProperty("OutputChi2overDoF");

                params = fit_alg->getProperty("Parameters");
                errs = fit_alg->getProperty("Errors");
                names = fit_alg->getProperty("ParameterNames");

                ostringstream res;
                res << "   Thru Algorithm: chiSq=" << setw(7) << chisq << endl;

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
                  updatePeakInformation(params, errs, names, TotVariance, TotIntensity,
                      TotSliceIntensity, TotSliceVariance, chisq, ncells);

                }
                else

                  done = true;

              }
              else
              {
                done = true;
                //std::cout<<"Not Enuf data done=true"<<std::endl;
              }

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
                //std::cout<<"incr progr by 1 step"<<std::endl;

              }
              else
              {
                if (dir > 0)
                {
                  prog.report(dChan / 2);


                }
                else
                {
                  prog.report(dChan);

                }
              }
              params.clear();
              errs.clear();
              names.clear();
              Attr.clear();

            }
        }

      } catch (std::exception &EE1)
      {
        std::cout << "Error in main reason=" << EE1.what() << std::endl;
        throw;
      }

      try
      {


        peak.setIntensity(TotIntensity);
        peak.setSigmaIntensity(sqrt(TotVariance));

        setProperty("OutputWorkspace", TabWS);//Guess what this does the Analysis data service

        setProperty("Intensity", TotIntensity);
        setProperty("SigmaIntensity", sqrt(TotVariance));



      } catch (std::exception &ss)
      {

        std::cout << "Error occurred XX " << ss.what() << std::endl;
      }

    }


    //Gets the Rectangular detector from an instrument
    boost::shared_ptr<const RectangularDetector> IntegratePeakTimeSlices::getPanel(Peak const & peak)
    {

      Geometry::Instrument_const_sptr Iptr = peak.getInstrument();
      std::string bankName = peak.getBankName();

      boost::shared_ptr<const IComponent> parent = Iptr->getComponentByName(bankName);

      if (parent->type().compare("RectangularDetector") != 0)
      {

        //std::cout<<"   getPanel C type="<<parent->type()<<std::endl;
        throw std::runtime_error("Improper Peak Argument");
      }

      boost::shared_ptr<const RectangularDetector> RDet = boost::shared_dynamic_cast<
          const RectangularDetector>(parent);
      return RDet;
    }


    // first element of )NrowsCols in the # of rows,the second is the # of columns
    void IntegratePeakTimeSlices::findNumRowsColsinPanel ( DataObjects::Peak           const &peak,
                                                           int                               &nPanelRows,
                                                           int                               & nPanelCols,
                                                           double                            & CellHeight,
                                                           double                            & CellWidth,
                                                           Mantid::Kernel::Logger            &g_log)
    {

      boost::shared_ptr<const RectangularDetector> RDet = getPanel(peak);

      if (!RDet)
      {
        g_log.error("No Rectangular detector associated with the peak");
        throw;
      }

      nPanelRows = (RDet->ypixels());
      nPanelCols = (RDet->xpixels());
      CellHeight = (RDet->ystep());
      CellWidth = RDet->xstep();
      if (nPanelRows < 4 || nPanelCols < 4 || CellHeight <= 0 || CellWidth <= 0)
      {
        //      .error("Odd Rectangular Detector Panel #rows, cols, Cellwidth, Cellheights="+
        //                        nPanelRows+", "+nPanelCols+", "+Cellwidth+", "+CellHeight);
        throw;
      }

    }



     double IntegratePeakTimeSlices::CalculatePanelRowColSpan(  DataObjects::Peak const &peak,
                                                                const double             dQ,
                                                                const double             ystep,
                                                                const double             xstep)
    {

      double Q = 0, ScatAngle = 0, dScatAngle = 0, DetSpan = 0, nrows = 0, ncols = 0;

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
        nrows = DetSpan / (ystep);//1.5 is heuristic.


        ncols = DetSpan / xstep;

        if (nrows > ncols)
        {
          if (nrows < 8)
          {
            return 8 + 3;
          }
          else
            return nrows + 3;

        }

        if (ncols < 8)
        {
          return 8 + 3;
        }

        else

          return ncols + 3;

      } catch (std::exception &s)
      {
        std::cout << "err in getNRowsCols, reason=" << s.what() << std::endl;
        return 0;
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
      TabWS->addColumn("double", "Start Row");
      TabWS->addColumn("double", "End Row");
      TabWS->addColumn("double", "Start Col");
      TabWS->addColumn("double", "End Col");
    }

    // returns StartRow,StartCol,NRows,NCols
    //dRow is tot # rows. dCol is tot # columns
    vector<int> IntegratePeakTimeSlices::CalculateStart_and_NRowsCols( const double CentRow,
                                                                       const double CentCol,
                                                                       const int dRow,
                                                                       const int dCol,
                                                                       const int nPanelRows,
                                                                       const int nPanelCols
                                                                      )
    {
      std::vector<int> Res;

      double StartRow = CentRow - (int) (dRow / 2);

      if (StartRow < 3)
        StartRow = 3;

      if (StartRow > nPanelRows - 3)
        StartRow = nPanelRows - 3;

      Res.push_back((int) StartRow);

      double StartCol = CentCol - (int) (dCol / 2);

      if (StartCol < 3)
        StartCol = 3;

      if (StartCol > nPanelCols - 3)
        StartCol = nPanelCols - 3;

      Res.push_back((int) StartCol);

      double EndRow = CentRow + (int) (dRow / 2);

      if (EndRow > nPanelRows - 3)
        EndRow = nPanelRows - 3;

      Res.push_back((int) (EndRow - StartRow + 1));

      double EndCol = CentCol + (int) (dCol / 2);

      if (EndCol > nPanelCols - 3)
        EndCol = nPanelCols - 3;

      Res.push_back((int) (EndCol - StartCol + 1));

      return Res;

    }



    void IntegratePeakTimeSlices::updateStats( const double          intensity,
                                               const double          variance,
                                               const int             row,
                                               const int             col,
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
    }



    void IntegratePeakTimeSlices::getInitParamValues(std::vector<double> const &StatBase,
                                                     const double               TotBoundaryIntensities,
                                                     const int                  nBoundaryCells)
    {
      double b = 0;
      if (nBoundaryCells > 0)
        b = TotBoundaryIntensities / nBoundaryCells;

      int nCells = (int) (StatBase[INRows] * StatBase[INCol]);
      double Den = StatBase[IIntensities] - b * nCells;

      if (Den <= 0)
      {
        b = 0;
        Den = (StatBase[IIntensities]);
        if (Den <= 1)
          Den = 1;
      }

      bool done = false;
      int ntimes = 0;
      double Mx, My, Sxx, Syy, Sxy;
      while (!done)
      {
        Mx = StatBase[ISSIx] - b * StatBase[ISSx];
        My = StatBase[ISSIy] - b * StatBase[ISSy];
        Sxx = (StatBase[ISSIxx] - b * StatBase[ISSxx] - Mx * Mx / Den) / Den;
        Syy = (StatBase[ISSIyy] - b * StatBase[ISSyy] - My * My / Den) / Den;
        Sxy = (StatBase[ISSIxy] - b * StatBase[ISSxy] - Mx * My / Den) / Den;
        ntimes++;
        done = true;
        if (Sxx <= 0 || Syy <= 0 || Sxy * Sxy / Sxx / Syy > .8)
        {

          if (b != 0)
            done = false;
          Den = StatBase[IIntensities];
          if (Den <= 1)
            Den = 1;
          b = 0;
        }
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
    void IntegratePeakTimeSlices::SetUpData( MatrixWorkspace_sptr                           & Data,
                                             MatrixWorkspace_sptr                     const & inpWkSpace,
                                             Geometry::RectangularDetector_const_sptr const &panel,
                                             const int                                       chan,
                                             std::vector<int>                               & Attr)
    {
      

      SetUpData1(Data, inpWkSpace, panel, chan, Attr, wi_to_detid_map, g_log);

      int nPanelRows = (panel->ypixels());
      int nPanelCols = (panel->xpixels());

      int NewNRows= 2*max<int>( 5,(int) (3* sqrt(ParameterValues[IVYY])+.5));
      int NewNCols = 2*max<int>(5,(int)( 3*sqrt(ParameterValues[IVXX])+.5));
      NewNRows = min<int>(2*30,NewNRows);
      NewNCols = min<int>(2*30,NewNCols);

      if( RectWidth > 0 && RectHeight > 0)
      {
         NewNRows = (int)(.5+RectHeight);
         NewNCols = (int)(.5+RectWidth);
       } else
       {
          RectWidth = NewNCols;
          RectHeight = NewNRows;
       }     

      vector<int> Attr1 =CalculateStart_and_NRowsCols(  ParameterValues[IYMEAN ],
                                                        ParameterValues[IXMEAN],
                                                        NewNRows,
                                                        NewNCols,
                                                        nPanelRows,
                                                        nPanelCols);

      SetUpData1(Data, inpWkSpace, panel, chan, Attr1, wi_to_detid_map, g_log);

      for( int i=0; i<4;i++)
        Attr[i] = Attr1[i];


    }



    void  IntegratePeakTimeSlices:: SetUpData1(API::MatrixWorkspace_sptr                                    &Data,
                                               API::MatrixWorkspace_sptr                              const &inpWkSpace,
                                               Mantid::Geometry::RectangularDetector_const_sptr        const &panel,
                                               const int                                              chan,
                                               std::vector<int>                                              &Attr,
                                               Mantid::detid2index_map                               *const  wi_to_detid_map,
                                               Kernel::Logger & g_log)
    {
      UNUSED_ARG(g_log);
      boost::shared_ptr<Workspace2D> ws = boost::shared_dynamic_cast<Workspace2D>(Data);
      
      std::vector<double> StatBase;
      for (int i = 0; i < NAttributes + 2; i++)
        StatBase.push_back(0);

      Mantid::MantidVecPtr pX;

      Mantid::MantidVec& xRef = pX.access();
      for (int j = 0; j < Attr[2] * Attr[3] + 1; j++)
      {
        xRef.push_back(j);
      }

      Data->setX(0, pX);
      Data->setX(1, pX);

      Mantid::MantidVecPtr yvals;
      Mantid::MantidVecPtr errs;

      Mantid::MantidVec&yvalB = yvals.access();
      Mantid::MantidVec &errB = errs.access();

      double TotBoundaryIntensities = 0;
      int nBoundaryCells = 0;

      for (int row = Attr[0]; row < Attr[0] + Attr[2]; row++)
        for (int col = Attr[1]; col < Attr[1] + Attr[3]; col++)
        {

          boost::shared_ptr<Detector> pixel = panel->getAtXY(col, row);

          // Find the workspace index for this detector ID
          Mantid::detid2index_map::iterator it;
          it = (*wi_to_detid_map).find(pixel->getID());
          size_t workspaceIndex = (it->second);

          Mantid::MantidVec histogram = inpWkSpace->readY(workspaceIndex);
          Mantid::MantidVec histoerrs = inpWkSpace->readE(workspaceIndex);

          double intensity = histogram[chan];
          double variance = histoerrs[chan] * histoerrs[chan];
          
          yvalB.push_back(intensity);
          errB.push_back(1);

          updateStats(intensity, variance, row, col, StatBase);
          if (row == Attr[0] || col == Attr[1])
          {
            TotBoundaryIntensities += intensity;
            nBoundaryCells++;

          }
          else if (row == (Attr[0] + Attr[2] - 1) || col == (Attr[1] + Attr[3] - 1))
          {

            TotBoundaryIntensities += intensity;
            nBoundaryCells++;
          }
        }
      
      ws->setData(0, yvals, errs);
      StatBase[IStartRow] = Attr[0];
      StatBase[IStartCol] = Attr[1];
      StatBase[INRows] = Attr[2];
      StatBase[INCol] = Attr[3];

      getInitParamValues(StatBase, TotBoundaryIntensities, nBoundaryCells);

      for (int i = 0; i < NAttributes; i++)
        AttributeValues[i] = StatBase[i];


    }

    std::string IntegratePeakTimeSlices::CalculateFunctionProperty_Fit()
    {

      std::ostringstream fun_str;

      fun_str << "name=BivariateNormal,";

      for (int i = 0; i < NParameters; i++)
        fun_str << ParameterNames[i] << "=" << ParameterValues[i] << ", ";

      for (size_t j = 0; j < NAttributes-1; j++)
      {
        fun_str << AttributeNames[j] << "=" << AttributeValues[j];
        if (j + 1 < NAttributes-1)
          fun_str << ", ";
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

      char logInfo[200];

      if (chisq < 0)
      {

        sprintf(logInfo, std::string("   Bad Slice- negative chiSq= %7.2f\n").c_str(), chisq);
        g_log.debug(std::string(logInfo));
        return false;
      }

      int ncells = (int) (AttributeValues[INRows] * AttributeValues[INCol]);

      if (AttributeValues[IIntensities] <= 0 || (AttributeValues[IIntensities] - params[Ibk] * ncells)
          <= 0)
      {

        sprintf(logInfo, std::string("   Bad Slice. Negative Counts= %7.2f\n").c_str(),
            AttributeValues[IIntensities] - params[Ibk] * ncells);
        g_log.debug(std::string(logInfo));
        return false;
      }

      double x = params[IIntensity] / (AttributeValues[IIntensities] - params[Ibk] * ncells);

      if (x < .8 || x > 1.25)// The fitted intensity should be close to tot intensity - background
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

        sprintf(logInfo, std::string("   Bad Slice. rel errors too large= %7.2f\n").c_str(),
            errs[ITINTENS] * sqrt(chisq));
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
                                                  const double                               chisq,
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
        newRowIndex = TabWS->rowCount();

      int TableRow = TabWS->insertRow(newRowIndex);

      int ncells = (int) (AttributeValues[INRows] * AttributeValues[INCol]);
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
      //cout<<"ISAWIntensity parts="<<","<<AttributeValues[IIntensities]<<","<<params[Ibk]<<","<<ncells <<endl;
      TabWS->getRef<double> (std::string("ISAWIntensityError"), TableRow) = CalculateIsawIntegrateError(
          params[Ibk], errs[Ibk], chisq, AttributeValues[IVariance], ncells);
      
      TabWS->getRef<double> (std::string("Time"), TableRow) = time;

      TabWS->getRef<double> (std::string("Start Row"), TableRow) = AttributeValues[IStartRow];
      TabWS->getRef<double> (std::string("End Row"), TableRow) = AttributeValues[IStartRow]
          + AttributeValues[INRows] - 1;

      TabWS->getRef<double> (std::string("Start Col"), TableRow) = AttributeValues[IStartCol];
      TabWS->getRef<double> (std::string("End Col"), TableRow) = AttributeValues[IStartCol]
          + AttributeValues[INCol] - 1;

    }

    
    
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

      double err = CalculateIsawIntegrateError(params[Ibk], errs[Ibk], chisqdivDOF, TotSliceVariance, ncells);

      TotIntensity += TotSliceIntensity - params[IBACK] * ncells;

      TotVariance += err * err;

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
}//namespace Crystal
}//namespace Mantid
//Attr indicies
