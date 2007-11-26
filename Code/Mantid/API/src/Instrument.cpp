#include "MantidAPI/Instrument.h"
#include "MantidKernel/Exception.h"
#include <algorithm>

namespace Mantid
{
namespace API
{

Kernel::Logger& Instrument::g_log = Kernel::Logger::get("Instrument");

/// Default constructor
Instrument::Instrument() : Geometry::CompAssembly(),_detectorsCacheValue(0)
{}

/// Constructor with name
Instrument::Instrument(const std::string& name) : Geometry::CompAssembly(name),_detectorsCacheValue(0)
{}

/// Copy constructor
Instrument::Instrument(const Instrument& A)  :
     Geometry::CompAssembly(A)
{}

/**	Gets a pointer to the source
* @returns a pointer to the source
*/
Geometry::ObjComponent* Instrument::getSource()
{
	Geometry::Component* ptrChild = getChild("Source");
	Geometry::ObjComponent *retVal = dynamic_cast<Geometry::ObjComponent*>(ptrChild);
	if (!retVal) throw std::bad_cast();
	return retVal;
}

/**	Gets a pointer to the Sample Position
* @returns a pointer to the Sample Position
*/
Geometry::ObjComponent* Instrument::getSamplePos()
{
	Geometry::Component* ptrChild = getChild("SamplePos");
	Geometry::ObjComponent *retVal = dynamic_cast<Geometry::ObjComponent*>(ptrChild);
	if (!retVal) throw std::bad_cast();
	return retVal;
}

/**	Gets a pointer to the Assembly of detectors
* @returns a pointer to the Assembly of detectors
*/
Geometry::CompAssembly* Instrument::getDetectors()
{
	if (!_detectorsCacheValue)
	{
		Geometry::Component* ptrChild = getChild("Detectors");
		_detectorsCacheValue = dynamic_cast<Geometry::CompAssembly*>(ptrChild);
	}

	Geometry::CompAssembly *retVal = _detectorsCacheValue;

	if (!retVal) throw std::bad_cast();
	return retVal;
}

/**	Gets a pointer to the requested detector
* @param detector_id the requested detector Id
* @returns a pointer to the Assembly of detectors
*/
Geometry::Detector* Instrument::getDetector(const int &detector_id)
{
	Geometry::CompAssembly *ptrDetectors = getDetectors();
	Geometry::Detector *retVal = 0;
	
	int noOfChildren = ptrDetectors->nelements();
	for (int i = 0; i < noOfChildren; i++)
	{
		Geometry::Component *loopPtr = (*ptrDetectors)[i];
		Geometry::Detector *ptrDetector = dynamic_cast<Geometry::Detector*>(loopPtr);
	
		if ((ptrDetector) && (ptrDetector->getID() == detector_id))
		{
			retVal = ptrDetector;
			break;
		}
	}
		
	if (!retVal)
	{
		throw Kernel::Exception::NotFoundError("Instrument: Detector is not found.","");
	}
	
	return retVal;
}

/**	Gets a pointer to the requested child component
* @param name the name of the object requested (case insensitive)
* @returns a pointer to the component
*/
Geometry::Component* Instrument::getChild(const std::string& name)
{
	Geometry::Component *retVal = 0;
	std::string searchName = name;
	std::transform(searchName.begin(), searchName.end(), searchName.begin(), toupper);

	int noOfChildren = this->nelements();
	for (int i = 0; i < noOfChildren; i++)
	{
		Geometry::Component *loopPtr = (*this)[i];
		std::string loopName = loopPtr->getName();
		std::transform(loopName.begin(), loopName.end(), loopName.begin(), toupper);
		if (loopName == searchName)
		{
			retVal = loopPtr;
		}
	}

	if (!retVal)
	{
		throw Kernel::Exception::NotFoundError("Instrument: Child "+ name + " is not found.",name);
	}
	
	return retVal;
}
	
} // namespace API
} // Namespace Mantid
