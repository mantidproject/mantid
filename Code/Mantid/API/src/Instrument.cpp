#include "MantidAPI/Instrument.h"
#include "V3D.h"
#include "MantidKernel/Exception.h"
#include <algorithm>

namespace Mantid
{
namespace API
{

Kernel::Logger& Instrument::g_log = Kernel::Logger::get("Instrument");

/// Default constructor
Instrument::Instrument() : Geometry::CompAssembly(),
                           _sourceCache(0),_samplePosCache(0)
{}

/// Constructor with name
Instrument::Instrument(const std::string& name) : Geometry::CompAssembly(name),
                                                  _sourceCache(0),_samplePosCache(0)
{}

/**	Gets a pointer to the source
* @returns a pointer to the source
*/
Geometry::ObjComponent* Instrument::getSource()
{
  if ( !_sourceCache )
    g_log.warning("In Instrument::getSource(). No source has been set.");
	return _sourceCache;
}

/**	Gets a pointer to the Sample Position
* @returns a pointer to the Sample Position
*/
Geometry::ObjComponent* Instrument::getSamplePos()
{
  if ( !_samplePosCache )
    g_log.warning("In Instrument::getSamplePos(). No SamplePos has been set.");
	return _samplePosCache;
}

/**	Gets a pointer to the requested detector
 *  @param detector_id the requested detector ID
 *  @returns a pointer to the Assembly of detectors
 *  @throw NotFoundError If the detector ID is not found
 */
Geometry::IDetector* Instrument::getDetector(const int &detector_id)
{
  std::map<int, Geometry::IDetector*>::iterator it;

  it = _detectorCache.find(detector_id);
  
	if ( it == _detectorCache.end() )	
  {
		throw Kernel::Exception::NotFoundError("Instrument: Detector is not found.","");
  }

	return it->second;
}

/** Get the L2 and TwoTheta for the given detector
 *  @param detector_id   The detector ID
 *  @param l2            Returns the sample-detector distance
 *  @param twoTheta      Returns the scattering angle for the given detector
 *  @throw NotFoundError If the detector ID is not found
 */
void Instrument::detectorLocation(const int &detector_id, double &l2, double &twoTheta)
{
  Geometry::V3D detectorPosition = getDetector(detector_id)->getPos();
  Geometry::V3D samplePosition = getSamplePos()->getPos();
  l2 = detectorPosition.distance(samplePosition);
  twoTheta = detectorPosition.zenith(samplePosition);
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

/** Mark a Component which has already been added to the Instrument class
* to be 'the' samplePos Component. For now it is assumed that we have
* at most one of these.
*
* @param comp Component to be marked (stored for later retrievel) as a "SamplePos" Component
*/
void Instrument::markAsSamplePos(Geometry::ObjComponent* comp)
{
  if ( !_samplePosCache )
    _samplePosCache = comp;
  else
    g_log.warning("Have already added samplePos component to the _samplePosCache.");
}

/** Mark a Component which has already been added to the Instrument class
* to be 'the' source Component. For now it is assumed that we have
* at most one of these.
*
* @param comp Component to be marked (stored for later retrievel) as a "source" Component
*/
void Instrument::markAsSource(Geometry::ObjComponent* comp)
{
  if ( !_sourceCache )
    _sourceCache = comp;
  else
    g_log.warning("Have already added source component to the _sourceCache.");
}

/** Mark a Component which has already been added to the Instrument class
* to be a Detector component. Add it to a detector cache for possible
* later retrievel
*
* @param det Component to be marked (stored for later retrievel) as a detector Component
*/
void Instrument::markAsDetector(Geometry::Detector* det)
{
  if ( !_detectorCache.insert( std::map<int, Geometry::Detector*>::value_type(det->getID(), det) ).second )
    g_log.warning("Not successful in adding Detector to _detectorCache.");
}


} // namespace API
} // Namespace Mantid
