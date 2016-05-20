/*
 * BivariateNormalTest.h
 *
 *  Created on: Apr 19, 2011
 *      Author: Ruth Mikkelson
 */

#ifndef BIVARIATENORMALTEST_H_
#define BIVARIATENORMALTEST_H_
#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/BivariateNormal.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/cow_ptr.h"
/*#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/GSLFunctions.h"
#include "MantidKernel/UnitFactory.h"
*/
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <numeric>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using Mantid::HistogramData::Points;
/**
 * Used for testing only
 */
class Jacob : public Jacobian {
private:
  Matrix<double> M;

public:
  Jacob(int nparams, int npoints) { M = Matrix<double>(nparams, npoints); }

  ~Jacob() override {}
  void set(size_t iY, size_t iP, double value) override { M[iP][iY] = value; }

  double get(size_t iY, size_t iP) override { return M[iP][iY]; }
};

class BivariateNormalTest : public CxxTest::TestSuite {
public:
  double NormVal(double Background, double Intensity, double Mcol, double Mrow,
                 double Vx, double Vy, double Vxy, double row, double col) {

    double uu = Vx * Vy - Vxy * Vxy;

    double coefNorm = .5 / M_PI / sqrt(uu);

    double expCoeffx2 = -Vy / 2 / uu;
    double expCoeffxy = Vxy / uu;
    double expCoeffy2 = -Vx / 2 / uu;
    double dx = col - Mcol;
    double dy = row - Mrow;
    return Background +
           coefNorm * Intensity *
               exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy +
                   expCoeffy2 * dy * dy);
  }
  void test_Normal() {
    BivariateNormal NormalFit;
    NormalFit.initialize();

    //  TS_ASSERT_EQUALS( NormalFit.nParams(),7);
    TS_ASSERT_EQUALS(NormalFit.nAttributes(), 1);
    TS_ASSERT_EQUALS(NormalFit.name(), std::string("BivariateNormal"));

    const int nCells = 30;
    MatrixWorkspace_sptr ws1 =
        WorkspaceFactory::Instance().create("Workspace2D", 3, nCells, nCells);
    Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>(ws1);

    double background = 0.05;
    double intensity = 562.95;
    double Mcol = 195.698196998;
    double Mrow = 44.252065014;
    double Vx = 5.2438470;
    double Vy = 3.3671409085;
    double Vxy = 2.243584414;

    bool CalcVariances = 0;

    Mantid::MantidVecPtr xvals, yvals, data;
    int sgn1 = 1;
    int sgn2 = 1;
    for (int i = 0; i < nCells; i++) {
      double x = 195 + sgn1;
      double y = 44 + sgn2;
      if (sgn1 > 0)
        if (sgn2 > 0)
          sgn2 = -sgn2;
        else {
          sgn1 = -sgn1;
          sgn2 = -sgn2;
        }
      else if (sgn2 > 0)
        sgn2 = -sgn2;
      else {
        sgn1 = -sgn1 + 1;
        sgn2 = sgn1;
      }
      xvals.access().push_back(x);
      yvals.access().push_back(y);
      double val =
          NormVal(background, intensity, Mcol, Mrow, Vx, Vy, Vxy, y, x);

      data.access().push_back(val);
    }

    Points x_vec_ptr(nCells);
    std::iota(begin(x_vec_ptr), end(x_vec_ptr), 0.0);
    double xx[nCells];
    for (int i = 0; i < nCells; i++) {
      xx[i] = i;
    }
    NormalFit.setAttributeValue("CalcVariances", CalcVariances);

    ws->setPoints(0, x_vec_ptr);
    ws->setData(0, data);
    ws->setData(1, xvals);
    ws->setData(2, yvals);

    NormalFit.setMatrixWorkspace(ws, 0, 0.0, 30.0);

    NormalFit.setParameter("Background", 0.05, true);
    NormalFit.setParameter("Intensity", 562.95, true);
    NormalFit.setParameter("Mcol", 195.698196998, true);
    NormalFit.setParameter("Mrow", 44.252065014, true);

    if (!CalcVariances) {
      NormalFit.setParameter("SScol", 5.2438470, true);
      NormalFit.setParameter("SSrow", 3.3671409085, true);
      NormalFit.setParameter("SSrc", 2.243584414, true);
    }

    std::vector<double> out(nCells);

    boost::shared_ptr<Jacob> Jac = boost::make_shared<Jacob>(7, nCells);

    NormalFit.functionDeriv1D(Jac.get(), xx, nCells);

    NormalFit.function1D(out.data(), xx, nCells);

    //  std::cout<<"-------------------------------------"<<std::endl;
    for (int i = 0; i < nCells; i++) {

      double x = xvals.access()[i];
      double y = yvals.access()[i];
      double d = NormVal(background, intensity, Mcol, Mrow, Vx, Vy, Vxy, y, x);

      TS_ASSERT_DELTA(d, out[i], .001);
    }
    // std::cout<<"\n-------------------------------------"<<std::endl;

    double Res[5][7] = {
        {1, 0.0410131, -1.21055, 5.93517, -3.04761, -4.03279, 3.79245},
        {1, 0.00388945, -2.25613, 2.63994, 0.870333, 1.13668, -2.33103},
        {1, 0.00510336, 0.616511, 2.78705, -0.31702, 0.75513, 1.10871},
        {1, 4.45298e-08, -5.92569e-05, 7.48318e-05, 6.66936e-05, 0.000106485,
         -0.000172435},
        {1, 3.35644e-05, 0.00910018, 0.0318031, -0.000328676, 0.02284,
         0.0186753}};

    for (int i = 0; i < nCells; i += 6) // points
    {

      for (int j = 0; j < 7; j++) // params
      {
        double u = Res[i / 6][j];
        double v = Jac->get(i, j);

        TS_ASSERT_DELTA(v, u, .001);
      }
    }
  }

  void testForCategories() {
    BivariateNormal forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Peak");
  }
};
#endif /* BIVARIATENORMALTEST_H_ */
