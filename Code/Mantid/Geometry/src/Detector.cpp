#include "Detector.h"

namespace Mantid
{
namespace Geometry
{
Detector::Detector():ObjComponent(),id(0)
{
}

Detector::Detector(const std::string& n, Component* reference):ObjComponent(n,reference),id(0)
{
}

Detector::~Detector()
{
}
void Detector::setID(int det_id)
{
	id=det_id;
}
int Detector::getID() const
{
	return id;
}

} //Namespace Geometry
} //Namespace Mantid
