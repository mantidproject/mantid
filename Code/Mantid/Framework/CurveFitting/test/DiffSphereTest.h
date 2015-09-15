#ifndef DIFFSPHERETEST_H_
#define DIFFSPHERETEST_H_

#include <iostream>
#include <fstream>
#include <limits>
#include <numeric>
#include <cxxtest/TestSuite.h>
#include <boost/lexical_cast.hpp>

// Include local copy of Valgrind header to avoid creating a dependency
#include "valgrind.h"

#include "MantidCurveFitting/DiffSphere.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Convolution.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AlgorithmFactory.h"

#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

class DiffSphereTest : public CxxTest::TestSuite
{
public:

  bool skipTests()
  {
    // Skip this test suite if running under valgrind as the Bessel function calls in DiffSphere sometimes return NaN in this situation.
    // It's something to do with boost using 80 bit precision where valgrind drops this to 64. See https://www.mail-archive.com/valgrind-users@lists.sourceforge.net/msg00974.html
    return ( RUNNING_ON_VALGRIND );
  }

  /// Convolve the elastic part with a resolution function, here a Gaussian
  void testDiffSphereElastic()
  {

    // define the fit function
    std::string funtion_string = "(composite=Convolution,FixResolution=true,NumDeriv=true;name=Gaussian,Height=1.0,PeakCentre=0.0,Sigma=0.002,ties=(Height=1.0,PeakCentre=0.0,Sigma=0.002);name=ElasticDiffSphere,Q=0.5,Height=47.014,Radius=3.567)";

    // Initialize the fit function in the Fit algorithm
    Mantid::CurveFitting::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING( fitalg.initialize() );
    TS_ASSERT( fitalg.isInitialized() );
    fitalg.setProperty( "Function", funtion_string );

    // create the data workspace by evaluating the fit function in the Fit algorithm
    auto data_workspace = generateWorkspaceFromFitAlgorithm( fitalg );
    //saveWorkspace( data_workspace, "/tmp/junk.nxs" ); // for debugging purposes only

    /* override the function with new parameters, then do the Fit. The effect of ElasticDiffSphere
     * is to multiply the Gaussian by height*[3*j_1(Q*Radius)/(Q*Radius)]^2. Thus, an increase in
     * parameter 'height' can be offset by an increase in the Radius. These parameters are coupled
     * and thus no unique fit exists. Thus, we fix parameter height and fit the radius.
    */
    funtion_string = "(composite=Convolution,NumDeriv=true;name=Gaussian,Height=1.0,PeakCentre=0.0,Sigma=0.002,ties=(Height=1.0,PeakCentre=0.0,Sigma=0.002);name=ElasticDiffSphere,Q=0.5,Height=47.014,Radius=6.0,ties=(Height=47.014))";
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
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Sigma" ), 0.002,  0.002* 0.001 ); // allow for a small percent variation
    //std::cout << "\nPeakCentre = " << fitalg_resolution->getParameter("PeakCentre") << "  Height= " << fitalg_resolution->getParameter("Height") << "  Sigma=" << fitalg_resolution->getParameter("Sigma") << "\n"; // only for debugging purposes

    // check the parameters of the elastic part
    Mantid::API::IFunction_sptr fitalg_structure_factor = fitalg_conv->getFunction( 1 );
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Height" ), 47.014, 47.014 * 0.05 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Radius" ), 3.567, 3.567 * 0.05 );      // allow for a small percent variation
    //std::cout << "\nGOAL: Height = 47.014,  Radius = 1.567\n"; // only for debugging purposes
    //std::cout << "OPTIMIZED: Height = " << fitalg_structure_factor->getParameter("Height") << "  Radius = " << fitalg_structure_factor->getParameter("Radius")<< "\n"; // only for debugging purposes

  }

  /* The weighted sum of the A_{n,l} coefficients is one
   * \sum_{n=0,l=0}^{n=\infty,l=\infty} (2*l+1) * A_{n,l}(Q*Radius) = 1, for all values of parameter Q and Radius
   * We don't have infinity terms, but 99 (including A_{0,0}) thus the sum will be close to one. The
   * sum is closer to 1 as the product Q*Radius decreases.
   */
  void testNormalization()
  {
    const double I(1.0);
    const double Q(1.0);
    const double D(1.0);

    double R(0.1); // We vary parameter R while keeping the other constant, same as varying Q*Radius
    const double dR(0.1);

    const double QR_max(20); // suggested value by Volino for the approximation of the 99 coefficients to break down

    // initialize the elastic part
    boost::shared_ptr<Mantid::CurveFitting::ElasticDiffSphere> elastic_part( new Mantid::CurveFitting::ElasticDiffSphere() );
    elastic_part -> setParameter( "Height", I );
    elastic_part -> setParameter( "Radius", R );
    elastic_part -> setAttributeValue( "Q", Q );
    elastic_part -> init();

    // initialize the inelastic part
    boost::shared_ptr<Mantid::CurveFitting::InelasticDiffSphere> inelastic_part( new Mantid::CurveFitting::InelasticDiffSphere() );
    inelastic_part -> setParameter( "Intensity", I );
    inelastic_part -> setParameter( "Radius", R );
    inelastic_part -> setParameter( "Diffusion", D );
    inelastic_part -> setAttributeValue( "Q", Q );
    inelastic_part -> init();

    // calculate the normalization over different values of Q*R
    while( Q*R < QR_max )
    {
      elastic_part -> setParameter( "Radius", R );
      double elastic_intensity = elastic_part -> HeightPrefactor(); // A_{0,0} coefficient
      inelastic_part -> setParameter( "Radius", R );
      std::vector< double > YJ = inelastic_part -> LorentzianCoefficients( Q * R ); // (2*l+1) * A_{n,l} coefficients
      double inelastic_intensity = std::accumulate( YJ.begin(), YJ.end(), 0.0 );
      TS_ASSERT_DELTA( elastic_intensity + inelastic_intensity, 1.0, 0.02 ); // Allow for a 2% deviation
      R += dR;
    }
  }

  void testDiffSphereInelasticWithQParam()
  {
    runDiffSphereInelasticTest(0.0, 0.20092);
  }

  void testDiffSphereInelasticWithWSIndex()
  {
    runDiffSphereInelasticTest(0.0);
  }

  void testDiffSphereInelasticWithShiftWithQParam()
  {
    runDiffSphereInelasticTest(0.2, 0.20092);
  }

  void testDiffSphereInelasticWithShiftWithWSIndex()
  {
    runDiffSphereInelasticTest(0.2);
  }

  void testDiffSphere()
  {
    // target parameters
    const double I_0(47.014);
    const double R_0(2.1);
    const double D_0(0.049);
    const double Q(0.5);

    // Initialize the fit function in the Fit algorithm
    Mantid::CurveFitting::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING( fitalg.initialize() );
    TS_ASSERT( fitalg.isInitialized() );
    std::ostringstream funtion_stream;
    funtion_stream << "(composite=Convolution,FixResolution=true,NumDeriv=true;name=Gaussian,Height=1.0,"
        << "PeakCentre=0.0,Sigma=0.002,ties=(Height=1.0,PeakCentre=0.0,Sigma=0.002);"
        << "name=DiffSphere,Q=" << boost::lexical_cast<std::string>( Q ) << ",Intensity="
        << boost::lexical_cast<std::string>( I_0 ) << ",Radius=" << boost::lexical_cast<std::string>( R_0 )
        << ",Diffusion=" << boost::lexical_cast<std::string>( D_0 ) << ")";
    fitalg.setProperty( "Function", funtion_stream.str() );

    // Find out whether ties were correctly applied
    Mantid::API::IFunction_sptr fitalg_function = fitalg.getProperty( "Function" ); // main function
    fitalg_function->initialize();
    auto fitalg_conv = boost::dynamic_pointer_cast<Mantid::CurveFitting::Convolution>( fitalg_function ) ; // cast to Convolution
    fitalg_function = fitalg_conv->getFunction( 1 ); // DiffSphere
    auto fitalg_structure_factor = boost::dynamic_pointer_cast<Mantid::CurveFitting::DiffSphere>( fitalg_function );

    fitalg_function = fitalg_structure_factor->getFunction( 0 );
    auto fitalg_elastic = boost::dynamic_pointer_cast<Mantid::CurveFitting::ElasticDiffSphere>( fitalg_function ) ;
    TS_ASSERT_DELTA( fitalg_elastic -> getParameter( "Height" ), I_0, std::numeric_limits<double>::epsilon() );
    TS_ASSERT_DELTA( fitalg_elastic -> getParameter( "Radius" ), R_0, std::numeric_limits<double>::epsilon() );
    TS_ASSERT_DELTA( fitalg_elastic -> getAttribute( "Q" ).asDouble(), Q, std::numeric_limits<double>::epsilon() );
    //std::cout << "Height=" << fitalg_elastic -> getParameter( "Height" ) << " Radius=" << fitalg_elastic -> getParameter( "Radius" ) << "\n"; // for debugging purposes only

    fitalg_function = fitalg_structure_factor->getFunction( 1 );
    auto fitalg_inelastic = boost::dynamic_pointer_cast<Mantid::CurveFitting::InelasticDiffSphere>( fitalg_function ) ;
    TS_ASSERT_DELTA( fitalg_inelastic -> getParameter( "Intensity" ), I_0, std::numeric_limits<double>::epsilon() );
    TS_ASSERT_DELTA( fitalg_inelastic -> getParameter( "Radius" ), R_0, std::numeric_limits<double>::epsilon() );
    TS_ASSERT_DELTA( fitalg_inelastic -> getParameter( "Diffusion" ), D_0, std::numeric_limits<double>::epsilon() );
    TS_ASSERT_DELTA( fitalg_inelastic -> getAttribute( "Q" ).asDouble(), Q, std::numeric_limits<double>::epsilon() );
    //std::cout << "Intensity=" << fitalg_inelastic->getParameter( "Intensity" ) << " Radius=" << fitalg_inelastic->getParameter( "Radius" ) << " Diffusion=" << fitalg_inelastic->getParameter( "Diffusion" ) <<"\n"; // for debugging purposes only

    // override the function with new parameters, our initial guess.
    double I = I_0 * ( 0.75 + ( 0.5 * std::rand() ) / RAND_MAX );
    double R = R_0 * ( 0.75 + ( 0.5 * std::rand() ) / RAND_MAX );
    double D = D_0 * ( 0.75 + ( 0.5 * std::rand() ) / RAND_MAX );
    funtion_stream.str( std::string() );
    funtion_stream.clear();
    funtion_stream << "(composite=Convolution,FixResolution=true,NumDeriv=true;name=Gaussian,Height=1.0,"
        << "PeakCentre=0.0,Sigma=0.002,ties=(Height=1.0,PeakCentre=0.0,Sigma=0.002);"
        << "name=DiffSphere,Q=" << boost::lexical_cast<std::string>( Q ) << ",Intensity="
        << boost::lexical_cast<std::string>( I ) << ",Radius=" << boost::lexical_cast<std::string>( R )
        << ",Diffusion=" << boost::lexical_cast<std::string>( D ) << ")";
    fitalg.setProperty( "Function", funtion_stream.str() );

    // create the data workspace by evaluating the fit function in the Fit algorithm
    auto data_workspace = generateWorkspaceFromFitAlgorithm( fitalg );
    //saveWorkspace( data_workspace, "/tmp/junk_data.nxs" ); // for debugging purposes only

    // Do the fit
    fitalg.setProperty( "InputWorkspace", data_workspace );
    fitalg.setPropertyValue( "WorkspaceIndex", "0" );
    TS_ASSERT_THROWS_NOTHING( TS_ASSERT( fitalg.execute() ) );
    TS_ASSERT( fitalg.isExecuted() );

    // check Chi-square is small
    const double chi_squared = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT_LESS_THAN( chi_squared, 0.001 );
    //std::cout << "\nchi_squared = " << chi_squared << "\n"; // only for debugging purposes

    // check the parameters of the resolution did not change
    Mantid::API::IFunction_sptr fitalg_resolution = fitalg_conv->getFunction( 0 );
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "PeakCentre" ), 0.0, 0.00001 );  // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Height" ), 1.0,  1.0 * 0.001 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Sigma" ), 0.002,  0.002* 0.001 ); // allow for a small percent variation
    //std::cout << "\nPeakCentre = " << fitalg_resolution->getParameter("PeakCentre") << "  Height= " << fitalg_resolution->getParameter("Height") << "  Sigma=" << fitalg_resolution->getParameter("Sigma") << "\n"; // only for debugging purposes

    // check the parameters of the DiffSphere close to the target parameters
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Intensity" ), I_0, I_0 * 0.05 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Radius" ), R_0, R_0 * 0.05 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Diffusion" ), D_0, D_0 * 0.05 ); // allow for a small percent variation
    //std::cout << "\nINITIAL GUESS: Intensity = "<<boost::lexical_cast<std::string>(I)<<",  Radius ="<<boost::lexical_cast<std::string>(R)<<",  Diffusion = "<<boost::lexical_cast<std::string>(D)<<"\n"; // only for debugging purposes
    //std::cout << "GOAL: Intensity = "<<boost::lexical_cast<std::string>(I_0)<<",  Radius = "<<boost::lexical_cast<std::string>(R_0)<<",  Diffusion = "<<boost::lexical_cast<std::string>(D_0)<<"\n"; // only for debugging purposes
    //std::cout << "OPTIMIZED: Intensity = " << fitalg_structure_factor->getParameter("Intensity") << "  Radius = " << fitalg_structure_factor->getParameter("Radius") << "  Diffusion = " << fitalg_structure_factor->getParameter("Diffusion") << "\n"; // only for debugging purposes
  }

private:
  void runDiffSphereInelasticTest(const double S, const double Q = Mantid::EMPTY_DBL())
  {
    // target fitting parameters
    const double I_0(47.014);
    const double R_0(2.1);
    const double D_0(0.049);

    double simQ = Q;
    if( Q == Mantid::EMPTY_DBL() )
      simQ = 0.20092;

    // Initialize the fit function in the Fit algorithm
    Mantid::CurveFitting::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING( fitalg.initialize() );
    TS_ASSERT( fitalg.isInitialized() );
    std::ostringstream funtion_stream;
    funtion_stream << "(composite=Convolution,FixResolution=true,NumDeriv=true;name=Gaussian,Height=1.0,"
        << "PeakCentre=0.0,Sigma=0.002,ties=(Height=1.0,PeakCentre=" << S << ",Sigma=0.002);"
        << "name=InelasticDiffSphere,Q=" << boost::lexical_cast<std::string>( simQ ) << ",Intensity="
        << boost::lexical_cast<std::string>( I_0 ) << ",Radius=" << boost::lexical_cast<std::string>( R_0 )
        << ",Diffusion=" << boost::lexical_cast<std::string>( D_0 )
        << ",Shift=" << boost::lexical_cast<std::string>(S) << ")";
    fitalg.setProperty( "Function", funtion_stream.str() );

    // create the data workspace by evaluating the fit function in the Fit algorithm
    auto data_workspace = generateWorkspaceFromFitAlgorithm( fitalg );
    //saveWorkspace( data_workspace, "/home/dan/junk_data.nxs" ); // for debugging purposes only

    // override the function with new parameters, our initial guess.
    double I = I_0 * ( 0.75 + ( 0.5 * std::rand() ) / RAND_MAX );
    double R = R_0 * ( 0.75 + ( 0.5 * std::rand() ) / RAND_MAX );
    double D = D_0 * ( 0.75 + ( 0.5 * std::rand() ) / RAND_MAX );
    funtion_stream.str( std::string() );
    funtion_stream.clear();
    funtion_stream << "(composite=Convolution,FixResolution=true,NumDeriv=true;name=Gaussian,Height=1.0,"
        << "PeakCentre=0.0,Sigma=0.002,ties=(Height=1.0,PeakCentre=" << S << ",Sigma=0.002);"
        << "name=InelasticDiffSphere";

    if( Q != Mantid::EMPTY_DBL() )
      funtion_stream << ",Q=" << boost::lexical_cast<std::string>( Q );

    funtion_stream << ",Intensity="
        << boost::lexical_cast<std::string>( I ) << ",Radius=" << boost::lexical_cast<std::string>( R )
        << ",Diffusion=" << boost::lexical_cast<std::string>( D )
        << ",Shift=" << boost::lexical_cast<std::string>(S) << ")";
    fitalg.setProperty( "Function", funtion_stream.str() );
    //auto before_workspace = generateWorkspaceFromFitAlgorithm( fitalg ); // for debugging purposes only
    //saveWorkspace( before_workspace, "/tmp/junk_before_fitting.nxs" ); // for debugging purposes only

    // Do the fit
    fitalg.setProperty( "InputWorkspace", data_workspace );
    fitalg.setPropertyValue( "WorkspaceIndex", "0" );
    TS_ASSERT_THROWS_NOTHING( TS_ASSERT( fitalg.execute() ) );
    TS_ASSERT( fitalg.isExecuted() );
    //auto after_workspace = generateWorkspaceFromFitAlgorithm( fitalg ); // for debugging purposes only
    //saveWorkspace( after_workspace, "/tmp/junk_after_fitting.nxs" ); // for debugging purposes only

    // check Chi-square is small
    const double chi_squared = fitalg.getProperty("OutputChi2overDoF");
    TS_ASSERT_LESS_THAN( chi_squared, 0.001 );
    //std::cout << "\nchi_squared = " << chi_squared << "\n"; // only for debugging purposes

    // check the parameters of the resolution did not change
    Mantid::API::IFunction_sptr fitalg_function = fitalg.getProperty( "Function" );
    auto fitalg_conv = boost::dynamic_pointer_cast<Mantid::CurveFitting::Convolution>( fitalg_function ) ;
    Mantid::API::IFunction_sptr fitalg_resolution = fitalg_conv->getFunction( 0 );

    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "PeakCentre" ), S, 0.00001 );  // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Height" ), 1.0,  1.0 * 0.001 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_resolution -> getParameter( "Sigma" ), 0.002,  0.002* 0.001 ); // allow for a small percent variation
    //std::cout << "\nPeakCentre = " << fitalg_resolution->getParameter("PeakCentre") << "  Height= " << fitalg_resolution->getParameter("Height") << "  Sigma=" << fitalg_resolution->getParameter("Sigma") << "\n"; // only for debugging purposes

    // check the parameters of the inelastic part close to the target parameters
    Mantid::API::IFunction_sptr fitalg_structure_factor = fitalg_conv->getFunction( 1 );
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Intensity" ), I_0, I_0 * 0.05 ); // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Radius" ), R_0, R_0 * 0.05 );      // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Diffusion" ), D_0, D_0 * 0.05 );      // allow for a small percent variation
    TS_ASSERT_DELTA( fitalg_structure_factor -> getParameter( "Shift" ), S, 0.0005 );      // allow for a small percent variation
    //std::cout << "\nINITIAL GUESS: Intensity = "<<boost::lexical_cast<std::string>(I)<<",  Radius ="<<boost::lexical_cast<std::string>(R)<<",  Diffusion = "<<boost::lexical_cast<std::string>(D)<<"\n"; // only for debugging purposes
    //std::cout << "GOAL: Intensity = "<<boost::lexical_cast<std::string>(I_0)<<",  Radius = "<<boost::lexical_cast<std::string>(R_0)<<",  Diffusion = "<<boost::lexical_cast<std::string>(D_0)<<"\n"; // only for debugging purposes
    //std::cout << "OPTIMIZED: Intensity = " << fitalg_structure_factor->getParameter("Intensity") << "  Radius = " << fitalg_structure_factor->getParameter("Radius") << "  Diffusion = " << fitalg_structure_factor->getParameter("Diffusion") << "\n"; // only for debugging purposes
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
    using namespace Mantid::Kernel;
    using namespace Mantid::Geometry;

    // initialize some frequency values centered around zero. Will work as dataX
    const size_t M = 1001;
    double dataX[ M ];
    const double dw = 0.0004; // typical bin width for BASIS@ORNL beamline, in micro-seconds
    for( size_t i = 0;  i < M;  i++ ) dataX[i] = (static_cast<double>(i) - M/2 ) * dw;

    // Evaluate the fitting function. Will work as dataY
    Mantid::API::FunctionDomain1DView dataXview( &dataX[0], M );
    Mantid::API::FunctionValues dataYvalues( dataXview );
    Mantid::API::IFunction_sptr fitalg_function = fitalg.getProperty( "Function" );
    fitalg_function -> function( dataXview, dataYvalues );

    // Create the workspace
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(1, M);

    // Create the instrument
    boost::shared_ptr<Instrument> inst(new Instrument("BASIS"));
    inst->setReferenceFrame(boost::shared_ptr<ReferenceFrame>(new ReferenceFrame(Y, Z, Left, "")));

    // Add the source position
    ObjComponent *source = new ObjComponent("moderator", ComponentCreationHelper::createSphere(0.1, V3D(0,0,0), "1"), inst.get());
    source->setPos(V3D(0.0, 0.0, -84.0));
    inst->add(source);
    inst->markAsSource(source);

    // Add the sample position
    ObjComponent *sample = new ObjComponent("samplePos", ComponentCreationHelper::createSphere(0.1, V3D(0,0,0), "1"), inst.get());
    inst->setPos(0.0, 0.0, 0.0);
    inst->add(sample);
    inst->markAsSamplePos(sample);

    // Add a detector
    Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
        0.05, 0.02, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
    Detector *det = new Detector("pixel-1", 1, pixelShape, inst.get()); // ID 5 is a valid detector for BASIS
    det->setPos(0.942677, 0.0171308, 4.63343); // Position of first detector on BASIS
    inst->add(det);
    inst->markAsDetector(det);

    // Set the instrument and spec-det mapping
    ws->setInstrument(inst);
    ws->getSpectrum(0)->addDetectorID(det->getID());

    // Set emergy mode and fixed energy
    ws->mutableRun().addLogData(new Mantid::Kernel::PropertyWithValue<std::string>("deltaE-mode", "Indirect"));
    ws->setEFixed(det->getID(), 2.08275); // EFixed of first detector on BASIS

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

}; // class DiffSphereTest

#endif /*DIFFSPHERETEST_H_*/
