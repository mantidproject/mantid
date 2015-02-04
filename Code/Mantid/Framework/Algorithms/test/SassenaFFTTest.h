#ifndef MANTID_ALGORITHMS_SASSENAFFTTEST_H_
#define MANTID_ALGORITHMS_SASSENAFFTTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath> #TODO HARDCODED INSTRUMENT
#include "MantidAlgorithms/SassenaFFT.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/SaveAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/PhysicalConstants.h"

using namespace Mantid;

class SassenaFFTTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static  SassenaFFTTest* createSuite() { return new SassenaFFTTest(); }
  static void destroySuite( SassenaFFTTest *suite ) { delete suite; }

  SassenaFFTTest() : T2ueV(1000.0/Mantid::PhysicalConstants::meVtoKelvin), ps2meV(4.136), nbins(2001) { }

  void test_init()
  {
    TS_ASSERT_THROWS_NOTHING( m_alg.initialize() )
    TS_ASSERT( m_alg.isInitialized() )
  }

  /// FFT of a real symmetric Gaussian
  void test_ZeroImaginary()
  {
    // params defines (height,stdev) values for fqt.Re, fqt.Im, and fqt0, respectively
    const double params[] = {1.0, 1.0, 0.0, 0.1, 0.1, 2.0}; // params[3]=0.0 entails no fqt.Im
    if( !m_alg.isInitialized() ){ m_alg.initialize(); }
    const std::string gwsName("Sassena");
    this->createGroupWorkspace( params, gwsName );
    m_alg.setPropertyValue("InputWorkspace", gwsName);
    // execute the algorithm
    TS_ASSERT_THROWS_NOTHING( m_alg.execute() );
    TS_ASSERT( m_alg.isExecuted() );
    //this->printWorkspace2D("/tmp/sqwZeroImaginary.dat",gwsName +"_sqw"); // uncomment line for debugging purposes only
    // The input real part was an exponential h*exp(-x^2/(2*s^2) with h=1.0,s=1.0
    // The Fourier transform is an exponential h'*exp(-x^2/(2*s'^2) with h'=sqrt(2*pi*s)=2.507 and s'=1/(2*pi*s)=0.159
    DataObjects::Workspace2D_const_sptr ws = API::AnalysisDataService::Instance().retrieveWS<DataObjects::Workspace2D>(gwsName +"_sqw");
    const double exponentFactor = 0.0;
    checkHeight(ws, sqrt(2*M_PI), exponentFactor);
    checkAverage(ws,0.0, exponentFactor);
    checkSigma(ws, 1.0/(2.0*M_PI), exponentFactor);
  }

   /* FFT of a real symmetric Gaussian with detailed balance condition
   *
   */
  void test_DetailedBalanceCondition(){
    const double T(100);
    // params defines (height,stdev) values for fqt.Re, fqt.Im, and fqt0, respectively
    const double params[] = {1.0, 1.0, 0.0, 0.1, 0.1, 2.0};
    if( !m_alg.isInitialized() ){ m_alg.initialize(); }
    const std::string gwsName("SassenaII");
    this->createGroupWorkspace( params, gwsName );
    m_alg.setPropertyValue("InputWorkspace", gwsName);
    m_alg.setProperty("DetailedBalance",true);
    m_alg.setProperty("Temp",T);
    // execute the algorithm
    TS_ASSERT_THROWS_NOTHING( m_alg.execute() );
    TS_ASSERT( m_alg.isExecuted() );
    // this->printWorkspace2D("/tmp/sqwDetailedBalanceCondition.dat",gwsName +"_sqw"); // uncomment line for debugging purposes only
    DataObjects::Workspace2D_const_sptr ws = API::AnalysisDataService::Instance().retrieveWS<DataObjects::Workspace2D>(gwsName +"_sqw");
    const double exponentFactor = -1.0/(2.0*T*T2ueV); // negative of the quantum-correction to classical S(Q,E): exp(E/(2*kT)
    checkHeight(ws, sqrt(2*M_PI),exponentFactor);
    checkAverage(ws,0.0, exponentFactor);
    checkSigma(ws, 1.0/(2.0*M_PI), exponentFactor);
  }

private:

  /**
   * Check the maximum value stored in the workspace when the detailed condition is applied
   * @param wsName name of the workspace
   * @param value compare to the maximum value stored in the Y-vector of the workspace
   * @exponentFactor negative of the exponent factor in the detailed balance condition.
   */
  void checkHeight(DataObjects::Workspace2D_const_sptr &ws, const double &value, const double &exponentFactor)
  {
    const double frErr=1E-03; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv;
    for(size_t i=0; i<nspectra; i++)
    {
      yv = ws->readY(i);
      size_t index = nbins/2; // This position should yield ws->readX(i).at(index)==0.0
      double x = ws->readX(i).at(index);
      double h = yv.at(index)*exp(exponentFactor*x); // remove the quantum-correction from ws
      double goldStandard = value/(1+ static_cast<double>(i) );
      double error1 = DBL_EPSILON*std::sqrt( static_cast<double>(yv.size()) ); //rounding error if value==0
      double error = std::max(error1, frErr*std::fabs(goldStandard));
      TS_ASSERT_DELTA( h, goldStandard, error );
    }
  }

  /**
   * Check the average.
   * @param wsName name of the workspace
   * @param value compare to the average
   * @exponentFactor negative of the exponent factor in the detailed balance condition.
   */
  void checkAverage(DataObjects::Workspace2D_const_sptr &ws, const double &value, const double &exponentFactor)
  {
    const double frErr=1E-03; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv,xv;
    double factor;  //remove the detailed balance condition
    for(size_t i=0; i<nspectra; i++)
    {
      double goldStandard = (1+ static_cast<double>(i) )*value; // recall each spectra was created with a different stdev
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
      double error1 = std::sqrt(DBL_EPSILON*std::sqrt( static_cast<double>(yv.size()) )); //rounding error if value==0
      double error = std::max(error1, frErr*std::fabs(goldStandard));
      TS_ASSERT_DELTA( average, goldStandard, error );
    }
  }

  /**
   * Check the standard deviation
   * @param wsName name of the workspace
   * @param value compare to the standard deviation
   * @exponentFactor negative of the exponent factor in the detailed balance condition.
   */
  void checkSigma(DataObjects::Workspace2D_const_sptr &ws, const double &value, const double &exponentFactor)
  {
    const double frErr=1E-03; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv,xv;
    double factor;  // remove the detailed balance condition
    for(size_t i=0; i<nspectra; i++)
    {
      double goldStandard = ps2meV*(1+ static_cast<double>(i) )*value; // recall each spectra was created with a different stdev
      double dx = (-2.0) * ws->readX(i).at(0); // extent along the X-axis
      yv = ws->readY(i);
      size_t index = nbins/2; // This position should yield ws->readX(i).at(index)==0.0
      double x = ws->readX(i).at(index);
      factor = exp(exponentFactor*x);
      double h = yv.at(index)*exp(x);
      xv = ws->readX(i);
      MantidVec::iterator itx = xv.begin();
      double sum = 0.0;
      for(MantidVec::iterator it = yv.begin(); it != yv.end(); ++it)
      {
        factor = exp(exponentFactor*(*itx));
        sum += (*it)*factor;
        ++itx;
      }
      sum *= dx / static_cast<double>(nbins);
      double sigma = sum / (h * std::sqrt(2*M_PI));
      double error1 = DBL_EPSILON*std::sqrt( static_cast<double>(yv.size()) ); //rounding error if value==0
      double error = std::max(error1, frErr*std::fabs(goldStandard));
      TS_ASSERT_DELTA( sigma, goldStandard, error );
    }
  }

  /**
   * Gaussian centered at zero and with positive times only
   * @param sigma standard deviation
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
  void createWorkspace2D( const std::string& wsName, const double& Heigth, const double& sigma0, const size_t& nspectra)
  {
    DataObjects::Workspace2D_sptr ws(new DataObjects::Workspace2D);
    ws->initialize(nspectra, nbins, nbins); // arguments are NVectors, XLength, and YLength
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");

    const double dt=0.01; // time unit, in picoseconds
    MantidVec xv;
    for(size_t i=0; i<nbins; i++)
    {
      int j = -static_cast<int>(nbins/2) + static_cast<int>(i);
      xv.push_back( dt*static_cast<double>(j) );
    }

    double sigma;
    MantidVec yv(nbins);
    // each spectra is a gaussian of same Height but different stdev
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
    API::AnalysisDataService::Instance().add( gwsName, gws);

    const size_t nspectra(4); // assume four Q-values
    std::string wsName = gwsName +"_fqt.Re";
    double Heigth = params[0];
    double sigma = params[1];
    this->createWorkspace2D( wsName, Heigth, sigma, nspectra);
    // this->printWorkspace2D("/tmp/fqt.Re.dat",wsName); // uncomment line for debugging purposes only
    gws->add(wsName);

    wsName = gwsName +"_fqt.Im";
    Heigth = params[2];
    sigma = params[3];
    this->createWorkspace2D( wsName, Heigth, sigma, nspectra);
    // this->printWorkspace2D("/tmp/fqt.Im.dat",wsName); // uncomment line for debugging purposes only
    gws->add(wsName);

    wsName = gwsName + "_fqt0";
    Heigth = params[4];
    sigma = params[5];
    this->createWorkspace2D( wsName, Heigth, sigma, 1);
    gws->add(wsName);

  } // void  createGroupWorkspace

  Algorithms::SassenaFFT m_alg;
  const double T2ueV; //conversion factor from Kelvin to ueV
  const double ps2meV; // conversion factor from picosecond to micro-eV
  const size_t nbins;
}; // class ApplyDetailedBalanceTest



#endif // MANTID_ALGORITHMS_SASSENAFFTTEST_H_
