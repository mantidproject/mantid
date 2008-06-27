//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/ObjComponent.h"

namespace Mantid
{
namespace Geometry
{

/** Constructor
 *  @param name   The name of the component
 *  @param parent The Parent geometry object of this component
 */
ObjComponent::ObjComponent(const std::string& name, Component* parent) :
  Component(name,parent), obj(0)
{
}

/** Constructor
 *  @param name   The name of the component
 *  @param shape  A pointer to the object describing the shape of this component
 *  @param parent The Parent geometry object of this component
 */
ObjComponent::ObjComponent(const std::string& name, Object* shape, Component* parent) :
  Component(name,parent), obj(shape)
{
}

/// Copy constructor
ObjComponent::ObjComponent(const ObjComponent& rhs) :
  Component(rhs), obj(rhs.obj)
{
}

/// Destructor
ObjComponent::~ObjComponent()
{
}

} // namespace Geometry
} // namespace Mantid
