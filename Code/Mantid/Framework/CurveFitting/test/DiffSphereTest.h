#ifndef DIFFSPHERETEST_H_
#define DIFFSPHERETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCurveFitting/Fit.h"
#include "MantidDataHandling/LoadRaw.h"

#include "MantidCurveFitting/DiffSphere.h"
#include "MantidCurveFitting/DeltaFunction.h"
#include "MantidCurveFitting/Convolution.h"
#include <boost/math/special_functions/bessel.hpp>
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

//same class as ConvolutionTest_Gauss in ConvolutionTest.h
class DiffSphereTest_Gauss: public IPeakFunction
{
public:
	DiffSphereTest_Gauss()
  {
    declareParameter("c");     // center of the peak
    declareParameter("h",1.);  // height of the peak
    declareParameter("s",1.);  // 1/(2*sigma^2)
  }

  std::string name()const{return "DiffSphereTest_Gauss";}

  void functionLocal(double* out, const double* xValues, const size_t nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-x*x*w);
    }
  }
  void functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      double e = h*exp(-x*x*w);
      out->set(i,0,x*h*e*w);
      out->set(i,1,e);
      out->set(i,2,-x*x*h*e);
    }
  }

  double centre()const
  {
    return getParameter(0);
  }

  double height()const
  {
    return getParameter(1);
  }

  double fwhm()const
  {
    return getParameter(2);
  }

  void setCentre(const double c)
  {
    setParameter(0,c);
  }
  void setHeight(const double h)
  {
    setParameter(1,h);
  }

  void setFwhm(const double w)
  {
    setParameter(2,w);
  }

};
DECLARE_FUNCTION(DiffSphereTest_Gauss); //register DiffSphereTest_Gauss in the DynamicFactory


class DiffSphereTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& x, Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
  // values extracted from theoretical stucture factor S(Q=0.9,w) of diffusion
  // within a sphere of radius 2.66Angstroms and diffusion coefficient 1.45
    unsigned int nvalues=200;
    //structure factor
    double S[]={
        0.16243,0.162411,0.162353,0.162257,0.162123,0.16195,0.16174,0.161492,0.161207,0.160886,
        0.160528,0.160135,0.159706,0.159243,0.158746,0.158216,0.157653,0.157059,0.156434,0.155779,
        0.155094,0.154382,0.153642,0.152876,0.152084,0.151268,0.150428,0.149566,0.148682,0.147778,
        0.146855,0.145913,0.144953,0.143977,0.142985,0.141979,0.140959,0.139927,0.138883,0.137828,
        0.136764,0.13569,0.134609,0.13352,0.132425,0.131324,0.130219,0.12911,0.127997,0.126882,
        0.125765,0.124647,0.123529,0.122411,0.121293,0.120177,0.119062,0.117951,0.116841,0.115736,
        0.114634,0.113536,0.112444,0.111356,0.110273,0.109197,0.108126,0.107062,0.106005,0.104955,
        0.103912,0.102877,0.101849,0.100829,0.099817,0.0988135,0.0978183,0.0968318,0.0958539,0.0948848,
        0.0939247,0.0929735,0.0920314,0.0910985,0.0901748,0.0892603,0.0883551,0.0874593,0.0865727,
        0.0856956,0.0848278,0.0839694,0.0831203,0.0822806,0.0814502,0.0806291,0.0798172,0.0790146,
        0.0782212,0.0774369,0.0766618,0.0758956,0.0751385,0.0743902,0.0736509,0.0729203,0.0721985,
        0.0714853,0.0707807,0.0700846,0.0693969,0.0687176,0.0680466,0.0673837,0.066729,0.0660823,
        0.0654436,0.0648127,0.0641895,0.0635741,0.0629663,0.0623659,0.061773,0.0611875,0.0606092,
        0.060038,0.059474,0.0589169,0.0583667,0.0578234,0.0572868,0.0567568,0.0562334,0.0557164,
        0.0552059,0.0547016,0.0542036,0.0537118,0.0532259,0.0527461,0.0522722,0.0518041,0.0513417,
        0.050885,0.0504339,0.0499882,0.049548,0.0491132,0.0486837,0.0482593,0.0478401,0.047426,
        0.0470169,0.0466126,0.0462133,0.0458187,0.0454289,0.0450437,0.0446631,0.044287,0.0439154,
        0.0435482,0.0431853,0.0428267,0.0424723,0.0421221,0.0417759,0.0414338,0.0410957,0.0407615,
        0.0404312,0.0401047,0.039782,0.0394629,0.0391475,0.0388358,0.0385276,0.0382228,0.0379216,
        0.0376237,0.0373293,0.0370381,0.0367502,0.0364655,0.036184,0.0359056,0.0356303,0.0353581,
        0.0350888,0.0348225,0.0345592,0.0342987,0.0340411,0.0337862,0.0335342,0.0332848,0.0330382,
        0.0327942,0.0325528,0.032314};
    double wi=0.0, dw=0.01; //initial and delta frequency
    double w=wi;
    double cc=0.1; //estimate errors as ten percent of the structure factor
    for(unsigned int i=0; i<nvalues; i++){
      x[i] = w;
      y[i] = S[i];
      e[i] = cc*S[i];
      w+=dw;
    }
  } // end of void getMockData

  //convolve a Gaussian resolution function with a delta-Dirac of type ElasticDiffSphere
  void testElasticDiffSphere()
  {
    Convolution conv;
    //set the resolution function
    double h = 3.0;  // height
    double a = 1.3;  // 1/(2*sigma^2)
    auto res = IFunction_sptr( new DiffSphereTest_Gauss() );
    res->setParameter("c",0);
    res->setParameter("h",h);
    res->setParameter("s",a);
    conv.addFunction(res);

    //set the "structure factor"
    double H=1.5, R=2.6, Q=0.7;
    auto eds = IFunction_sptr( new ElasticDiffSphere() );
    eds->setParameter("Height",H);
    eds->setParameter("Radius",R);
    eds->setParameter("Q",Q);
    conv.addFunction(eds);

    //set up some frequency values centered around zero
    const int N = 117;
    double w[N],w0,dw = 0.13;
    w0=-dw*int(N/2);
    for(int i=0;i<N;i++) w[i] = w0 + i*dw;

    FunctionDomain1DView xView(&w[0],N);
    FunctionValues out(xView);
    //convolve. The result must be the resolution multiplied by factor H*hf);
    conv.function(xView,out);
    double hpf = pow(3*boost::math::sph_bessel(1,Q*R)/(Q*R),2); // height pre-factor
    for(int i=0;i<N;i++){
      TS_ASSERT_DELTA(out.getCalculated(i),H*hpf*h*exp(-w[i]*w[i]*a),1e-10);
    }
  }// end of testElasticDiffSphere

  void testInelasticDiffSphere()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "InelasticDiffSphereMockData";
    int histogramNumber = 1;
    int timechannels = 200;

    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    Mantid::MantidVec& x = ws2D->dataX(0); // x-values (frequencies)
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (structure factor)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of the structure factor
    getMockData(x, y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up DiffSphere fitting function
    InelasticDiffSphere fn;
    fn.initialize();

    //Start with some initial values
    fn.setParameter("Intensity",0.4);
    fn.setParameter("Radius",1.1);
    fn.setParameter("Diffusion",1.2);
    fn.setParameter("Q",0.7);
    fn.tie("Q","0.7"); //set tie for Parameter Q, since it's supposed to be a constant

    alg2.setPropertyValue("Function",fn.asString());

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","200");

    // execute fit
    TS_ASSERT_THROWS_NOTHING( TS_ASSERT( alg2.execute() ) );

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.001,0.001);

    auto out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    TS_ASSERT_DELTA( out->getParameter("Radius"), 2.66 ,0.05);
    TS_ASSERT_DELTA( out->getParameter("Diffusion"), 1.45 ,0.05);
    TS_ASSERT_DELTA( out->getParameter("Q"), 0.7 ,0.001);
    AnalysisDataService::Instance().remove(wsName);

  }// end of void testInelastic

  //assert the ties between the elastic and inelastic contributions
  void testDiffSphereTies()
  {
    double I=2.9, Q=0.7, R=2.3, D=0.45;
    DiffSphere ds;
    ds.setParameter("f1.Intensity",I);
    ds.setParameter("f1.Q",Q);
    ds.setParameter("f1.Radius",R);
    ds.setParameter("f1.Diffusion",D);

    auto ids = boost::dynamic_pointer_cast<InelasticDiffSphere>(ds.getFunction(1));
    TS_ASSERT_EQUALS(ids->getParameter("Intensity"),I);
    TS_ASSERT_EQUALS(ids->getParameter("Q"),Q);
    TS_ASSERT_EQUALS(ids->getParameter("Radius"),R);
    TS_ASSERT_EQUALS(ids->getParameter("Diffusion"),D);

    ds.applyTies();

    auto eds = boost::dynamic_pointer_cast<ElasticDiffSphere>(ds.getFunction(0));
    TS_ASSERT_EQUALS(eds->getParameter("Height"),I);
    TS_ASSERT_EQUALS(eds->getParameter("Q"),Q);
    TS_ASSERT_EQUALS(eds->getParameter("Radius"),R);
  }


  //generate first data from convolution of a Gaussian and a DiffSphere function.
  //Then reset the DiffSphere parameters and do the fit to recover the original values
  void testDiffSphere(){
    Convolution conv;

    //set the resolution function
    double h = 3.0;  // height
    double a = 1.3;  // 1/(2*sigma^2)
    auto res = IFunction_sptr( new DiffSphereTest_Gauss() );
    res->setParameter("c",0);
    res->setParameter("h",h);
    res->setParameter("s",a);
    conv.addFunction(res);

    //set the structure factor
    double I=2.9, Q=0.7, R=2.3, D=0.45;
    auto ds = IFunction_sptr( new DiffSphere() );
    ds->setParameter("f1.Intensity",I);
    ds->setParameter("f1.Q",Q);
    ds->setParameter("f1.Radius",R);
    ds->setParameter("f1.Diffusion",D);
    ds->applyTies(); //update the ties between elastic and inelastic parts

    conv.addFunction(ds);

    //set up some frequency values centered around zero
    const int N = 117;
    double w[N],in[N],w0,dw = 0.13;
    w0=-dw*int(N/2);
    for(int i=0;i<N;i++) w[i] = w0 + i*dw;

    FunctionDomain1DView xView(&w[0],N);
    FunctionValues values(xView);
    //obtain the set of values from the convolution and store in array 'in'
    conv.function(xView,values);

    //Initialize now the parameters to some other values.
    ds->setParameter("f0.Height",1.0);
    ds->setParameter("f0.Radius",1.1);
    ds->setParameter("f0.Q",1.2);

    ds->setParameter("f1.Intensity",1.3);
    ds->setParameter("f1.Radius",1.4);
    ds->setParameter("f1.Diffusion",1.5);
    ds->setParameter("f1.Q",1.6);

    ds->applyTies();  //update the ties between elastic and inelastic parts

    //Set up the workspace
    std::string wsName = "ConvGaussDiffSphere";
    int histogramNumber = 1;
    int timechannels = N;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    Mantid::MantidVec& x = ws2D->dataX(0); // x-values (frequencies)
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (structure factor)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of the structure factor
    const double cc=0.1;
    for(int i=0; i<N; i++){
      x[i] = w[i];
      y[i] = values.getCalculated(i);
      e[i] = cc*in[i];
    }
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D)); //put workspace in the data service

    //Initialize the fitting algorithm
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );
    alg2.setPropertyValue("Function",conv.asString());

  } // end of testDiffSphere

};

#endif /*DIFFSPHERETEST_H_*/
