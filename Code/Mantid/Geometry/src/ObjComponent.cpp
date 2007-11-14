#include "ObjComponent.h"

namespace Mantid
{
namespace Geometry
{
ObjComponent::ObjComponent():Component()
{
}
ObjComponent::ObjComponent(const std::string& n, Component* reference):Component(n,reference)
{
}

ObjComponent::~ObjComponent()
{
}

} // Namespace Geometry
} // Namespace Mantid
