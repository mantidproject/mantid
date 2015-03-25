#ifndef DIFFROTDISCRETECIRCLETEST_H_
#define DIFFROTDISCRETECIRCLETEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/shared_ptr.hpp>

#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/DiffRotDiscreteCircle.h"
#include "MantidCurveFitting/Convolution.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AlgorithmFactory.h"

class DiffRotDiscreteCircleTest :  public CxxTest::TestSuite
{

public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DiffRotDiscreteCircleTest *createSuite() { return new DiffRotDiscreteCircleTest(); }
  static void destroySuite( DiffRotDiscreteCircleTest *suite ) { delete suite; }


  // convolve the elastic part with a resolution function, here a Gaussian
  void testDiffRotDiscreteCircleElastic()
  {
    // initialize the resolution function
    const double w0 = random_value( -1.0, 1.0 );
    const double h = random_value( 1.0, 1000.0 );
    const double fwhm = random_value( 1.0, 100.0 );
    boost::shared_ptr<Mantid::CurveFitting::Gaussian> resolution( new Mantid::CurveFitting::Gaussian() );
    resolution -> initialize(); // declare parameters
    resolution -> setCentre( w0 );
    resolution -> setHeight( h );
    resolution -> setFwhm( fwhm );

    // initialize the structure factor as the elastic part of DiffRotDiscreteCircle
    const double I = random_value( 1.0, 1000.0 );
    const double r = random_value( 0.3, 9.8 );
    const double Q = 0.9;
    const int N = 6;
    boost::shared_ptr<Mantid::CurveFitting::ElasticDiffRotDiscreteCircle> structure_factor( new Mantid::CurveFitting::ElasticDiffRotDiscreteCircle() );
    structure_factor -> setParameter( "Height", I );
    structure_factor -> setParameter( "Radius", r );
    structure_factor -> setAttributeValue( "Q", Q );
    structure_factor -> setAttributeValue( "N", N );

    // initialize the convolution function
    Mantid::CurveFitting::Convolution conv;
    conv.addFunction( resolution );
    conv.addFunction( structure_factor );

    // initialize some frequency values centered around zero
    const size_t M = 4001;
    double w[ M ];
    const double dw = random_value(0.1, 0.5); // bin width
    for( size_t i = 0;  i < M;  i++ ) w[i] = (static_cast<double>(i) - M/2 ) * dw;
    Mantid::API::FunctionDomain1DView xView( &w[0], M );
    Mantid::API::FunctionValues out( xView );

    // convolve
    conv.function( xView, out );

    // Result must be the resolution function multiplied by the intensity of ElasticDiffRotDiscreteCircle
    double scaling = I * structure_factor -> HeightPrefactor();
    Mantid::API::FunctionValues out_resolution( xView );
    resolution -> function( xView, out_resolution );
    for( size_t i = 0;  i < M;  i++ )
      TS_ASSERT_DELTA( out.getCalculated( i ), scaling * out_resolution.getCalculated( i ), 1e-3 );

  } // testDiffRotDiscreteCircleElastic


  void testDiffRotDiscreteCircleInelastic()
  {
    runDiffRotDiscreteCircleInelasticTest(0.0);
  }


  void testDiffRotDiscreteCircleInelasticWithShift()
  {
    runDiffRotDiscreteCircleInelasticTest(0.5);
  }


  /* Check the particular case for N = 3
   * In this case, the inelastic part should reduce to a single Lorentzian in 'w':
   *   ( 2 / pi ) * A1( Q ) * ( 3 * tao / ( 9 + ( w * tao )**2 ) )
   *   A1( Q ) = ( 1 / 3 ) * ( 1 - j0( Q * R * sqrt( 3 ) ) )
   *   j0( x ) = sin( x ) / x
   */
  void testDiffRotDiscreteCircleInelasticN3()
  {
    const double I = 2.9;
    const double R = 2.3;
    const double tao = 0.468;
    const double Q = 0.9;

    // generate data workspace with the single lorentzian function
    auto data_workspace = generateN3Workspace( I, R, tao, Q );
    //saveWorkspace( data_workspace, "/tmp/junk_single_lorentzian.nxs" ); // for debugging purposes only

    // initialize the fitting function string
    // Parameter units are assumed in micro-eV, Angstroms, Angstroms**(-1), and nano-seconds. Intensities have arbitrary units
    std::string funtion_string = "name=InelasticDiffRotDiscreteCircle,N=3,Q=0.9,Intensity=2.9,Radius=2.3,Decay=0.468";

    // Do a fit with no iterations
    Mantid::CurveFitting::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING( fitalg.initialize() );
    TS_ASSERT( fitalg.isInitialized() );
    fitalg.setProperty( "Function", funtion_string );
    fitalg.setProperty( "MaxIterations", 0 ); // no iterations
    fitalg.setProperty( "InputWorkspace", data_workspace );
    fitalg.setPropertyValue( "WorkspaceIndex", "0" );
    TS_ASSERT_THROWS_NOTHING( TS_ASSERT( fitalg.execute() ) );
    TS_ASSERT( fitalg.isExecuted() );

    // create temporary workspace to check Y-values produced by the Fit algorithm  // for debugging purposes only
    //auto temp_workspace = generateWorkspaceFromFitAlgorithm( fitalg );           // for debugging purposes only
    //saveWorkspace( temp_workspace, "/tmp/junk_from_fit_algorithm.nxs" );         // for debugging purposes only

    // check the parameters of the InelasticDiffRotDiscreteCircle did not change
    Mantid::API::IFunction_sptr  fitalg_structure_factor = fitalg.getProperty( "Function" );
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Intensity" ), I, I * 0.01 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Radius" ), R, R * 0.01 );      // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Decay" ), tao, tao * 0.01 );       // allow for a small percent variation
    //std::cout << "\nGOAL: Intensity = 2.9,  Radius = 2.3,  Decay = 0.468\n"; // only for debugging purposes
    //std::cout << "OPTIMIZED: Intensity = " << fitalg_structure_factor->getParameter("Intensity") << "  Radius = " << fitalg_structure_factor->getParameter("Radius") << "  Decay = " << fitalg_structure_factor->getParameter("Decay") << "\n"; // only for debugging purposes

    // check Chi-square is small
    const double chi_squared = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT_LESS_THAN( chi_squared, 1e-12 );
    //std::cout << "\nchi_squared = " << chi_squared << "\n"; // only for debugging purposes

  }


  /// check ties between elastic and inelastic parts
  void testDiffRotDiscreteCircleTies()
  {
    const double I = 2.9;
    const double R = 2.3;
    const double tao = 0.45;
    const double Q = 0.7;
    const int N = 4;
    Mantid::CurveFitting::DiffRotDiscreteCircle func;
    func.init();
    func.setParameter( "f1.Intensity", I );
    func.setParameter( "f1.Radius" , R );
    func.setParameter( "f1.Decay", tao );
    func.setAttributeValue( "Q" , Q );
    func.setAttributeValue( "N", N );

    // check values where correctly initialized
    auto ids = boost::dynamic_pointer_cast<Mantid::CurveFitting::InelasticDiffRotDiscreteCircle>( func.getFunction(1) );
    TS_ASSERT_EQUALS( ids->getParameter("Intensity"), I );
    TS_ASSERT_EQUALS( ids->getParameter("Radius"), R );
    TS_ASSERT_EQUALS( ids->getParameter("Decay"), tao );
    TS_ASSERT_EQUALS( ids->getAttribute("Q").asDouble(), Q );
    TS_ASSERT_EQUALS( ids->getAttribute("Q").asDouble(), Q );

    //check the ties were applied correctly
    func.applyTies(); //elastic parameters are tied to inelastic parameters
    auto eds = boost::dynamic_pointer_cast<Mantid::CurveFitting::ElasticDiffRotDiscreteCircle>( func.getFunction(0) );
    TS_ASSERT_EQUALS( eds->getParameter("Height") , I );
    TS_ASSERT_EQUALS( eds->getParameter("Radius") , R );
    TS_ASSERT_EQUALS( eds->getAttribute("Q").asDouble(), Q );
  }


  /// check aliases in the composite function
  void testDiffRotDiscreteCircleAliases()
  {
    const double I = 2.9;
    const double R = 2.3;
    const double tao = 0.45;

    // This should set parameters of the inelastic part
    Mantid::CurveFitting::DiffRotDiscreteCircle func;
    func.init();
    func.setParameter( "Intensity", I );
    func.setParameter( "Radius", R );
    func.setParameter( "Decay", tao );

    // check the parameter of the inelastic part
    auto ifunc = boost::dynamic_pointer_cast<Mantid::CurveFitting::InelasticDiffRotDiscreteCircle>( func.getFunction(1) );
    TS_ASSERT_EQUALS( ifunc -> getParameter( "Intensity" ), I );
    TS_ASSERT_EQUALS( ifunc -> getParameter( "Radius" ), R );
    TS_ASSERT_EQUALS( ifunc -> getParameter( "Decay" ), tao );

    // check the parameters of the elastic part
    func.applyTies(); //elastic parameters are tied to inelastic parameters
    auto efunc = boost::dynamic_pointer_cast<Mantid::CurveFitting::ElasticDiffRotDiscreteCircle>( func.getFunction(0) );
    TS_ASSERT_EQUALS( efunc -> getParameter( "Height" ) , I );
    TS_ASSERT_EQUALS( efunc -> getParameter( "Radius" ) , R );

  } // testDiffRotDiscreteCircleAliases


  /// Fit the convolution of the jumping diffusion with a Gaussian resolution function
  void testDiffRotDiscreteCircle()
  {
    // initialize the fitting function in a Fit algorithm
    // Parameter units are assumed in micro-eV, Angstroms, Angstroms**(-1), and nano-seconds. Intensities have arbitrary units
    std::string funtion_string = "(composite=Convolution,FixResolution=true,NumDeriv=true;name=Gaussian,Height=1,PeakCentre=0,Sigma=20,ties=(Height=1,PeakCentre=0,Sigma=20);(name=DiffRotDiscreteCircle,N=3,NumDeriv=true,Q=0.5,Intensity=47.014,Radius=1.567,Decay=7.567))";

    // Initialize the fit function in the Fit algorithm
    Mantid::CurveFitting::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING( fitalg.initialize() );
    TS_ASSERT( fitalg.isInitialized() );
    fitalg.setProperty( "Function", funtion_string );

    // create the data workspace by evaluating the fit function in the Fit algorithm
    auto data_workspace = generateWorkspaceFromFitAlgorithm( fitalg );
    //saveWorkspace( data_workspace, "/tmp/junk.nxs" ); // for debugging purposes only

    //override the function with new parameters, then do the Fit
    funtion_string = "(composite=Convolution,FixResolution=true,NumDeriv=true;name=Gaussian,Height=1,PeakCentre=0,Sigma=20,ties=(Height=1,PeakCentre=0,Sigma=20);(name=DiffRotDiscreteCircle,N=3,NumDeriv=true,Q=0.5,Intensity=10.0,Radius=1.567,Decay=20.0))";
    fitalg.setProperty( "Function", funtion_string );
    fitalg.setProperty( "InputWorkspace", data_workspace );
    fitalg.setPropertyValue( "WorkspaceIndex", "0" );
    TS_ASSERT_THROWS_NOTHING( TS_ASSERT( fitalg.execute() ) );
    TS_ASSERT( fitalg.isExecuted() );

    // check Chi-square is small
    const double chi_squared = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT_LESS_THAN( chi_squared, 0.001 );
    //std::cout << "\nchi_squared = " << chi_squared << "\n"; // only for debugging purposes

    // check the parameters of the resolution did not change
    Mantid::API::IFunction_sptr fitalg_function = fitalg.getProperty( "Function" );
    auto fitalg_conv = boost::dynamic_pointer_cast<Mantid::CurveFitting::Convolution>( fitalg_function ) ;
    Mantid::API::IFunction_sptr fitalg_resolution = fitalg_conv->getFunction( 0 );
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "PeakCentre" ), 0.0, 0.00001 );  // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Height" ), 1.0,  1.0 * 0.001 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Sigma" ), 20.0,  20.0* 0.001 ); // allow for a small percent variation
    //std::cout << "\nPeakCentre = " << fitalg_resolution->getParameter("PeakCentre") << "  Height= " << fitalg_resolution->getParameter("Height") << "  Sigma=" << fitalg_resolution->getParameter("Sigma") << "\n"; // only for debugging purposes

    // check the parameters of the DiffRotDiscreteCircle
    Mantid::API::IFunction_sptr fitalg_structure_factor = fitalg_conv->getFunction( 1 );
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Intensity" ), 47.014, 47.014 * 0.05 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Radius" ), 1.567, 1.567 * 0.05 );      // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Decay" ), 7.567, 7.567 * 0.05 );       // allow for a small percent variation
    //std::cout << "\nGOAL: Intensity = 47.014,  Radius = 1.567,  Decay = 7.567\n"; // only for debugging purposes
    //std::cout << "OPTIMIZED: Intensity = " << fitalg_structure_factor->getParameter("Intensity") << "  Radius = " << fitalg_structure_factor->getParameter("Radius") << "  Decay = " << fitalg_structure_factor->getParameter("Decay") << "\n"; // only for debugging purposes

  } // testDiffRotDiscreteCircle


private:
  /// Fit the convolution of the inelastic part with a Gaussian resolution function
  void runDiffRotDiscreteCircleInelasticTest(const double S)
  {
    /* Note: it turns out that parameters Intensity and Radius are highly covariant, so that more than one minimum exists.
     * Thus, I tied parameter Radius. This is OK since one usually knows the radius of the circle of the jumping diffusion
     */
    const double I(47.014);
    const double R(1.567);
    const double tao(7.567);

    // initialize the fitting function in a Fit algorithm
    // Parameter units are assumed in micro-eV, Angstroms, Angstroms**(-1), and nano-seconds. Intensities have arbitrary units
    std::ostringstream function_stream;
    function_stream << "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                    << "name=Gaussian,Height=1.0,PeakCentre=0.0,Sigma=20.0,"
                    << "ties=(Height=1.0,PeakCentre=0.0,Sigma=20.0);"
                    << "name=InelasticDiffRotDiscreteCircle,N=3,Q=0.5,"
                    << "Intensity=" << I
                    << ",Radius=" << R
                    << ",Decay=" << tao
                    << ",Shift=" << S << ")";

    // Initialize the fit function in the Fit algorithm
    Mantid::CurveFitting::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING( fitalg.initialize() );
    TS_ASSERT( fitalg.isInitialized() );
    fitalg.setProperty( "Function", function_stream.str() );

    function_stream.str( std::string() );
    function_stream.clear();

    // create the data workspace by evaluating the fit function in the Fit algorithm
    auto data_workspace = generateWorkspaceFromFitAlgorithm( fitalg );
    //saveWorkspace( data_workspace, "/tmp/junk.nxs" ); // for debugging purposes only

    //override the function with new parameters, then do the Fit
    function_stream << "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                   << "name=Gaussian,Height=1.0,PeakCentre=0.0,Sigma=20.0,"
                   << "ties=(Height=1.0,PeakCentre=0.0,Sigma=20.0);"
                   << "name=InelasticDiffRotDiscreteCircle,N=3,Q=0.5,"
                   << "Intensity=10.0,Radius=1.567,Decay=20.0"
                   << ",ties=(Radius=" << R << "))";
    fitalg.setProperty( "Function", function_stream.str() );
    fitalg.setProperty( "InputWorkspace", data_workspace );
    fitalg.setPropertyValue( "WorkspaceIndex", "0" );
    TS_ASSERT_THROWS_NOTHING( TS_ASSERT( fitalg.execute() ) );
    TS_ASSERT( fitalg.isExecuted() );

    // check Chi-square is small
    const double chi_squared = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT_LESS_THAN( chi_squared, 0.001 );
    //std::cout << "\nchi_squared = " << chi_squared << "\n"; // only for debugging purposes

    // check the parameters of the resolution did not change
    Mantid::API::IFunction_sptr fitalg_function = fitalg.getProperty( "Function" );
    auto fitalg_conv = boost::dynamic_pointer_cast<Mantid::CurveFitting::Convolution>( fitalg_function ) ;
    Mantid::API::IFunction_sptr fitalg_resolution = fitalg_conv->getFunction( 0 );
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "PeakCentre" ), 0.0, 0.00001 );  // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Height" ), 1.0,  1.0 * 0.001 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Sigma" ), 20.0,  20.0* 0.001 ); // allow for a small percent variation
    //std::cout << "\nPeakCentre = " << fitalg_resolution->getParameter("PeakCentre") << "  Height= " << fitalg_resolution->getParameter("Height") << "  Sigma=" << fitalg_resolution->getParameter("Sigma") << "\n"; // only for debugging purposes

    // check the parameters of the inelastic part
    Mantid::API::IFunction_sptr fitalg_structure_factor = fitalg_conv->getFunction( 1 );
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Intensity" ), I, I * 0.05 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Radius" ), R, R * 0.05 );      // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Decay" ), tao, tao * 0.05 );       // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Shift" ), S, 0.00001 );       // allow for a small percent variation
    //std::cout << "\nGOAL: Intensity = 47.014,  Radius = 1.567,  Decay = 7.567\n"; // only for debugging purposes
    //std::cout << "OPTIMIZED: Intensity = " << fitalg_structure_factor->getParameter("Intensity") << "  Radius = " << fitalg_structure_factor->getParameter("Radius") << "  Decay = " << fitalg_structure_factor->getParameter("Decay") << "\n"; // only for debugging purposes

  } // runDiffRotDiscreteCircleInelasticTest


  /// returns a real value from a uniform distribution
  double random_value(const double & a, const double & b)
  {
    boost::mt19937 rng;
    boost::uniform_real<double> distribution( a, b );
    return distribution(rng);
  }


  /// save the domain and the values of a function to a nexus file
  void saveValues( Mantid::API::IFunction_sptr &function_pointer, Mantid::API::FunctionDomain1DView &xView, const std::string &filename )
  {
    Mantid::API::FunctionValues dataYvalues( xView );
    function_pointer -> function( xView, dataYvalues ); // evaluate the function
    const size_t M = xView.size();
    // create temporaray workspace.
    auto temp_ws = WorkspaceCreationHelper::Create2DWorkspace(1, static_cast<int>( M ) );
    for( size_t i = 0;  i < M;  i++ )
    {
      temp_ws -> dataX( 0 )[ i ] = xView[ i ];
      temp_ws -> dataY( 0 )[ i ] = dataYvalues.getCalculated( i );
      temp_ws -> dataE( 0 )[ i ] = 0.1 * dataYvalues.getCalculated( i ); // assume the error is 10% of the actual value
    }
    const double dw = xView[1]-xView[0]; // bin width
    temp_ws -> dataX( 0 )[ M ] =  temp_ws -> dataX( 0 )[ M - 1 ] + dw;
    //  save workspace to file.
    auto save = Mantid::API::AlgorithmFactory::Instance().create( "SaveNexus", 1 );
    if ( !save ) throw std::runtime_error( "Algorithm not created" );
    save -> initialize();
    save -> setProperty( "Filename", filename );
    save -> setProperty( "InputWorkspace", temp_ws );
    save->execute();

    // some cleaning
    Mantid::API::AnalysisDataService::Instance().remove( temp_ws -> getName() );

  }


  /* Create a workspace with the following single lorentzian in 'w'
   *   ( 2 / pi ) * A1( Q ) * ( 3 * tao / ( 9 + ( w * tao )**2 ) )
   *   A1( Q ) = ( 1 / 3 ) * ( 1 - j0( Q * R * sqrt( 3 ) ) )
   *   j0( x ) = sin( x ) / x
   */
  Mantid::DataObjects::Workspace2D_sptr generateN3Workspace( const double & I, const double & R, const double & tao, const double & Q )
  {
    const double hbar = 0.658211626; // plank constant in meV*THz (or ueV*PHz)
    const double rate = hbar / tao; // conversion from picosec to mili-eV, or from nanosec to micro-eV

    // calculate prefix A1. Better be verbose for clarity
    const double x = Q * R * sqrt( 3.0 );
    const double j0 = sin( x ) / x;
    const double A1 = ( 1.0 / 3.0 ) * ( 1.0 - j0 );

    // initialize some frequency values centered around zero. Will work as dataX
    const size_t M = 1001;
    double dataX[ M ];
    const double dw = 0.4; // typical bin width for BASIS@ORNL beamline, in micro-seconds
    for( size_t i = 0;  i < M;  i++ ) dataX[i] = (static_cast<double>(i) - M/2 ) * dw;

    // create the workspace
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1, M );
    double fractional_error = 0.01; // error taken as a percent of the signal
    for( size_t i = 0;  i < M;  i++ )
    {
      double bin_boundary = dataX[ i ] - dw / 2.0; // bin boundaries are shifted by half the bind width
      double y = I * ( 2.0 / M_PI ) * A1 * ( 3.0 * rate / ( 9.0 * rate * rate + dataX[ i ] * dataX[ i ]) ); // verbose for clarity
      ws -> dataX( 0 )[ i ] = bin_boundary ;
      ws -> dataY( 0 )[ i ] = y;
      ws -> dataE( 0 )[ i ] = fractional_error * y; // assume the error is a small percent of the actual value
    }
    ws -> dataX( 0 )[ M ] = dataX[ M - 1 ] + dw/2; // recall number of bin boundaries is 1 + #bins

   // return now the workspace
   return ws;
  }


  /// save a worskapece to a nexus file
  void saveWorkspace( Mantid::DataObjects::Workspace2D_sptr &ws, const std::string &filename )
  {
    auto save = Mantid::API::AlgorithmFactory::Instance().create( "SaveNexus", 1 );
    if ( !save ) throw std::runtime_error( "Algorithm not created" );
    save -> initialize();
    save -> setProperty( "Filename", filename );
    save -> setProperty( "InputWorkspace", ws );
    save->execute();
  }


  // create a data workspace using a Fit algorithm
  Mantid::DataObjects::Workspace2D_sptr generateWorkspaceFromFitAlgorithm( Mantid::CurveFitting::Fit & fitalg )
  {
    // initialize some frequency values centered around zero. Will work as dataX
    const size_t M = 1001;
    double dataX[ M ];
    const double dw = 0.4; // typical bin width for BASIS@ORNL beamline, in micro-seconds
    for( size_t i = 0;  i < M;  i++ ) dataX[i] = (static_cast<double>(i) - M/2 ) * dw;

    // Evaluate the fitting function. Will work as dataY
    Mantid::API::FunctionDomain1DView dataXview( &dataX[0], M );
    Mantid::API::FunctionValues dataYvalues( dataXview );
    Mantid::API::IFunction_sptr fitalg_function = fitalg.getProperty( "Function" );
    fitalg_function -> function( dataXview, dataYvalues );

    // create the workspace
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1, M );
    double fractional_error = 0.01; // error taken as a percent of the signal
    for( size_t i = 0;  i < M;  i++ )
    {
      ws -> dataX( 0 )[ i ] = dataX[ i ] - dw/2; // bin boundaries are shifted by half the bind width
      ws -> dataY( 0 )[ i ] = dataYvalues.getCalculated( i );
      ws -> dataE( 0 )[ i ] = fractional_error * dataYvalues.getCalculated( i ); // assume the error is a small percent of the actual value
    }
    ws -> dataX( 0 )[ M ] = dataX[ M - 1 ] + dw/2; // recall number of bin boundaries is 1 + #bins

   // return now the workspace
   return ws;
  }

};




















#endif /* DIFFROTDISCRETECIRCLETEST_H_ */
