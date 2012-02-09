#include "MantidCurveFitting/BivariateNormal.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <sstream>

using namespace Mantid::API;

namespace Mantid
{
  namespace CurveFitting
  {
    DECLARE_FUNCTION(BivariateNormal)
    //Indices into the Attrib array
    #define StartRow   0
    #define StartCol   1
    #define NRows   2
    #define NCols   3
    #define S_int   4
    #define S_xint   5
    #define S_yint   6
    #define S_x2int   7
    #define S_y2int   8
    #define S_xyint   9
    #define S_y   10
    #define S_x   11
    #define S_x2   12
    #define S_y2   13
    #define S_xy   14
    #define S_1   15

    //Indices into the LastParams array
    #define IBACK   0
    #define ITINTENS   1
    #define IXMEAN   2
    #define IYMEAN   3
    #define IVXX   4
    #define IVYY   5
    #define IVXY   6

    Kernel::Logger& BivariateNormal::g_log= Kernel::Logger::get("BivariateNormal");

    BivariateNormal::BivariateNormal():BackgroundFunction()
    {
      LastParams = 0;
      SIxx = SIyy = SIxy = Sxx = Syy = Sxy = -1; //Var and CoVar calc from parameters
      expVals = 0;
      Attrib = new double[16];
      if( AttNames.size() >1)
        return;
      AttNames.push_back(std::string("StartRow"));
      AttNames.push_back(std::string("StartCol"));
      AttNames.push_back(std::string("NRows"));
      AttNames.push_back(std::string("NCols"));
      AttNames.push_back(std::string("Intensities"));
      AttNames.push_back(std::string("SSIx"));
      AttNames.push_back(std::string("SSIy"));
      AttNames.push_back(std::string("SSIxx"));
      AttNames.push_back(std::string("SSIyy"));
      AttNames.push_back(std::string("SSIxy"));
      AttNames.push_back(std::string("SSy"));
      AttNames.push_back(std::string("SSx"));
      AttNames.push_back(std::string("SSxx"));
      AttNames.push_back(std::string("SSyy"));
      AttNames.push_back(std::string("SSxy"));
      AttNames.push_back(std::string("NCells")); //Not Set
    }

    BivariateNormal::~BivariateNormal()
    {

      if (!Attrib)
        delete[] Attrib;

      if (!expVals)
        delete[] expVals;

      if (!LastParams)
        delete[] LastParams;

      if (!BackConstraint)
        delete BackConstraint;

      if (!MeanxConstraint)
        delete MeanxConstraint;

      if (!MeanyConstraint)
        delete MeanyConstraint;

      if (!IntensityConstraint)
        delete IntensityConstraint;

    }

    // overwrite IFunction base class methods

    void BivariateNormal::functionMW(double *out, const double *xValues, const size_t nData)const
    {

      UNUSED_ARG(xValues);
      int Nrows = (int) getAttribute("NRows").asDouble();
      int Ncols = (int) getAttribute("NCols").asDouble();
      //  double *Attrib = new double[nAttributes()];
      double *LastParams = new double[7];
      double *expVals = new double[Nrows*Ncols];
      double uu,coefNorm,expCoeffx2,expCoeffy2,expCoeffxy;
      bool isNaNs;


      initCommon(  LastParams,expVals, uu,coefNorm,expCoeffx2,
          expCoeffy2,expCoeffxy,isNaNs);
      std::ostringstream str;
      str<<"Params="<<LastParams[0]<<","<<LastParams[1]<<","<<LastParams[2]<<","
          <<LastParams[3]<<","<<LastParams[4]<<","<<LastParams[5]<<","<<LastParams[6] <<std::endl;
      g_log.debug(str.str());
      for (size_t i = 0; i < nData; i++)
      {
        if( isNaNs)
          out[i] =10000;
        else
        {
          out[i] = LastParams[IBACK] + coefNorm * LastParams[ITINTENS] * expVals[i];
          if( out[i] !=out[i])
          {
            out[i]=100000;
            isNaNs=true;
          }
        }

      }
      //delete[] Attrib;
      delete[] LastParams;
      delete[] expVals;
    }

    void BivariateNormal::fit(const std::vector<double>&,const std::vector<double>&)
    {
      initCommon();
    }

    void BivariateNormal::functionDerivMW(API::Jacobian *out, const double *xValues, const size_t nData)
    {
      UNUSED_ARG(xValues);
      UNUSED_ARG(nData);
      initCommon();
      int x = 0;

      double uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];
      int startRow = (int) getAttribute("StartRow").asDouble();
      int startCol = (int) getAttribute("StartCol").asDouble();
      int Nrows = (int) getAttribute("NRows").asDouble();
      int Ncols = (int) getAttribute("NCols").asDouble();

      if( nData <=0)
        return;

      for (int r = startRow; r < startRow + Nrows; r++)
      {
        for (int c = startCol; c < startCol + Ncols; c++)
        {
          out->set(x, IBACK, 1);
          out->set(x, ITINTENS, expVals[x] * coefNorm);

          double coefExp = coefNorm * LastParams[ITINTENS];
          double coefx = LastParams[IVYY] / uu;
          double coefy = -LastParams[IVXY] / uu;
          out->set(x, IXMEAN, coefExp * expVals[x] * (coefx * (c - LastParams[IXMEAN]) + coefy * (r
              - LastParams[IYMEAN])));

          uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];
          coefExp = coefNorm * LastParams[ITINTENS];
          coefx = -LastParams[IVXY] / uu;
          coefy = LastParams[IVXX] / uu;
          out->set(x, IYMEAN, coefExp * expVals[x] * (coefx * (c - LastParams[IXMEAN]) + coefy * (r
              - LastParams[IYMEAN])));

          uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];
          coefExp = coefNorm * LastParams[ITINTENS];
          double coefx2 = LastParams[IVYY] * LastParams[IVYY] / 2 / uu / uu;
          double coefy2 = LastParams[IVXY] * LastParams[IVXY] / 2 / uu / uu;

          double coefxy = -LastParams[IVXY] * LastParams[IVYY] / uu / uu;
          double C = -LastParams[IVYY] / 2 / uu;
          out->set(x, IVXX, coefExp * expVals[x] * (C + coefx2 * (c - LastParams[IXMEAN]) * (c
              - LastParams[IXMEAN]) + coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) + coefy2
              * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN])));

          uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];
          coefExp = coefNorm * LastParams[ITINTENS];
          coefx2 = LastParams[IVXY] * LastParams[IVXY] / 2 / uu / uu;

          coefy2 = LastParams[IVXX] * LastParams[IVXX] / 2 / uu / uu;

          coefxy = -LastParams[IVXY] * LastParams[IVXX] / uu / uu;
          C = -LastParams[IVXX] / 2 / uu;
          out->set(x, IVYY, coefExp * expVals[x] * (C + coefx2 * (c - LastParams[IXMEAN]) * (c
              - LastParams[IXMEAN]) + coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) + coefy2
              * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN])));

          uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];
          coefExp = coefNorm * LastParams[ITINTENS];
          coefx2 = -LastParams[IVYY] * LastParams[IVXY] / uu / uu;
          coefy2 = -LastParams[IVXX] * LastParams[IVXY] / uu / uu;
          coefxy = (uu + 2 * LastParams[IVXY] * LastParams[IVXY]) / uu / uu;
          C = LastParams[IVXY] / uu;
          out->set(x, IVXY, coefExp * expVals[x] * (C + coefx2 * (c - LastParams[IXMEAN]) * (c
              - LastParams[IXMEAN]) + coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) + coefy2
              * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN])));

          x++;
        }
      }
    }

    void BivariateNormal::init()
    {
      declareParameter("Background", 0.00);
      declareParameter("Intensity", 0.00);
      declareParameter("Mcol", 0.00);
      declareParameter("Mrow", 0.00);
      declareParameter("SScol", 0.00);
      declareParameter("SSrow", 0.00);
      declareParameter("SSrc", 0.00);
    }

    void BivariateNormal::initCommon()
    {
      bool ParamsOK = true;
      bool CommonsOK = true;
      if (!expVals)
        CommonsOK = false;

      if (SIxx < 0)
      {
        Attrib[S_1] = Attrib[NRows] * Attrib[NCols];
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

        ParamsOK = false;
        CommonsOK = false;

        BackConstraint = (new BoundaryConstraint(this, "Background", 0, Attrib[S_int] / Attrib[S_1]));
        addConstraint( BackConstraint);
        double maxIntensity = Attrib[S_int] + 3 * sqrt(Attrib[S_int]);
        if (maxIntensity < 100)
          maxIntensity = 100;
        IntensityConstraint = new BoundaryConstraint(this, "Intensity", 0, maxIntensity);
        addConstraint( IntensityConstraint);

        double minMeany = Attrib[StartRow] + Attrib[NRows] / 10;
        if (minMeany < Attrib[StartRow] + 2)
          minMeany = Attrib[StartRow] + 2;
        double maxMeany = Attrib[NRows] * .9;
        if (maxMeany > Attrib[NRows] - 2)
          maxMeany = Attrib[NRows] - 2;
        maxMeany += Attrib[StartRow];
        MeanyConstraint = new BoundaryConstraint(this, "Mrow", minMeany, maxMeany);

        addConstraint( MeanyConstraint);

        double minMeanx = Attrib[NCols] / 10;
        if (minMeanx < 3)
          minMeanx = 3;
        double maxMeanx = Attrib[NCols] * .9;
        if (maxMeanx > Attrib[NCols] - 2)
          maxMeanx = Attrib[NCols] - 2;
        maxMeanx += Attrib[StartCol];
        minMeanx += Attrib[StartCol];
        MeanxConstraint = new BoundaryConstraint(this, "Mcol", minMeanx, maxMeanx);
        addConstraint( MeanxConstraint);

      }

      if (!LastParams)
      {
        LastParams = new double[nParams()];
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

        std::ostringstream ssxx, ssyy, ssxy;

        ssyy << std::string("(") << (SIyy) << "+(Mrow-" << (mIy) << ")*(Mrow-" << (mIy) << ")*"
            << Attrib[S_int] << "-Background*" << (Syy) << "-Background*(Mrow-" << (my) << ")*(Mrow-"
            << (my) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1])
            << ")";

        // std::cout<<"row formula="<< ssyy.str()<<std::endl;
        if( getTie( IVYY) == NULL)
        {
          tie("SSrow", ssyy.str());

        }
        //  std::cout << "  ddK" <<pt->eval()<< std::endl;

        ssxx << std::string("(") << (SIxx) << "+(Mcol-" << (mIx) << ")*(Mcol-" << (mIx) << ")*"
            << Attrib[S_int] << "-Background*" << (Sxx) << "-Background*(Mcol-" << (mx) << ")*(Mcol-"
            << (mx) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1])
            << ")";

        // std::cout<<"col formula="<< ssxx.str()<<std::endl;
        //API::ParameterTie* ptx =
        if( getTie( IVXX) == NULL)
        {
          tie("SScol", ssxx.str());

        }
        // std::cout << "  ddK" <<ptx->eval()<< std::endl;

        ssxy << std::string("(") << (SIxy) << "+(Mcol-" << (mIx) << ")*(Mrow-" << (mIy) << ")*"
            << Attrib[S_int] << "-Background*" << (Sxy) << "-Background*(Mcol-" << (mx) << ")*(Mrow-"
            << (my) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1])
            << ")";

        //std::cout<<"cov formula="<< ssxy.str()<<std::endl;
        if( getTie( IVXY) == NULL)
        {
          tie("SSrc", ssxy.str());

        }
        //std::cout << "   ddK" <<ptxy->eval()<< std::endl;


        CommonsOK = false;
      }

      if (!CommonsOK)
      {

        double uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];

        coefNorm = .5 / M_PI / sqrt(uu);

        expCoeffx2 = -LastParams[IVYY] / 2 / uu;
        expCoeffxy = LastParams[IVXY] / uu;
        expCoeffy2 = -LastParams[IVXX] / 2 / uu;

        int x = 0;
        int startRow = (int) getAttribute("StartRow").asDouble();
        int startCol = (int) getAttribute("StartCol").asDouble();
        int Nrows = (int) getAttribute("NRows").asDouble();
        int Ncols = (int) getAttribute("NCols").asDouble();
        expVals = new double[Nrows * Ncols];

        for (int r = startRow; r < startRow + Nrows; r++)
          for (int c = startCol; c < startCol + Ncols; c++)
          {

            double dx = c - LastParams[IXMEAN];
            double dy = r - LastParams[IYMEAN];
            expVals[x] = exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy + expCoeffy2 * dy * dy);
            x++;
          }

      }
    }

    void BivariateNormal::initCommon( double* LastParams,double* expVals,
        double &uu,double &coefNorm,double &expCoeffx2,double &expCoeffy2,double &expCoeffxy,
        bool &isNaNs)const
    {
      //std::cout<<"in INIt Common1"<< std::endl;
      isNaNs= false;
      for (size_t i = 0; i < nParams(); i++)
      {
        LastParams[i] = getParameter(i);
      }
      uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];

      coefNorm = .5 / M_PI / sqrt(uu);

      expCoeffx2 = -LastParams[IVYY] / 2 / uu;
      expCoeffxy = LastParams[IVXY] / uu;
      expCoeffy2 = -LastParams[IVXX] / 2 / uu;

      int x = 0;
      int startRow = (int) getAttribute("StartRow").asDouble();
      int startCol = (int) getAttribute("StartCol").asDouble();
      int Nrows = (int) getAttribute("NRows").asDouble();
      int Ncols = (int) getAttribute("NCols").asDouble();
      //expVals = new double[Nrows * Ncols];

      for (int r = startRow; r < startRow + Nrows; r++)
        for (int c = startCol; c < startCol + Ncols; c++)
        {

          double dx = c - LastParams[IXMEAN];
          double dy = r - LastParams[IYMEAN];
          expVals[x] = exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy + expCoeffy2 * dy * dy);
          if( expVals[x]!=expVals[x])
            isNaNs=true;
          x++;
        }


    }

    std::vector<std::string> BivariateNormal::getAttributeNames() const
    {
      return AttNames;

    }

    Mantid::API::IFitFunction::Attribute BivariateNormal::getAttribute(const std::string &attName) const
    {
      int I = -1;

      for (size_t i = 0; i < nAttributes() && I < 0; i++)
        if (attName.compare(AttNames[i]) == 0)
          I = (int)i;

      if (I >= 0)
        return Mantid::API::IFitFunction::Attribute(Attrib[I]);

      throw std::runtime_error("No such attribute");
    }


    void BivariateNormal::setAttribute(const std::string &attName,
        const Mantid::API::IFitFunction::Attribute &att)
    {
      int I = -1;
      for (size_t i = 0; i < nAttributes() && I < 0; i++)
      {
        if (attName.compare(AttNames[i]) == 0)
          I = (int)i;
      }

      SIxx = -1;
      if (I >= 0)
      {
        Attrib[I] = att.asDouble();
        // initCommon();
        return;
      }

      throw std::runtime_error("No such attribute");
    }


    bool BivariateNormal::hasAttribute(const std::string &attName) const
    {
      int I = -1;

      for (size_t i = 0; i < nAttributes() && I < 0; i++)
        if (attName.compare(AttNames[i]) == 0)
          I = (int)i;

      if (I >= 0)
        return true;

      return false;
    }

  }//namespace curveFitting
}//namespaceMantid
