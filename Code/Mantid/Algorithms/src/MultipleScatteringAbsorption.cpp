//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MultipleScatteringAbsorption.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"

#include <iostream>
#include <stdexcept>

namespace Mantid
{
namespace Algorithms
{
DECLARE_ALGORITHM(MultipleScatteringAbsorption)   // Register the class into the algorithm factory

using namespace Kernel;
using namespace API;
using std::vector;

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

  static const double  H_ES  = 6.62606876e-27;               // h in erg seconds
  static const double  MN_KG = Mantid::PhysicalConstants::NeutronMass;  // mass of neutron(kg)
  static const double  ANGST_PER_US_PER_M = H_ES/MN_KG/1000.;

/**
 * Initialize the properties to default values
 */
void MultipleScatteringAbsorption::init()
{
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace",
                  "",Direction::Input), "The name of the input workspace.");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace",
                  "",Direction::Output), "The name of the output workspace.");

  declareProperty("AttenuationXSection", 2.8, "Coefficient 1, absorption cross section / 1.81" );
  declareProperty("ScatteringXSection", 5.1, "Coefficient 3, total scattering cross section" );
  declareProperty("SampleNumberDensity", 0.0721, "Coefficient 2, density" );
  declareProperty("CylinderSampleRadius", 0.3175, "Sample radius, in cm" );
}


/** 
 * Execute the algorithm
 */
void MultipleScatteringAbsorption::exec()
{
  // common information
  API::MatrixWorkspace_sptr in_WS = getProperty("InputWorkspace");
  double radius     = getProperty("CylinderSampleRadius");
  double coeff1     = getProperty("AttenuationXSection");
  double coeff2     = getProperty("SampleNumberDensity");
  double coeff3     = getProperty("ScatteringXSection");

  // geometry stuff
  size_t nHist = in_WS->getNumberHistograms();
  Geometry::IInstrument_const_sptr instrument = in_WS->getInstrument();
  if (instrument == NULL)
    throw std::runtime_error("Failed to find instrument attached to InputWorkspace");
  Geometry::IObjComponent_const_sptr source = instrument->getSource();
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  if (source == NULL)
    throw std::runtime_error("Failed to find source in the instrument for InputWorkspace");
  if (sample == NULL)
    throw std::runtime_error("Failed to find sample in the instrument for InputWorkspace");
  double l1 = source->getDistance(*sample);

  // Create the new workspace
  MatrixWorkspace_sptr out_WS = WorkspaceFactory::Instance().create(in_WS, nHist,
      in_WS->readX(0).size(), in_WS->readY(0).size());

  for (size_t index = 0; index < nHist; ++index) {
    Geometry::IDetector_sptr det = in_WS->getDetector(index);
    if (det == NULL)
      throw std::runtime_error("Failed to find detector");
    double l2 = det->getDistance(*sample);
    double tth_rad = in_WS->detectorTwoTheta(det);
    double total_path = l1 + l2;

    MantidVec tof_vec = in_WS->readX(index);
    MantidVec y_vec   = in_WS->readY(index);

    apply_msa_correction( total_path, tth_rad, radius,
                          coeff1, coeff2, coeff3,
                          tof_vec, y_vec);

    out_WS->dataX(index).assign( tof_vec.begin(), tof_vec.end() );
    out_WS->dataY(index).assign(   y_vec.begin(),   y_vec.end() );
  }
  
  setProperty("OutputWorkspace",out_WS);
}


/**
 * Set up the Z table for the specified two theta angle (in degrees).
 */
void MultipleScatteringAbsorption::ZSet(const double angle_rad, vector<double>& Z)
{
  double theta_rad = angle_rad * .5;
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
double MultipleScatteringAbsorption::AttFac(const double sigir, const double sigsr,
                                            const vector<double>& Z)
{
  double facti = 1.0;
  double att   = 0.0;

  for( size_t i = 0; i <= 5; i++ )
  {
    double facts = 1.0;
    for( size_t j = 0; j <= 5; j++ )
    {
      if( i+j <= 5 )
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
inline double MultipleScatteringAbsorption::wavelength( double path_length_m, double tof_us )
{
  return ANGST_PER_US_PER_M * tof_us / path_length_m;
}


/**
 * Alter the values in the y_vals[] to account for multiple scattering.
 * Parameter total_path is in meters, and the sample radius is in cm.
 */
void MultipleScatteringAbsorption::apply_msa_correction( 
                double total_path, double angle_deg, double radius,
                double coeff1,  double coeff2, double coeff3,
                vector<double>& tof, vector<double>& y_val)
{
  const double  coeff4 =  1.1967;
  const double  coeff5 = -0.8667;

  bool is_histogram = false;
  if ( tof.size() == y_val.size() + 1 )
    is_histogram = true;
  else if (tof.size() == y_val.size())
    is_histogram = false;

  double wl_val, Q2,
         sigabs, sigir, sigsr, sigsct,
         delta, deltp,
         temp;

  vector<double> Z(Z_initial, Z_initial+Z_size);   // initialize Z array for this angle
  ZSet(angle_deg, Z);

  Q2     = coeff1 * coeff2;
  sigsct = coeff2 * coeff3;

  size_t n_ys = y_val.size();
  for ( size_t j = 0; j < n_ys; j++ )
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
}


} // namespace Algorithm
} // namespace Mantid
