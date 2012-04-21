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
#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <exception>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;
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

  double NormVal( double Background, double Intensity, double Mcol, double Mrow, double Vx,
                  double Vy, double Vxy, double row, double col)
  {

    double uu = Vx* Vy - Vxy * Vxy;

    double coefNorm = .5 / M_PI / sqrt(uu);

    double expCoeffx2 = -Vy / 2 / uu;
    double expCoeffxy = Vxy / uu;
    double expCoeffy2 = -Vx / 2 / uu;
    double dx = col - Mcol;
    double dy = row - Mrow;
    return  Background + coefNorm * Intensity *
                 exp(expCoeffx2 * dx * dx + expCoeffxy * dx * dy + expCoeffy2 * dy * dy);

  }
  void test_Normal()
    {
      BivariateNormal NormalFit;
      NormalFit.initialize();

      TS_ASSERT_EQUALS( NormalFit.nParams(),7);
      TS_ASSERT_EQUALS( NormalFit.nAttributes(),1);
      TS_ASSERT_EQUALS( NormalFit.name() , std::string("BivariateNormal"));

      const int nCells = 30;
      MatrixWorkspace_sptr ws1 = WorkspaceFactory::Instance().create("Workspace2D",3,nCells,nCells);
      Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>(ws1);

      double background =0.05;
      double intensity=562.95;
      double Mcol =195.698196998;
      double Mrow =44.252065014;
      double Vx =5.2438470;
      double Vy =3.3671409085;
      double Vxy = 2.243584414;


      Mantid::MantidVecPtr xvals, yvals, data;
      int sgn1 = 1;
      int sgn2 =1;
      for( int i= 0; i< nCells; i++)
        {
          double x = 195 + sgn1;
          double y= 44 + sgn2;
          if( sgn1 >0) if( sgn2 >0)sgn2=-sgn2;else {sgn1=-sgn1;sgn2=-sgn2;}
          else if(sgn2>0)sgn2=-sgn2;else {sgn1=-sgn1+1; sgn2=sgn1;}
          xvals.access().push_back( x );
          yvals.access().push_back( y );
          data.access().push_back( NormVal(background,intensity,Mcol,
                                      Mrow,Vx,Vy, Vxy, y, x));
        }


      Mantid::MantidVecPtr x_vec_ptr;
      double xx[nCells];
      for (int i = 0; i < nCells; i++)
         {
            xx[i] = i;
            x_vec_ptr.access().push_back((double) i );
         }


      ws->setX(0,x_vec_ptr);
      ws->setData(0,data);
      ws->setData(1,xvals);
      ws->setData(2,yvals);


      NormalFit.setMatrixWorkspace(ws, 0, 0.0, 30.0 );


      NormalFit.setParameter("Background", 0.05, true);
      NormalFit.setParameter("Intensity", 562.95, true);
      NormalFit.setParameter("Mcol",195.698196998, true);
      NormalFit.setParameter("Mrow", 44.252065014, true);
      NormalFit.setParameter("SScol", 5.2438470, true);
      NormalFit.setParameter("SSrow",3.3671409085, true);
      NormalFit.setParameter("SSrc", 2.243584414, true);


      NormalFit.setAttributeValue("CalcVariances", 1);

      std::vector<double> out(nCells);
      NormalFit.function1D(out.data(), xx, nCells);

      for( int i=0; i< nCells; i++)
      {

         double x = xvals.access()[i];
         double y = yvals.access()[i];
         double d = NormVal(background,intensity,Mcol,
             Mrow,Vx,Vy, Vxy, y, x);
         TS_ASSERT_DELTA( d, out[i],.001);
      }


    double Res[5][7]={{95.2508,       0.0605304,      -9.80469,      14.2329,     0,     0,      0},
                      { -5.45129,    0.000499636,      -0.264636,      0.233329,    0,     0,      0},
                      {5.19915,       0.00688051,        1.1584,        4.0905,     0,     0,      0},
                     { 1,            1.49829e-13,       2.14182e-10,    -2.1001e-10, 0,    0,     0},
                     {0.945744,      3.10687e-05,      0.0183605,     0.0324357,    0,    0,      0}};




      boost::shared_ptr<Jacob> Jac(new Jacob(7, nCells));

     NormalFit.functionDeriv1D(Jac.get(), xx, nCells);


/*     for( int i=0; i< nCells; i+=6)//points
      {
        std::cout<<i;
        for( int j=0;j< 7;j++)//params
          {
          std::cout<<","<<Jac->get(i,j);
          }
        std::cout<<std::endl;
      }
*/
      for( int i=0; i< nCells; i+=6)//points
      {


         for( int j=0;j< 7;j++)//params
         {
            double u = Res[i/6][j];
            double v = Jac->get(i,j);

            TS_ASSERT_DELTA(v,u,.001);
          }
       }


 /*   std::vector<double>out1(nCells);
      std::vector<double>out2(nCells);
      for( size_t param = 0; param < 4; param++ )
      {
       double x = NormalFit.getParameter( param);
       NormalFit.applyTies();
       NormalFit.function1D( out1.data(), xx,nCells);

       NormalFit.setParameter(param, x+.001);

       NormalFit.applyTies();
       NormalFit.function1D(out2.data(),xx,nCells);
       NormalFit.setParameter(param, x);
       std::cout<<"param "<<param<<"=";
       for( int i=0; i<nCells; i++)
         std::cout<<"["<<Jac.get(i,param)<<","<<(out2[i]-out1[i])/.001<<"]";
       std::cout<<std::endl;

      }
*/
    }

  void trestActual()
  {
    BivariateNormal NormalFit;
    std::cout<<"--------------------------------- Start actual----------------------"<<std::endl;
          NormalFit.initialize();

          TS_ASSERT_EQUALS( NormalFit.nParams(),7);
          TS_ASSERT_EQUALS( NormalFit.nAttributes(),1);
          TS_ASSERT_EQUALS( NormalFit.name() , std::string("BivariateNormal"));

          const int nCells = 30;
          MatrixWorkspace_sptr ws1 = WorkspaceFactory::Instance().create("Workspace2D",3,nCells,nCells);
          Workspace2D_sptr ws = boost::dynamic_pointer_cast<Workspace2D>(ws1);

          double background =0.05;
          double intensity=562.95;
          double Mcol =195.698196998;
          double Mrow =44.252065014;
          double Vx =5.2438470;
          double Vy =3.3671409085;
          double Vxy = 2.243584414;


          Mantid::MantidVecPtr xvals, yvals, data;
          int sgn1 = 1;
          int sgn2 =1;
          for( int i= 0; i< nCells; i++)
            {
              double x = 195 + sgn1;
              double y= 44 + sgn2;
              if( sgn1 >0) if( sgn2 >0)sgn2=-sgn2;else {sgn1=-sgn1;sgn2=-sgn2;}
              else if(sgn2>0)sgn2=-sgn2;else {sgn1=-sgn1+1; sgn2=sgn1;}
              xvals.access().push_back( x );
              yvals.access().push_back( y );
              data.access().push_back( NormVal(background,intensity,Mcol,
                                          Mrow,Vx,Vy, Vxy, y, x));
            }


          Mantid::MantidVecPtr x_vec_ptr;
          double xx[nCells];
          for (int i = 0; i < nCells; i++)
             {
                xx[i] = i;
                x_vec_ptr.access().push_back((double) i );
             }


          ws->setX(0,x_vec_ptr);
          ws->setData(0,data);
          ws->setData(1,xvals);
          ws->setData(2,yvals);


          NormalFit.setMatrixWorkspace(ws, 0, 0.0, 30.0 );


          NormalFit.setParameter("Background", 0.05, true);
          NormalFit.setParameter("Intensity", 562.95, true);
          NormalFit.setParameter("Mcol",195.70, true);
          NormalFit.setParameter("Mrow", 44.25, true);
          NormalFit.setParameter("SScol",3.28854,true);// 5.25, true);
          NormalFit.setParameter("SSrow",3.31161,true);//3.37, true);
          NormalFit.setParameter("SSrc", 2.28099,true);//2.24, true);


          NormalFit.setAttributeValue("CalcVariances", 1);
          Jacob Jac(7, nCells);
          NormalFit.functionDeriv1D( &Jac,xx,nCells);
          double out1[nCells];
          /*    NormalFit.CalcVxy=false;
          NormalFit.CalcVyy= false;
          NormalFit.removeTie(5);
          NormalFit.removeTie(6);
          NormalFit.functionDeriv1D( &Jac,xx,nCells);
       */   NormalFit.applyTies();
          NormalFit.function1D(out1, xx, nCells);
          double out2[nCells];

          for( int param =0; param<7;param++)
          {
            double opV = NormalFit.getParameter( param);
            NormalFit.setParameter( param, opV+.001);
            NormalFit.applyTies();

            NormalFit.function1D(out2, xx, nCells);
            std::cout<<"param "<<param<<"=";
            for( int i=0; i< nCells;i++)
               std::cout<<"["<<(out2[i]-out1[i])/.001<<","<< Jac.get(i,param)<<"]";
            std::cout<<std::endl;
            NormalFit.setParameter( param, opV);
          }




  }

  void testForCategories()
  {
    BivariateNormal forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Peak" );
  }


};
#endif /* BIVARIATENORMALTEST_H_ */
