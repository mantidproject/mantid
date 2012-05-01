#include "MantidCurveFitting/BivariateNormal.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>

using namespace Mantid::API;

namespace Mantid
{
  namespace CurveFitting
  {

DECLARE_FUNCTION(BivariateNormal)

// Indicies into Attrib array( local variable in initCommon
#define S_int   0
#define S_xint   1
#define S_yint   2
#define S_x2int   3
#define S_y2int   4
#define S_xyint   5
#define S_y   6
#define S_x   7
#define S_x2   8
#define S_y2   9
#define S_xy   10
#define S_1   11

//Indicies into the LastParams array
#define IBACK   0
#define ITINTENS   1
#define IXMEAN   2
#define IYMEAN   3
#define IVXX   4
#define IVYY   5
#define IVXY   6

Kernel::Logger& BivariateNormal::g_log= Kernel::Logger::get("BivariateNormal");

BivariateNormal::BivariateNormal() : API::IFunction1D(), expVals(NULL)

{
  LastParams[IVXX] = -1;
//  g_log.setLevel(7);
   CalcVxx= CalcVyy= CalcVxy= false;
  //BackConstraint = IntensityConstraint= MeanxConstraint = MeanyConstraint = NULL;
  expVals = NULL;
  CalcVxx = CalcVxy = CalcVyy = false;
}

BivariateNormal::~BivariateNormal()
{

 if( expVals)
     delete [] expVals;

 expVals = NULL;



}

// overwrite IFunction base class methods

 void BivariateNormal::function1D(double *out, const double *xValues, const size_t nData) const
 {

   UNUSED_ARG(xValues);
   UNUSED_ARG(nData);

   double coefNorm, expCoeffx2, expCoeffy2, expCoeffxy;
   int NCells;
   bool isNaNs;
   API::MatrixWorkspace_const_sptr ws = getMatrixWorkspace();
   MantidVec D = ws->dataY(0);
   MantidVec X = ws->dataY(1);
   MantidVec Y = ws->dataY(2);
   initCoeff(D, X, Y, coefNorm, expCoeffx2, expCoeffy2, expCoeffxy, NCells);

   NCells = std::min<int>((int)nData, NCells);

   double Background = getParameter(IBACK);
   double Intensity = getParameter(ITINTENS);
   double Xmean = getParameter(IXMEAN);
   double Ymean = getParameter(IYMEAN);
   std::ostringstream inf;

   int x = 0;
   isNaNs = false;
   double chiSq = 0;
   inf << "F Parameters=";
   for (size_t k = 0; k < nParams(); k++)
     inf << "," << getParameter(k);
   inf << "\n";

   for (int i = 0; i < NCells; i++)
   {
     if (isNaNs)
       out[x] = 10000;
     else
     {
       double dx = X[i] - Xmean;
       double dy = Y[i] - Ymean;
       out[x] = Background + coefNorm * Intensity * exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy
           + expCoeffy2 * dy * dy);

       if (out[x] != out[x])
       {
         out[x] = 100000;
         isNaNs = true;
       }

     }
     double diff = out[x]-D[x];
   //  inf<<"("<<Y[i]<<","<<X[i]<<","<<out[x]<<","<<
   //      D[x]<<")";
     chiSq +=diff*diff;

     x++;
   }

   inf << "\n"<<"    chiSq =" << chiSq << "\n";
   g_log.debug(inf.str());

 }



void BivariateNormal::functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData)
{
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);

 initCommon();

  std::ostringstream inf;
  inf << "***Parameters=" ;
  for( size_t k=0; k < nParams();k++)
    inf << "," << getParameter(k);
  inf << "\n";
  g_log.debug( inf.str());

  std::vector<double>outt(nData);
  function1D(outt.data()  ,xValues,nData);
  if( nData <=0)
      return;

  double uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];

  API::MatrixWorkspace_const_sptr  ws= getMatrixWorkspace();
  MantidVec X =ws->dataY(1);
  MantidVec Y =ws->dataY(2);

  for( int x=0; x<NCells; x++)
   {
      double r = Y[x];
      double c = X[x];

      out->set(x, IBACK, 1.0);
      out->set(x, ITINTENS, expVals[x] * coefNorm);

      double coefExp = coefNorm * LastParams[ITINTENS];

      double coefxy = LastParams[IVXY] / uu;
      double coefx2 = -LastParams[IVYY] /2/ uu;


      out->set(x, IXMEAN, coefExp * expVals[x] * (-2*coefx2 * (c - LastParams[IXMEAN]) - coefxy * (r
          - LastParams[IYMEAN])));


      coefExp = coefNorm * LastParams[ITINTENS];

      double coefy2 = -LastParams[IVXX] / 2/uu;

      out->set(x, IYMEAN, coefExp * expVals[x] * (-coefxy * (c - LastParams[IXMEAN]) - 2*coefy2 * (r
          - LastParams[IYMEAN])));


      coefExp = coefNorm * LastParams[ITINTENS];

      double C = -LastParams[IVYY] / 2 / uu;

      double SIVXX =  coefExp * expVals[x] * (C +
                -LastParams[IVYY]/uu*  (coefx2 * (c - LastParams[IXMEAN])* (c- LastParams[IXMEAN]) +
              coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) + coefy2
                * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN]))
              -(r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN])/2/uu)   ;


      coefExp = coefNorm * LastParams[ITINTENS];

      C = -LastParams[IVXX] / 2 / uu;


      double SIVYY =coefExp * expVals[x] * (C +
          -LastParams[IVXX]/uu*  (coefx2 * (c - LastParams[IXMEAN])* (c- LastParams[IXMEAN]) +
        coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) + coefy2
          * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN]))
        -(c - LastParams[IXMEAN]) * (c - LastParams[IXMEAN])/2/uu)   ;



      coefExp = coefNorm * LastParams[ITINTENS];

      C = LastParams[IVXY] / uu;


      double SIVXY =coefExp * expVals[x] * (C +
          2*LastParams[IVXY]/uu*  (coefx2 * (c - LastParams[IXMEAN])* (c- LastParams[IXMEAN]) +
              coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) + coefy2
             * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN]))
        +(r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN])/uu)   ;


      if( !CalcVxx)
         out->set(x, IVXX, SIVXX);
      else
      {
       // out->set(x,IVXX,0.0);
        double bdderiv = out->get(x,IBACK);



        bdderiv += SIVXX*(-Sxx-(LastParams[IXMEAN]-mx)*(LastParams[IXMEAN]-mx)*TotN+
            LastParams[IVXX]*TotN) /(TotI -LastParams[IBACK]*
                    TotN);

        out->set(x, IBACK, bdderiv);

        double mxderiv = out->get(x, IXMEAN);
        mxderiv += SIVXX*(2*(LastParams[IXMEAN]-mIx)*

            TotI-2*LastParams[IBACK]*(LastParams[IXMEAN]
                - mx)*TotN)/(TotI -LastParams[IBACK]*TotN) ;
        out->set(x,IXMEAN, mxderiv);

      }
      if( !CalcVyy )
         out->set(x, IVYY, SIVYY);
      else
      {
       // out->set(x,IVYY, 0.0);
        double bdderiv = out->get(x,IBACK);

        bdderiv += SIVYY*(-Syy-(LastParams[IYMEAN]-my)*(LastParams[IYMEAN]-my)*TotN+
            LastParams[IVYY]*TotN) /(TotI -LastParams[IBACK]*
                    TotN);
        out->set(x, IBACK, bdderiv);
        double myderiv = out->get(x, IYMEAN);

        myderiv += SIVYY*(2*(LastParams[IYMEAN]-mIy)*

            TotI-2*LastParams[IBACK]*(LastParams[IYMEAN]
                - my)*TotN)/(TotI -LastParams[IBACK]*TotN) ;
        out->set(x,IYMEAN, myderiv);
      }
      if( !CalcVxy)
      {
        out->set(x, IVXY, SIVXY);

      }
      else
      {
       // out->set(x,IVXY, 0.0);
        double bdderiv = out->get(x,IBACK);
        bdderiv += SIVXY*(-Sxy-(LastParams[IYMEAN]-my)*(LastParams[IXMEAN]-mx)*TotN+
        LastParams[IVXY]*TotN) /(TotI -LastParams[IBACK]*
                  TotN);
         out->set(x, IBACK, bdderiv);
         double myderiv = out->get(x, IYMEAN);
          myderiv += SIVXY*((LastParams[IXMEAN]-mIx)*

          TotI-LastParams[IBACK]*(LastParams[IXMEAN]
              - mx)*TotN)/(TotI -LastParams[IBACK]*TotN) ;
         out->set(x,IYMEAN, myderiv);
         double mxderiv = out->get(x, IXMEAN);
          mxderiv += SIVXY*((LastParams[IYMEAN]-mIy)*
              TotI-LastParams[IBACK]*(LastParams[IYMEAN]
                               - my)*TotN)/(TotI -LastParams[IBACK]*TotN) ;
         out->set(x,IXMEAN, mxderiv);

      }

   }

}

void BivariateNormal::init()
{
  declareParameter("Background", 0.00);
  declareParameter("Intensity" , 0.00);
  declareParameter("Mcol" , 0.00, "Mean column(x) value");
  declareParameter("Mrow" , 0.00, "Mean row(y) value");
  declareParameter("SScol", 0.00, "Variance of the column(x) values");
  declareParameter("SSrow", 0.00, "Variance of the row(y) values");
  declareParameter("SSrc" , 0.00, "Covariance of the column(x) and row(y) values");

  CalcVariances = false;

  NCells = -1;
  LastParams[IVXX]=-1;

}

void BivariateNormal::initCommon()
 {


  bool ParamsOK = true;
  bool CommonsOK = true;
  if (!expVals )
    CommonsOK = false;



  API::MatrixWorkspace_const_sptr  ws= getMatrixWorkspace();
  MantidVec D =ws->dataY(0);
  MantidVec X =ws->dataY(1);
  MantidVec Y =ws->dataY(2);

  if( NCells < 0 )
  {
    NCells =(int) std::min<size_t>(D.size(), std::min<size_t>(X.size(), Y.size()));
    CommonsOK = false;
  }

  double Attrib[12]={0.0};

  double MinX, MinY, MaxX, MaxY, MaxD, MinD;
  MinX= MaxX = X[0];
  MinY= MaxY = Y[0];
  MaxD = MinD = D[0];

  if (!CommonsOK)
      {

        for (int i = 0; i < NCells; i++)
        {
          Attrib[S_int] += D[i];
          Attrib[S_xint] += D[i] * X[i];
          Attrib[S_yint] += D[i] * Y[i];
          Attrib[S_x2int] += D[i] * X[i] * X[i];
          Attrib[S_y2int] += D[i] * Y[i] * Y[i];
          Attrib[S_xyint] += D[i] * X[i] * Y[i];

          Attrib[S_y] += Y[i];
          Attrib[S_x] += X[i];
          Attrib[S_x2] += X[i] * X[i];
          Attrib[S_y2] += Y[i] * Y[i];
          Attrib[S_xy] += X[i] * Y[i];
          Attrib[S_1] += 1.0;

          if (X[i] < MinX)
            MinX = X[i];
          if (X[i] > MaxX)
            MaxX = X[i];
          if (Y[i] < MinY)
            MinY = Y[i];
          if (Y[i] > MaxY)
            MaxY = Y[i];
          if (D[i] < MinD)
            MinD = D[i];
          if (D[i] > MaxD)
            MaxD = D[i];

        }



        mIx = Attrib[S_xint] / Attrib[S_int];
        mIy = Attrib[S_yint] / Attrib[S_int];
        mx = Attrib[S_x] / Attrib[S_1];
        my = Attrib[S_y] / Attrib[S_1];

        SIxx = Attrib[S_x2int] - (Attrib[S_xint] * Attrib[S_xint]) / Attrib[S_int];
        SIyy = Attrib[S_y2int] - (Attrib[S_yint]) * (Attrib[S_yint]) / Attrib[S_int];
        SIxy = Attrib[S_xyint] - (Attrib[S_xint]) * (Attrib[S_yint]) / Attrib[S_int];

        Sxx = Attrib[S_x2] - (Attrib[S_x]) * (Attrib[S_x]) / Attrib[S_1];
        Syy = Attrib[S_y2] - (Attrib[S_y]) * (Attrib[S_y]) / Attrib[S_1];
        Sxy = Attrib[S_xy] - (Attrib[S_x]) * (Attrib[S_y]) / Attrib[S_1];


        //CommonsOK = false;

        TotI = Attrib[S_int];
        TotN = Attrib[S_1];


       // CommonsOK = false;

        if( getConstraint(0) == NULL)
        {

          addConstraint((new BoundaryConstraint(this, "Background", 0, Attrib[S_int] / Attrib[S_1])));

        }



        double maxIntensity = Attrib[S_int] + 3 * sqrt(Attrib[S_int]);

        if (maxIntensity < 100)
          maxIntensity = 100;


        if( getConstraint(1) == NULL)
        {
          addConstraint(new BoundaryConstraint(this, "Intensity", 0, maxIntensity));
        }


        double minMeany = MinY * .9 + .1 * MaxY;
        double maxMeany = MinY * .1 + .9 * MaxY;

        if( getConstraint(3) == NULL)
       {
          addConstraint(new BoundaryConstraint(this, "Mrow", minMeany, maxMeany));

        }

        double minMeanx = MinX * .9 + .1 * MaxX;
        double maxMeanx = MinX * .1 + .9 * MaxX;
        if( getConstraint(2)==NULL)
        {
          addConstraint(new BoundaryConstraint(this, "Mcol", minMeanx, maxMeanx));
        }


 //Cannot seem to change penalyt factor for LevenBerg algorithm. Causes
 // *  gsl_multifit_fdfsolver_set(m_gslSolver, &gslContainer, m_data->initFuncParams); to seg fault??
    for( int p=0; p < (int)nParams();p++)
        {
          IConstraint* constr = getConstraint( p );
          if( constr )
          {
            double penalty = constr->getPenaltyFactor() + 10 * MaxD;
            //std::cout<<"initCommon G"<<p<<","<<penalty<<std::endl;
            constr->setPenaltyFactor( penalty );

          }
        } //std::cout<<"initCommon H"<<std::endl;

//*/
        if (CalcVariances)
        {
          std::ostringstream ssxx, ssyy, ssxy;

          ssyy << std::string("(") << (SIyy) << "+(Mrow-" << (mIy) << ")*(Mrow-" << (mIy) << ")*"
              << Attrib[S_int] << "-Background*" << (Syy) << "-Background*(Mrow-" << (my) << ")*(Mrow-"
              << (my) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*"
              << (Attrib[S_1]) << ")";

          if (getTie(IVYY) == NULL)
          {
            tie("SSrow", ssyy.str());
            CalcVxx=true;

          }

          ssxx << std::string("(") << (SIxx) << "+(Mcol-" << (mIx) << ")*(Mcol-" << (mIx) << ")*"
              << Attrib[S_int] << "-Background*" << (Sxx) << "-Background*(Mcol-" << (mx) << ")*(Mcol-"
              << (mx) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*"
              << (Attrib[S_1]) << ")";

          if (getTie(IVXX) == NULL)
          {
            tie("SScol", ssxx.str());
            CalcVyy=true;
          }

          ssxy << std::string("(") << (SIxy) << "+(Mcol-" << (mIx) << ")*(Mrow-" << (mIy) << ")*"
              << Attrib[S_int] << "-Background*" << (Sxy) << "-Background*(Mcol-" << (mx) << ")*(Mrow-"
              << (my) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*"
              << (Attrib[S_1]) << ")";

          if (getTie(IVXY) == NULL)
          {
            tie("SSrc", ssxy.str());
            CalcVxy = true;
          }
        }
        CommonsOK = true;
      }

  if (LastParams[IVXX] < 0)
  {
    ParamsOK = false;
    CommonsOK = false;

  }
  else
    for (size_t i = 0; i < nParams() && ParamsOK; i++)
      if (getParameter(i) != LastParams[i])
        ParamsOK = false;


  if (!ParamsOK)
  {

        for (size_t i = 0; i < nParams(); i++)
          LastParams[i] = getParameter(i);

        double Varxx = LastParams[IVXX];

        if (CalcVxx)
        {
          Varxx = (SIxx + (getParameter("Mcol") - mIx) * (getParameter("Mcol") - mIx) * TotI
              - getParameter("Background") * Sxx - getParameter("Background") * (getParameter("Mcol")
              - (mx)) * (getParameter("Mcol") - (mx)) * TotN) / (TotI - getParameter("Background")
              * TotN);
          LastParams[IVXX] = Varxx;

        }

       double Varyy = LastParams[IVYY];

        if (CalcVyy)
        {
          Varyy = (SIyy + (getParameter("Mrow") - (mIy)) * (getParameter("Mrow") - (mIy)) * TotI
              - getParameter("Background") * (Syy) - getParameter("Background") * (getParameter("Mrow")
              - (my)) * (getParameter("Mrow") - (my)) * TotN) / (TotI - getParameter("Background")
              * TotN);
          LastParams[IVYY] = Varyy;
        }
        double Varxy = LastParams[IVXY];

        if (CalcVxy)
        {
          Varxy = ((SIxy) + (getParameter("Mcol") - (mIx)) * (getParameter("Mrow") - (mIy)) * TotI
              - getParameter("Background") * (Sxy) - getParameter("Background") * (getParameter("Mcol")
              - (mx)) * (getParameter("Mrow") - (my)) * TotN) / (TotI - getParameter("Background")
              * TotN);

          LastParams[IVXY] = Varxy;
        }

  }


  if (!CommonsOK || !ParamsOK)
  {

    int NCells1;
    initCoeff( D, X, Y, coefNorm,  expCoeffx2, expCoeffy2,  expCoeffxy,
                   NCells1);

    delete [] expVals;
    expVals = new double[NCells];

    for( int i=0; i< NCells;i++)
      {

        double dx = X[i] - LastParams[IXMEAN];
        double dy = Y[i] - LastParams[IYMEAN];
        expVals[i] = exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy + expCoeffy2 * dy * dy);

      }

  }

  }

void BivariateNormal::initCoeff( MantidVec &D,
                                 MantidVec &X,
                                 MantidVec &Y,
                                 double &coefNorm,
                                 double &expCoeffx2,
                                 double & expCoeffy2,
                                 double & expCoeffxy,
                                 int   &NCells) const
   {

     double Varxx = getParameter( IVXX);

     if( CalcVxx)
     {
       Varxx = (SIxx +(getParameter("Mcol")- mIx)*(getParameter("Mcol")-mIx)*

        TotI -getParameter("Background")*Sxx -getParameter("Background")*(getParameter("Mcol")
             - (mx))*(getParameter("Mcol")-(mx) )*TotN)/(TotI -getParameter("Background")*
            TotN) ;

     }

     double Varyy = getParameter( IVYY);


     if( CalcVyy)
     {
       Varyy = (SIyy +(getParameter("Mrow")- (mIy))*(getParameter("Mrow")- (mIy) )*
       TotI -getParameter("Background")* (Syy) -getParameter("Background")*(getParameter("Mrow")- (my) )*
       (getParameter("Mrow")-
        (my) )* TotN )/( TotI -getParameter("Background")*
            TotN );

     }
     double Varxy = getParameter( IVXY);


     if( CalcVxy)
     {
       Varxy =( (SIxy) +(getParameter("Mcol")- (mIx) )*(getParameter("Mrow") - (mIy) )*
           TotI -getParameter("Background")* (Sxy) -getParameter("Background")*(getParameter("Mcol")- (mx) )*
            (getParameter("Mrow")-
        (my) )* TotN)/( TotI -getParameter("Background")*
            TotN);

     }
     double uu = Varxx* Varyy - Varxy * Varxy;

     coefNorm = .5 / M_PI / sqrt(uu);

     expCoeffx2 = -Varyy / 2 / uu;
     expCoeffxy = Varxy / uu;
     expCoeffy2 = -Varxx / 2 / uu;


     NCells =(int) std::min<size_t>( D.size(), std::min<size_t>(X.size(),Y.size()));

   }


  }//namespace curveFitting
  }//namespaceMantid
