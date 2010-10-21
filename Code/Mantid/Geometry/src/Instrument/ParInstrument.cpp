#include "MantidGeometry/Instrument/ParInstrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ParObjComponent.h"
#include "MantidGeometry/Instrument/ParDetector.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"

#include <algorithm>

namespace Mantid
{
  namespace Geometry
  {


    Kernel::Logger& ParInstrument::g_log = Kernel::Logger::get("ParInstrument");

    /// Constructor with name
    ParInstrument::ParInstrument(const boost::shared_ptr<Instrument> instr, const Kernel::cow_ptr<Geometry::ParameterMap> map)
      : ParCompAssembly(instr.get(),*map),m_instr(instr),m_parmap(map)
    {}

    /**	return reference to detector cache 
    * @returns a reference to the detector cache hold by the ParInstrument
    */
    std::map<int, Geometry::IDetector_sptr> ParInstrument::getDetectors() const
    { 
      std::map<int, IDetector_sptr> res,dets = m_instr->getDetectors();
      for(std::map<int, IDetector_sptr>::const_iterator it=dets.begin();it!=dets.end();it++)
        res.insert(std::pair<int, IDetector_sptr>
        (it->first,IDetector_sptr( new ParDetector( dynamic_cast<Detector*>(it->second.get()), *m_parmap  ))));
      return res;
    }

    /**	Gets a pointer to the source
    * @returns a pointer to the source
    */
    Geometry::IObjComponent_sptr ParInstrument::getSource() const
    {
      return IObjComponent_sptr(new ParObjComponent(dynamic_cast<const Instrument*>(m_base)->_sourceCache,m_map));
    }

    /**	Gets a pointer to the Sample Position
    * @returns a pointer to the Sample Position
    */
    Geometry::IObjComponent_sptr ParInstrument::getSample() const
    {
      return IObjComponent_sptr(new ParObjComponent(dynamic_cast<const Instrument*>(m_base)->_sampleCache,m_map));
    }

    /**  Get a shared pointer to a component by its ID
    *   @param id ID
    *   @return A pointer to the component.
    */
    boost::shared_ptr<Geometry::IComponent> ParInstrument::getComponentByID(Geometry::ComponentID id)
    {
      IComponent* base = (IComponent*)(id);
      Detector* dc = dynamic_cast<Detector*>(base);
      CompAssembly* ac = dynamic_cast<CompAssembly*>(base);
      ObjComponent* oc = dynamic_cast<ObjComponent*>(base);
      Component* cc = dynamic_cast<Component*>(base);
      if (dc)
        return boost::shared_ptr<IComponent>(new ParDetector(dc,m_map));
      else if (ac)
        return boost::shared_ptr<IComponent>(new ParCompAssembly(ac,m_map));
      else if (oc)
        return boost::shared_ptr<IComponent>(new ParObjComponent(oc,m_map));
      else if (cc)
        return boost::shared_ptr<IComponent>(new ParametrizedComponent(cc,m_map));
      else
      {
        throw std::runtime_error("ParInstrument::getComponentByID: Error creating parametruzed component.");
      }
      return boost::shared_ptr<IComponent>() ;
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
      boost::shared_ptr<Detector> det = boost::dynamic_pointer_cast<Detector>(dynamic_cast<const Instrument*>(m_base)->getDetector(detector_id));
      return IDetector_sptr(new Geometry::ParDetector(det.get(),m_map));
    }

    IInstrument::plottables_const_sptr ParInstrument::getPlottable() const
    {
      // Get the 'base' plottable components
      IInstrument::plottables_const_sptr objs = dynamic_cast<const Instrument*>(m_base)->getPlottable();
      // Get a reference to the underlying vector, casting away the constness so that we
      // can modify it to get our result rather than creating another long vector
      IInstrument::plottables & res = const_cast<IInstrument::plottables&>(*objs);
      const plottables::size_type total = res.size();
      for(plottables::size_type i = 0; i < total; ++i)
      {
        if ( boost::dynamic_pointer_cast<const Detector>(objs->at(i)) )
          res[i] = IObjComponent_const_sptr(new ParDetector(dynamic_cast<const Detector*>(objs->at(i).get()),m_map));
        else
          res[i] = IObjComponent_const_sptr(new ParObjComponent(dynamic_cast<const ObjComponent*>(objs->at(i).get()),m_map));
      }
      return objs;
    }
    /** This method returns monitor detector ids
    * @return a vector holding detector ids of  monitors
    */
    const std::vector<int> ParInstrument::getMonitors()const
    {
      return  m_instr->getMonitors();
    }
    /**	Gets a pointer to the monitor from its ID
    *  @param   detector_id The requested detector ID
    *  @returns A pointer to the detector object
    */
    Geometry::IDetector_sptr ParInstrument::getMonitor(const int &detector_id) const
    {
      boost::shared_ptr<Geometry::Detector> det = boost::dynamic_pointer_cast<Geometry::Detector>(dynamic_cast<const Instrument*>(m_base)->getMonitor(detector_id));
      return Geometry::IDetector_sptr(new Geometry::ParDetector(det.get(),m_map));
    }

  } // namespace Geometry
} // Namespace Mantid
