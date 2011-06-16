/*
 * IntegratePeakTimeSlices.cpp
 *
 *  Created on: May 5, 2011
 *      Author: ruth
 */
//#include "MantidCurveFitting/BivariateNormal.h"
#include "MantidCrystal/IntegratePeakTimeSlices.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IFitFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
//
#include <vector>
#include <algorithm>
#include <math.h>
   // Register the class into the algorithm factory
using namespace Mantid::Algorithms;
using namespace Mantid;

//using namespace Mantid::CurveFitting; This does not link with the
//                                    CurveFitting dll, Use Fit algorithm
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

DECLARE_ALGORITHM(IntegratePeakTimeSlices)

std::vector<std::string> AttrNames;
std::vector<std::string> ParamNames;
//Attr indicies
int IStartRow=0;
int IStartCol=1;
int INRows=2;
int INCol=3;
int ISSIxx=4;
int ISSIyy=5;
int ISSIxy=6;
int ISSxx=7;
int ISSyy=8;
int ISSxy=9;
int ISSIx=10;
int ISSIy=11;
int ISSx=12;
int ISSy=13;
int IIntensities=14;

//Parameter indicies
int IBACK = 0;
int ITINTENS = 1;
int IXMEAN = 2;
int IYMEAN = 3;
int IVXX = 4;
int IVYY = 5;
int IVXY = 6;
/*
//place in WorkspaceProperty.cpp so getProperty works with PeaksWorkspace
//    Unfortunately PeaksWorkspace needs an == operator
template DLLExport class PropertyWithValue< PeaksWorkspace>;
*/
IntegratePeakTimeSlices::IntegratePeakTimeSlices():Algorithm()
 {
  if( AttrNames.size() <3)
  {
    AttrNames.clear();
    AttrNames.push_back("StartRow");
    AttrNames.push_back("StartCol");
    AttrNames.push_back("NRows");
    AttrNames.push_back("NCol");
    AttrNames.push_back("SSIxx");
    AttrNames.push_back("SSIyy");
    AttrNames.push_back("SSIxy");
    AttrNames.push_back("SSxx");
    AttrNames.push_back("SSyy");
    AttrNames.push_back("SSxy");
    AttrNames.push_back("SSIx");
    AttrNames.push_back("SSIy");
    AttrNames.push_back("SSx");
    AttrNames.push_back("SSy");
    AttrNames.push_back("Intensities");

  }
  if( ParamNames.size() <2)
  {
    ParamNames.clear();
    ParamNames.push_back("Background");
    ParamNames.push_back("Intensity");
    ParamNames.push_back("Mcol");
    ParamNames.push_back("Mrow");
    ParamNames.push_back("SScol");
    ParamNames.push_back("SSrow");
    ParamNames.push_back("SSrc");
  }
 }
     /// Destructor
IntegratePeakTimeSlices::~IntegratePeakTimeSlices()
{

}


void IntegratePeakTimeSlices::init()
{
     declareProperty(new WorkspaceProperty< MatrixWorkspace >("InputWorkspace", "", Direction::Input)
          , "A 2D workspace with X values of time of flight");
      declareProperty(new WorkspaceProperty<TableWorkspace>("TableWorkspace","",Direction::Output), "Name of the output workspace.");
      declareProperty(new WorkspaceProperty<PeaksWorkspace>("Peaks","",Direction::InOut), "Workspace of Peaks");
      declareProperty("PeakIndex",0,"Index of peak in PeaksWorkspace to integrate");

      declareProperty("PeakQspan", .3, "Max magnitude of Q of Peak to Q of Peak Center");
}
//std::vector<int>
void getnPanelRowsCols(Peak peak, double dQ, std::vector<int> NrowsCols);
int  getdChanSpan(Peak peak, double dQ, MatrixWorkspace_sptr mwspc, int specNum);
double getdRowSpan(  Peak peak, double dQ, double ystep, double xstep);
void SetUpOutputWSCols( TableWorkspace_sptr TabWS);
int getSpectra( int PixID,MatrixWorkspace_sptr mwspc);
boost::shared_ptr<const RectangularDetector> getPanel( Peak peak);
std::vector<int> getStartNRowCol( double CentRow,double CentCol, int dRow, int dCol,std::vector<int> NrowsCols);
std::vector<std::vector<double> >  SetUpData(MatrixWorkspace_sptr Data,MatrixWorkspace_sptr inpWkSpace,
boost::shared_ptr<const RectangularDetector> panel,int chan,std::vector<int>Attr);
std::string CalcFunctionProperty( std::vector<std::vector<double> > ParamAttr);
bool GoodFit1(std::vector<double >params,std::vector<double >errs,std::vector<std::string >names,
                                        double chisq, std::vector<std::vector<double > >ParamAttr );
void UpdateOutputWS( TableWorkspace_sptr TabWS,int dir,std::vector<double >params,std::vector<double >errs,
    std::vector<std::string>names, double chisq, std::vector<std::vector<double > >ParamAttr, std::string method);
std::vector<std::vector<double> > FitwOldMethod(MatrixWorkspace_sptr Data,std::vector<std::vector<double > >ParamAttr,
                     std::vector<double >params,  std::vector<std::string >names);
void updatepeakInf( std::vector<double >params,std::vector<double >errs,std::vector<std::string >names,
                               double &TotVariance, double &TotIntensity);
int find( std::string oneName, std::vector<std::string> nameList);
void IntegratePeakTimeSlices::exec()
{

    double dQ = getProperty("PeakQspan");
    MatrixWorkspace_sptr inpWkSpace = getProperty("InputWorkspace");
    PeaksWorkspace_sptr peaksW;
    peaksW = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(getProperty("Peaks")));

    int indx = getProperty("PeakIndex");

    Peak peak =peaksW->getPeak(indx);

    std::vector<int>NrowsCols;
    int specNum= getSpectra( peak.getDetectorID(), inpWkSpace);
    getnPanelRowsCols( peak, dQ, NrowsCols);

    double dRow = getdRowSpan( peak,dQ, NrowsCols[2], NrowsCols[3]);
    int dChan = getdChanSpan( peak,dQ,inpWkSpace, specNum);

    TableWorkspace_sptr TabWS =boost::shared_ptr<TableWorkspace>( new TableWorkspace(1));
    SetUpOutputWSCols( TabWS);

    double TotVariance=0;
    double TotIntensity = 0;
    int lastRow= peak.getRow();
    int Row0=lastRow;
    int lastCol = peak.getCol();
    int Col0 =lastCol;

    boost::shared_ptr<const RectangularDetector> panel = getPanel( peak );

    for( int dir= 1; dir >=-1; dir -=2)
    {
      bool done = false;

      for( int chan = 0; chan < dChan/2 && !done; chan++)
        if( dir < 0 && chan ==0)
        {
          lastRow = Row0;
          lastCol = Col0;
        }else
         {
           int nchan = chan;
           if( chan <=1)
             chan =2;

           std::vector<int> Attr= getStartNRowCol( Row0+(lastRow-Row0)/chan*(chan+1.0),
                             Col0+(lastCol-Col0)/chan*(chan+1.0), (int)dRow, (int) dRow, NrowsCols);


           MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                  std::string("Workspace1D"),
                  1,
                 Attr[2]*Attr[3]+1,
                 Attr[2]*Attr[3] );
           std::vector<std::vector<double> > ParamAttr = SetUpData(Data,inpWkSpace,panel,chan,Attr);


           IAlgorithm_sptr fit_alg;
           try
           {
             fit_alg = createSubAlgorithm("Fit");
           } catch (Exception::NotFoundError&)
           {
              g_log.error("Can't locate Fit algorithm");
              throw ;
            }
           fit_alg->setProperty("InputWorkspace", Data);
           fit_alg->setProperty("WorkspaceIndex", 0);
           fit_alg->setProperty("StartX", 0);
           fit_alg->setProperty("EndX", Attr[2]*Attr[3]);
           fit_alg->setProperty("MaxIterations", 5000);
           fit_alg->setProperty("Output", "fit");
           std::string fun_str =CalcFunctionProperty( ParamAttr);
           fit_alg->setProperty("Function", fun_str);
           fit_alg->executeAsSubAlg();
           double chisq = fit_alg->getProperty("OutputChi2overDoF");

           std::vector<double> params = fit_alg->getProperty("Parameters");
           std::vector<double> errs = fit_alg->getProperty("Errors");
           std::vector<std::string> names = fit_alg->getProperty("ParameterNames");
           std::string method ="L";
           if( GoodFit1( params,errs,names, chisq, ParamAttr))
           {
             UpdateOutputWS( TabWS, dir, params,errs,names, chisq, ParamAttr, method);
           }else
           {
             std::vector<std::vector<double> > res=
                 FitwOldMethod(Data,ParamAttr, params,names);
             if( res.size() < 1 )
               done = true;
             else
             {
               method ="X";
               params = res[0];
               errs = res[1];

               UpdateOutputWS( TabWS, dir, params,errs,names, chisq, ParamAttr, method);
               done =true;
             }

           }
           if( !done)
           {

               updatepeakInf( params,errs,names, TotVariance, TotIntensity);
              //Now set up the center for this peak
               int i = find( "Mrow", names);
               lastRow =params[i];
               i = find("Mcol", names);
               lastCol= params[i];
           }



         }
    }
    //peak.set inti and sigi
    //set output workspace property


}
boost::shared_ptr<const RectangularDetector> getPanel( Peak peak)
    {
     IDetector_sptr detPtr = peak.getDetector();
     boost::shared_ptr< const IComponent > parent = detPtr->getParent();
     if( parent->type().compare("RectangularDetector") !=0)
     {
       throw std::runtime_error("Improper Peak Argument");
     }

     boost::shared_ptr<const RectangularDetector>RDet =
                     boost::shared_dynamic_cast<const RectangularDetector>(parent);
     return RDet;
    }

// first element of NrowsCols in the # of rows,the second is the # of columns
  void getnPanelRowsCols(Peak peak, double dQ, std::vector<int> NrowsCols)
    {

     boost::shared_ptr<const RectangularDetector>RDet = getPanel( peak);


     std::vector<int>v;
     v.clear();
     v.push_back( RDet->ypixels());
     v.push_back( RDet->xpixels());
     v.push_back(RDet->ystep());
     v.push_back( RDet->xstep());

    }

  double getQ( Peak peak)
  {
    V3D pos =peak.getDetPos();
     std::vector<double> x;
     std::vector<double> y;
     x.push_back( peak.getTOF());
     Mantid::Kernel::Units::MomentumTransfer MT;
     double twoTheta = asin(pos.Z()/pos.norm());
     MT.fromTOF( x, y,peak.getL1(), peak.getL2(),twoTheta, 0,1.0,1.0);
     return x[0];

  }
  int getSpectra( int PixID,MatrixWorkspace_sptr mwspc)
  {
    const Geometry::ISpectraDetectorMap &map = mwspc->spectraMap();
     std::vector<int> detList;
     detList.push_back( PixID);
     std::vector<int>spectra = map.getSpectra( detList);
     return spectra[0];
  }

double getdRowSpan( Peak peak, double dQ, double ystep, double xstep)
{
  double Q = getQ( peak);
  V3D pos = peak.getDetPos();
  double ScatAngle = asin(pos.Z()/pos.norm());
  double dScatAngle = dQ/Q*tan(ScatAngle);
  double DetSpan = pos.norm()*dScatAngle;  //s=r*theta
  double nrows = DetSpan/( ystep);
  double ncols = DetSpan/xstep;
  if( nrows > ncols)
  {
    if( nrows < 4)
      return 4;
    else
      return nrows;

  }
  if( ncols < 4)
    return 4;
  else
    return ncols;


}
int find( Mantid::MantidVec X, double time)
{
 int sgn = 1;

 if( X[0] > X[1])
   sgn = -1;

 if(sgn*( X[0]-time) >=0)
   return 0;

 if( sgn*( time-X[X.size()-1])>=0)
   return X.size()-1;

  for( int i=0; i< X.size()-1; i++)
  {
    if( sgn*(time-X[i])>=0 && sgn*(X[i+1]-time) >=0)
      return i;
  }

  return -1;
}

int  getdChanSpan(Peak peak, double dQ, MatrixWorkspace_sptr mwspc,int spectra)
{
  double Q = getQ( peak);

  V3D pos = peak.getDetPos();
  double time = peak.getTOF();
  double dtime = dQ/Q*time;

  Mantid::MantidVec X = mwspc->dataX( spectra);
  int chanCenter = find( X, time);
  int chanLeft = find(X, time-dtime);
  int chanRight = find( X, time + dtime);

  int dchan = abs(chanCenter-chanLeft);
  if( abs(chanRight-chanCenter) >dchan)
     dchan = abs(chanRight-chanCenter);
  return 2*dchan+2;
}

void SetUpOutputWSCols( TableWorkspace_sptr TabWS)
{
     TabWS->addColumn("double","Background");
     TabWS->addColumn("double","Intensity");
     TabWS->addColumn("double","Mcol");
     TabWS->addColumn("double","Mrow");
     TabWS->addColumn("double","SScol");
     TabWS->addColumn("double","SSrow");
     TabWS->addColumn("double","SSrc");
     TabWS->addColumn("double","NCells");
     TabWS->addColumn("double","ChiSqr");
     TabWS->addColumn("double","ActIntensity");
}
// returns StartRow,StartCol,NRows,NCols
std::vector<int> getStartNRowCol( double CentRow,
                                  double CentCol,
                                  int dRow,
                                  int dCol,
                                  std::vector<int> NRowsCols)
    {
      std::vector<int> Res;
      double StartRow = CentRow - (int)(dRow/2);
      if(StartRow <3)
        StartRow = 3;
      Res.push_back( (int)StartRow);

      double StartCol = CentCol - (int)(dCol/2);
      if(StartCol <3)
          StartCol = 3;
      Res.push_back( (int)StartCol);

     double EndRow = CentRow + (int)(dRow/2);
     if(EndRow  > NRowsCols[0]-3)
          EndRow = NRowsCols[0]-3;
     Res.push_back( (int)(EndRow-StartRow+1));


     double EndCol = CentCol + (int)(dCol/2);
     if(EndCol >NRowsCols[0]-3)
         EndCol =NRowsCols[0]- 3;
     Res.push_back( (int)(EndCol -StartCol +1));

     return Res;

    }

void updateStats( double intensity, int row, int col,std::vector<double>  StatBase )
{
  StatBase[ ISSIxx]=col*col*intensity;
  StatBase[ ISSIyy]=intensity*row*row;
  StatBase[ ISSIxy]=intensity*row*col;
  StatBase[ ISSxx]=col*col;
  StatBase[ ISSyy]=row*row;
  StatBase[ ISSxy]=row*col;
  StatBase[ ISSIx]=intensity*col;
  StatBase[ ISSIy]=intensity*row;
  StatBase[ ISSx]=col;
  StatBase[ ISSy]=row;
  StatBase[ IIntensities]=intensity;
}

std::vector<double> getInitParamValues(std::vector<double> StatBase, double TotBoundaryIntensities,
    int nBoundaryCells)
{
  double b = 0;
  if (nBoundaryCells > 0)
    b = TotBoundaryIntensities / nBoundaryCells;
  int nCells = StatBase[INRows] * StatBase[INCol];
  double Den = StatBase[IIntensities] - b * nCells;
  if (Den <= 0)
  {
    b = 0;
    Den = StatBase[IIntensities];
  }
  bool done = false;
  double Mx ,My,Sxx,Syy ,Sxy;
  while (!done)
  {
    Mx = StatBase[ISSIx] - b * StatBase[ISSx];
    My = StatBase[ISSIy] - b * StatBase[ISSy];
    Sxx = (StatBase[ISSIxx] - b * StatBase[ISSxx] - Mx * Mx / Den) / Den;
    Syy = (StatBase[ISSIyy] - b * StatBase[ISSyy] - My * My / Den) / Den;
    Sxy = (StatBase[ISSIxy] - b * StatBase[ISSxy] - Mx * My / Den) / Den;
    done = true;
    if (Sxx <= 0 || Syy <= 0 || Sxy * Sxy / Sxx / Syy > .8)
    {

      if (b > 0)
        done = false;

      b = 0;
    }
  }
  std::vector<double> Res;
  Res.push_back(b);
  Res.push_back(StatBase[IIntensities]);
  Res.push_back( Mx/Den);
  Res.push_back(My/Den);
  Res.push_back( Sxx);
  Res.push_back( Syy);
  Res.push_back( Sxy);
  return Res;
}

//Data is the slice subdate, inpWkSpace is ALl the data,
std::vector<std::vector<double> >  SetUpData(MatrixWorkspace_sptr Data,MatrixWorkspace_sptr inpWkSpace,
                  boost::shared_ptr<const RectangularDetector> panel,int chan,std::vector<int>Attr)
    {
        boost::shared_ptr<Workspace1D> ws = boost::shared_dynamic_cast<Workspace1D>( Data);

        std::vector<double>  StatBase;
        for( int i=0; i<15+2; i++ )
          StatBase.push_back(0);

        Kernel::cow_ptr<  MantidVec  > pX;

        MantidVec& xRef = pX.access();
        for( int j=0; j <Attr[2]*Attr[3]+1; j++ )
        {
          xRef.push_back(j);
        }
        Data->setX(0, pX );

        MantidVecPtr yvals;
        MantidVecPtr errs;
        MantidVec&yvalB = yvals.access();
        MantidVec &errB= errs.access();
        double TotBoundaryIntensities =0;
        int nBoundaryCells =0;
        for( int row= Attr[0]; row < Attr[0]+Attr[2]; row++)
          for( int col=Attr[1];col< Attr[1]+Attr[3]; col++)
          {
            boost::shared_ptr<  Detector  >   pixel =panel->getAtXY( col,row);
            int spectra = getSpectra( pixel->getID(),inpWkSpace);
            MantidVec histogram =inpWkSpace->dataY(spectra);
            double intensity = histogram[ chan ];
            yvalB.push_back(intensity);
            errB.push_back(1);
            updateStats( intensity, row, col, StatBase);
            if( row==Attr[0]||col ==Attr[1])
            {
              TotBoundaryIntensities +=intensity;
              nBoundaryCells++;
            }else if( row== (Attr[0]+Attr[2]-1)|| col==(Attr[1]+Attr[3]-1))
            {

              TotBoundaryIntensities +=intensity;
              nBoundaryCells++;
            }
          }
        ws->setData(yvals, errs);
        StatBase[ IStartRow]=Attr[0];
          StatBase[   IStartCol]= Attr[1];
          StatBase[ INRows]=Attr[2];
          StatBase[ INCol]=Attr[3];
        std::vector<double> InitParams= getInitParamValues( StatBase,
                                        TotBoundaryIntensities,nBoundaryCells);
        std::vector<std::vector<double> > Res;
        Res.push_back(InitParams);
        Res.push_back(StatBase);

        return Res;
    }

std::string CalcFunctionProperty( std::vector<std::vector<double> > ParamAttr)
{
  std::vector<double> ParamValues = ParamAttr[0];
  std::vector<double> AttrValues = ParamAttr[0];

  std::ostringstream fun_str;
  fun_str <<"name=BivariateNormal,";
  for( int i=0; i<7; i++)
    fun_str<<ParamNames[i]<<"="<<ParamValues[i]<<",";

  for( int j=0; j<AttrValues.size(); j++)
  {
    fun_str <<AttrNames[j]<<"="<<AttrValues[j];
    if( j +1 < AttrValues.size())
      fun_str <<",";
  }
    return fun_str.str();
}

int find( std::string oneName, std::vector<std::string> nameList)
{
  for( int i=0; i< nameList.size(); i++)
    if( oneName.compare(nameList[i])==0)
      return i;
}
  bool GoodFit1(std::vector<double >params,std::vector<double >errs,std::vector<std::string >names,
                        double chisq, std::vector<std::vector<double > >ParamAttr )
  {
    int Ibk= find("Background", names);
    int IIntensity = find("Intensity", names);
    int IVx  = find("SScol", names);
    int IVy = find("SSrow", names);
    int IVxy = find("SSrc", names);
    if( chisq < 0)
      return false;

    std::vector<double>Attr = ParamAttr[1];
    int ncells = Attr[INRows]*Attr[INCol];
    if( Attr[IIntensities] <= 0 || (Attr[IIntensities]-params[Ibk]*ncells) <=0)
      return false;

    double x =params[IIntensity]/(Attr[IIntensities]-params[Ibk]*ncells);
    if( x > .8 || x < 1.25 )
      return false;

    //Assume will use Intensity parameter
    if( errs[ITINTENS]/params[ITINTENS] >.1)
      return false;

    //Check weak peak. Max theoretical height should be more than 3
    double maxPeakHeightTheoretical = params[ITINTENS]/2/M_PI/
        sqrt( params[IXMEAN]*params[IYMEAN]-params[IVXY]*params[IVXY]);
    if(maxPeakHeightTheoretical < 3)
      return false;
    return true;
  }

  void UpdateOutputWS( TableWorkspace_sptr TabWS,int dir,std::vector<double >params,std::vector<double >errs,
      std::vector<std::string>names, double chisq, std::vector<std::vector<double > >ParamAttr, std::string method)
  {
    int Ibk= find("Background", names);
       int IIntensity = find("Intensity", names);
       int IVx  = find("SScol", names);
       int IVy = find("SSrow", names);
       int IVxy = find("SSrc", names);
  }

  std::vector<std::vector<double> > FitwOldMethod(MatrixWorkspace_sptr Data,std::vector<std::vector<double > >ParamAttr, std::vector<double >params,
                         std::vector<std::string >names)
  {
    std::vector<std::vector<double> >Res;
    int Ibk= find("Background", names);
       int IIntensity = find("Intensity", names);
       int IVx  = find("SScol", names);
       int IVy = find("SSrow", names);
       int IVxy = find("SSrc", names);
    return Res;
  }

  void updatepeakInf( std::vector<double >params,std::vector<double >errs,std::vector<std::string >names,
                                 double &TotVariance, double &TotIntensity)
  {
    int Ibk= find("Background", names);
       int IIntensity = find("Intensity", names);
       int IVx  = find("SScol", names);
       int IVy = find("SSrow", names);
       int IVxy = find("SSrc", names);
  }

