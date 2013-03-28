/*
 * PeakHKLErrorsTest.h
 *
 *  Created on: Jan 27, 2013
 *      Author: ruth
 */

#ifndef PANELHKLERRORSTEST_H_
#define PANELHKLERRORSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/PeakHKLErrors.h"
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


    class PeakHKLErrorsTest: public CxxTest::TestSuite
    {

     public:

      PeakHKLErrorsTest(){}

      void test_data()
      {
        LoadIsawPeaks alg;
        alg.initialize();
        alg.setProperty("Filename","TOPAZ_5637_8.peaks");
        alg.setProperty("OutputWorkspace","abcd");
        alg.execute();
        alg.setProperty("OutputWorkspace","abcd");
        DataObjects::PeaksWorkspace_sptr peaks =alg.getProperty("OutputWorkspace");
        //std::cout<<"Peaks number="<<peaks->getNumberPeaks()<<std::endl;

        LoadIsawUB loadUB;
        loadUB.initialize();
        loadUB.setProperty("InputWorkspace", alg.getPropertyValue("OutputWorkspace") );
        loadUB.setProperty("Filename","ls5637.mat");
        loadUB.execute();


        PeakHKLErrors peakErrs;
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
        std::vector<double>out(3*NPeaks);
        std::vector<double>out1(3*NPeaks);
        std::vector<double> xValues(3*NPeaks);
        for( int i=0; i<NPeaks ;i++)
        {
          xValues[3*i]=i;
          xValues[3*i+1]=i;
          xValues[3*i+2]=i;
        }

        peakErrs.function1D( out.data(),xValues.data(),(size_t) (3*NPeaks));

      //       std::cout<<out[0]<<","<<out[4]<<","<<out[8]<<","<<out[12]<<","<<out[16]<<",";
      // std::cout<<std::endl;

        TS_ASSERT_DELTA(-0.0074152,   out[0], .01 );
        TS_ASSERT_DELTA( -0.00969701,out[4], .01 );
        TS_ASSERT_DELTA(0.0120299, out[8], .01 );
        TS_ASSERT_DELTA( -0.0060874, out[12], .01 );
        TS_ASSERT_DELTA( -0.0103673, out[16], .01 );

        boost::shared_ptr<Jacob>Jac(new Jacob ((int)peakErrs.nParams(), (int)(3*NPeaks)));
        peakErrs.functionDeriv1D(Jac.get(),xValues.data(),(size_t)(3*NPeaks));

        TS_ASSERT_DELTA( Jac->get(1,0), -1.39557, .4);
        TS_ASSERT_DELTA( Jac->get(3,1),2.24071,.4);
        TS_ASSERT_DELTA( Jac->get(10,2),-6.80222, .4);
        TS_ASSERT_DELTA( Jac->get(55,3), .188203,.1);
        TS_ASSERT_DELTA( Jac->get(85,3),.127,.1 );
        TS_ASSERT_DELTA( Jac->get(235,4), -.05,.1);
        TS_ASSERT_DELTA( Jac->get(110,5),.0678, .1);
        TS_ASSERT_DELTA( Jac->get(100,0), -1.4782, .4);
        TS_ASSERT_DELTA( Jac->get(200,1), -8.24138, .4);
        TS_ASSERT_DELTA( Jac->get(160,2), -12.7745,.1);
        TS_ASSERT_DELTA( Jac->get(80,4),-.0943 ,.1);

        /*   std::cout<<"Derivatives"<<std::endl;
        std::cout<< Jac->get(1,0)<<std::endl;
        std::cout<< Jac->get(3,1)<<std::endl;
        std::cout<< Jac->get(10,2)<<std::endl;
        std::cout<< Jac->get(55,3)<<std::endl;
        std::cout<< Jac->get(85,3)<<std::endl;
        std::cout<< Jac->get(235,4)<<std::endl;
        std::cout<< Jac->get(110,5)<<std::endl;
        std::cout<< Jac->get(100,0)<<std::endl;
        std::cout<< Jac->get(200,1)<<std::endl;
        std::cout<< Jac->get(160,2)<<std::endl;
        std::cout<< Jac->get(80,4)<<std::endl;

         double offset =.0001;
        for( size_t param=0; param < peakErrs.nParams(); param++)
        {
         // std::cout<<"Result for param="<<param<< peakErrs.parameterName(param)<<std::endl;
          double paramValSav= peakErrs.getParameter(param);
          peakErrs.setParameter(param, paramValSav+offset);
          peakErrs.function1D( out.data(),xValues.data(),(size_t) 3*NPeaks);

          peakErrs.setParameter(param, paramValSav-offset);
          peakErrs.function1D( out1.data(),xValues.data(),(size_t) 3*NPeaks);

          peakErrs.setParameter(param, paramValSav);
          for( int p = 0; p <= 3*peaks->getNumberPeaks(); p += 10 )
          {
            double calc= (out[p] - out1[p]) / (2*offset);
            //std::cout<<"("<<calc<<","<<Jac->get(p,param)<<")";
            //if( (p+1) %350 ==0)
            //  std::cout<<std::endl;
            double delta =.8;

            if( param >= 3) delta =.003;
            if( fabs( calc-Jac->get(p,param)) >  delta)
            {   std::cout<<"param and peak="<<param<<","<<p<<","<< fabs( calc-Jac->get(p,param))<<std::endl;
                std::cout<<"      out2,out1,offset="<<out1[p]<<","<<out[p]<<","<<offset<<","<<Jac->get(p,param)<<std::endl;
          //  TS_ASSERT_DELTA( calc,Jac->get(p,param), delta );//NOTE: may differ a LOT when 2 of hkl values are about as
                                                             //far from int on different sides
            }
          }
          if( param >= 2)
            offset =.05;
          if(param >= 5)
            offset =.001;

        }*/

      }
    };

#endif /* PANELHKLERRORSTEST_H_ */
