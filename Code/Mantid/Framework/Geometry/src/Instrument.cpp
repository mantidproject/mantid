#include "MantidGeometry/Instrument.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace Mantid::Kernel;
using Mantid::Kernel::Exception::NotFoundError;
using Mantid::Kernel::Exception::InstrumentDefinitionError;

namespace Mantid
{
  namespace Geometry
  {

    Kernel::Logger& Instrument::g_log = Kernel::Logger::get("Instrument");

    /// Default constructor
    Instrument::Instrument() : CompAssembly(),
      _detectorCache(),_sourceCache(0),_sampleCache(0),
      m_defaultViewAxis("Z+")
    {}

    /// Constructor with name
    Instrument::Instrument(const std::string& name) : CompAssembly(name),
      _detectorCache(),_sourceCache(0),_sampleCache(0),
      m_defaultViewAxis("Z+")
    {}

    /** Constructor to create a parametrized instrument
     * @param instr :: instrument for parameter inclusion
     * @param map :: parameter map to include
     **/
    Instrument::Instrument(const boost::shared_ptr<Instrument> instr, ParameterMap_sptr map)
    : CompAssembly(instr.get(), map.get() ),
      _sourceCache(instr->_sourceCache), _sampleCache(instr->_sampleCache),
      m_defaultViewAxis(instr->m_defaultViewAxis),
      m_instr(instr), m_map_nonconst(map),
      m_ValidFrom(instr->m_ValidFrom), m_ValidTo(instr->m_ValidTo)
    {
    }


    /// Pointer to the 'real' instrument, for parametrized instruments
    boost::shared_ptr<Instrument> Instrument::baseInstrument() const
    {
      if (m_isParametrized)
        return m_instr;
      else
        throw std::runtime_error("Instrument::baseInstrument() called for a non-parametrized instrument.");
    }

    /**
     * Pointer to the ParameterMap holding the parameters of the modified instrument components.
     * @return parameter map from modified instrument components
     */
    ParameterMap_sptr Instrument::getParameterMap() const
    {
      if (m_isParametrized)
        return m_map_nonconst;
      else
        throw std::runtime_error("Instrument::getParameterMap() called for a non-parametrized instrument.");
    }





    //------------------------------------------------------------------------------------------
    /**	Fills a copy of the detector cache
    * @returns a map of the detectors hold by the instrument
    */
    void Instrument::getDetectors(detid2det_map & out_map) const
    { 
      if (m_isParametrized)
      {
        //Get the base instrument detectors
        out_map.clear();
        const std::map<detid_t, IDetector_sptr> & in_dets = dynamic_cast<const Instrument*>(m_base)->_detectorCache;
        //And turn them into parametrized versions
        for(std::map<detid_t, IDetector_sptr>::const_iterator it=in_dets.begin();it!=in_dets.end();it++)
        {
          out_map.insert(std::pair<detid_t, IDetector_sptr>
          (it->first, ParComponentFactory::createDetector(it->second.get(), m_map)));
        }
      }
      else
      {
        //You can just return the detector cache directly.
        out_map = _detectorCache;
      }
    }


    //------------------------------------------------------------------------------------------
    /** Return a vector of detector IDs in this instrument */
    std::vector<detid_t> Instrument::getDetectorIDs(bool skipMonitors) const
    {
      std::vector<detid_t> out;
      if (m_isParametrized)
      {
        const std::map<detid_t, IDetector_sptr> & in_dets = dynamic_cast<const Instrument*>(m_base)->_detectorCache;
        for(std::map<detid_t, IDetector_sptr>::const_iterator it=in_dets.begin();it!=in_dets.end();it++)
          if (!skipMonitors || !it->second->isMonitor())
            out.push_back(it->first);
      }
      else
      {
        const std::map<detid_t, IDetector_sptr> & in_dets = _detectorCache;
        for(std::map<detid_t, IDetector_sptr>::const_iterator it=in_dets.begin();it!=in_dets.end();it++)
          if (!skipMonitors || !it->second->isMonitor())
            out.push_back(it->first);
      }
      return out;
    }


    //------------------------------------------------------------------------------------------
    /** Fill a vector with all the detectors contained (at any depth) in a named component. For example,
     * you might have a bank10 with 4 tubes with 100 pixels each; this will return the
     * 400 contained Detector objects.
     *
     * @param[out] dets :: vector filled with detector pointers
     * @param bankName :: name of the parent component assembly that contains detectors.
     *        The name must be unique, otherwise the first matching component (getComponentByName)
     *        is used.
     */
    void Instrument::getDetectorsInBank(std::vector<IDetector_const_sptr> & dets, const std::string & bankName) const
    {
      boost::shared_ptr<const IComponent> comp = this->getComponentByName(bankName);
      boost::shared_ptr<const ICompAssembly> bank = boost::dynamic_pointer_cast<const ICompAssembly>(comp);
      if (bank)
      {
        // Get a vector of children (recursively)
        std::vector<boost::shared_ptr<const IComponent> > children;
        bank->getChildren(children, true);
        std::vector<boost::shared_ptr<const IComponent> >::iterator it;
        for (it = children.begin(); it != children.end(); it++)
        {
          IDetector_const_sptr det = boost::dynamic_pointer_cast<const IDetector>(*it);
          if (det)
          {
            dets.push_back( det );
          }
        }
      }
    }


    //------------------------------------------------------------------------------------------
    /** Gets a pointer to the source
     *   @returns a pointer to the source
     */
    IObjComponent_sptr Instrument::getSource() const
    {
      if ( !_sourceCache )
      {
        g_log.warning("In Instrument::getSource(). No source has been set.");
        return boost::shared_ptr<IObjComponent>(_sourceCache,NoDeleting());
      }
      else if (m_isParametrized)
      {
        return IObjComponent_sptr(new ObjComponent(dynamic_cast<const Instrument*>(m_base)->_sourceCache,m_map));
      }
      else
      {
        return boost::shared_ptr<IObjComponent>(_sourceCache,NoDeleting());
      }
    }

    //------------------------------------------------------------------------------------------
    /** Gets a pointer to the Sample Position
     *  @returns a pointer to the Sample Position
     */
    IObjComponent_sptr Instrument::getSample() const
    {
      if ( !_sampleCache )
      {
        g_log.warning("In Instrument::getSamplePos(). No SamplePos has been set.");
        return boost::shared_ptr<IObjComponent>(_sampleCache,NoDeleting());
      }
      else if (m_isParametrized)
      {
        return IObjComponent_sptr(new ObjComponent(dynamic_cast<const Instrument*>(m_base)->_sampleCache,m_map));
      }
      else
      {
        return boost::shared_ptr<IObjComponent>(_sampleCache,NoDeleting());
      }
    }

    /** Gets the beam direction (i.e. source->sample direction).
    *  Not virtual because it relies the getSample() & getPos() virtual functions
    *  @returns A unit vector denoting the direction of the beam
    */
    Kernel::V3D Instrument::getBeamDirection() const
    {
      V3D retval = getSample()->getPos() - getSource()->getPos();
      retval.normalize();
      return retval;
    }

    //------------------------------------------------------------------------------------------
    /**  Get a shared pointer to a component by its ID, const version
    *   @param id :: ID
    *   @return A pointer to the component.
    */
    boost::shared_ptr<const IComponent> Instrument::getComponentByID(ComponentID id) const
    {
      const IComponent* base = (const IComponent*)(id);
      if (m_isParametrized)
        return ParComponentFactory::create(boost::shared_ptr<const IComponent>(base,NoDeleting()),m_map);
      else
        return boost::shared_ptr<const IComponent>(base, NoDeleting());
    }

    /**
    * Find a component by name.
    * @param cname :: The name of the component. If there are multiple matches, the first one found is returned.
    * @returns A shared pointer to the component
    */
    boost::shared_ptr<const IComponent> Instrument::getComponentByName(const std::string & cname) const
    {
      boost::shared_ptr<const IComponent> node = boost::shared_ptr<const IComponent>(this, NoDeleting());
      // Check the instrument name first
      if( this->getName() == cname )
      {
        return node;
      }
      // Otherwise Search the instrument tree using a breadth-first search algorithm since most likely candidates
      // are higher-level components
      // I found some useful info here http://www.cs.bu.edu/teaching/c/tree/breadth-first/
      std::deque<boost::shared_ptr<const IComponent> > nodeQueue;
      // Need to be able to enter the while loop
      nodeQueue.push_back(node);
      while( !nodeQueue.empty() )
      {
        node = nodeQueue.front();
        nodeQueue.pop_front();
        int nchildren(0);
        boost::shared_ptr<const ICompAssembly> asmb = boost::dynamic_pointer_cast<const ICompAssembly>(node);
        if( asmb )
        {
          nchildren = asmb->nelements();
        }
        for( int i = 0; i < nchildren; ++i )
        {
          boost::shared_ptr<const IComponent> comp = (*asmb)[i];
          if( comp->getName() == cname )
          {
            return comp;
          }
          else
          {
            nodeQueue.push_back(comp);
          }
        }
      }// while-end

      // If we have reached here then the search failed
      return boost::shared_ptr<const IComponent>();
    }

    /** Find all components in an Instrument Definition File (IDF) with a given name. If you know a component
     *  has a unique name use instead getComponentByName(), which is as fast or faster for retrieving a uniquely
     *  named component.
     *  @param cname :: The name of the component. If there are multiple matches, the first one found is returned.
     *  @returns Pointers to components
     */
    std::vector<boost::shared_ptr<const IComponent> > Instrument::getAllComponentsWithName(const std::string & cname) const
    {
      boost::shared_ptr<const IComponent> node = boost::shared_ptr<const IComponent>(this, NoDeleting());
      std::vector<boost::shared_ptr<const IComponent> > retVec;
      // Check the instrument name first
      if( this->getName() == cname )
      {
        retVec.push_back(node);
      }
      // Same algorithm as used in getComponentByName() but searching the full tree
      std::deque<boost::shared_ptr<const IComponent> > nodeQueue;
      // Need to be able to enter the while loop
      nodeQueue.push_back(node);
      while( !nodeQueue.empty() )
      {
        node = nodeQueue.front();
        nodeQueue.pop_front();
        int nchildren(0);
        boost::shared_ptr<const ICompAssembly> asmb = boost::dynamic_pointer_cast<const ICompAssembly>(node);
        if( asmb )
        {
          nchildren = asmb->nelements();
        }
        for( int i = 0; i < nchildren; ++i )
        {
          boost::shared_ptr<const IComponent> comp = (*asmb)[i];
          if( comp->getName() == cname )
          {
            retVec.push_back(comp);
          }
          else
          {
            nodeQueue.push_back(comp);
          }
        }
      }// while-end

      // If we have reached here then the search failed
      return retVec;
    }

    /**	Gets a pointer to the detector from its ID
    *  Note that for getting the detector associated with a spectrum, the SpectraDetectorMap::getDetector
    *  method should be used rather than this one because it takes account of the possibility of more
    *  than one detector contibuting to a single spectrum
    *  @param   detector_id The requested detector ID
    *  @returns A pointer to the detector object
    *  @throw   NotFoundError If no detector is found for the detector ID given
    */
    IDetector_sptr Instrument::getDetector(const detid_t &detector_id) const
    {
      if (m_isParametrized)
      {
        IDetector_sptr baseDet = m_instr->getDetector(detector_id);
        return ParComponentFactory::createDetector(baseDet.get(), m_map);
      }
      else
      {
        std::map<detid_t, IDetector_sptr >::const_iterator it = _detectorCache.find(detector_id);
        if ( it == _detectorCache.end() )
        {
          g_log.debug() << "Detector with ID " << detector_id << " not found." << std::endl;
          std::stringstream readInt;
          readInt << detector_id;
          throw Kernel::Exception::NotFoundError("Instrument: Detector with ID " + readInt.str() + " not found.","");
        }

        return it->second;
      }
    }

    /**
     * Returns a pointer to the geometrical object for the given set of IDs
     * @param det_ids :: A list of detector ids
     *  @returns A pointer to the detector object
     *  @throw   NotFoundError If no detector is found for the detector ID given
     */
    IDetector_sptr Instrument::getDetector(const std::vector<detid_t> &det_ids) const
    {
      const size_t ndets(det_ids.size());
      if( ndets == 1)
      {
  return this->getDetector(det_ids[0]);
      }
      else
      {
  boost::shared_ptr<DetectorGroup> det_group(new DetectorGroup());
  bool warn(false);
  for( size_t i = 0; i < ndets; ++i )
  {
    det_group->addDetector(this->getDetector(det_ids[i]), warn);
  }
  return det_group;
      }
    }

    /**
     * Returns a list of Detectors for the given detectors ids
     * 
     */
    std::vector<IDetector_sptr> Instrument::getDetectors(const std::vector<detid_t> &det_ids) const
    {
      std::vector<IDetector_sptr> dets_ptr;
      dets_ptr.reserve(det_ids.size());
      std::vector<detid_t>::const_iterator it;
      for ( it = det_ids.begin(); it != det_ids.end(); ++it )
      {
        dets_ptr.push_back(this->getDetector(*it));
      }
      return dets_ptr;
    } 

    /**
     * Returns a list of Detectors for the given detectors ids
     *
     */
    std::vector<IDetector_sptr> Instrument::getDetectors(const std::set<detid_t> &det_ids) const
    {
      std::vector<IDetector_sptr> dets_ptr;
      dets_ptr.reserve(det_ids.size());
      std::set<detid_t>::const_iterator it;
      for ( it = det_ids.begin(); it != det_ids.end(); ++it )
      {
        dets_ptr.push_back(this->getDetector(*it));
      }
      return dets_ptr;
    }


    /**	Gets a pointer to the monitor from its ID
    *  @param   detector_id The requested detector ID
    *  @return A pointer to the detector object
    *  @throw   NotFoundError If no monitor is found for the detector ID given
    */
    IDetector_sptr Instrument::getMonitor(const int &detector_id)const
    {
      //No parametrized monitors - I guess ....

      std::vector<detid_t>::const_iterator itr;
      itr=find(m_monitorCache.begin(),m_monitorCache.end(),detector_id);
      if ( itr == m_monitorCache.end() )
      {
        g_log.debug() << "monitor with ID " << detector_id << " not found." << std::endl;
        std::stringstream readInt;
        readInt << detector_id;
        throw Kernel::Exception::NotFoundError("Instrument: Detector with ID " + readInt.str() + " not found.","");
      }
      IDetector_sptr monitor=getDetector(detector_id);

      return monitor;
    }


    /**	Gets a pointer to the requested child component
    * @param name :: the name of the object requested (case insensitive)
    * @returns a pointer to the component
    */
    IComponent* Instrument::getChild(const std::string& name) const
    {
      IComponent *retVal = 0;
      std::string searchName = name;
      std::transform(searchName.begin(), searchName.end(), searchName.begin(), toupper);

      int noOfChildren = this->nelements();
      for (int i = 0; i < noOfChildren; i++)
      {
        //The proper (parametrized or not) component will be returned by [i] operator.
        IComponent *loopPtr = (*this)[i].get();
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
    * @param comp :: Component to be marked (stored for later retrievel) as a "SamplePos" Component
    */
    void Instrument::markAsSamplePos(ObjComponent* comp)
    {
      if (m_isParametrized)
        throw std::runtime_error("Instrument::markAsSamplePos() called on a parametrized Instrument object.");

      if ( !_sampleCache )
        _sampleCache = comp;
      else
        g_log.warning("Have already added samplePos component to the _sampleCache.");
    }

    /** Mark a Component which has already been added to the Instrument (as a child component)
    * to be 'the' source Component. For now it is assumed that we have
    * at most one of these.
    *
    * @param comp :: Component to be marked (stored for later retrievel) as a "source" Component
    */
    void Instrument::markAsSource(ObjComponent* comp)
    {
      if (m_isParametrized)
        throw std::runtime_error("Instrument::markAsSource() called on a parametrized Instrument object.");

      if ( !_sourceCache )
        _sourceCache = comp;
      else
        g_log.warning("Have already added source component to the _sourceCache.");
    }

    /** Mark a Component which has already been added to the Instrument (as a child component)
    * to be a Detector by adding it to a detector cache.
    *
    * @param det :: Component to be marked (stored for later retrievel) as a detector Component
    *
    */
    void Instrument::markAsDetector(IDetector* det)
    {
      if (m_isParametrized)
        throw std::runtime_error("Instrument::markAsDetector() called on a parametrized Instrument object.");

      //Create a (non-deleting) shared pointer to it
      IDetector_sptr det_sptr = IDetector_sptr(det, NoDeleting() );
      if ( !_detectorCache.insert( std::map<int, IDetector_sptr >::value_type(det->getID(), det_sptr) ).second )
      {
        std::stringstream convert;
        convert << det->getID();
        g_log.error() << "Not successful in adding Detector with ID = " << convert.str() << " and name = " << det->getName() << " to _detectorCache." << std::endl;
      }
    }

    /** Mark a Component which has already been added to the Instrument class
    * as a monitor and add it to the detector cache.
    *
    * @param det :: Component to be marked (stored for later retrieval) as a detector Component
    *
    * @throw Exception::ExistsError if cannot add detector to cache
    */
    void Instrument::markAsMonitor(IDetector* det)
    {
      if (m_isParametrized)
        throw std::runtime_error("Instrument::markAsMonitor() called on a parametrized Instrument object.");

      // attempt to add monitor to instrument detector cache
      markAsDetector(det);

      // mark detector as a monitor
      Detector *d = dynamic_cast<Detector*>(det);
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
    const std::vector<detid_t> Instrument::getMonitors()const
    {
      //Monitors cannot be parametrized. So just return the base.
      if (m_isParametrized)
        return dynamic_cast<const Instrument*>(m_base)->m_monitorCache;
      else
        return m_monitorCache;
    }


   /**
    * Get the bounding box for this instrument. It is simply the sum of the bounding boxes of its children excluding the source
    * @param assemblyBox :: [Out] The resulting bounding box is stored here.
    */
    void Instrument::getBoundingBox(BoundingBox & assemblyBox) const
    {
      if (m_isParametrized)
      {
        // Check cache for assembly
        if( m_map->getCachedBoundingBox(this, assemblyBox ) )
        {
          return;
        }
        // Loop over the children and define a box large enough for all of them
        ComponentID sourceID = getSource()->getComponentID();
        assemblyBox = BoundingBox(); // this makes the instrument BB always axis aligned
        int nchildren = nelements();
        for(int i = 0; i < nchildren; ++i)
        {
          IComponent_sptr comp = this->getChild(i);
          if( comp && comp->getComponentID() != sourceID )
          {
            BoundingBox compBox;
            comp->getBoundingBox(compBox);
            assemblyBox.grow(compBox);
          }
        }
        //Set the cache
        m_map->setCachedBoundingBox(this, assemblyBox);

      }
      else
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
    }


    boost::shared_ptr<const std::vector<IObjComponent_const_sptr> > Instrument::getPlottable() const
    {
      if (m_isParametrized)
      {
        // Get the 'base' plottable components
        boost::shared_ptr<const std::vector<IObjComponent_const_sptr> > objs = m_instr->getPlottable();

        // Get a reference to the underlying vector, casting away the constness so that we
        // can modify it to get our result rather than creating another long vector
        std::vector<IObjComponent_const_sptr> & res = const_cast<std::vector<IObjComponent_const_sptr>&>(*objs);
        const std::vector<IObjComponent_const_sptr>::size_type total = res.size();
        for(std::vector<IObjComponent_const_sptr>::size_type i = 0; i < total; ++i)
        {
          res[i] = boost::dynamic_pointer_cast<const Detector>(
              ParComponentFactory::create(objs->at(i), m_map));
        }
        return objs;

      }
      else
      {
        // Base instrument
        boost::shared_ptr<std::vector<IObjComponent_const_sptr> > res( new std::vector<IObjComponent_const_sptr> );
        res->reserve(_detectorCache.size()+10);
        appendPlottable(*this,*res);
        return res;
      }

    }


    void Instrument::appendPlottable(const CompAssembly& ca,std::vector<IObjComponent_const_sptr>& lst) const
    {
      for(int i=0;i<ca.nelements();i++)
      {
        IComponent* c = ca[i].get();
        CompAssembly* a = dynamic_cast<CompAssembly*>(c);
        if (a) appendPlottable(*a,lst);
        else
        {
          Detector* d = dynamic_cast<Detector*>(c);
          ObjComponent* o = dynamic_cast<ObjComponent*>(c);
          if (d)
            lst.push_back(IObjComponent_const_sptr(d,NoDeleting()));
          else if (o)
            lst.push_back(IObjComponent_const_sptr(o,NoDeleting()));
          else
            g_log.error()<<"Unknown comp type\n";
        }
      }
    }
    
    /**
     * Swap the references to the base component and ParameterMap. The instrument has to be slightly special
     * @param base A pointer to the base component
     * @param map A pointer to the parameter map
     */
    void Instrument::swap(const Instrument* base, const ParameterMap * map)
    {
      m_instr = boost::shared_ptr<Instrument>(const_cast<Instrument*>(base), NoDeleting());
      m_map_nonconst = ParameterMap_sptr(const_cast<ParameterMap*>(map), NoDeleting());
      Component::swap(base, map);
    }
    



    const double CONSTANT = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);


    //-----------------------------------------------------------------------
    /** Calculate the conversion factor (tof -> d-spacing) for a single pixel.
     *
     * @param l1 :: Primary flight path.
     * @param beamline: vector = samplePos-sourcePos = a vector pointing from the source to the sample,
     *        the length of the distance between the two.
     * @param beamline_norm: (source to sample distance) * 2.0 (apparently)
     * @param samplePos: position of the sample
     * @param det: Geometry object representing the detector (position of the pixel)
     * @param offset: value (close to zero) that changes the factor := factor * (1+offset).
     * @param vulcancorrection:  boolean to use l2 from Rectangular Detector parent
     * @return conversion factor for pixel
     */
    double Instrument::calcConversion(const double l1,
                          const Kernel::V3D &beamline,
                          const double beamline_norm,
                          const Kernel::V3D &samplePos,
                          const IDetector_const_sptr &det,
                          const double offset,
                          bool vulcancorrection)
    {
      if (offset <= -1.) // not physically possible, means result is negative d-spacing
      {
        std::stringstream msg;
        msg << "Encountered offset of " << offset << " which converts data to negative d-spacing\n";
        throw std::logic_error(msg.str());
      }

      // Get the sample-detector distance for this detector (in metres)

      // The scattering angle for this detector (in radians).
      Kernel::V3D detPos;
      if (vulcancorrection)
      {
        detPos = det->getParent()->getPos();
      }
      else
      {
        detPos = det->getPos();
      }

      // Now detPos will be set with respect to samplePos
      detPos -= samplePos;
      // 0.5*cos(2theta)
      double l2=detPos.norm();
      double halfcosTwoTheta=detPos.scalar_prod(beamline)/(l2*beamline_norm);
      // This is sin(theta)
      double sinTheta=sqrt(0.5-halfcosTwoTheta);
      const double numerator = (1.0+offset);
      sinTheta *= (l1+l2);
      return (numerator * CONSTANT) / sinTheta;
    }


    //-----------------------------------------------------------------------
    /** Calculate the conversion factor (tof -> d-spacing)
     * for a LIST of detectors assigned to a single spectrum.
     */
    double Instrument::calcConversion(const double l1,
                          const Kernel::V3D &beamline,
                          const double beamline_norm,
                          const Kernel::V3D &samplePos,
                          const Instrument_const_sptr &instrument,
                          const std::vector<detid_t> &detectors,
                          const std::map<detid_t,double> &offsets,
                          bool vulcancorrection)
    {
      double factor = 0.;
      double offset;
      for (std::vector<detid_t>::const_iterator iter = detectors.begin(); iter != detectors.end(); ++iter)
      {
        std::map<detid_t,double>::const_iterator off_iter = offsets.find(*iter);
        if( off_iter != offsets.end() )
        {
          offset = offsets.find(*iter)->second;
        }
        else
        {
          offset = 0.;
        }
        factor += calcConversion(l1, beamline, beamline_norm, samplePos,
                                 instrument->getDetector(*iter), offset, vulcancorrection);
      }
      return factor / static_cast<double>(detectors.size());
    }


    //------------------------------------------------------------------------------------------------
    /** Get several instrument parameters used in tof to D-space conversion
     *
     * @param instrument
     * @param l1 :: primary flight path (source-sample distance)
     * @param beamline :: vector of the direction and length of the beam (source to samepl)
     * @param beamline_norm :: 2 * the length of beamline
     * @param samplePos :: vector of the position of the sample
     */
    void Instrument::getInstrumentParameters(double & l1, Kernel::V3D & beamline,
        double & beamline_norm, Kernel::V3D & samplePos) const
    {
      // Get some positions
      const IObjComponent_sptr sourceObj = this->getSource();
      if (sourceObj == NULL)
      {
        throw Exception::InstrumentDefinitionError("Failed to get source component from instrument");
      }
      const Kernel::V3D sourcePos = sourceObj->getPos();
      samplePos = this->getSample()->getPos();
      beamline = samplePos-sourcePos;
      beamline_norm=2.0*beamline.norm();

      // Get the distance between the source and the sample (assume in metres)
      IObjComponent_const_sptr sample = this->getSample();
      try
      {
        l1 = this->getSource()->getDistance(*sample);
      }
      catch (Exception::NotFoundError &)
      {
        throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance ", this->getName());
      }
    }



  } // namespace Geometry
} // Namespace Mantid
