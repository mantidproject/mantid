#include "Detector.h"

namespace Mantid
{
namespace Geometry
{
  ///Constructor
Detector::Detector():ObjComponent(),id(0)
{
}

/** Constructor
 * @param n The name of the component
 * @param reference the parent component
 */
Detector::Detector(const std::string& n, Component* reference):ObjComponent(n,reference),id(0)
{
}
///Destructor
Detector::~Detector()
{
}
/** Sets the detector id
 * @param det_id the detector id
 */
void Detector::setID(int det_id)
{
	id=det_id;
}
/** Gets the detector id
 * @returns the detector id
 */
int Detector::getID() const
{
	return id;
}

} //Namespace Geometry
} //Namespace Mantid
