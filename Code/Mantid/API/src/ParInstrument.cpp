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
std::map<int, Geometry::IDetector_sptr> ParInstrument::getDetectors() const
{ 
  std::map<int, Geometry::IDetector_sptr> res,dets = m_instr->getDetectors();
  for(std::map<int, Geometry::IDetector_sptr>::const_iterator it=dets.begin();it!=dets.end();it++)
    res.insert(std::pair<int, Geometry::IDetector_sptr>
    (it->first,Geometry::IDetector_sptr( new Geometry::ParDetector( dynamic_cast<Geometry::Detector*>(it->second.get()), m_parmap.get()  ))));
  return res;
}

/**	Gets a pointer to the source
* @returns a pointer to the source
*/
Geometry::IObjComponent_sptr ParInstrument::getSource() const
{
  return Geometry::IObjComponent_sptr(new Geometry::ParObjComponent(dynamic_cast<const Instrument*>(m_base)->_sourceCache,m_map));
}

/**	Gets a pointer to the Sample Position
* @returns a pointer to the Sample Position
*/
Geometry::IObjComponent_sptr ParInstrument::getSample() const
{
  return Geometry::IObjComponent_sptr(new Geometry::ParObjComponent(dynamic_cast<const Instrument*>(m_base)->_sampleCache,m_map));
}

/**  Get a shared pointer to a component by its ID
 *   @param id ID
 *   @return A pointer to the component.
 */
boost::shared_ptr<Geometry::IComponent> ParInstrument::getComponentByID(Geometry::ComponentID id)
{
    Geometry::IComponent* base = (Geometry::IComponent*)(id);
    Geometry::Detector* dc = dynamic_cast<Geometry::Detector*>(base);
    Geometry::CompAssembly* ac = dynamic_cast<Geometry::CompAssembly*>(base);
    Geometry::ObjComponent* oc = dynamic_cast<Geometry::ObjComponent*>(base);
    Geometry::Component* cc = dynamic_cast<Geometry::Component*>(base);
    if (dc)
      return boost::shared_ptr<Geometry::IComponent>(new Geometry::ParDetector(dc,m_map));
    else if (ac)
      return boost::shared_ptr<Geometry::IComponent>(new Geometry::ParCompAssembly(ac,m_map));
    else if (oc)
      return boost::shared_ptr<Geometry::IComponent>(new Geometry::ParObjComponent(oc,m_map));
    else if (cc)
      return boost::shared_ptr<Geometry::IComponent>(new Geometry::ParametrizedComponent(cc,m_map));
    else
    {
      throw std::runtime_error("ParInstrument::getComponentByID: Error creating parametruzed component.");
    }
    return  boost::shared_ptr<Geometry::IComponent>() ;
}

/**	Gets a pointer to the detector from its ID
 *  Note that for getting the detector associated with a spectrum, the SpectraDetectorMap::getDetector
 *  method should be used rather than this one because it takes account of the possibility of more
 *  than one detector contibuting to a single spectrum
 *  @param   detector_id The requested detector ID
 *  @returns A pointer to the detector object
 *  @throw   NotFoundError If no detector is found for the detector ID given
 */
Geometry::IDetector_sptr ParInstrument::getDetector(const int &detector_id) const
{
  boost::shared_ptr<Geometry::Detector> det = boost::dynamic_pointer_cast<Geometry::Detector>(dynamic_cast<const Instrument*>(m_base)->getDetector(detector_id));
  return Geometry::IDetector_sptr(new Geometry::ParDetector(det.get(),m_map));
}

std::vector<Geometry::IObjComponent_sptr> ParInstrument::getPlottable() const
{
  std::vector<Geometry::IObjComponent_sptr> res;
  std::vector<Geometry::IObjComponent_sptr> objs = dynamic_cast<const Instrument*>(m_base)->getPlottable();
  for(std::vector<Geometry::IObjComponent_sptr>::const_iterator it = objs.begin();it!= objs.end();it++)
  {
    if ( dynamic_cast<Geometry::Detector*>(it->get()) )
      res.push_back(Geometry::IObjComponent_sptr(new Geometry::ParDetector(dynamic_cast<Geometry::Detector*>(it->get()),m_map)));
    else
      res.push_back(Geometry::IObjComponent_sptr(new Geometry::ParObjComponent(dynamic_cast<Geometry::ObjComponent*>(it->get()),m_map)));
  }
  return res;
}


} // namespace API
} // Namespace Mantid
