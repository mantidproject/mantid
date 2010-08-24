//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MultipleScatteringAbsorption.h"
#include "MantidKernel/Exception.h"


#include <iostream>

namespace Mantid
{
namespace Algorithms
{
DECLARE_ALGORITHM(MultipleScatteringAbsorption)   // Register the class into the algorithm factory

using namespace Kernel;
using namespace API;

  // Constants required internally only, so make them static 

  static const double C[] =
               { 0.730284,-0.249987,0.019448,-0.000006,0.000249,-0.000004,
                 0.848859,-0.452690,0.056557,-0.000009,0.000000,-0.000006,
                 1.133129,-0.749962,0.118245,-0.000018,-0.001345,-0.000012,
                 1.641112,-1.241639,0.226247,-0.000045,-0.004821,-0.000030,
                 0.848859,-0.452690,0.056557,-0.000009,0.000000,-0.000006,
                 1.000006,-0.821100,0.166645,-0.012096,0.000008,-0.000126,
                 1.358113,-1.358076,0.348199,-0.038817,0.000022,-0.000021,
                 0.0,0.0,0.0,0.0,0.0,0.0,
                 1.133129,-0.749962,0.118245,-0.000018,-0.001345,-0.000012,
                 1.358113,-1.358076,0.348199,-0.038817,0.000022,-0.000021,
                 0.0,0.0,0.0,0.0,0.0,0.0,
                 0.0,0.0,0.0,0.0,0.0,0.0,
                 1.641112,-1.241639,0.226247,-0.000045,-0.004821,-0.000030,
                 0.0,0.0,0.0,0.0,0.0,0.0,
                 0.0,0.0,0.0,0.0,0.0,0.0,
                 0.0,0.0,0.0,0.0,0.0,0.0 };

  static const int Z_size = 36;       // Caution, this must be updated if the
                                      // algorithm is changed to use a different
                                      // size Z array.
  static const double Z_initial[] = 
               { 1.0,0.8488263632,1.0,1.358122181,2.0,3.104279270,
                 0.8488263632,0.0,0.0,0.0,0.0,0.0,
                 1.0,0.0,0.0,0.0,0.0,0.0,
                 1.358122181,0.0,0.0,0.0,0.0,0.0,
                 2.0,0.0,0.0,0.0,0.0,0.0,
                 3.104279270,0.0,0.0,0.0,0.0,0.0 };


/**
 * Initialize the properties to default values
 */
void MultipleScatteringAbsorption::init()
{
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace",
                  "",Direction::Input), "The name of the input workspace.");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace",
                  "",Direction::Output), "The name of the output workspace.");

  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("WorkspaceIndex", 0, mustBePositive,
                  "Spectrum index for Multiple Scattering Absorption Corrections");

  declareProperty("AttenuationXSection", 2.8, "Coefficient 1, absorption cross section / 1.81" );
  declareProperty("ScatteringXSection", 5.1, "Coefficient 3, total scattering cross section" );
  declareProperty("SampleNumberDensity", 0.0721, "Coefficient 2, density" );
  declareProperty("TotalFlightPath", 62.602, "Total Flight Path Length in meters" );
  declareProperty("ScatteringAngle", 129.824, "Scattering angle, two theta, in degrees" );
  declareProperty("CylinderSampleRadius", 0.3175, "Sample radius, in cm" );
}


/** 
 * Execute the algorithm
 */
void MultipleScatteringAbsorption::exec()
{
  API::MatrixWorkspace_sptr in_WS = getProperty("InputWorkspace");

  int index         = getProperty("WorkspaceIndex");
  int num_tofs      = in_WS->readX(index).size();
  int num_ys        = in_WS->readY(index).size();
  double total_path = getProperty("TotalFlightPath");
  double angle_deg  = getProperty("ScatteringAngle");
  double radius     = getProperty("CylinderSampleRadius");
  double coeff1     = getProperty("AttenuationXSection");
  double coeff2     = getProperty("SampleNumberDensity");
  double coeff3     = getProperty("ScatteringXSection");

  MantidVec tof_vec = in_WS->readX(index);
  MantidVec y_vec   = in_WS->readY(index);

  double* tof   = new double[ num_tofs ];
  double* y_val = new double[ num_ys ];

  for ( int i = 0; i < num_tofs; i++ )
    tof[i] = tof_vec[i];

  for ( int i = 0; i < num_ys; i++ )
    y_val[i] = y_vec[i];

  apply_msa_correction( total_path, angle_deg, radius,
                        coeff1, coeff2, coeff3,
                        tof,   num_tofs,
                        y_val, num_ys );

  for ( int i = 0; i < num_ys; i++ )
    y_vec[i] = y_val[i];

  API::MatrixWorkspace_sptr out_WS = 
    API::WorkspaceFactory::Instance().create( in_WS, 1, in_WS->readX(index).size(),
                                                        in_WS->readY(index).size() );

  out_WS->dataX(0).assign( tof_vec.begin(), tof_vec.end() );
  out_WS->dataY(0).assign(   y_vec.begin(),   y_vec.end() );   
  
  setProperty("OutputWorkspace",out_WS);

  delete [] tof;
  delete [] y_val;
}


/**
 * Set up the Z table for the specified two theta angle (in degrees).
 */
void MultipleScatteringAbsorption::ZSet( double angle_deg, double Z[] )
{
  double theta_rad = angle_deg * M_PI / 360;
  int l, J;
  double sum;

  for( int i = 1; i <= 4; i++ )
  {
    for( int j = 1; j <= 4; j++ )
    {
      int iplusj = i + j;
      if ( iplusj <= 5 )
      {
        l = 0;
        J = 1 + l + 6 * (i-1) + 6 * 4 * (j-1);
        sum = C[J-1];

        for( l = 1; l <= 5; l++ )
        {
          J   = 1 + l + 6 * (i-1) + 6 * 4 * (j-1);
          sum = sum + C[ J-1 ] * cos( l * theta_rad );
        }
        J      = 1 + i + 6 * j;
        Z[ J-1 ] = sum;
      }
    }
  }
}


/**
 * Evaluate the AttFac function for a given sigir and sigsr.
 */
double MultipleScatteringAbsorption::AttFac( float sigir, float sigsr, double Z[] )
{
  double facti = 1.0;
  double att   = 0.0;

  for( int i = 0; i <= 5; i++ )
  {
    double facts = 1.0;
    for( int j = 0; j <= 5; j++ )
    {
      int iplusj = i+j;
      if( iplusj <= 5 )
      {
        int J = 1 + i + 6 * j;
        att   = att + Z[J-1] * facts * facti;
        facts = -facts * sigsr / (j+1);
      }
   }
  facti = -facti * sigir / (i+1);
  }
  return att;
}


/**
 *  Calculate the wavelength at a specified total path in meters and
 *  time-of-flight in microseconds.
 */
double MultipleScatteringAbsorption::wavelength( double path_length_m, double tof_us )
{
  const double  H_ES  = 6.62606876e-27;               // h in erg seconds
  const double  MN_KG = 1.67492716e-27;               // mass of neutron(kg)
  const double  ANGST_PER_US_PER_M = H_ES/MN_KG/1000;

  return ANGST_PER_US_PER_M * tof_us / path_length_m;
}


/**
 * Alter the values in the y_vals[] to account for multiple scattering.
 * Parameter total_path is in meters, and the sample radius is in cm.
 */
void MultipleScatteringAbsorption::apply_msa_correction( 
                double total_path, double angle_deg, double radius,
                double coeff1,  double coeff2, double coeff3,
                double tof[],   int    n_tofs,
                double y_val[], int    n_ys )
{
  const double  coeff4 =  1.1967;
  const double  coeff5 = -0.8667;

  bool is_histogram = 0;
  if ( n_tofs == n_ys + 1 )
    is_histogram = 1;
  else if ( n_tofs == n_ys )
    is_histogram = 0;

  double wl_val, Q2,
         sigabs, sigir, sigsr, sigsct,
         delta, deltp,
         temp;

  double* Z = new double[Z_size];            // initialize Z array for this angle
    for ( int i = 0; i < Z_size; i++ )
      Z[i] = Z_initial[i];

  ZSet( angle_deg, Z );

  Q2     = coeff1 * coeff2;
  sigsct = coeff2 * coeff3;

  for ( int j = 0; j < n_ys; j++ )
  {
    if ( is_histogram )
      wl_val = (wavelength(total_path,tof[j]) + wavelength(total_path,tof[j+1]))/2;
    else
      wl_val = wavelength(total_path,tof[j]);

    sigabs = Q2 * wl_val;
    sigir  = ( sigabs + sigsct ) * radius;
    sigsr  = sigir;
    temp   = AttFac( sigir, sigsr, Z );

    delta = coeff4 * sigir + coeff5 * sigir * sigir;
    deltp = (delta * sigsct) / (sigsct + sigabs) ;

    y_val[j] *= ( 1.0 - deltp ) / temp;
  }
  delete [] Z;
}


} // namespace Algorithm
} // namespace Mantid
