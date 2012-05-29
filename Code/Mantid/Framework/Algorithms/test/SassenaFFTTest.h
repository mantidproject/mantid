#ifndef MANTID_ALGORITHMS_SASSENAFFTTEST_H_
#define MANTID_ALGORITHMS_SASSENAFFTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SassenaFFT.h"

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

  void test_exec()
  {
    if( !m_alg.isInitialized() ) m_alg.initialize();

    // register a Sassena-like group workspace
    const std::string gwsName("Sassena");
    this->createGroupWorkspace( gwsName );
    m_alg.setPropertyValue("InputWorkspace", gwsName);

    // execute the algorithm
    TS_ASSERT_THROWS_NOTHING( m_alg.execute() );
    TS_ASSERT( m_alg.isExecuted() );

    // compare the results
  }

private:
  /**
   * Gaussian centered at zero and with positive times only
   * @param sigma standard deviation
   * @param nbins number of time points
   */
  void Gaussian( MantidVec& yv, const double& sigma, const size_t& nbins)
  {
    const double& Height = 1.0;
    const double& dt = 1.0;
    double z;
    for(size_t i=0; i<nbins; i++)
    {
      z = i * dt / sigma;
      yv.push_back( Height*exp(-z*z/2.0) );
    }
  } // void Gaussian

  /// generate a Workspace2D, each spectra is half a gaussian
  void createWorkspace( const std::string& wsName, const double& sigma0, const size_t &nbins, const size_t& nspectra)
  {
    DataObjects::Workspace2D_sptr ws(new DataObjects::Workspace2D);
    ws->initialize(nspectra, nbins, nbins);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");

    const double dt=1.0;
    MantidVec xv;
    for(size_t i=0; i<nbins; i++)
    {
      xv.push_back( dt*i );
    }

    double scaling = (1.*nspectra)/sigma0;
    double sigma;
    MantidVec yv;
    for (size_t i=0; i<nspectra; i++)
    {
      ws->dataX(i) = xv;
      sigma = sigma0 - i * scaling;
      this->Gaussian(yv, sigma, nbins);
      ws->dataY(i) = yv;
    }

    API::AnalysisDataService::Instance().add( wsName, ws);
  } // void createWorkspace

  /// create a group workspace with 'real' and 'imaginary' workspaces
  void  createGroupWorkspace( const std::string& gwsName)
  {
    API::WorkspaceGroup_sptr gws = new(API::WorkspaceGroup);

    const size_t nbins(100);
    const size_t nspectra(10);

    std::string wsName = gwsName +"_fqt.Re";
    double sigma0 = 10.0;
    this->createWorkspace( wsName, sigma0, nbins, nspectra);
    gws->add(wsName);

    wsName = gwsName +"_fqt.Im";
    sigma0 = 1.0; // assume a comparatively smaller imaginary part
    this->createWorkspace( wsName, sigma0, nbins, nspectra);
    gws->add(wsName);

    API::AnalysisDataService::Instance().add( gwsName, gws);
  } // void  createGroupWorkspace

  Algorithms::SassenaFFT m_alg;
}; // class ApplyDetailedBalanceTest



#endif MANTID_ALGORITHMS_SASSENAFFTTEST_H_
