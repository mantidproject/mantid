//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DiffSphere.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <boost/math/special_functions/bessel.hpp>
#include "MantidAPI/ParameterTie.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(DiffSphere);

ElasticDiffSphere::ElasticDiffSphere()
{
  //declareParameter("Height", 1.0); //parameter "Height" already declared in constructor of base class DeltaFunction
  declareParameter( "Radius", 1.0, "Sphere radius" );
  declareAttribute( "Q", API::IFunction::Attribute( 1.0 ) );
}

void ElasticDiffSphere::init()
{
  // Ensure positive values for Height and Radius
  BoundaryConstraint* HeightConstraint = new BoundaryConstraint( this, "Height" , 0 , true );
  addConstraint( HeightConstraint );

  BoundaryConstraint* RadiusConstraint = new BoundaryConstraint( this, "Radius", 0 , true );
  addConstraint( RadiusConstraint );
}

double ElasticDiffSphere::HeightPrefactor() const
{
  const double R = getParameter( "Radius" );
  const double Q = getAttribute( "Q" ).asDouble();
  return pow( 3 * boost::math::sph_bessel( 1, Q * R ) / ( Q * R ), 2 );
}

DECLARE_FUNCTION(InelasticDiffSphere);

// initialize class attribute m_xnl with a list of coefficients in string format
void InelasticDiffSphere::initXnlCoeff(){
  /* List of 98 coefficients sorted by increasing value (F.Volino,Mol. Phys. 41,271-279,1980)
   * For each coefficient, the triad (coeff, l, n) is defined
   */
  size_t ncoeff = 98;

  double xvalues[] =
  {
    2.081576, 3.342094, 4.493409, 4.514100, 5.646704, 5.940370, 6.756456, 7.289932,
    7.725252, 7.851078, 8.583755, 8.934839, 9.205840, 9.840446, 10.010371, 10.613855,
    10.904122, 11.070207, 11.079418, 11.972730, 12.143204, 12.279334, 12.404445,
    13.202620, 13.295564, 13.472030, 13.846112, 14.066194, 14.258341, 14.590552,
    14.651263, 15.244514, 15.310887, 15.579236, 15.819216, 15.863222, 16.360674,
    16.609346, 16.977550, 17.042902, 17.117506, 17.220755, 17.408034, 17.947180,
    18.127564, 18.356318, 18.453241, 18.468148, 18.742646, 19.262710, 19.270294,
    19.496524, 19.581889, 19.862424, 20.221857, 20.371303, 20.406581, 20.538074,
    20.559428, 20.795967, 21.231068, 21.537120, 21.578053, 21.666607, 21.840012,
    21.899697, 21.999955, 22.578058, 22.616601, 22.662493, 23.082796, 23.106568,
    23.194996, 23.390490, 23.519453, 23.653839, 23.783192, 23.906450, 24.360789,
    24.382038, 24.474825, 24.689873, 24.850085, 24.899636, 25.052825, 25.218652,
    25.561873, 25.604057, 25.724794, 25.846084, 26.012188, 26.283265, 26.516603,
    26.552589, 26.666054, 26.735177, 26.758685, 26.837518
  };

  size_t lvalues[] =
  {
    1, 2, 0, 3, 4, 1, 5, 2, 0, 6, 3, 7, 1, 4, 8, 2, 0, 5, 9, 3, 10, 6, 1, 11, 4, 7,
    2, 0, 12, 5, 8, 3, 13, 1, 9, 6, 14, 4, 10, 2, 7, 0, 15, 5, 11, 8, 16, 3, 1, 6, 12,
    17, 9, 4, 2, 0, 13, 18, 7, 10, 5, 14, 19, 3, 8, 1, 11, 6, 20, 15, 4, 9, 12, 2, 0,
    21, 16, 7, 10, 13, 5, 22, 3, 17, 1, 8, 14, 11, 23, 6, 18, 4, 9, 2, 0, 15, 24, 12
  };

  size_t nvalues[] =
  {
    0, 0, 1, 0, 0, 1, 0, 1, 2, 0, 1, 0, 2, 1, 0, 2, 3, 1, 0, 2, 0, 1, 3, 0, 2, 1, 3,
    4, 0, 2, 1, 3, 0, 4, 1, 2, 0, 3, 1, 4, 2, 5, 0, 3, 1, 2, 0, 4, 5, 3, 1, 0, 2, 4, 5,
    6, 1, 0, 3, 2, 4, 1, 0, 5, 3, 6, 2, 4, 0, 1, 5, 3, 2, 6, 7, 0, 1, 4, 3, 2, 5, 0,
    6, 1, 7, 4, 2, 3, 0, 5, 1, 6, 4, 7, 8, 2, 0, 3
  };

  for( size_t i = 0;  i < ncoeff;  i += 3 )
  {
    xnlc coeff;
    coeff.x = xvalues[ i ];
    coeff.l = lvalues[ i ];
    coeff.n = nvalues[ i ];
    m_xnl.push_back( coeff );
  }
}

//initialize a set of coefficients that will remain constant during fitting
void InelasticDiffSphere::initAlphaCoeff(){
  for( std::vector< xnlc >::const_iterator it = m_xnl.begin();  it != m_xnl.end();  ++it )
  {
    double x = it->x;   // eigenvalue for a (n, l) pair
    double l = ( double )( it->l );
    m_alpha.push_back( ( 2.0 * l + 1 ) * ( 6.0 * x*x / ( x*x - l*(l + 1) ) ) / M_PI );
  }
}

/* Factor "J" is defined as [Q*a*j(l+1,Q*a) - l*j(l,Q*a)] / [(Q*a)^2 - x^2]
 * Both numerator and denominator goes to zero when Q*a approaches x, giving rise to
 * numerical indeterminacies. To avoid them, we will interpolate linearly.
 */
void InelasticDiffSphere::initLinJlist()
{
  for( size_t i = 0;  i < m_xnl.size();  i++ )
  {
    linearJ abJ;
    double x = m_xnl[ i ].x;  // eigenvalue for a (n, l) pair
    unsigned int l = ( unsigned int )( m_xnl[ i ].l );
    double Qa = x - m_divZone; //left of the numerical divergence point
    double J0 = ( Qa * boost::math::sph_bessel( l + 1, Qa ) - l * boost::math::sph_bessel( l, Qa ) ) / ( Qa*Qa - x*x );
    Qa = x + m_divZone; //right of the numerical divergence point
    double J1 = ( Qa * boost::math::sph_bessel( l + 1, Qa ) - l * boost::math::sph_bessel( l, Qa ) ) / ( Qa*Qa - x*x );
    abJ.slope = ( J1 - J0 ) / ( 2 * m_divZone );  //slope of the linear interpolation
    abJ.intercept = J0 - abJ.slope * ( x - m_divZone ); //intercept of the linear interpolation
    m_linearJlist.push_back( abJ ); //store the parameters of the linear interpolation for this it->x
  }
}

InelasticDiffSphere::InelasticDiffSphere() : lmax( 24 ), m_divZone( 0.1 )
{
  declareParameter( "Intensity", 1.0, "scaling factor" );
  declareParameter( "Radius", 1.0, "Sphere radius" );
  declareParameter( "Diffusion", 1.0, "Diffusion coefficient, in units of" );

  declareAttribute( "Q", API::IFunction::Attribute( 1.0 ) );
}

void InelasticDiffSphere::init()
{
  // Ensure positive values for Intensity, Radius, and Diffusion coefficient
  BoundaryConstraint* IntensityConstraint = new BoundaryConstraint( this, "Intensity", 0, true );
  addConstraint(IntensityConstraint);

  BoundaryConstraint* RadiusConstraint = new BoundaryConstraint( this, "Radius", 0, true );
  addConstraint( RadiusConstraint );

  BoundaryConstraint* DiffusionConstraint = new BoundaryConstraint( this, "Diffusion", 0, true );
  addConstraint( DiffusionConstraint );

  initXnlCoeff();   // initialize m_xnl with the list of coefficients xnlist
  initAlphaCoeff(); // initialize m_alpha, certain factors constant over the fit
  initLinJlist();   // initialize m_linearJlist, linear interpolation around numerical divergence
}

//calculate the coefficients for each Lorentzian
std::vector< double > InelasticDiffSphere::LorentzianCoefficients( double a ) const
{
  //precompute the 2+lmax spherical bessel functions (26 in total)
  std::vector< double > jl( 2 + lmax );
  for( size_t l = 0;  l < 2 + lmax;  l++ )
  {
    jl[ l ] = boost::math::sph_bessel( ( unsigned int )( l ), a );
  }

  //store the coefficient of each Lorentzian in vector YJ(a,w)
  size_t ncoeff = m_xnl.size();
  std::vector< double > YJ( ncoeff );

  for( size_t i = 0;  i < ncoeff;  i++ )
  {
    double x  = m_xnl[ i ].x;
    unsigned int l  = ( unsigned int )( m_xnl[ i ].l );
    //compute  factors Y and J
    double Y = m_alpha[ i ]; //Y is independent of parameters a and w, and independent of data x
    double J;
    if( fabs( a - x ) > m_divZone )
    {
      J = ( a * jl[l + 1] - l * jl[l] ) / ( a*a - x*x );
    }
    else
    {
      J = m_linearJlist[ i ].slope * a + m_linearJlist[ i ].intercept; // linear interpolation instead
    }
    YJ[ i ] = Y * J * J;
  }

  return YJ;
} // end of LorentzianCoefficients


void InelasticDiffSphere::function1D( double* out, const double* xValues, const size_t nData ) const
{
  const double I = getParameter( "Intensity" );
  const double R = getParameter( "Radius" );
  const double D = getParameter( "Diffusion" );
  const double Q = getAttribute( "Q" ).asDouble();

  std::vector<double> YJ;
  YJ = LorentzianCoefficients( Q * R );
  for (size_t i = 0;  i < nData;  i++)
  {
    double x = xValues[ i ];
    out[ i ] = 0.0;
    size_t ncoeff = m_xnl.size();
    for ( size_t n = 0;  n < ncoeff;  n++ )
    {
      double z = m_xnl[ n ].x;  // eigenvalue
      double zw = z * z * D / ( R * R );   // HWHM
      double L = zw / ( zw * zw + x * x ); //Lorentzian
      out[i] += I * YJ[ n ] * L;
    }
  }
}

/** Calculate numerical derivatives.
 * @param domain :: The domain of the function
 * @param jacobian :: A Jacobian matrix. It is expected to have dimensions of domain.size() by nParams().

void InelasticDiffSphere::calNumericalDeriv2( const API::FunctionDomain& domain, API::Jacobian& jacobian )
{
  const double minDouble = std::numeric_limits< double >::min();
  const double epsilon = std::numeric_limits< double >::epsilon() * 100;
  double stepPercentage = 0.001; // step percentage
  double step; // real step
  double cutoff = 100.0 * minDouble / stepPercentage;
  size_t nParam = nParams();
  size_t nData = domain.size();

  FunctionValues minusStep( domain );
  FunctionValues plusStep( domain );

  //PARALLEL_CRITICAL(numeric_deriv)
  {
    applyTies(); // just in case
    function( domain, minusStep );
  }

  for (size_t iP = 0; iP < nParam; iP++ )
  {
    if ( isActive( iP ) )
    {
      const double val = activeParameter( iP );
      if ( fabs( val ) < cutoff )
      {
        step = epsilon;
      }
      else
      {
        step = val * stepPercentage;
      }

      double paramPstep = val + step;
      //PARALLEL_CRITICAL(numeric_deriv)
      {
        setActiveParameter( iP, paramPstep );
        applyTies();
        function( domain, plusStep );
        setActiveParameter( iP, val );
      }

      step = paramPstep - val;
      for ( size_t i = 0;  i < nData;  i++ )
      {
        jacobian.set( i, iP, ( plusStep.getCalculated( i ) - minusStep.getCalculated( i ) ) / step );
      }
    }
  }
} // calNumericalDeriv()
*/

/*
/// Using numerical derivative
void InelasticDiffSphere::functionDeriv( const API::FunctionDomain& domain, API::Jacobian& jacobian )
{
  this->calNumericalDeriv( domain, jacobian );
  return;
}
*/

/*
/// Using numerical derivative
void InelasticDiffSphere::functionDeriv1D( Jacobian* jacobian, const double* xValues, const size_t nData )
{
  FunctionDomain1DView domain( xValues, nData );
  this->calNumericalDeriv( domain, *jacobian );
}
*/

/* Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 */
void DiffSphere::trickleDownAttribute( const std::string& name )
{
  for( size_t iFun = 0;  iFun < nFunctions();  iFun++ )
  {
    IFunction_sptr fun = getFunction( iFun );
    if( fun->hasAttribute( name ) )
    {
      fun->setAttribute( name, this->getAttribute( name ) );
    }
  }
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffSphere::declareAttribute( const std::string & name, const API::IFunction::Attribute & defaultValue )
{
  API::ImmutableCompositeFunction::declareAttribute( name, defaultValue );
  trickleDownAttribute( name );
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffSphere::setAttribute( const std::string & name, const Attribute & att )
{
  API::ImmutableCompositeFunction::setAttribute( name, att );
  trickleDownAttribute( name );
}

void DiffSphere::init()
{
  m_elastic = boost::dynamic_pointer_cast< ElasticDiffSphere >( API::FunctionFactory::Instance().createFunction( "ElasticDiffSphere" ) );
  addFunction( m_elastic );
  m_inelastic = boost::dynamic_pointer_cast< InelasticDiffSphere >( API::FunctionFactory::Instance().createFunction( "InelasticDiffSphere" ) );
  addFunction( m_inelastic );

  this->setAttributeValue( "NumDeriv", true );

  this->declareAttribute( "Q", API::IFunction::Attribute( 1.0 ) );

  //Set the aliases
  this->setAlias( "f0.Height", "elasticHeight" );
  this->setAlias( "f0.Radius", "elasticRadius" );
  this->setAlias( "f1.Intensity", "Intensity" );
  this->setAlias( "f1.Radius", "Radius" );
  this->setAlias( "f1.Diffusion", "Diffusion" );

  //Set the ties between Elastic and Inelastic parameters
  this->addDefaultTies( "f0.Height=f1.Intensity,f0.Radius=f1.Radius" );
  this->applyTies();
}

} // namespace CurveFitting
} // namespace Mantid
