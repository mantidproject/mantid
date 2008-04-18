#include "MantidAPI/Instrument.h"
#include "V3D.h"
#include "MantidKernel/Exception.h"
#include "DetectorGroup.h"
#include <algorithm>

namespace Mantid
{
namespace API
{

Kernel::Logger& Instrument::g_log = Kernel::Logger::get("Instrument");

/// Default constructor
Instrument::Instrument() : Geometry::CompAssembly(),
                           _detectorCache(),_sourceCache(0),_samplePosCache(0)
{}

/// Constructor with name
Instrument::Instrument(const std::string& name) : Geometry::CompAssembly(name),
                           _detectorCache(),_sourceCache(0),_samplePosCache(0)
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

/**	Gets a pointer to the detector related to a particular spectrum
 *  @param   spectrumNo The spectrum number 
 *  @returns A pointer to the detector object
 *  @throw   NotFoundError If no detector is found for the spectrum number given
 */
Geometry::IDetector* Instrument::getDetector(const int &spectrumNo)
{
  /// @todo Sort out mapping so that spectrum number is the key, not detector ID
  // For now, just pretend they're the same
  std::map<int, Geometry::IDetector*>::iterator it;

  it = _detectorCache.find(spectrumNo);
  
	if ( it == _detectorCache.end() )	
  {
		throw Kernel::Exception::NotFoundError("Instrument: Detector is not found.","");
  }

	return it->second;
}

/** Get the L2 and TwoTheta for the given detector
 *  @param spectrumNo    The spectrum number 
 *  @param l2            Returns the sample-detector distance
 *  @param twoTheta      Returns the scattering angle for the given detector
 *  @throw NotFoundError If no detector is found for the spectrum number given or the sample has not been set
 */
void Instrument::detectorLocation(const int &spectrumNo, double &l2, double &twoTheta)
{
  if ( !_samplePosCache ) throw Kernel::Exception::NotFoundError("Instrument: Sample position has not been set","");
  Geometry::V3D detectorPosition = getDetector(spectrumNo)->getPos();
  Geometry::V3D samplePosition = getSamplePos()->getPos();
  l2 = detectorPosition.distance(samplePosition);
  twoTheta = detectorPosition.zenith(samplePosition);
}

/** Group several detector objects in the map into a single DetectorGroup object.
 *  The new grouped object will be be linked to the first spectrum in the argument vector
 *  @param spectra A vector containing the spectrum numbers whose detectors should be grouped
 *  @throw NotFoundError If any spectrum number does not have an associated detector
 */
void Instrument::groupDetectors(const std::vector<int> &spectra)
{
  Geometry::DetectorGroup *group = new Geometry::DetectorGroup;
  
  // Loop over the spectrum numbers
  std::vector<int>::const_iterator it;
  for (it = spectra.begin(); it != spectra.end(); ++it)
  {
    group->addDetector(getDetector(*it));
    // Remove the detector just added from the map
    _detectorCache.erase(*it);
  }
  
  // Add a new entry in the map with the key of the first element
  _detectorCache[spectra[0]] = group;
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
void Instrument::markAsDetector(Geometry::IDetector* det)
{
  if ( !_detectorCache.insert( std::map<int, Geometry::IDetector*>::value_type(det->getID(), det) ).second )
    g_log.warning("Not successful in adding Detector to _detectorCache.");
}


} // namespace API
} // Namespace Mantid
