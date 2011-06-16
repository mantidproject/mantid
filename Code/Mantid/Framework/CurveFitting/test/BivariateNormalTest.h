/*
 * BivariateNormalTest.h
 *
 *  Created on: Apr 19, 2011
 *      Author: Ruth Mikkelson
 */

#ifndef BIVARIATENORMALTEST_H_
#define BIVARIATENORMALTEST_H_
#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BivariateNormal.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
/*#include "MantidAPI/IFitFunction.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/GSLFunctions.h"
#include "MantidKernel/UnitFactory.h"
*/
#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <exception>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

/**
 * Used for testing only
 */
class Jacob: public Jacobian
{
private:
  Matrix<double> M;
public:
  Jacob(int nparams, int npoints)
  {
    M = Matrix<double> (nparams, npoints);
  }

  ~Jacob()
  {

  }
  void set(size_t iY, size_t iP, double value)
  {
    M[iP][iY] = value;
  }

  double get(size_t iY, size_t iP)
  {
    return M[iP][iY];
  }

};

class BivariateNormalTest: public CxxTest::TestSuite
{
public:

  void test_Normal()
  {
    BivariateNormal NormalFit;

    try
    {
      NormalFit.setAttribute("StartRow", IFitFunction::Attribute(195.0));

      NormalFit.setAttribute("StartCol", IFitFunction::Attribute(222.0));

      NormalFit.setAttribute("NRows", IFitFunction::Attribute(33.));
      NormalFit.setAttribute("NCols", IFitFunction::Attribute(26.));

      NormalFit.setAttribute("Intensities", IFitFunction::Attribute(79.0));
      NormalFit.setAttribute("SSIx", IFitFunction::Attribute(18490.0));
      NormalFit.setAttribute("SSIy", IFitFunction::Attribute(16625.0));
      NormalFit.setAttribute("SSIxx", IFitFunction::Attribute(4331900.0));
      NormalFit.setAttribute("SSIyy", IFitFunction::Attribute(3506435.0));
      NormalFit.setAttribute("SSIxy", IFitFunction::Attribute(3890399.0));
      NormalFit.setAttribute("SSx", IFitFunction::Attribute(201201.0));
      NormalFit.setAttribute("SSy", IFitFunction::Attribute(181038.0));
      NormalFit.setAttribute("SSxx", IFitFunction::Attribute(4.7229897E7));
      NormalFit.setAttribute("SSyy", IFitFunction::Attribute(3.827681E7));
      NormalFit.setAttribute("SSxy", IFitFunction::Attribute(4.2453411E7));

      NormalFit.initialize();

      NormalFit.setParameter("Background", 0.0, true);
      NormalFit.setParameter("Intensity", 79.0, true);
      NormalFit.setParameter("Mcol", 234.0506329, true);
      NormalFit.setParameter("Mrow", 210.443037975, true);
      NormalFit.setParameter("SScol", 54.4784490, true);
      NormalFit.setParameter("SSrow", 98.98093254, true);
      NormalFit.setParameter("SSrc", -8.76926775, true);

      int nCells = 33 * 26;
      double* x = new double[nCells];
      for (int i = 0; i < nCells; i++)
        x[i] = i;

      double* out = new double[nCells];
      NormalFit.function(out, x, nCells);

      TS_ASSERT_LESS_THAN(fabs(out[0] - .0096733), .00004);
      TS_ASSERT_LESS_THAN(fabs(out[(int) (nCells / 4)] - 0.0857393), .00004);
      TS_ASSERT_LESS_THAN(fabs(out[(int) (2 * nCells / 4)] - 0.170591), .00004);
      TS_ASSERT_LESS_THAN(fabs(out[(int) (3 * nCells / 4)] - .0685042), .00004);
      delete[] out;

      Jacob Jac(7, nCells);

      NormalFit.functionDeriv(&Jac, x, nCells);
      delete[] x;
      size_t p = 1;
      {
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)0, p) - 0.000122447), .000004);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (nCells / 4), p) - 0.00108531), .000004);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (2 * nCells / 4), p) - 0.00215938), .000004);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (3 * nCells / 4), p) - 0.000867142), .000004);

      }
      p = 2;
      {
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)0, p) + 0.0024171), .00003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)(nCells / 4), p) + 0.0107128), .00003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (2 * nCells / 4), p) - 0.0031733), .00003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (3 * nCells / 4), p) - 0.00983232), .00003);

      }
      p = 3;
      {
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)0, p) + 0.00172312), .00003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (nCells / 4), p) + 0.00739412), .00003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (2 * nCells / 4), p) - 0.00124563), .00003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (3 * nCells / 4), p) - 0.00679517), .00003);

      }
      p = 4;
      {
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)0, p) - 0.000211919), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (nCells / 4), p) + 0.00012903), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)(2 * nCells / 4), p) + 0.00155881), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (3 * nCells / 4), p) - 6.77865e-05), .000003);

      }
      p = 5;
      {
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)0, p)) - 0.000103899, .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (nCells / 4), p) + 0.000120544), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (2 * nCells / 4), p) + 0.000869656), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (3 * nCells / 4), p) + 1.40357e-05), .000003);

      }
      p = 6;
      {
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t)0, p) - 0.000414602), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (nCells / 4), p) - 0.000782418), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (2 * nCells / 4), p) + 0.000258266), .000003);
        TS_ASSERT_LESS_THAN(fabs(Jac.get((size_t) (3 * nCells / 4), p) - 0.000862286), .000003);
        ;

      }

    } catch (...)
    {
      std::cout << "Error occurred" << std::endl;
      return;
    }

    return;

  }

  void test_Bounds()
  {
    BivariateNormal NormalFit;
    int nCells = 33 * 26;
    MatrixWorkspace_sptr ws1 = WorkspaceFactory::Instance().create("Workspace2D",1,nCells,nCells);
    Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>(ws1);

    NormalFit.setAttribute("StartRow", IFitFunction::Attribute(195.0));

    NormalFit.setAttribute("StartCol", IFitFunction::Attribute(222.0));

    NormalFit.setAttribute("NRows", IFitFunction::Attribute(33.));

    NormalFit.setAttribute("NCols", IFitFunction::Attribute(26.));
    NormalFit.setAttribute("Intensities", IFitFunction::Attribute(79.0));
    NormalFit.setAttribute("SSIx", IFitFunction::Attribute(18490.0));
    NormalFit.setAttribute("SSIy", IFitFunction::Attribute(16625.0));
    NormalFit.setAttribute("SSIxx", IFitFunction::Attribute(4331900.0));
    NormalFit.setAttribute("SSIyy", IFitFunction::Attribute(3506435.0));
    NormalFit.setAttribute("SSIxy", IFitFunction::Attribute(3890399.0));
    NormalFit.setAttribute("SSx", IFitFunction::Attribute(201201.0));
    NormalFit.setAttribute("SSy", IFitFunction::Attribute(181038.0));
    NormalFit.setAttribute("SSxx", IFitFunction::Attribute(4.7229897E7));
    NormalFit.setAttribute("SSyy", IFitFunction::Attribute(3.827681E7));
    NormalFit.setAttribute("SSxy", IFitFunction::Attribute(4.2453411E7));

    NormalFit.initialize();

    NormalFit.setParameter("Background",  2 + 79 / (33 * 26) , true);
    NormalFit.setParameter("Intensity", 79.0, true);
    NormalFit.setParameter("Mcol", 234.0506329, true);
    NormalFit.setParameter("Mrow", 110.44037975, true);
    NormalFit.setParameter("SScol", 54.4784490, true);
    NormalFit.setParameter("SSrow", 98.98093254, true);
    NormalFit.setParameter("SSrc", -8.76926775, true);


    double* x = new double[nCells];
    for (int i = 0; i < nCells; i++)
      x[i] = i;

    MantidVecPtr x_vec_ptr;
    for (int i = 0; i < nCells; i++)
      x_vec_ptr.access().push_back( x[i] );
    
    ws->setX(0,x_vec_ptr);

    NormalFit.setWorkspace( ws, std::string("StartX=0,EndX=857,WorkspaceIndex=0") );

    double* out = new double[nCells];
    NormalFit.function(out, x, nCells);

    double sav = out[0];
    
    NormalFit.addPenalty(out);
    TS_ASSERT_LESS_THAN(fabs(out[0] - sav - 89767.5), .5);
    delete[] x;
    delete[] out;
  }

};
#endif /* BIVARIATENORMALTEST_H_ */
