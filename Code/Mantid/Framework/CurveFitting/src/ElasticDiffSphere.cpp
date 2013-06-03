//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ElasticDiffSphere.h"
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

DECLARE_FUNCTION(ElasticDiffSphere);

ElasticDiffSphere::ElasticDiffSphere(){
  //declareParameter("Height", 1.0); //parameter "Height" already declared in constructor of base class DeltaFunction
  declareParameter("Radius", 1.0, "Sphere radius");

  // Ensure positive values for Height and Radius
  BoundaryConstraint* HeightConstraint = new BoundaryConstraint(this,"Height",0,true);
  addConstraint(HeightConstraint);

  BoundaryConstraint* RadiusConstraint = new BoundaryConstraint(this,"Radius",0,true);
  addConstraint(RadiusConstraint);

  declareAttribute( "Q", API::IFunction::Attribute(1.0) );

}

double ElasticDiffSphere::HeightPrefactor() const{
  const double R = getParameter("Radius");
  const double Q = getAttribute("Q").asDouble();
  return pow(3*boost::math::sph_bessel(1,Q*R)/(Q*R),2);
}

} // namespace CurveFitting
} // namespace Mantid
