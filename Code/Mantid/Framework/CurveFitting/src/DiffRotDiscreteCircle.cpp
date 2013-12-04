/*WIKI*
== Summary ==

This fitting function models the dynamics structure factor of a particle undergoing discrete jumps on N-sites
evenly distributed in a circle. The particle can only jump to neighboring sites.
This is the most common type of discrete rotational diffusion in a circle.

The fitting parameters are the inverse of the transition rate, <math>\tau</math>
and the circle radius <math>r</math>

<math> S(Q,E) = A_0(Q,r) \delta (\omega) + \frac{1}{\pi} \sum_{l=1}^{N-1} A_l (Q,r) \frac{\tau_l}{1+(\omega \tau_l)^2} </math>

<math> A_l(Q,r) = \frac{1}{N} \sum_{k=1}^{N} j_0( 2 Q r sin(\frac{\pi k}{N}) ) cos(\frac{2\pi lk}{N}) </math>

<math> \tau_l^{-1} = 4 \tau^{-1} sin^2(\frac{\pi l}{N}) </math>

If the energy units are <math>\mu</math>eV, then <math>\tau</math> is expressed in nano-seconds. If E-units are meV then
<math>\tau</math> is expressed in pico-seconds.

== Properties ==

{| border="1" cellpadding="5" cellspacing="0"
!Order
!Name
!Default
!Description
|-
|1
|Intensity
|0.1
|Intensity of the peak
|-
|2
|Radius
|1.0
|Circle radius
|-
|3
|Decay
|100.0
|inverse of the transition rate (ps if energy in meV; ns if energy in <math>\mu</math>eV)
|}


[[Category:Fit_functions]]
*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <limits>
#include "MantidCurveFitting/DiffRotDiscreteCircle.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include "MantidAPI/ParameterTie.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Mantid
{
namespace CurveFitting
{

DECLARE_FUNCTION(ElasticDiffRotDiscreteCircle);
DECLARE_FUNCTION(InelasticDiffRotDiscreteCircle);
DECLARE_FUNCTION(DiffRotDiscreteCircle);

ElasticDiffRotDiscreteCircle::ElasticDiffRotDiscreteCircle(){
  //declareParameter("Height", 1.0); //parameter "Height" already declared in constructor of base class DeltaFunction
  declareParameter( "Radius", 1.0, "Circle radius" );

  // Ensure positive values for Height and Radius
  BoundaryConstraint* HeightConstraint = new BoundaryConstraint( this, "Height", std::numeric_limits<double>::epsilon(), true );
  addConstraint( HeightConstraint );

  BoundaryConstraint* RadiusConstraint = new BoundaryConstraint( this, "Radius", std::numeric_limits<double>::epsilon(), true );
  addConstraint( RadiusConstraint );

  declareAttribute( "Q", API::IFunction::Attribute(0.5) );
  declareAttribute( "N", API::IFunction::Attribute(3) );

}

double ElasticDiffRotDiscreteCircle::HeightPrefactor() const{
  const double R = getParameter( "Radius" );
  const double Q = getAttribute( "Q" ).asDouble();
  const int N = getAttribute( "N" ).asInt();
  double aN = 0;
  for ( int k = 1;  k < N;  k++ )
  {
	double x = 2 * Q * R * sin( M_PI * k / N );
	aN += sin( x ) / x;  // spherical Besell function of order zero j0==sin(x)/x
  }
  aN += 1; // limit for j0 when k==N, or x==0
  return aN / N;
}

InelasticDiffRotDiscreteCircle::InelasticDiffRotDiscreteCircle() : m_t2e(4.136)
{
  declareParameter( "Intensity",1.0, "scaling factor" );
  declareParameter( "Radius", 1.0, "Circle radius" );
  declareParameter( "Decay", 1.0, "Inverse of transition rate, in nanoseconds or picoseconds" );

  declareAttribute( "Q", API::IFunction::Attribute( 0.5 ) );
  declareAttribute( "N", API::IFunction::Attribute( 3 ) );

  // Ensure positive values for Intensity, Radius, and decay
  BoundaryConstraint* IntensityConstraint = new BoundaryConstraint( this, "Intensity", std::numeric_limits< double >::epsilon(), true );
  addConstraint(IntensityConstraint);

  BoundaryConstraint* RadiusConstraint = new BoundaryConstraint( this, "Radius", std::numeric_limits< double >::epsilon(), true );
  addConstraint( RadiusConstraint );

  BoundaryConstraint* DecayConstraint = new BoundaryConstraint( this, "Decay", std::numeric_limits< double >::epsilon(), true );
  addConstraint( DecayConstraint );

}


void InelasticDiffRotDiscreteCircle::function1D( double *out, const double* xValues, const size_t nData ) const
{
  const double I = getParameter( "Intensity" );
  const double R = getParameter( "Radius" );
  const double rate = m_t2e / getParameter( "Decay" ); // micro-eV or mili-eV
  const double Q = getAttribute( "Q" ).asDouble();
  const int N = getAttribute( "N" ).asInt();

  double sph[ N ];
  for ( int k = 1;  k < N;  k++ )
  {
	double x = 2 * Q * R * sin( M_PI * k / N );
	sph[ k ] = sin( x ) / x; // spherical Besell function of order zero 'j0' is sin(x)/x
  }

  double ratel[ N ];
  for ( int l = 1;  l < ( N - 1 );  l++)
  {
	ratel[ l ] = rate * 4 * pow( sin( M_PI * l / N ), 2 ); // notice that 0 < l/N < 1
  }

  for ( size_t i = 0;  i < nData;  i++ )
  {
	double w = xValues[ i ];
	double S = 0.0;
	for ( int l = 1;  l < ( N - 1 ); l++ )
	{
	  double lorentzian = ratel[ l ] / ( ratel[ l ] * ratel[ l ] + w * w );
	  double al = 0.0;
	  for ( int k = 1;  k < N;  k++ )
	  {
		double y = 2 * M_PI * l * k / N;
		al += cos( y ) * sph[ k ];
	  }
	  al += 1; // limit for j0 when k==N, or x==0
	  al /= N;
	  S += al * lorentzian;
	}
	out[ i ] = I * S / M_PI;
  }
}

/* Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 */
void DiffRotDiscreteCircle::trickleDownAttribute( const std::string & name )
{
  for ( size_t iFun = 0;  iFun < nFunctions(); iFun++ )
  {
    API::IFunction_sptr fun = getFunction( iFun );
    if( fun -> hasAttribute( name ) )
      fun -> setAttribute( name, this -> getAttribute( name ) );
  }
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffRotDiscreteCircle::declareAttribute( const std::string & name, const API::IFunction::Attribute & defaultValue )
{
  API::ImmutableCompositeFunction::declareAttribute( name, defaultValue );
  trickleDownAttribute( name );
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffRotDiscreteCircle::setAttribute( const std::string& name, const Attribute& att )
{
  API::ImmutableCompositeFunction::setAttribute( name, att );
  trickleDownAttribute( name );
}

DiffRotDiscreteCircle::DiffRotDiscreteCircle()
{
  m_elastic = boost::dynamic_pointer_cast<ElasticDiffRotDiscreteCircle>( API::FunctionFactory::Instance().createFunction( "ElasticDiffRotDiscreteCircle" ) );
  addFunction( m_elastic );
  m_inelastic = boost::dynamic_pointer_cast<InelasticDiffRotDiscreteCircle>( API::FunctionFactory::Instance().createFunction( "InelasticDiffRotDiscreteCircle" ) );
  addFunction( m_inelastic );

  this->setAttributeValue( "NumDeriv", true );

  this->declareAttribute( "Q", API::IFunction::Attribute( 0.5 ) );
  this->declareAttribute( "N", API::IFunction::Attribute( 3 ) );

  //Set the aliases
  this->setAlias( "f1.Intensity", "Intensity" );
  this->setAlias( "f1.Radius", "Radius" );
  this->setAlias( "f1.Decay", "Decay" );

  //Set the ties between Elastic and Inelastic parameters
  this->addDefaultTies( "f0.Height=f1.Intensity,f0.Radius=f1.Radius" );
  this->applyTies();

}

} // namespace CurveFitting
} // namespace Mantid
