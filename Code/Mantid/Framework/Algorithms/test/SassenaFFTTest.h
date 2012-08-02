#ifndef MANTID_ALGORITHMS_SASSENAFFTTEST_H_
#define MANTID_ALGORITHMS_SASSENAFFTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SassenaFFT.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/SaveAscii.h"
#include "MantidAPI/FileProperty.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Mantid;

class SassenaFFTTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static  SassenaFFTTest* createSuite() { return new SassenaFFTTest(); }
  static void destroySuite( SassenaFFTTest *suite ) { delete suite; }

  SassenaFFTTest() { }

  void test_init()
  {
    TS_ASSERT_THROWS_NOTHING( m_alg.initialize() )
    TS_ASSERT( m_alg.isInitialized() )
  }

  /// FFT of a real symmetric Gaussian
  void test_ZeroImaginary()
  {
    const double params[] = {1.0, 1.0, 0.0, 0.1, 0.1, 2.0}; // three pairs of (Heigth,sigma) values
    if( !m_alg.isInitialized() ){ m_alg.initialize(); }
    const std::string gwsName("Sassena");
    this->createGroupWorkspace( params, gwsName );
    m_alg.setPropertyValue("InputWorkspace", gwsName);
    // execute the algorithm
    TS_ASSERT_THROWS_NOTHING( m_alg.execute() );
    TS_ASSERT( m_alg.isExecuted() );
    //this->printWorkspace2D("/tmp/sqwZeroImaginary.dat",gwsName +"_sqw");
    // The input real part was an exponential h*exp(-x^2/(2*s^2) with h=1.0,s=1.0
    // The Fourier transform is an exponential h'*exp(-x^2/(2*s'^2) with h'=sqrt(2*pi*s)=2.507 and s=2*pi/s=0.159
    DataObjects::Workspace2D_const_sptr ws = API::AnalysisDataService::Instance().retrieveWS<DataObjects::Workspace2D>(gwsName +"_sqw");
    const double exponentFactor = 0.0;
    checkHeigth(ws, sqrt(2*M_PI), exponentFactor);
    checkAverage(ws,0.0, exponentFactor);
    checkSigma(ws, 1.0/(2.0*M_PI), exponentFactor);
  }

   /* FFT of a real symmetric Gaussian with detailed balance condition
   *
   */
  void test_DetailedBalanceCondition(){
    const double T(100);
    const double params[] = {1.0, 1.0, 0.0, 0.1, 0.1, 2.0}; // three pairs of (Heigth,sigma) values
    if( !m_alg.isInitialized() ){ m_alg.initialize(); }
    const std::string gwsName("SassenaII");
    this->createGroupWorkspace( params, gwsName );
    m_alg.setPropertyValue("InputWorkspace", gwsName);
    m_alg.setProperty("DetailedBalance",true);
    m_alg.setProperty("Temp",T);
    // execute the algorithm
    TS_ASSERT_THROWS_NOTHING( m_alg.execute() );
    TS_ASSERT( m_alg.isExecuted() );
    this->printWorkspace2D("/tmp/sqwDetailedBalanceCondition.dat",gwsName +"_sqw");
    DataObjects::Workspace2D_const_sptr ws = API::AnalysisDataService::Instance().retrieveWS<DataObjects::Workspace2D>(gwsName +"_sqw");
    const double exponentFactor = -11.604/(2.0*T);
    checkHeigth(ws, sqrt(2*M_PI),exponentFactor);
    checkAverage(ws,0.0, exponentFactor);
    checkSigma(ws, 1.0/(2.0*M_PI), exponentFactor);
  }

private:

  /**
   * Check the maximum value stored in the workspace when the detailed condition is applied
   * @param wsName name of the workspace
   * @param value compare to the maximum value stored in the Y-vector of the workspace
   * @exponentFactor inverse of the exponent factor in the detailed balance condition.
   */
  void checkHeigth(DataObjects::Workspace2D_const_sptr &ws, const double &value, const double &exponentFactor)
  {
    const double frErr=1E-03; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv;
    for(size_t i=0; i<nspectra; i++)
    {
      yv = ws->readY(i);
      MantidVec::iterator it = std::max_element( yv.begin(), yv.end() );
      size_t index = std::distance( yv.begin(), it);
      double x = ws->readX(i).at(index);
      double factor = exp(exponentFactor*x);
      double h = (*it)*factor;
      double goldStandard = value/(1+ static_cast<double>(i) );
      double error1 = DBL_EPSILON*std::sqrt( yv.size() ); //rounding error if value==0
      double error = fmax(error1, frErr*std::fabs(goldStandard));
      TS_ASSERT_DELTA( h, goldStandard, error );
    }
  }

  /**
   * Check the average.
   * @param wsName name of the workspace
   * @param value compare to the average
   * @exponentFactor inverse of the exponent factor in the detailed balance condition.
   */
  void checkAverage(DataObjects::Workspace2D_const_sptr &ws, const double &value, const double &exponentFactor)
  {
    const double frErr=1E-03; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv,xv;
    double factor;  //remove the detailed balance condition
    for(size_t i=0; i<nspectra; i++)
    {
      double goldStandard = (1+ static_cast<double>(i) )*value;
      xv = ws->readX(i);
      yv = ws->readY(i);
      double sum = 0.0;
      double average = 0.0;
      MantidVec::iterator itx = xv.begin();
      for(MantidVec::iterator it = yv.begin(); it != yv.end(); ++it)
      {
        factor = exp(exponentFactor*(*itx));
        sum += (*it)*factor;
        average += (*it)*(*itx)*factor;
        ++itx;
      }
      average /= sum;
      double error1 = DBL_EPSILON*std::sqrt( yv.size() ); //rounding error if value==0
      double error = fmax(error1, frErr*std::fabs(goldStandard));
      TS_ASSERT_DELTA( average, goldStandard, error );
    }
  }

  /**
   * Check the standard deviation
   * @param wsName name of the workspace
   * @param value compare to the standard deviation
   * @exponentFactor inverse of the exponent factor in the detailed balance condition.
   */
  void checkSigma(DataObjects::Workspace2D_const_sptr &ws, const double &value, const double &exponentFactor)
  {
    const double frErr=1E-03; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv,xv;
    double factor;  //remove the detailed balance condition
    for(size_t i=0; i<nspectra; i++)
    {
      double goldStandard = (1+ static_cast<double>(i) )*value;
      double dx = (-2.0) * ws->readX(i).at(0); // extent along the X-axis
      yv = ws->readY(i);
      MantidVec::iterator it = std::max_element( yv.begin(), yv.end() );
      size_t index = std::distance( yv.begin(), it);
      double x = ws->readX(i).at(index);
      factor = exp(exponentFactor*x);
      double h = (*it)*exp(x);
      xv = ws->readX(i);
      MantidVec::iterator itx = xv.begin();
      size_t nbins = yv.size();
      double sum = 0.0;
      for(MantidVec::iterator it = yv.begin(); it != yv.end(); ++it)
      {
        factor = exp(exponentFactor*(*itx));
        sum += (*it)*factor;
        ++itx;
      }
      sum *= dx / static_cast<double>(nbins);
      double sigma = sum / (h * std::sqrt(2*M_PI));
      double error1 = DBL_EPSILON*std::sqrt( yv.size() ); //rounding error if value==0
      double error = fmax(error1, frErr*std::fabs(goldStandard));
      TS_ASSERT_DELTA( sigma, goldStandard, error );
    }
  }

  /**
   * Gaussian centered at zero and with positive times only
   * @param sigma standard deviation
   * @param nbins number of time points
   */
  void Gaussian( MantidVec& xv, MantidVec& yv, const double& Heigth, const double& sigma)
  {
    double z;
    for(size_t i=0; i<xv.size(); i++)
    {
      z = xv[i]/sigma;
      yv[i] = Heigth*exp(-z*z/2.0);
    }
  } // void Gaussian

  /// print workspace to an Ascii file
  void printWorkspace2D( const std::string & fName, const std::string & wName)
  {
    Mantid::DataHandling::SaveAscii sa;
    if( !sa.isInitialized() ){ sa.initialize();}
    sa.setProperty("Filename",fName);
    sa.setProperty("InputWorkspace", wName);
    sa.execute();
  }

  /* Generate a Workspace2D with nspectra.
   * Each spectra is a gaussian centered at the origin and with a different standard deviation sigma.
   * sigma increases as sigma0*2^nspectra
   */
  void createWorkspace2D( const std::string& wsName, const double& Heigth, const double& sigma0, const size_t &nbins, const size_t& nspectra)
  {
    DataObjects::Workspace2D_sptr ws(new DataObjects::Workspace2D);
    ws->initialize(nspectra, nbins, nbins);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");

    const double dt=0.01;
    MantidVec xv;
    for(size_t i=0; i<nbins; i++)
    {
      int j = -static_cast<int>(nbins/2) + static_cast<int>(i);
      xv.push_back( dt*static_cast<double>(j) );
    }

    double sigma;
    MantidVec yv(nbins);
    for (size_t i=0; i<nspectra; i++)
    {
      ws->dataX(i) = xv;
      sigma = sigma0/(1+ static_cast<double>(i) );
      this->Gaussian(xv, yv, Heigth, sigma);
      ws->dataY(i) = yv;
    }

    API::AnalysisDataService::Instance().add( wsName, ws);
  } // void createWorkspace

  /* Create a group workspace with 'real' and 'imaginary' workspaces
   * Each workspace has nspectra, each spectra is a Gaussian centered at the origin
   * X-axis will run from -(nbins/2)*dt=-10.0 to (nbins/2)*dt=10.0
   */
  void  createGroupWorkspace( const double * params, const std::string& gwsName)
  {
    API::WorkspaceGroup_sptr gws(new API::WorkspaceGroup);

    const size_t nbins(2001);
    const size_t nspectra(4);
    //double dt=0.01; //this parameter has been moved to createWorkspace2D()
    std::string wsName = gwsName +"_fqt.Re";
    double Heigth = params[0];
    double sigma = params[1];
    this->createWorkspace2D( wsName, Heigth, sigma, nbins, nspectra);
    //this->printWorkspace2D("/tmp/fqt.Re.dat",wsName);
    gws->add(wsName);

    wsName = gwsName +"_fqt.Im";
    Heigth = params[2];
    sigma = params[3];
    this->createWorkspace2D( wsName, Heigth, sigma, nbins, nspectra);
    //this->printWorkspace2D("/tmp/fqt.Im.dat",wsName);
    gws->add(wsName);

    wsName = gwsName + "_fqt0";
    Heigth = params[4];
    sigma = params[5];
    this->createWorkspace2D( wsName, Heigth, sigma, nbins, 1);
    gws->add(wsName);

    API::AnalysisDataService::Instance().add( gwsName, gws);
  } // void  createGroupWorkspace

  Algorithms::SassenaFFT m_alg;
}; // class ApplyDetailedBalanceTest



#endif // MANTID_ALGORITHMS_SASSENAFFTTEST_H_
