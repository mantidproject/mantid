#include "MantidGeometry/ObjComponent.h"

namespace Mantid
{
namespace Geometry
{
/** No arg construtor
*/
ObjComponent::ObjComponent():Component()
{
}
/** Constructor
 * @param n The name of the component
 * @param reference the Parent geometry object of this component
 */
ObjComponent::ObjComponent(const std::string& n, Component* reference):Component(n,reference)
{
}
///Destructor
ObjComponent::~ObjComponent()
{
}

} // Namespace Geometry
} // Namespace Mantid
