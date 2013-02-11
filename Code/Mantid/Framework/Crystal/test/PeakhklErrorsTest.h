/*
 * PeakhklErrorsTest.h
 *
 *  Created on: Jan 27, 2013
 *      Author: ruth
 */

#ifndef PANELHKLERRORSTEST_H_
#define PANELHKLERRORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/PeakhklErrors.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/IndexPeaks.h"
#include "MantidAPI/Jacobian.h"
#include "MantidKernel/Matrix.h"
using Mantid::DataObjects::PeaksWorkspace_sptr;
using namespace Mantid;
using namespace Crystal;
using Mantid::API::IFunction;

class Jacob: public API::Jacobian
{
private:
  Kernel::Matrix<double> M;

public:
  Jacob(int nparams, int npoints)
  {
    M = Kernel::Matrix<double> (nparams, npoints);
  }

  virtual ~ Jacob()
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


    class PeakhklErrorsTest: public CxxTest::TestSuite
    {

     public:

      PeakhklErrorsTest(){}

      void test_data()
      {
        LoadIsawPeaks alg;
        alg.initialize();
        alg.setProperty("Filename","TOPAZ_5637_8.peaks");
        alg.setProperty("OutputWorkspace","abcd");
        alg.execute();
        alg.setProperty("OutputWorkspace","abcd");
        DataObjects::PeaksWorkspace_sptr peaks =alg.getProperty("OutputWorkspace");
        std::cout<<"Peaks number="<<peaks->getNumberPeaks()<<std::endl;

        LoadIsawUB loadUB;
        loadUB.initialize();
        loadUB.setProperty("InputWorkspace", alg.getPropertyValue("OutputWorkspace") );
        loadUB.setProperty("Filename","ls5637.mat");
        loadUB.execute();


        PeakhklErrors peakErrs;
        peakErrs.setAttribute( std::string("PeakWorkspaceName"),IFunction::Attribute("abcd"));
        peakErrs.setAttribute("OptRuns",IFunction::Attribute("/5638/"));
        peakErrs.initialize();
        std::string OptRuns[]={"5638"};
        double chis[]=  { 135.0};
        double phis[]=  { -.02  };
        double omegas[]={ 60  };
        for( int i=0; i <1;i++)
        {
          std::string name= "chi"+OptRuns[i];
          peakErrs.setParameter(name, chis[i]);
          name= "phi"+OptRuns[i];
          peakErrs.setParameter(name, phis[i]);
          name= "omega"+OptRuns[i];
          peakErrs.setParameter(name, omegas[i]);
        }

        peakErrs.setParameter("SampleXOffset",0.0);
        peakErrs.setParameter("SampleYOffset",0.0);
        peakErrs.setParameter("SampleZOffset",0.0);

        const int NPeaks(peaks->getNumberPeaks());
        std::vector<double>out(NPeaks);
        std::vector<double>out1(NPeaks);
        std::vector<double> xValues(NPeaks);
        for( int i=0; i<peaks->getNumberPeaks();i++)
          xValues[i]=i;

        peakErrs.function1D( out.data(),xValues.data(),(size_t) peaks->getNumberPeaks());

        //std::cout<<out[0]<<","<<out[1]<<","<<out[2]<<","<<out[3]<<","<<out[4]<<std::endl;
        TS_ASSERT_DELTA( -0.021081,   out[0], .01 );
        TS_ASSERT_DELTA( -0.00969701 ,out[1], .01 );
        TS_ASSERT_DELTA( -0.0170111, out[2], .01 );
        TS_ASSERT_DELTA( -0.0237843, out[3], .01 );
        TS_ASSERT_DELTA( -0.0277816, out[4], .01 );

        boost::shared_ptr<Jacob>Jac(new Jacob ((int)peakErrs.nParams(), (int)peaks->getNumberPeaks()));
        peakErrs.functionDeriv1D(Jac.get(),xValues.data(),(size_t)peaks->getNumberPeaks());

        double offset =.0001;
        for( size_t param=1; param < peakErrs.nParams(); param+=2)
        {
         //std::cout<<"Result for param="<<param<<std::endl;
          double paramValSav= peakErrs.getParameter(param);
          peakErrs.setParameter(param, paramValSav+offset);
          peakErrs.function1D( out.data(),xValues.data(),(size_t) peaks->getNumberPeaks());

          peakErrs.setParameter(param, paramValSav-offset);
          peakErrs.function1D( out1.data(),xValues.data(),(size_t) peaks->getNumberPeaks());

          peakErrs.setParameter(param, paramValSav);
          for( int p = 0; p <= peaks->getNumberPeaks(); p += 40 )
          {
            double calc= (out[p] - out1[p]) / (2*offset);
            //std::cout<<"("<<calc<<","<<Jac->get(p,param)<<")";
            //if( (p+1) %350 ==0)
            //  std::cout<<std::endl;
            double delta =.8;

            if( param >= 3) delta =.003;
            if( fabs( calc-Jac->get(p,param)) >  delta)
              std::cout<<"param and peak="<<param<<","<<p<<","<< fabs( calc-Jac->get(p,param))<<std::endl;
            TS_ASSERT_DELTA( calc,Jac->get(p,param), delta );//NOTE: may differ a LOT when 2 of hkl values are about as
                                                             //far from int on different sides
          }
          if( param > 2)
            offset =.005;

        }
      }
    };

#endif /* PANELHKLERRORSTEST_H_ */
