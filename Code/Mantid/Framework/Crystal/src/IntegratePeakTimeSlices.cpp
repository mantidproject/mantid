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
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/SpectraDetectorTypes.h"
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
   
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
//using namespace Mantid::CurveFitting;
//namespace Mantid
//{
namespace Crystal
{

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


IntegratePeakTimeSlices::IntegratePeakTimeSlices():Algorithm()
 {
  if( AttrNames.size() <3)
  {
    if( AttrNames.size() > 0)
       AttrNames.clear();
    AttrNames.push_back("StartRow");
    AttrNames.push_back("StartCol");
    AttrNames.push_back("NRows");
    AttrNames.push_back("NCols");
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
    if( ParamNames.size() > 0)
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

     // declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace","",Direction::Output), 
      //                                                       "Name of the output workspace with Log info");
     
      declareProperty(new WorkspaceProperty<PeaksWorkspace>("Peaks","",Direction::Input), "Workspace of Peaks");

      declareProperty("PeakIndex",0,"Index of peak in PeaksWorkspace to integrate");

      declareProperty("PeakQspan", .03, "Max magnitude of Q of Peak to Q of Peak Center. |Q|=1/d");
}


//----------------------- Declaration of methods used in exec method ---------------------------

void getnPanelRowsCols(Peak peak,  int &nPanelRows, int & nPanelCols, double & CellHeight,
                                            double & CellWidth);

int  getdChanSpan(Peak peak, double dQ, Mantid::MantidVec X, int specNum, int& Centerchan);

double getdRowSpan(  Peak peak, double dQ, double ystep, double xstep);

void SetUpOutputWSCols( TableWorkspace_sptr &TabWS);

int getSpectra( int PixID,MatrixWorkspace_sptr &mwspc);

boost::shared_ptr<const RectangularDetector> getPanel( Peak peak);

std::vector<int> getStartNRowCol( double CentRow,double CentCol, int dRow, int dCol, 
                                 int nPanelRows, int nPanelCols);

std::vector<std::vector<double> >  SetUpData(MatrixWorkspace_sptr Data,MatrixWorkspace_sptr inpWkSpace,
                boost::shared_ptr<const RectangularDetector> panel,int chan,std::vector<int>Attr);

std::string CalcFunctionProperty( std::vector<std::vector<double> > ParamAttr);

bool GoodFit1(std::vector<double >params,std::vector<double >errs,std::vector<std::string >names,
                                        double chisq, std::vector<std::vector<double > >ParamAttr ); 

void UpdateOutputWS( TableWorkspace_sptr &TabWS,int dir,std::vector<double >params,std::vector<double >errs,
    std::vector<std::string>names, double chisq, std::vector<std::vector<double > >ParamAttr, double time);


void updatepeakInf( std::vector<double >params,std::vector<double >errs,std::vector<std::string >names,
                               double &TotVariance, double &TotIntensity,double TotSliceIntensity, double chisqdivDOF, int ncelss);

int find( std::string oneName, std::vector<std::string> nameList);

void show( std::string Descr,TableWorkspace_sptr &table,int StringCol);

bool EnoughData( std::vector<std::vector<double > >ParamAttr );

//___________________________________ end methods start exec -----------------------------

void IntegratePeakTimeSlices::exec()
{
    int nPanelRows, 
        nPanelCols;
    double CellHeight, 
          CellWidth;

    double dQ = getProperty("PeakQspan");

    MatrixWorkspace_sptr inpWkSpace = getProperty("InputWorkspace");
    PeaksWorkspace_sptr peaksW;
    peaksW =getProperty("Peaks");

    int indx = getProperty("PeakIndex");
    IPeak &peak =peaksW->getPeak(indx);
    

    double TotVariance=0;
    double TotIntensity = 0;
    int lastRow= peak.getRow();
    int Row0=lastRow;
    int lastCol = peak.getCol();
    int Col0 =lastCol;

    TableWorkspace_sptr TabWS =boost::shared_ptr<TableWorkspace>( new TableWorkspace(0));

    try{
   
    int detID= peak.getDetectorID();
    int specNum= getSpectra(detID, inpWkSpace);

    getnPanelRowsCols( peak, nPanelRows, nPanelCols, CellHeight, CellWidth);
    double dRow = getdRowSpan( peak,dQ, CellHeight, CellWidth);
    
    int Chan;

    Mantid::MantidVec X = inpWkSpace->dataX( specNum);
    int dChan = getdChanSpan( peak,dQ,X, specNum,Chan);

    
    SetUpOutputWSCols( TabWS);


    boost::shared_ptr<const RectangularDetector> panel = getPanel( peak );
    IAlgorithm_sptr  fit_alg;
    double time;
    int ncells;
    //std::cout<<"Time/chan middle="<<peak.getTOF()<<","<<Chan<<","<<dChan<<std::endl;
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
           int xchan = Chan+dir*chan;
           if( nchan <=1)
             nchan =1;

           size_t topIndex = xchan+1;
           if( topIndex >=X.size())
              topIndex = X.size()-1;
           time = (X[xchan]+X[ topIndex ])/2.0;   
         
           std::vector<std::vector<double> > ParamAttr;
     
           std::vector<int> Attr= getStartNRowCol( lastRow,//Row0+(lastRow-Row0)/nchan*(chan+1.0),
                             lastCol,//Col0+(lastCol-Col0)/nchan*(chan+1.0), 
                              2*(int)dRow, 2*(int) dRow, 
                              nPanelRows,nPanelCols);

           
           MatrixWorkspace_sptr Data = WorkspaceFactory::Instance().create(
                              std::string("Workspace2D"), 2,  Attr[2]*Attr[3]+1, Attr[2]*Attr[3] );
          
           ParamAttr = SetUpData(Data,inpWkSpace,panel,xchan,Attr);
           
           ncells = (int)(Attr[INRows]*Attr[INCol]);

           std::vector<double> params;
           std::vector<double> errs;
           std::vector<std::string> names;
           if( EnoughData( ParamAttr))
           {
              fit_alg = createSubAlgorithm("Fit");
             
              fit_alg->setProperty("InputWorkspace", Data);
              fit_alg->setProperty("WorkspaceIndex", 0);
              fit_alg->setProperty("StartX", 0.0);
              fit_alg->setProperty("EndX", 0.0+Attr[2]*Attr[3]);
              fit_alg->setProperty("MaxIterations", 5000);
              fit_alg->setProperty("Output", "fit");

              std::string fun_str =CalcFunctionProperty( ParamAttr); 

              fit_alg->setProperty("Function", fun_str);
              //std::cout<<"function string ="<<fun_str<<std::endl;
              fit_alg->executeAsSubAlg();
              //std::cout<<"Through fit execution"<<std::endl;

             /*    TableWorkspace_sptr ParamsR = boost::dynamic_pointer_cast<TableWorkspace>
                                (AnalysisDataService::Instance().retrieve("fit_Parameters"));
	       std::cout<<"Retrieved pARAMETERS"<<std::endl;
              // Workspace2D_sptr Dat= boost::dynamic_pointer_cast<Workspace2D>
              //                    (AnalysisDataService::Instance().retrieve("fit_Workspace"));
	      //std::cout<<"Retrieved Workspace"<<std::endl;
	   
             TableWorkspace_sptr Cov = boost::dynamic_pointer_cast<TableWorkspace>
                       (AnalysisDataService::Instance().retrieve("fit_NormalisedCovarianceMatrix"));

             show( "Parameters",ParamsR,0);
             show("Covariances",Cov,0);
	    */
     

              double chisq = fit_alg->getProperty("OutputChi2overDoF");

              params = fit_alg->getProperty("Parameters");
              errs = fit_alg->getProperty("Errors");
              names = fit_alg->getProperty("ParameterNames");
              //std::cout<<"Params"<<std::endl;
              //for( size_t kk=0;kk<params.size();kk++)
              //    std::cout<<params[kk]<<"  ,  " ;

              //std::cout<<std::endl;

              if( GoodFit1( params,errs,names, chisq, ParamAttr))
              {
                 UpdateOutputWS( TabWS, dir, params,errs,names, chisq, ParamAttr, time);
                 double TotSliceIntensity = ParamAttr[1][IIntensities];
                 updatepeakInf( params,errs,names, TotVariance, TotIntensity,
	                     TotSliceIntensity,chisq, ncells);

              }else
           
                 done = true;

           }else
           {
              done =true;
              std::cout<<"Not Enuf data done=true"<<std::endl;
           }


           if( !done)
           {
              
             //Now set up the center for this peak
              int i = find( "Mrow", names);
              lastRow = (int)params[i];
              i = find("Mcol", names);
              lastCol= (int)params[i];
            
            }



    }
    }

    }catch( std::exception &EE1)
    {
        std::cout<<"Error in main reaspm="<< EE1.what()<<std::endl;
        throw;
    }
    
    peak.setIntensity( TotIntensity);
    peak.setSigmaIntensity( sqrt(TotVariance));
    std::cout<<"Result TotIntensity, stdDev="<<TotIntensity<<","<<sqrt(TotVariance)<<std::endl;  
    std::cout<<"Set in pksworkspc "<<peaksW->getPeak(indx).getIntensity()<<std::endl;
   declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace","",Direction::Output), 
                             "Name of the output workspace with Log info");
    AnalysisDataService::Instance().add("OutputWorkspace", TabWS);
    setPropertyValue("OutputWorkspace","OutputWorkspace");
    //setProperty<TableWorkspace_sptr>( "OutputWorkspace", TabWS);
     

     

 declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksResult","",Direction::Output), "Resultant Workspace of Peaks");


    PeaksWorkspace_sptr pks = peaksW;
    AnalysisDataService::Instance().add("PeaksResult", pks);
    
    this->setPropertyValue("PeaksResult","PeaksResult");
   // this->setProperty<PeaksWorkspace_sptr>("PeaksResult",pks);
  show( "ResTable",TabWS,-1);
   // set output workspace property


}

//Gets the Rectangular detector from an instrument
boost::shared_ptr<const RectangularDetector> getPanel( Peak peak)
{

  Mantid::Geometry::IInstrument_const_sptr Iptr= peak.getInstrument() ;
  std::string bankName = peak.getBankName();

   boost::shared_ptr< const IComponent > parent = Iptr->getComponentByName( bankName);

   if( parent->type().compare("RectangularDetector") !=0)
     {

       std::cout<<"   getPanel C type="<<parent->type()<<std::endl;
       throw std::runtime_error("Improper Peak Argument");
     }
   
   boost::shared_ptr<const RectangularDetector>RDet =
               boost::shared_dynamic_cast<const RectangularDetector>(parent);
   return RDet;
}

// first element of )NrowsCols in the # of rows,the second is the # of columns
void getnPanelRowsCols(Peak peak, int &nPanelRows, int &nPanelCols,
                                  double &CellHeight, double &CellWidth  )
{

  boost::shared_ptr<const RectangularDetector>RDet = getPanel( peak);


   nPanelRows = ( RDet->ypixels());
   nPanelCols = ( RDet->xpixels());
   CellHeight = (RDet->ystep());
   CellWidth = RDet->xstep();//TODO why not int

 }

double getQ( Peak peak)
{
  V3D pos =peak.getDetPos();

  std::vector<double> x;
  std::vector<double> y;

  x.push_back( peak.getTOF());

  Mantid::Kernel::Units::MomentumTransfer MT;

  double twoTheta = acos(pos.Z()/pos.norm());

  MT.fromTOF( x, y,peak.getL1(), peak.getL2(),twoTheta, 0,1.0,1.0);

  return x[0];

  }

//returns workspace index
int getSpectra( int PixID,  MatrixWorkspace_sptr &mwspc)
{
   Mantid::detid2index_map *map = mwspc->getDetectorIDToWorkspaceIndexMap(true);
   Mantid::detid_t  id = (Mantid::detid_t)PixID;

   Mantid::detid2index_map::iterator  it;

   it = (*map).find(id);

   size_t Res = (*it).second;

   return (int)Res;    

  }

double getdRowSpan( Peak peak, double dQ, double ystep, double xstep)
{
  

  double Q=0,ScatAngle=0,dScatAngle=0,DetSpan=0, nrows=0, ncols=0;

  try
  {
  Q = getQ( peak)/2/M_PI;


  V3D pos = peak.getDetPos();


  ScatAngle = asin(pos.Z()/pos.norm());


  dScatAngle = 2*dQ/Q*tan(ScatAngle);
  //std::cout<<" getdRowSpan D,d2thet"<<dScatAngle<<std::endl;

  DetSpan = pos.norm()*dScatAngle;  //s=r*theta

  DetSpan = fabs(DetSpan);
  nrows = DetSpan/( ystep);//1.5 is heuristic.


  ncols = DetSpan/xstep;

  if( nrows > ncols)
  {  
    if( nrows < 5)
     {
       return 5;
     }
    else
      return nrows;

  }

  if( ncols < 5)
  {
    return 5;
   }

  else

    return ncols;

  }catch( std::exception &s)
  {
    std::cout<< "err in getNRowsCols, reason="<<s.what()<<std::endl;
    return 0;
  }

}

//Finds an item in a MantidVec of time bin boundaries
int find( Mantid::MantidVec X, double time)
{
 int sgn = 1;

 if( X[0] > X[1])
   sgn = -1;

 if(sgn*( X[0]-time) >=0)
   return 0;

 if( sgn*( time-X[X.size()-1])>=0)
   return (int)X.size()-1;

  for( size_t i=0; i< X.size()-(size_t)1; i++)
  {
    if( sgn*(time-X[i])>=0 && sgn*(X[i+(size_t)1]-time) >=0)
      return (int)i;
  }

  return -1;
}


int  getdChanSpan(Peak peak, double dQ, Mantid::MantidVec X,int spectra, int &Centerchan)
{
  UNUSED_ARG( spectra);
  double Q = getQ( peak)/2/M_PI;

  V3D pos = peak.getDetPos();
  double time = peak.getTOF();
  double dtime = dQ/Q*time;

  
//std::cout<<"XScale:";
 // for( size_t i=0; i < X.size(); i++)
//      std::cout<<X[i]<<",";
//  std::cout<<std::endl;
  int chanCenter = find( X, time);

  Centerchan = chanCenter;
  int chanLeft = find(X, time-dtime);
  int chanRight = find( X, time + dtime);

  int dchan = abs(chanCenter-chanLeft);

  if( abs(chanRight-chanCenter) >dchan)
     dchan = abs(chanRight-chanCenter);

  return 2*dchan+6;//heuristic should be a lot more
}


void SetUpOutputWSCols( TableWorkspace_sptr &TabWS)
{
     TabWS->addColumn("double","Time");
     TabWS->addColumn("double","Background");
     TabWS->addColumn("double","Intensity");
     TabWS->addColumn("double","Mcol");
     TabWS->addColumn("double","Mrow");
     TabWS->addColumn("double","SScol");
     TabWS->addColumn("double","SSrow");
     TabWS->addColumn("double","SSrc");
     TabWS->addColumn("double","NCells");
     TabWS->addColumn("double","ChiSqr");
     TabWS->addColumn("double","TotIntensity");
     TabWS->addColumn("double","BackgroundError");
     TabWS->addColumn("double","FitIntensityError");
     TabWS->addColumn("double","ISAWIntensity");
     TabWS->addColumn("double","ISAWIntensityError");
     TabWS->addColumn("double", "Start Row");
     TabWS->addColumn("double", "End Row");
     TabWS->addColumn("double", "Start Col");
     TabWS->addColumn("double", "End Col");
}

// returns StartRow,StartCol,NRows,NCols
std::vector<int> getStartNRowCol( double CentRow,
                                  double CentCol,
                                  int dRow,
                                  int dCol,
                                  int nPanelRows,
                                  int nPanelCols
                                )
    {
      //std::cout<<"Args getStartRC="<<CentRow<<","<<CentCol<<","<<dRow<<","<<dCol<<","<<nPanelRows<<","<<nPanelCols<<std::endl;
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
     if(EndRow  > nPanelRows-3)
          EndRow =nPanelRows-3;

     Res.push_back( (int)(EndRow-StartRow+1));


     double EndCol = CentCol + (int)(dCol/2);
     if(EndCol >nPanelCols-3)
         EndCol =nPanelCols- 3;

     Res.push_back( (int)(EndCol -StartCol +1));

     return Res;

    }

void updateStats( double intensity, int row, int col,std::vector<double> & StatBase )
{
  StatBase[ ISSIxx]+=col*col*intensity;
  StatBase[ ISSIyy]+=intensity*row*row;
  StatBase[ ISSIxy]+=intensity*row*col;
  StatBase[ ISSxx]+=col*col;
  StatBase[ ISSyy]+=row*row;
  StatBase[ ISSxy]+=row*col;
  StatBase[ ISSIx]+=intensity*col;
  StatBase[ ISSIy]+=intensity*row;
  StatBase[ ISSx]+=col;
  StatBase[ ISSy]+=row;
  StatBase[ IIntensities]+=intensity;
}

std::vector<double> getInitParamValues(std::vector<double> StatBase, double TotBoundaryIntensities,
    int nBoundaryCells)
{
  double b = 0;
  if (nBoundaryCells > 0)
    b = TotBoundaryIntensities / nBoundaryCells;

  int nCells = (int)(StatBase[INRows] * StatBase[INCol]);
  double Den = StatBase[IIntensities] - b * nCells;

  if (Den <= 0)
  {
    b = 0;
    Den = (StatBase[IIntensities]);
    if( Den <=1) Den = 1;
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

        done = false;
      Den =StatBase[IIntensities];
      if( Den <=1)
        Den = 1;
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
        boost::shared_ptr<Workspace2D> ws = boost::shared_dynamic_cast<Workspace2D>( Data);
        std::vector<double>  StatBase;
        for( int i=0; i<15+2; i++ )
          StatBase.push_back(0);

        Mantid::MantidVecPtr pX;

        Mantid::MantidVec& xRef = pX.access();
        for( int j=0; j <Attr[2]*Attr[3]+1; j++ )
        {
          xRef.push_back(j);
        }

        Data->setX(0, pX );
        Data->setX(1,pX);

        Mantid::MantidVecPtr yvals;
        Mantid::MantidVecPtr errs;

        Mantid::MantidVec&yvalB = yvals.access();
        Mantid::MantidVec &errB= errs.access();
        
//         std::cout<<"Data for chan "<< chan <<std::endl;
        double TotBoundaryIntensities =0;
        int nBoundaryCells =0;
        for( int row= Attr[0]; row < Attr[0]+Attr[2]; row++)
          for( int col=Attr[1];col< Attr[1]+Attr[3]; col++)
          {
            boost::shared_ptr<  Detector  >   pixel =panel->getAtXY( col,row);
            int spectra = getSpectra( pixel->getID(),inpWkSpace);
            Mantid::MantidVec histogram =inpWkSpace->dataY(spectra);

            double intensity = histogram[ chan ];
//            std::cout<<intensity<<",";
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
//        std::cout<<std::endl;
        ws->setData(0,yvals, errs);
        StatBase[ IStartRow]=Attr[0];
        StatBase[   IStartCol]= Attr[1];
        StatBase[ INRows]=Attr[2];
        StatBase[ INCol]=Attr[3];

        std::vector<double> InitParams= getInitParamValues( StatBase,
                                        TotBoundaryIntensities,nBoundaryCells);

        std::vector<std::vector<double> > Res;
        Res.push_back(InitParams);
        Res.push_back(StatBase);

       // for( int k=0; k<InitParams.size();k++)
      //      std::cout<< InitParams[k]<<std::endl;
       // std::cout<<std::endl;
       // std::cout<<"Stats from setup"<< std::endl;
       // for( int k1=0; k1<StatBase.size();k1++)
       //     std::cout<<StatBase[k1]<<std::endl;;
       // std::cout<<std::endl;
        return Res;
    }


std::string CalcFunctionProperty( std::vector<std::vector<double> > ParamAttr)
{ 

  std::vector<double> ParamValues = ParamAttr[0];

  std::vector<double> AttrValues = ParamAttr[1];

  std::ostringstream fun_str;

  fun_str <<"name=BivariateNormal,";

  for( int i=0; i<7; i++)
    fun_str<<ParamNames[i]<<"="<<ParamValues[i]<<", ";

  for( size_t j=0; j<AttrNames.size(); j++)
  {
    fun_str <<AttrNames[j]<<"="<<AttrValues[j];
    if( j +1 < AttrNames.size())
      fun_str <<", ";
  }

  return fun_str.str();
}

int find( std::string oneName, std::vector<std::string> nameList)
{
  for( size_t i=0; i< nameList.size(); i++)
    if( oneName.compare(nameList[i])== (int)0)
      return (int)i;

  return -1;
}
 

bool GoodFit1(std::vector<double >params,
              std::vector<double >errs,
             std::vector<std::string >names,
             double chisq, 
             std::vector<std::vector<double > >ParamAttr )
  {
    int Ibk= find("Background", names);
    int IIntensity = find("Intensity", names);
 
    std::vector<double>Attr = ParamAttr[1];
 
    if( chisq < 0)
      return false;

    
    int ncells = (int)(Attr[INRows]*Attr[INCol]);
 

    if( Attr[IIntensities] <= 0 || (Attr[IIntensities]-params[Ibk]*ncells) <=0)
      {  //std::cout<<"A"<<Attr[IIntensities]<<","<<(Attr[IIntensities]-params[Ibk]*ncells)<<std::endl;
           return false;
       }
 

    double x =params[IIntensity]/(Attr[IIntensities]-params[Ibk]*ncells);

 
    if( x < .8 || x >1.25 )// The fitted intensity should be close to tot intensity - background
      {  //std::cout<<"B"<<x<<std::endl;
         return false;
      }


    //Assume will use Intensity parameter
    //double Err = errs[ITINTENS]*sqrt(chisq/ncells*4);//Use this error .
    if( errs[ITINTENS]*sqrt(chisq)/params[ITINTENS] >.1)
      {//std::cout<<"C"<<( errs[ITINTENS]*sqrt(chisq)/params[ITINTENS] )<<std::endl;
        return false;
       }
    
    
    //Check weak peak. Max theoretical height should be more than 3

    double maxPeakHeightTheoretical = params[ITINTENS]/2/M_PI/
        sqrt( params[IVXX]*params[IVYY]-params[IVXY]*params[IVXY]);
    
    
    if(maxPeakHeightTheoretical < 1.5)
      {  //std::cout<<"D"<<maxPeakHeightTheoretical<<std::endl;
         return false;
      }
     
    
    return true;
  }

  //Calculate error used by ISAW. It "doubles" the background error.
  double CalcIsawIntError( double background, double backError, double ChiSqOverDOF, double TotIntensity, int ncells)
  {
    UNUSED_ARG(ChiSqOverDOF)
    double Variance = TotIntensity + (backError*backError*TotIntensity/ncells) *ncells*ncells +
                                  background*ncells;
    return sqrt(Variance);
                                      
  }


  void UpdateOutputWS( TableWorkspace_sptr &TabWS,
                       int dir,
                       std::vector<double >params,
                       std::vector<double >errs,
                       std::vector<std::string>names, 
                       double chisq, 
                       std::vector<std::vector<double > >ParamAttr,
                       double time)
  {
    int Ibk= find("Background", names);
    int IIntensity = find("Intensity", names);
    int IVx  = find("SScol", names);
    int IVy = find("SSrow", names);
    int IVxy = find("SSrc", names);
    int Irow =find("Mrow", names);
    int Icol =find("Mcol", names);
       
    int newRowIndex =0;
    if( dir>0)
       newRowIndex = TabWS->rowCount();
    int TableRow = TabWS->insertRow( newRowIndex);
       
    int ncells =(int)(ParamAttr[1][INRows]*ParamAttr[1][INCol]);
    TabWS->getRef<double>(std::string("Background"), TableRow)=params[Ibk];
    TabWS->getRef<double>(std::string("Intensity"),TableRow)=params[IIntensity];
    TabWS->getRef<double>(std::string("FitIntensityError"),TableRow)=errs[IIntensity]*sqrt(chisq);
    TabWS->getRef<double>(std::string("Mcol"),TableRow)=params[Icol];
    TabWS->getRef<double>(std::string("Mrow"),TableRow)=params[Irow];
    
    TabWS->getRef<double>(std::string("SScol"),TableRow)=params[IVx];
    TabWS->getRef<double>(std::string("SSrow"),TableRow)=params[IVy];
    TabWS->getRef<double>(std::string("SSrc"),TableRow)=params[IVxy];
    TabWS->getRef<double>(std::string("NCells"),TableRow)=ncells;
    TabWS->getRef<double>(std::string("ChiSqr"),TableRow)=chisq;

    TabWS->getRef<double>(std::string("TotIntensity"),TableRow)=ParamAttr[1][IIntensities];
    TabWS->getRef<double>(std::string("BackgroundError"),TableRow)=errs[Ibk]*sqrt(chisq);
    TabWS->getRef<double>(std::string("ISAWIntensity"),TableRow)=ParamAttr[1][IIntensities] ;
    TabWS->getRef<double>(std::string("ISAWIntensityError"),TableRow)=CalcIsawIntError( params[Ibk], 
                                                                                      errs[Ibk], 
										      chisq, 
										      ParamAttr[1][IIntensities],
										      ncells);
    TabWS->getRef<double>(std::string("Time"),TableRow)=time ;
    TabWS->getRef<double>(std::string("Start Row"),TableRow)=ParamAttr[1][IStartRow] ;
    TabWS->getRef<double>(std::string("End Row"),TableRow)=ParamAttr[1][IStartRow] + ParamAttr[1][INRows]-1;
    TabWS->getRef<double>(std::string("Start Col"),TableRow)=ParamAttr[1][IStartCol] ;
    TabWS->getRef<double>(std::string("End Col"),TableRow)=ParamAttr[1][IStartCol] + ParamAttr[1][INCol]-1 ;

       
  }

  void updatepeakInf( std::vector<double >params,
                      std::vector<double >errs,
                      std::vector<std::string >names,
                      double &TotVariance, 
                      double &TotIntensity, 
                      double TotSliceIntensity,
		      double chiSqdevDOF, 
                      int ncells)
  {
    int Ibk= find("Background", names);
    //int IIntensity = find("Intensity", names);
    
    double err =CalcIsawIntError( params[Ibk], errs[Ibk], 
				   chiSqdevDOF,
				   TotSliceIntensity, ncells);
				   
     TotIntensity +=TotSliceIntensity -params[IBACK]*ncells;
     TotVariance += err*err;    
    
  }

void show( std::string Descr, TableWorkspace_sptr &table,int StringCol)
{
   std::cout<<"        "<<Descr<<std::endl;
   std::vector<std::string> ColNames = table->getColumnNames();
   for( size_t i=0; i<ColNames.size();i++)
       std::cout<<ColNames[i]<<" ";
   std::cout<<std::endl;

   for( int row=0; row<table->rowCount(); row++)
   {
      for( int col=0; col<(int)ColNames.size(); col++)
        if( col==StringCol)
	  std::cout<<table->String(row,col);
	else
	   std::cout<< table->Double(row,col)<<" ";

      std::cout<<std::endl;

   }
}

bool EnoughData( std::vector<std::vector<double > >ParamAttr )
{
//If all "zeroes, const value" or all on one line can cause problems
//  Var x, Var y about 0(-background). and Varx*Vary-Covxy*Covxy ~=
  std::vector<double> Attr = ParamAttr[1];
  std::vector<double> Param     =ParamAttr[0];
  double VIx0_num = Attr[ISSIxx] -2*Param[IXMEAN]*Attr[ISSIx] +Param[IXMEAN]*Param[IXMEAN]*Attr[IIntensities];
  double VIy0_num = Attr[ISSIyy] -2*Param[IYMEAN]*Attr[ISSIy] +Param[IYMEAN]*Param[IYMEAN]*Attr[IIntensities];
  double VIxy0_num =Attr[ISSIxy] -Param[IXMEAN]*Attr[ISSIy] -Param[IYMEAN]*Attr[ISSIx] +Param[IYMEAN]*Param[IXMEAN]*Attr[IIntensities];

  double Vx0_num = Attr[ISSxx] -2*Param[IXMEAN]*Attr[ISSx] +Param[IXMEAN]*Param[IXMEAN]*Attr[INRows]*Attr[INCol];
  double Vy0_num = Attr[ISSyy] -2*Param[IYMEAN]*Attr[ISSy] +Param[IYMEAN]*Param[IYMEAN]*Attr[INRows]*Attr[INCol];
  double Vxy0_num =Attr[ISSIxy] -Param[IXMEAN]*Attr[ISSIy] -Param[IYMEAN]*Attr[ISSIx] +Param[IYMEAN]*Param[IXMEAN]*Attr[INRows]*Attr[INCol];

  double Denominator = Attr[IIntensities] -Param[IBACK]*Attr[INRows]*Attr[INCol];
 
  
  double Vx= (VIx0_num-Param[IBACK]*Vx0_num)/Denominator;
  double Vy= (VIy0_num-Param[IBACK]*Vy0_num)/Denominator;
  double Vxy= (VIxy0_num-Param[IBACK]*Vxy0_num)/Denominator;

  double Z =4*M_PI*M_PI*( Vx*Vy-Vxy*Vxy);
  if( fabs(Z)<.10)
     return false;

  return true;  


   
}
}//namespace Crystal
//}//namespace Mantid
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
