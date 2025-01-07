// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/BivariateNormal.h"

#include "MantidAPI/FunctionFactory.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidHistogramData/HistogramY.h"

#include "MantidKernel/PhysicalConstants.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using namespace Mantid::API;

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
using namespace Constraints;
using namespace HistogramData;

namespace {
/// static logger
Kernel::Logger g_log("BivariateNormal");
} // namespace

DECLARE_FUNCTION(BivariateNormal)

// Indicies into Attrib array( local variable in initCommon
#define S_int 0
#define S_xint 1
#define S_yint 2
#define S_x2int 3
#define S_y2int 4
#define S_xyint 5
#define S_y 6
#define S_x 7
#define S_x2 8
#define S_y2 9
#define S_xy 10
#define S_1 11

// Indicies into the LastParams array
#define IBACK 0
#define ITINTENS 1
#define IXMEAN 2
#define IYMEAN 3
#define IVXX 4
#define IVYY 5
#define IVXY 6

BivariateNormal::BivariateNormal()
    : API::ParamFunction(), CalcVxx(false), CalcVyy(false), CalcVxy(false), NCells(0), CalcVariances(false), mIx(0.0),
      mx(0.0), mIy(0.0), my(0.0), SIxx(0.0), SIyy(0.0), SIxy(0.0), Sxx(0.0), Syy(0.0), Sxy(0.0), TotI(0.0), TotN(0.0),
      Varx0(-1.0), Vary0(-1.0), expVals(nullptr), uu(0.0), coefNorm(0.0), expCoeffx2(0.0), expCoeffy2(0.0),
      expCoeffxy(0.0) {
  LastParams[IVXX] = -1;
  Varx0 = -1;
}

BivariateNormal::~BivariateNormal() { delete[] expVals; }

// overwrite IFunction base class methods

void BivariateNormal::function1D(double *out, const double *xValues, const size_t nData) const {

  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
  if (nData == 0)
    return;

  if (Varx0 < 0) {
    for (size_t i = 0; i < nData; i++)
      out[i] = 0.0;
    return;
  }
  double coefNorm, expCoeffx2, expCoeffy2, expCoeffxy, Varxx, Varxy, Varyy;
  int NCells;
  bool isNaNs;

  API::MatrixWorkspace_const_sptr ws = getMatrixWorkspace();

  const auto &D = ws->y(0);
  const auto &X = ws->y(1);
  const auto &Y = ws->y(2);
  int K = 1;

  if (nParams() > 4)
    K = 3;

  getConstraint(IBACK)->setPenaltyFactor(K * 3000);

  double badParams = initCoeff(D, X, Y, coefNorm, expCoeffx2, expCoeffy2, expCoeffxy, NCells, Varxx, Varxy, Varyy);

  std::ostringstream inf;
  inf << "F Parameters=";
  for (size_t k = 0; k < nParams(); k++)
    inf << "," << getParameter(k);
  if (nParams() < 6)
    inf << "," << Varxx << "," << Varyy << "," << Varxy;
  inf << '\n';

  NCells = std::min<int>(static_cast<int>(nData), NCells);

  double Background = getParameter(IBACK);
  double Intensity = getParameter(ITINTENS);
  double Xmean = getParameter(IXMEAN);
  double Ymean = getParameter(IYMEAN);

  double DDD = std::min<double>(10, 10 * std::max<double>(0, -Background));
  int x = 0;
  isNaNs = false;
  double chiSq = 0;

  // double penalty =0;

  for (int i = 0; i < NCells; i++) {
    // double pen =0;
    if (badParams > 0)
      out[x] = badParams;
    else if (isNaNs)
      out[x] = 10000;
    else {
      double dx = X[i] - Xmean;
      double dy = Y[i] - Ymean;
      out[x] =
          Background + coefNorm * Intensity * exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy + expCoeffy2 * dy * dy);
      out[x] = out[x] + DDD;

      if (out[x] != out[x]) {
        out[x] = 100000;
        isNaNs = true;
      }
    }
    double diff = out[x] - D[x];
    chiSq += diff * diff;

    x++;
  }
  inf << "Constr:";
  for (size_t i = 0; i < nParams(); i++) {
    IConstraint *constr = getConstraint(i);
    if (constr)
      inf << i << "=" << constr->check() << ";";
  }
  inf << "\n\n    chiSq =" << chiSq << "     nData " << nData << '\n';
  g_log.debug(inf.str());
}

void BivariateNormal::functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) {
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
  if (nData == 0)
    return;
  double penDeriv = initCommon();

  std::ostringstream inf;
  inf << "***penalty(" << penDeriv << "),Parameters=";
  for (size_t k = 0; k < 7; k++)
    inf << "," << LastParams[k];
  inf << '\n';
  g_log.debug(inf.str());

  std::vector<double> outf(nData, 0.0);
  function1D(outf.data(), xValues, nData);

  double uu = LastParams[IVXX] * LastParams[IVYY] - LastParams[IVXY] * LastParams[IVXY];
  /*  if( uu <=0)
  {
    penDeriv = -2*uu;
    uu=1;
  }
  */
  API::MatrixWorkspace_const_sptr ws = getMatrixWorkspace();
  const auto &X = ws->y(1);
  const auto &Y = ws->y(2);

  for (int x = 0; x < NCells; x++) {

    double penaltyDeriv = penDeriv;

    double r = Y[x];
    double c = X[x];

    out->set(x, IBACK, +1.0);

    if (penaltyDeriv <= 0)
      out->set(x, ITINTENS, expVals[x] * coefNorm);
    else if (LastParams[ITINTENS] < 0)
      out->set(x, ITINTENS, -.01);
    else
      out->set(x, ITINTENS, 0.01);

    double coefExp = coefNorm * LastParams[ITINTENS];

    double coefxy = LastParams[IVXY] / uu;
    double coefx2 = -LastParams[IVYY] / 2 / uu;

    if (penaltyDeriv <= 0)
      out->set(x, IXMEAN,
               penaltyDeriv +
                   coefExp * expVals[x] * (-2 * coefx2 * (c - LastParams[IXMEAN]) - coefxy * (r - LastParams[IYMEAN])));
    else // if(LastParams[IXMEAN] < 0)
      out->set(x, IXMEAN, 0);
    // else
    //     out->set(x,IXMEAN,0);

    coefExp = coefNorm * LastParams[ITINTENS];

    double coefy2 = -LastParams[IVXX] / 2 / uu;

    if (penaltyDeriv <= 0)
      out->set(x, IYMEAN,
               penaltyDeriv +
                   coefExp * expVals[x] * (-coefxy * (c - LastParams[IXMEAN]) - 2 * coefy2 * (r - LastParams[IYMEAN])));
    else // if(LastParams[IYMEAN] < 0)
      out->set(x, IYMEAN, 0);
    // else
    //     out->set(x,IYMEAN,0);

    double M = 1;
    if (nParams() < 5)
      M = 10;
    coefExp = coefNorm * LastParams[ITINTENS];

    double C = -LastParams[IVYY] / 2 / uu;

    double SIVXX;
    if (penaltyDeriv <= 0)
      SIVXX = coefExp * expVals[x] *
              (C +
               -LastParams[IVYY] / uu *
                   (coefx2 * (c - LastParams[IXMEAN]) * (c - LastParams[IXMEAN]) +
                    coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) +
                    coefy2 * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN])) -
               (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN]) / 2 / uu);
    else if (LastParams[IVXX] < .01)
      SIVXX = -M;
    else
      SIVXX = 0;

    if (LastParams[IVXX] > 1.2 * Varx0 && !CalcVariances)
      SIVXX = 0;
    if (LastParams[IVXX] < .8 * Varx0)
      SIVXX = 0;
    coefExp = coefNorm * LastParams[ITINTENS];

    C = -LastParams[IVXX] / 2 / uu;

    double SIVYY;
    if (penaltyDeriv <= 0)
      SIVYY = coefExp * expVals[x] *
              (C +
               -LastParams[IVXX] / uu *
                   (coefx2 * (c - LastParams[IXMEAN]) * (c - LastParams[IXMEAN]) +
                    coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) +
                    coefy2 * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN])) -
               (c - LastParams[IXMEAN]) * (c - LastParams[IXMEAN]) / 2 / uu);

    else if (LastParams[IVYY] < .01)
      SIVYY = -M;
    else
      SIVYY = 0;

    if (LastParams[IVYY] > 1.2 * Vary0 && !CalcVariances)
      SIVYY = 0;

    if (LastParams[IVYY] < .8 * Vary0)
      SIVYY = 0;
    coefExp = coefNorm * LastParams[ITINTENS];

    C = LastParams[IVXY] / uu;

    double SIVXY;
    if (penaltyDeriv <= 0)
      SIVXY = coefExp * expVals[x] *
              (C +
               2 * LastParams[IVXY] / uu *
                   (coefx2 * (c - LastParams[IXMEAN]) * (c - LastParams[IXMEAN]) +
                    coefxy * (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) +
                    coefy2 * (r - LastParams[IYMEAN]) * (r - LastParams[IYMEAN])) +
               (r - LastParams[IYMEAN]) * (c - LastParams[IXMEAN]) / uu);

    else if (uu < 0)
      SIVXY = 2 * LastParams[IVXY];
    else
      SIVXY = 0;

    if (!CalcVxx && nParams() > 6)
      out->set(x, IVXX, penaltyDeriv + SIVXX);
    else {
      // out->set(x,IVXX,0.0);
      double bdderiv = out->get(x, IBACK);

      bdderiv += SIVXX *
                 (-Sxx - (LastParams[IXMEAN] - mx) * (LastParams[IXMEAN] - mx) * TotN + LastParams[IVXX] * TotN) /
                 (TotI - LastParams[IBACK] * TotN);

      out->set(x, IBACK, bdderiv);

      double mxderiv = out->get(x, IXMEAN);
      mxderiv += SIVXX *
                 (2 * (LastParams[IXMEAN] - mIx) *

                      TotI -
                  2 * LastParams[IBACK] * (LastParams[IXMEAN] - mx) * TotN) /
                 (TotI - LastParams[IBACK] * TotN);
      out->set(x, IXMEAN, mxderiv);
    }
    if (!CalcVyy && nParams() > 6)
      out->set(x, IVYY, penaltyDeriv + SIVYY);
    else {
      // out->set(x,IVYY, 0.0);
      double bdderiv = out->get(x, IBACK);

      bdderiv += SIVYY *
                 (-Syy - (LastParams[IYMEAN] - my) * (LastParams[IYMEAN] - my) * TotN + LastParams[IVYY] * TotN) /
                 (TotI - LastParams[IBACK] * TotN);
      out->set(x, IBACK, bdderiv);
      double myderiv = out->get(x, IYMEAN);

      myderiv += SIVYY *
                 (2 * (LastParams[IYMEAN] - mIy) *

                      TotI -
                  2 * LastParams[IBACK] * (LastParams[IYMEAN] - my) * TotN) /
                 (TotI - LastParams[IBACK] * TotN);
      out->set(x, IYMEAN, myderiv);
    }
    if (!CalcVxy && nParams() > 6) {
      out->set(x, IVXY, penaltyDeriv + SIVXY);

    } else {
      // out->set(x,IVXY, 0.0);
      double bdderiv = out->get(x, IBACK);
      bdderiv += SIVXY *
                 (-Sxy - (LastParams[IYMEAN] - my) * (LastParams[IXMEAN] - mx) * TotN + LastParams[IVXY] * TotN) /
                 (TotI - LastParams[IBACK] * TotN);
      out->set(x, IBACK, bdderiv);
      double myderiv = out->get(x, IYMEAN);
      myderiv += SIVXY *
                 ((LastParams[IXMEAN] - mIx) *

                      TotI -
                  LastParams[IBACK] * (LastParams[IXMEAN] - mx) * TotN) /
                 (TotI - LastParams[IBACK] * TotN);
      out->set(x, IYMEAN, myderiv);
      double mxderiv = out->get(x, IXMEAN);
      mxderiv += SIVXY * ((LastParams[IYMEAN] - mIy) * TotI - LastParams[IBACK] * (LastParams[IYMEAN] - my) * TotN) /
                 (TotI - LastParams[IBACK] * TotN);
      out->set(x, IXMEAN, mxderiv);
    }
  }
}

void BivariateNormal::init() {
  declareParameter("Background", 0.00);
  declareParameter("Intensity", 0.00);
  declareParameter("Mcol", 0.00, "Mean column(x) value");
  declareParameter("Mrow", 0.00, "Mean row(y) value");

  CalcVariances = false;

  NCells = -1;
  LastParams[IVXX] = -1;
}

double BivariateNormal::initCommon() {

  double penalty = 0;
  bool ParamsOK = true;
  bool CommonsOK = true;
  if (!expVals)
    CommonsOK = false;

  API::MatrixWorkspace_const_sptr ws = getMatrixWorkspace();
  const auto &D = ws->y(0);
  const auto &X = ws->y(1);
  const auto &Y = ws->y(2);

  if (NCells < 0) {
    NCells = static_cast<int>(std::min<size_t>(D.size(), std::min<size_t>(X.size(), Y.size())));
    CommonsOK = false;
  }

  double Attrib[12] = {0.0};

  double MinX, MinY, MaxX, MaxY, MaxD, MinD;
  MinX = MaxX = X[0];
  MinY = MaxY = Y[0];
  MaxD = MinD = D[0];

  if (!CommonsOK) {

    for (int i = 0; i < NCells; i++) {
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

    // CommonsOK = false;

    TotI = Attrib[S_int];
    TotN = Attrib[S_1];

    // CommonsOK = false;

    if (getConstraint(0) == nullptr) {

      addConstraint((std::make_unique<BoundaryConstraint>(this, "Background", 0, Attrib[S_int] / Attrib[S_1])));
    }

    double maxIntensity = Attrib[S_int] + 3 * sqrt(Attrib[S_int]);

    if (maxIntensity < 100)
      maxIntensity = 100;

    if (getConstraint(1) == nullptr) {
      addConstraint(std::make_unique<BoundaryConstraint>(this, "Intensity", 0, maxIntensity));
    }

    double minMeany = MinY * .9 + .1 * MaxY;
    double maxMeany = MinY * .1 + .9 * MaxY;

    if (getConstraint(3) == nullptr) {
      addConstraint(std::make_unique<BoundaryConstraint>(this, "Mrow", minMeany, maxMeany));
    }

    double minMeanx = MinX * .9 + .1 * MaxX;
    double maxMeanx = MinX * .1 + .9 * MaxX;
    if (getConstraint(2) == nullptr) {
      addConstraint(std::make_unique<BoundaryConstraint>(this, "Mcol", minMeanx, maxMeanx));
    }

    if (CalcVariances && nParams() > 6) {
      std::ostringstream ssxx, ssyy, ssxy;

      ssyy << std::string("(") << (SIyy) << "+(Mrow-" << (mIy) << ")*(Mrow-" << (mIy) << ")*" << Attrib[S_int]
           << "-Background*" << (Syy) << "-Background*(Mrow-" << (my) << ")*(Mrow-" << (my) << ")*" << Attrib[S_1]
           << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1]) << ")";

      if (getTie(IVYY) == nullptr) {
        tie("SSrow", ssyy.str());
        CalcVxx = true;
      }

      ssxx << std::string("(") << (SIxx) << "+(Mcol-" << (mIx) << ")*(Mcol-" << (mIx) << ")*" << Attrib[S_int]
           << "-Background*" << (Sxx) << "-Background*(Mcol-" << (mx) << ")*(Mcol-" << (mx) << ")*" << Attrib[S_1]
           << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1]) << ")";

      if (getTie(IVXX) == nullptr) {
        tie("SScol", ssxx.str());
        CalcVyy = true;
      }

      ssxy << std::string("(") << (SIxy) << "+(Mcol-" << (mIx) << ")*(Mrow-" << (mIy) << ")*" << Attrib[S_int]
           << "-Background*" << (Sxy) << "-Background*(Mcol-" << (mx) << ")*(Mrow-" << (my) << ")*" << Attrib[S_1]
           << ")/(" << (Attrib[S_int]) << "-Background*" << (Attrib[S_1]) << ")";

      if (getTie(IVXY) == nullptr) {
        tie("SSrc", ssxy.str());
        CalcVxy = true;
      }
    }
    CommonsOK = true;
  }

  if (LastParams[IVXX] < 0) {
    ParamsOK = false;
    CommonsOK = false;

  } else
    for (size_t i = 0; i < nParams() && ParamsOK; i++)
      if (getParameter(i) != LastParams[i])
        ParamsOK = false;

  if (!ParamsOK) {

    for (size_t i = 0; i < nParams(); i++)
      LastParams[i] = getParameter(i);
  }

  if (!CommonsOK || !ParamsOK) {

    int NCells1;
    double Varxx, Varxy, Varyy;

    Varxx = Varxy = Varyy = -1;
    penalty = initCoeff(D, X, Y, coefNorm, expCoeffx2, expCoeffy2, expCoeffxy, NCells1, Varxx, Varxy, Varyy);

    if (Varx0 < 0 && penalty <= 0) {
      Varx0 = Varxx;
      Vary0 = Varyy;
    }

    LastParams[IVXX] = Varxx;
    LastParams[IVXY] = Varxy;
    LastParams[IVYY] = Varyy;

    delete[] expVals;
    expVals = new double[NCells];

    for (int i = 0; i < NCells; i++) {

      double dx = X[i] - LastParams[IXMEAN];
      double dy = Y[i] - LastParams[IYMEAN];
      expVals[i] = exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy + expCoeffy2 * dy * dy);
    }
  }
  return penalty;
}

double BivariateNormal::initCoeff(const HistogramY &D, const HistogramY &X, const HistogramY &Y, double &coefNorm,
                                  double &expCoeffx2, double &expCoeffy2, double &expCoeffxy, int &NCells,
                                  double &Varxx, double &Varxy, double &Varyy) const {

  double Background = getParameter("Background");
  bool zeroDenom = false;
  if (TotI == 0 && TotN == 0)
    zeroDenom = true;
  else if (TotI - Background * TotN <= 0)
    zeroDenom = true;
  if (CalcVxx || nParams() < 6) {
    Varxx =
        (SIxx + (getParameter("Mcol") - mIx) * (getParameter("Mcol") - mIx) * TotI - getParameter("Background") * Sxx -
         getParameter("Background") * (getParameter("Mcol") - (mx)) * (getParameter("Mcol") - (mx)) * TotN) /
        (TotI - getParameter("Background") * TotN);

    if (Varx0 > 0) {
      Varxx = std::min<double>(Varxx, 1.21 * Varx0);
      Varxx = std::max<double>(Varxx, .79 * Varx0);
    }

  } else {
    Varxx = getParameter(IVXX);
  }

  double Mrow = getParameter("Mrow");

  if (CalcVyy || nParams() < 6) {

    Varyy = (SIyy + (Mrow - (mIy)) * (Mrow - (mIy)) * TotI -
             getParameter("Background") * (Syy)-Background * (Mrow - (my)) * (Mrow - (my)) * TotN) /
            (TotI - Background * TotN);
    if (Vary0 > 0) {
      Varyy = std::min<double>(Varyy, 1.21 * Vary0);
      Varyy = std::max<double>(Varyy, .79 * Vary0);
    }
  } else {
    Varyy = getParameter(IVYY);
  }

  if (CalcVxy || nParams() < 6) {
    Varxy = ((SIxy) + (getParameter("Mcol") - (mIx)) * (getParameter("Mrow") - (mIy)) * TotI -
             getParameter("Background") * (Sxy)-getParameter("Background") * (getParameter("Mcol") - (mx)) *
                 (getParameter("Mrow") - (my)) * TotN) /
            (TotI - getParameter("Background") * TotN);

  } else {
    Varxy = getParameter(IVXY);
  }

  double uu = Varxx * Varyy - Varxy * Varxy;
  double penalty;
  if (zeroDenom)
    penalty = 200;
  else
    penalty = std::max<double>(0, -Varxx + .01) + std::max<double>(0, -Varyy + .01) + std::max<double>(0, -uu + .01);

  if (fabs(uu) < .01) {
    if (uu < 0)
      uu = -.01;
    else
      uu = .01;
  }

  NCells = static_cast<int>(std::min<size_t>(D.size(), std::min<size_t>(X.size(), Y.size())));
  if (zeroDenom) {
    coefNorm = expCoeffx2 = expCoeffy2 = 1;
    expCoeffxy = 0;
    Varxx = Varyy = 5;
    Varxy = 0;
    return penalty;
  }
  coefNorm = .5 / M_PI / sqrt(fabs(uu));

  expCoeffx2 = -fabs(Varyy) / 2 / fabs(uu);
  expCoeffxy = Varxy / uu;
  expCoeffy2 = -fabs(Varxx) / 2 / fabs(uu);

  if (nParams() < 5)
    penalty *= 10;

  return penalty;
}

} // namespace Mantid::CurveFitting::Functions
