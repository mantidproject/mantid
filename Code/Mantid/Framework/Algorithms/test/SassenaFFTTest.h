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
    //this->printWorkspace2D("/tmp/sqw.dat",gwsName +"_sqw");
    // The input real part was an exponential h*exp(-x^2/(2*s^2) with h=1.0,s=1.0
    // The Fourier transform is an exponential h'*exp(-x^2/(2*s'^2) with h'=sqrt(2*pi*s)=2.507 and s=2*pi/s=0.159
    DataObjects::Workspace2D_const_sptr ws = API::AnalysisDataService::Instance().retrieveWS<DataObjects::Workspace2D>(gwsName +"_sqw");
    checkHeigth(ws, sqrt(2*M_PI) );
    checkSigma(ws, 1.0/(2.0*M_PI) );
  }

private:

  /**
   * Check the maximum value stored in the workspace
   * @param wsName name of the workspace
   * @param value compare to the maximum value stored in the Y-vector of the workspace
   */
  void checkHeigth(DataObjects::Workspace2D_const_sptr &ws, const double &value)
  {
    const double frErr=1E-04; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv;
    for(size_t i=0; i<nspectra; i++)
    {
      double goldStandard = value/(1+ static_cast<double>(i) );
      yv = ws->readY(i);
      double h = *std::max_element( yv.begin(), yv.end() );
      TS_ASSERT_DELTA( h, goldStandard, frErr );
    }
  }

  /**
   * Check the standar deviation
   * @param wsName name of the workspace
   * @param value compare to the standard deviation
   */
  void checkSigma(DataObjects::Workspace2D_const_sptr &ws, const double &value)
  {
    const double frErr=1E-04; //allowed fractional error
    const size_t nspectra = ws->getNumberHistograms();
    MantidVec yv,xv;
    for(size_t i=0; i<nspectra; i++)
    {
      double goldStandard = (1+ static_cast<double>(i) )*value;
      double dx = (-2.0) * ws->readX(i).at(0); // extent along the X-axis
      yv = ws->readY(i);
      double h = *std::max_element( yv.begin(), yv.end() );
      size_t nbins = yv.size();
      double sum = std::accumulate(yv.begin(), yv.end(), 0.0) * dx / static_cast<double>(nbins);
      double sigma = sum / (h * std::sqrt(2*M_PI));
      TS_ASSERT_DELTA( sigma, goldStandard, frErr );
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

  /// generate a Workspace2D, each spectra is half a gaussian
  void createWorkspace2D( const std::string& wsName, const double& Heigth, const double& sigma0, const size_t &nbins, const size_t& nspectra)
  {
    DataObjects::Workspace2D_sptr ws(new DataObjects::Workspace2D);
    ws->initialize(nspectra, nbins, nbins);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");

    const double dt=0.01;
    MantidVec xv;
    for(size_t i=0; i<nbins; i++)
    {
      int j = -static_cast<int>(nbins)/2 + static_cast<int>(i);
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

  /// create a group workspace with 'real' and 'imaginary' workspaces
  void  createGroupWorkspace( const double * params, const std::string& gwsName)
  {
    API::WorkspaceGroup_sptr gws(new API::WorkspaceGroup);

    const size_t nbins(2000);
    const size_t nspectra(4);

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
