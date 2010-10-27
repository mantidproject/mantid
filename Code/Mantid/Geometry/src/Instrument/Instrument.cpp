#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include <algorithm>
#include <iostream>

namespace Mantid
{
  namespace Geometry
  {

    Kernel::Logger& Instrument::g_log = Kernel::Logger::get("Instrument");

    /// Default constructor
    Instrument::Instrument() : Geometry::CompAssembly(),
      _detectorCache(),_sourceCache(0),_sampleCache(0),
      m_defaultViewAxis("Z+")
    {}

    /// Constructor with name
    Instrument::Instrument(const std::string& name) : Geometry::CompAssembly(name),
      _detectorCache(),_sourceCache(0),_sampleCache(0),
      m_defaultViewAxis("Z+")
    {}


    /**	return reference to detector cache 
    * @returns a map of the detectors hold by the instrument
    */
    std::map<int, Geometry::IDetector_sptr> Instrument::getDetectors() const
    { 
      std::map<int, Geometry::IDetector_sptr> res;
      for(std::map<int, Geometry::IDetector*>::const_iterator it=_detectorCache.begin();it!=_detectorCache.end();it++)
        res.insert(std::pair<int, Geometry::IDetector_sptr>(it->first,Geometry::IDetector_sptr(it->second,NoDeleting())));
      return res;
    }

    /**	Gets a pointer to the source
    * @returns a pointer to the source
    */
    Geometry::IObjComponent_sptr Instrument::getSource() const
    {
      if ( !_sourceCache )
        g_log.warning("In Instrument::getSource(). No source has been set.");
      return boost::shared_ptr<Geometry::IObjComponent>(_sourceCache,NoDeleting());
    }

    /**	Gets a pointer to the Sample Position
    * @returns a pointer to the Sample Position
    */
    Geometry::IObjComponent_sptr Instrument::getSample() const
    {
      if ( !_sampleCache )
        g_log.warning("In Instrument::getSamplePos(). No SamplePos has been set.");
      return boost::shared_ptr<Geometry::IObjComponent>(_sampleCache,NoDeleting());
    }

    /**  Get a shared pointer to a component by its ID
    *   @param id ID
    *   @return A pointer to the component.
    */
    boost::shared_ptr<Geometry::IComponent> Instrument::getComponentByID(Geometry::ComponentID id)
    {
      return boost::shared_ptr<Geometry::IComponent>((Geometry::IComponent*)id,NoDeleting());
    }

    /**	Gets a pointer to the detector from its ID
    *  Note that for getting the detector associated with a spectrum, the SpectraDetectorMap::getDetector
    *  method should be used rather than this one because it takes account of the possibility of more
    *  than one detector contibuting to a single spectrum
    *  @param   detector_id The requested detector ID
    *  @returns A pointer to the detector object
    *  @throw   NotFoundError If no detector is found for the detector ID given
    */
    Geometry::IDetector_sptr Instrument::getDetector(const int &detector_id) const
    {
      std::map<int, Geometry::IDetector*>::const_iterator it;

      it = _detectorCache.find(detector_id);

      if ( it == _detectorCache.end() )
      {
        g_log.debug() << "Detector with ID " << detector_id << " not found." << std::endl;
        std::stringstream readInt;
        readInt << detector_id;
        throw Kernel::Exception::NotFoundError("Instrument: Detector with ID " + readInt.str() + " not found.","");
      }

      return Geometry::IDetector_sptr(it->second,NoDeleting());
    }

    /**	Gets a pointer to the monitor from its ID
    *  @param   detector_id The requested detector ID
    *  @return A pointer to the detector object
    *  @throw   NotFoundError If no monitor is found for the detector ID given
    */
    Geometry::IDetector_sptr Instrument::getMonitor(const int &detector_id)const
    {
      std::vector<int>::const_iterator itr;
      itr=find(m_monitorCache.begin(),m_monitorCache.end(),detector_id);
      if ( itr == m_monitorCache.end() )
      {
        g_log.debug() << "monitor with ID " << detector_id << " not found." << std::endl;
        std::stringstream readInt;
        readInt << detector_id;
        throw Kernel::Exception::NotFoundError("Instrument: Detector with ID " + readInt.str() + " not found.","");
      }
      Geometry::IDetector_sptr monitor=getDetector(detector_id);

      return monitor;
    }
    /**	Gets a pointer to the requested child component
    * @param name the name of the object requested (case insensitive)
    * @returns a pointer to the component
    */
    Geometry::IComponent* Instrument::getChild(const std::string& name) const
    {
      Geometry::IComponent *retVal = 0;
      std::string searchName = name;
      std::transform(searchName.begin(), searchName.end(), searchName.begin(), toupper);

      int noOfChildren = this->nelements();
      for (int i = 0; i < noOfChildren; i++)
      {
        Geometry::IComponent *loopPtr = (*this)[i].get();
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

    /** Mark a Component which has already been added to the Instrument (as a child component)
    * to be 'the' samplePos Component. For now it is assumed that we have
    * at most one of these.
    *
    * @param comp Component to be marked (stored for later retrievel) as a "SamplePos" Component
    */
    void Instrument::markAsSamplePos(Geometry::ObjComponent* comp)
    {
      if ( !_sampleCache )
        _sampleCache = comp;
      else
        g_log.warning("Have already added samplePos component to the _sampleCache.");
    }

    /** Mark a Component which has already been added to the Instrument (as a child component)
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

    /** Mark a Component which has already been added to the Instrument (as a child component)
    * to be a Detector by adding it to a detector cache.
    *
    * @param det Component to be marked (stored for later retrievel) as a detector Component
    *
    */
    void Instrument::markAsDetector(Geometry::IDetector* det)
    {
      if ( !_detectorCache.insert( std::map<int, Geometry::IDetector*>::value_type(det->getID(), det) ).second )
      {
        std::stringstream convert;
        convert << det->getID();
        g_log.error() << "Not successful in adding Detector " << convert << " to _detectorCache." << std::endl;
        //throw Kernel::Exception::ExistsError("Not successful in adding Detector to _detectorCache.", convert.str());
      }
    }

    /** Mark a Component which has already been added to the Instrument class
    * as a monitor and add it to the detector cache.
    *
    * @param det Component to be marked (stored for later retrieval) as a detector Component
    *
    * @throw Exception::ExistsError if cannot add detector to cache
    */
    void Instrument::markAsMonitor(Geometry::IDetector* det)
    {
      // attempt to add monitor to instrument detector cache
      markAsDetector(det);

      // mark detector as a monitor
      Geometry::Detector *d = dynamic_cast<Geometry::Detector*>(det);
      if (d)
      {
        d->markAsMonitor();
        m_monitorCache.push_back(det->getID());
      }
      else
      {
        throw std::invalid_argument("The IDetector pointer does not point to a Detector object");
      }
    }
    /** This method returns monitor detector ids
    *@return a vector holding detector ids of  monitors
    */
    const std::vector<int> Instrument::getMonitors()const
    {
      return m_monitorCache;
    }

   /**
    * Get the bounding box for this instrument. It is simply the sum of the bounding boxes of its children excluding the source
    * @param assemblyBox [Out] The resulting bounding box is stored here.
    */
    void Instrument::getBoundingBox(BoundingBox & assemblyBox) const
    {
      if( !m_cachedBoundingBox )
      {
        m_cachedBoundingBox = new BoundingBox();
        ComponentID sourceID = getSource()->getComponentID();
        // Loop over the children and define a box large enough for all of them
        for (const_comp_it it = m_children.begin(); it != m_children.end(); ++it)
        {
          BoundingBox compBox;
          IComponent *component = *it;
          if(component && component->getComponentID() != sourceID)
          {
            component->getBoundingBox(compBox);
            m_cachedBoundingBox->grow(compBox);
          }
        }
      }
      // Use cached box
      assemblyBox = *m_cachedBoundingBox;
    }


    IInstrument::plottables_const_sptr Instrument::getPlottable() const
    {
      boost::shared_ptr<std::vector<Geometry::IObjComponent_const_sptr> > res(
        new std::vector<Geometry::IObjComponent_const_sptr> );
      res->reserve(_detectorCache.size()+10);
      appendPlottable(*this,*res);
      return res;
    }

    void Instrument::appendPlottable(const Geometry::CompAssembly& ca,std::vector<Geometry::IObjComponent_const_sptr>& lst) const
    {
      for(int i=0;i<ca.nelements();i++)
      {
        Geometry::IComponent* c = ca[i].get();
        Geometry::CompAssembly* a = dynamic_cast<Geometry::CompAssembly*>(c);
        if (a) appendPlottable(*a,lst);
        else
        {
          Geometry::Detector* d = dynamic_cast<Geometry::Detector*>(c);
          Geometry::ObjComponent* o = dynamic_cast<Geometry::ObjComponent*>(c);
          if (d)
            lst.push_back(Geometry::IObjComponent_const_sptr(d,NoDeleting()));
          else if (o)
            lst.push_back(Geometry::IObjComponent_const_sptr(o,NoDeleting()));
          else
            g_log.error()<<"Unknown comp type\n";
        }
      }
    }


  } // namespace Geometry
} // Namespace Mantid
