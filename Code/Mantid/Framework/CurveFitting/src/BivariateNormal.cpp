/*
 * BivariateNormal.cpp
 *
 *  Created on: Apr 12, 2011
 *      Author: ruth
 */

#include "MantidCurveFitting/BivariateNormal.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/ParameterTie.h"
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <sstream>

using namespace Mantid::API;

//Indicies into the Attrib array
int StartRow = 0;
int StartCol = 1;
int NRows = 2;
int NCols = 3;
int S_int = 4;
int S_xint = 5;
int S_yint = 6;
int S_x2int = 7;
int S_y2int = 8;
int S_xyint = 9;
int S_y = 10;
int S_x = 11;
int S_x2 = 12;
int S_y2 = 13;
int S_xy = 14;
int S_1 = 15;

//Indicies into the LastParams array
int IBACK = 0;
int ITINTENS = 1;
int IXMEAN = 2;
int IYMEAN = 3;
int IVXX = 4;
int IVYY = 5;
int IVXY = 6;

BivariateNormal::BivariateNormal()
{
  LastParams = 0;
  SIxx = SIyy = SIxy = Sxx = Syy = Sxy = -1; //Var and CoVar calc from parameters
  expVals = 0;
  Attrib = new double[16];

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
    delete Attrib;

  if (!expVals)
    delete expVals;

  if (!LastParams)
    delete LastParams;

  if (!BackConstraint)
    delete BackConstraint;

  if (!MeanxConstraint)
    delete MeanxConstraint;

  if (!MeanyConstraint)
    delete MeanyConstraint;

  if (!IntensityConstraint)
    delete IntensityConstraint;

}

/// overwrite IFunction base class methods


void BivariateNormal::function(double *out, const double *xValues, const int &nData)
{
  initCommon();

  for (int i = 0; i < nData; i++)
    out[i] = LastParams[IBACK] + coefNorm * LastParams[ITINTENS] * expVals[i];

}

void BivariateNormal::functionDeriv(API::Jacobian *out, const double *xValues, const int &nData)
{
  initCommon();
  int x = 0;
  double uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];
  int startRow = (int) getAttribute("StartRow").asDouble();
  int startCol = (int) getAttribute("StartCol").asDouble();
  int Nrows = (int) getAttribute("NRows").asDouble();
  int Ncols = (int) getAttribute("NCols").asDouble();
  for (int r = startRow; r < startRow + Nrows; r++)
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
    if (minMeany < Attrib[StartRow] + 3)
      minMeany = Attrib[StartRow] + 3;
    double maxMeany = Attrib[NRows] * .9;
    if (maxMeany > Attrib[NRows] - 3)
      maxMeany = Attrib[NRows] - 3;
    maxMeany += Attrib[StartRow];
    MeanyConstraint = new BoundaryConstraint(this, "Mrow", minMeany, maxMeany);

    addConstraint( MeanyConstraint);

    double minMeanx = Attrib[NCols] / 10;
    if (minMeanx < 3)
      minMeanx = 3;
    double maxMeanx = Attrib[NCols] * .9;
    if (maxMeanx > Attrib[NCols] - 3)
      maxMeanx = Attrib[NCols] - 3;
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
    for (int i = 0; i < nParams() && ParamsOK; i++)
      if (getParameter(i) != LastParams[i])
        ParamsOK = false;

  if (!ParamsOK)
  {

    for (int i = 0; i < nParams(); i++)
      LastParams[i] = getParameter(i);

    std::ostringstream ssxx, ssyy, ssxy;

    ssyy << std::string("(") << (SIyy) << "+(Mrow-" << (mIy) << ")*(Mrow-" << (mIy) << ")*"
        << Attrib[S_int] << "-Background*" << (Syy) << "-Background*(Mrow-" << (my) << ")*(Mrow-"
        << (my) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1])
        << ")";

    // std::cout<<"row formula="<< ssyy.str()<<std::endl;
    API::ParameterTie* pt = tie("SSrow", ssyy.str());
    //  std::cout << "  ddK" <<pt->eval()<< std::endl;

    ssxx << std::string("(") << (SIxx) << "+(Mcol-" << (mIx) << ")*(Mcol-" << (mIx) << ")*"
        << Attrib[S_int] << "-Background*" << (Sxx) << "-Background*(Mcol-" << (mx) << ")*(Mcol-"
        << (mx) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1])
        << ")";

    // std::cout<<"col formula="<< ssxx.str()<<std::endl;
    API::ParameterTie* ptx = tie("SScol", ssxx.str());
    // std::cout << "  ddK" <<ptx->eval()<< std::endl;

    ssxy << std::string("(") << (SIxy) << "+(Mcol-" << (mIx) << ")*(Mrow-" << (mIy) << ")*"
        << Attrib[S_int] << "-Background*" << (Sxy) << "-Background*(Mcol-" << (mx) << ")*(Mrow-"
        << (my) << ")*" << Attrib[S_1] << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1])
        << ")";

    //std::cout<<"cov formula="<< ssxy.str()<<std::endl;
    API::ParameterTie* ptxy = tie("SSrc", ssxy.str());
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

std::vector<std::string> BivariateNormal::getAttributeNames() const
{
  return AttNames;

}

Mantid::API::IFitFunction::Attribute BivariateNormal::getAttribute(const std::string &attName) const
{
  int I = -1;

  for (int i = 0; i < nAttributes() && I < 0; i++)
    if (attName.compare(AttNames[i]) == 0)
      I = i;

  if (I >= 0)
    return Mantid::API::IFitFunction::Attribute(Attrib[I]);

  throw std::runtime_error("No such attribute");
}


void BivariateNormal::setAttribute(const std::string &attName,
    const Mantid::API::IFitFunction::Attribute &att)
{
  int I = -1;
  for (int i = 0; i < nAttributes() && I < 0; i++)
  {
        if (attName.compare(AttNames[i]) == 0)
      I = i;
  }

  SIxx = -1;
  if (I >= 0)
  {
    Attrib[I] = att.asDouble();
    return;
  }

  throw std::runtime_error("No such attribute");
}


bool BivariateNormal::hasAttribute(const std::string &attName) const
{
  int I = -1;

  for (int i = 0; i < nAttributes() && I < 0; i++)
    if (attName.compare(AttNames[i]) == 0)
      I = i;

  if (I >= 0)
    return true;

  return false;
}

