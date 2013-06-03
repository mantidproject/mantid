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

/* Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 */
void DiffSphere::trickleDownAttribute( const std::string& name )
{
  for(size_t iFun=0;iFun<nFunctions();iFun++)
  {
    IFunction_sptr fun = getFunction(iFun);
    if( fun->hasAttribute( name ) )
      fun->setAttribute( name, this->getAttribute(name) );
  }
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffSphere::declareAttribute(const std::string & name,const API::IFunction::Attribute & defaultValue)
{
  API::ImmutableCompositeFunction::declareAttribute(name, defaultValue);
  trickleDownAttribute( name );
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffSphere::setAttribute(const std::string& name,const Attribute& att)
{
  API::ImmutableCompositeFunction::setAttribute( name, att );
  trickleDownAttribute( name );
}

DiffSphere::DiffSphere()
{
  m_elastic = boost::dynamic_pointer_cast<ElasticDiffSphere>(API::FunctionFactory::Instance().createFunction("ElasticDiffSphere"));
  addFunction( m_elastic );
  m_inelastic = boost::dynamic_pointer_cast<InelasticDiffSphere>(API::FunctionFactory::Instance().createFunction("InelasticDiffSphere"));
  addFunction( m_inelastic );

  this->setAttributeValue("NumDeriv", true );

  this->declareAttribute("Q", API::IFunction::Attribute(1.0) );

  //Set the aliases
  this->setAlias("f0.Height","elasticHeight");
  this->setAlias("f0.Radius","elasticRadius");
  this->setAlias("f1.Intensity","Intensity");
  this->setAlias("f1.Radius","Radius");
  this->setAlias("f1.Diffusion","Diffusion");

  //Set the ties between Elastic and Inelastic parameters
  this->addDefaultTies("f0.Height=f1.Intensity,f0.Radius=f1.Radius");
  this->applyTies();
}

} // namespace CurveFitting
} // namespace Mantid
