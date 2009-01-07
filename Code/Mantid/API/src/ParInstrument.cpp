#include "MantidAPI/ParInstrument.h"
#include "MantidAPI/Instrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/DetectorGroup.h"
#include "MantidGeometry/ParObjComponent.h"
#include "MantidGeometry/ParDetector.h"
#include <algorithm>
#include <iostream>

namespace Mantid
{
namespace API
{

Kernel::Logger& ParInstrument::g_log = Kernel::Logger::get("ParInstrument");

/// Constructor with name
ParInstrument::ParInstrument(const boost::shared_ptr<Instrument> instr, const boost::shared_ptr<Geometry::ParameterMap> map)
:Geometry::ParCompAssembly(instr.get(),map.get()),m_instr(instr),m_parmap(map)
{}

  
/**	return reference to detector cache 
* @returns a reference to the detector cache hold by the ParInstrument
*/
std::map<int,  boost::shared_ptr<Geometry::IDetector> > ParInstrument::getDetectors()
{ 
    std::map<int,  boost::shared_ptr<Geometry::IDetector> > res,dets = m_instr->getDetectors();
    for(std::map<int,  boost::shared_ptr<Geometry::IDetector> >::iterator it=dets.begin();it!=dets.end();it++)
        res.insert(std::pair<int,  boost::shared_ptr<Geometry::IDetector> >
        (it->first,boost::shared_ptr<Geometry::IDetector>(  new Geometry::ParDetector( dynamic_cast<Geometry::Detector*>(it->second.get()), m_parmap.get()  ))));
    return res;
}

/**	Gets a pointer to the source
* @returns a pointer to the source
*/
boost::shared_ptr<Geometry::IObjComponent> ParInstrument::getSource() const
{
    return boost::shared_ptr<Geometry::IObjComponent>(new Geometry::ParObjComponent(dynamic_cast<const Instrument*>(m_base)->_sourceCache,m_map));
}

/**	Gets a pointer to the Sample Position
* @returns a pointer to the Sample Position
*/
boost::shared_ptr<Geometry::IObjComponent> ParInstrument::getSample() const
{
    return boost::shared_ptr<Geometry::IObjComponent>(new Geometry::ParObjComponent(dynamic_cast<const Instrument*>(m_base)->_sampleCache,m_map));
}

/**	Gets a pointer to the detector from its ID
 *  Note that for getting the detector associated with a spectrum, the SpectraDetectorMap::getDetector
 *  method should be used rather than this one because it takes account of the possibility of more
 *  than one detector contibuting to a single spectrum
 *  @param   detector_id The requested detector ID
 *  @returns A pointer to the detector object
 *  @throw   NotFoundError If no detector is found for the detector ID given
 */
boost::shared_ptr<Geometry::IDetector> ParInstrument::getDetector(const int &detector_id) const
{
    boost::shared_ptr<Geometry::Detector> det = boost::dynamic_pointer_cast<Geometry::Detector>(dynamic_cast<const Instrument*>(m_base)->getDetector(detector_id));
    return boost::shared_ptr<Geometry::IDetector>(new Geometry::ParDetector(det.get(),m_map));
}

/** Returns the 2Theta scattering angle for a detector
 *  @param det A pointer to the detector object (N.B. might be a DetectorGroup)
 *  @return The scattering angle (0 < theta < pi)
 */
const double ParInstrument::detectorTwoTheta(const boost::shared_ptr<Geometry::IDetector> det) const
{
    const Geometry::V3D samplePos = this->getSample()->getPos();
  const Geometry::V3D beamLine = samplePos - this->getSource()->getPos();
  const Geometry::V3D sampleDetVec = det->getPos() - samplePos;
  return sampleDetVec.angle(beamLine);
}

std::vector< boost::shared_ptr<Geometry::IObjComponent> > ParInstrument::getPlottable()const
{
    std::vector< boost::shared_ptr<Geometry::IObjComponent> > res;
    std::vector< boost::shared_ptr<Geometry::IObjComponent> > objs = dynamic_cast<const Instrument*>(m_base)->getPlottable();
    for(std::vector< boost::shared_ptr<Geometry::IObjComponent> >::iterator it = objs.begin();it!= objs.end();it++)
    {
        if ( dynamic_cast<Geometry::Detector*>(it->get()) )
        res.push_back(boost::shared_ptr<Geometry::IObjComponent>(new Geometry::ParDetector(dynamic_cast<Geometry::Detector*>(it->get()),m_map)));
        else
        res.push_back(boost::shared_ptr<Geometry::IObjComponent>(new Geometry::ParObjComponent(dynamic_cast<Geometry::ObjComponent*>(it->get()),m_map)));
    }
    return res;
}


} // namespace API
} // Namespace Mantid
