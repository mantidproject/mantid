//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
  namespace Geometry
  {
    namespace
    {
      // static logger
      Kernel::Logger g_log("DetectorGroup");
    }

    using Kernel::V3D;
    using Kernel::Quat;
   

    /**
     * Default constructor
     */
    DetectorGroup::DetectorGroup() :
      IDetector(), m_id(), m_detectors(),group_topology(undef)
    {
    }

    /** Constructor that takes a list of detectors to add
    *  @param dets :: The vector of IDetector pointers that this virtual detector will hold
    *  @param warnAboutMasked :: If true a log message at warning level will be generated if a one of the detectors in dets is masked. 
    *  @throw std::invalid_argument If an empty vector is passed as argument
    */
    DetectorGroup::DetectorGroup(const std::vector<IDetector_const_sptr>& dets, bool warnAboutMasked) :
    IDetector(), m_id(), m_detectors(),group_topology(undef)
    {
      if ( dets.empty() )
      {
        g_log.error("Illegal attempt to create an empty DetectorGroup");
        throw std::invalid_argument("Empty DetectorGroup objects are not allowed");
      }
      std::vector<IDetector_const_sptr>::const_iterator it;
      for (it = dets.begin(); it != dets.end(); ++it)
      {
        addDetector(*it, warnAboutMasked);
      }
    }

    /// Destructor
    DetectorGroup::~DetectorGroup()
    {
    }

    /** Add a detector to the collection
    *  @param det ::  A pointer to the detector to add
    *  @param warn :: Whether to issue warnings to the log
    */
    void DetectorGroup::addDetector(IDetector_const_sptr det, bool& warn)
    {
      // the topology of the group become undefined and needs recalculation if new detector has been added to the group
      group_topology = undef;
      // Warn if adding a masked detector
      if ( warn && det->isMasked() )
      {
        g_log.warning() << "Adding a detector (ID:" << det->getID() << ") that is flagged as masked." << std::endl;
        warn = false;
      }
      

      // For now at least, the ID is the same as the first detector that is added
      if ( m_detectors.empty() ) m_id = det->getID();

      if ( (! m_detectors.insert( DetCollection::value_type(det->getID(), det) ).second ) && warn)
      {
        g_log.warning() << "Detector with ID " << det->getID() << " is already in group." << std::endl;
        warn = false;
      }
    }

    detid_t DetectorGroup::getID() const
    {
      return m_id;
    }

    std::size_t DetectorGroup::nDets() const
    {
      return m_detectors.size();
    }

    /** Returns the position of the DetectorGroup.
    *  In the absence of a full surface/solid angle implementation, this is a simple
    *  average of the component detectors (i.e. there's no weighting for size or if one
    *  or more of the detectors is masked). Also, no regard is made to whether a
    *  constituent detector is itself a DetectorGroup - it's just treated as a single,
    *  pointlike object with the same weight as any other detector.
    *  @return a V3D object of the detector group position
    */
    V3D DetectorGroup::getPos() const
    {
      V3D newPos;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        newPos += (*it).second->getPos();
      }
      // We can have very small values (< Tolerance) of each component that should be zero
      if (std::abs(newPos[0]) < Mantid::Kernel::Tolerance) newPos[0] = 0.0;
      if (std::abs(newPos[1]) < Mantid::Kernel::Tolerance) newPos[1] = 0.0;
      if (std::abs(newPos[2]) < Mantid::Kernel::Tolerance) newPos[2] = 0.0;

      return newPos /= static_cast<double>(m_detectors.size()); // protection against divide by zero in V3D
    }

    /// Gives the average distance of a group of detectors from the given component
    double DetectorGroup::getDistance(const IComponent& comp) const
    {
      double result = 0.0;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        result += (*it).second->getDistance(comp);
      }
      return result/static_cast<double>(m_detectors.size());
    }

    /// Gives the average angle of a group of detectors from the observation point, relative to the axis given
    double DetectorGroup::getTwoTheta(const V3D& observer, const V3D& axis) const
    {
      double result = 0.0;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        const V3D sampleDetVec = (*it).second->getPos() - observer;
        result += sampleDetVec.angle(axis);
      }
      return result/static_cast<double>(m_detectors.size());
    }

    /*
    Gives the average angle of a group of detectors from the observation point, relative to the axis given.
    Returned values are signed according to the rotation direction relative to the axis and the instrument up direction.
    @param observer : observer (usually sample)
    @param axis : scattering axis.
    @param instrumentUp : Instrument up direction
    @return signed theta
    */
    double DetectorGroup::getSignedTwoTheta(const Kernel::V3D& observer, const Kernel::V3D& axis, const Kernel::V3D& instrumentUp) const
    {
      double result = 0.0;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        const V3D sampleDetVec = it->second->getPos() - observer;
        double angle = sampleDetVec.angle(axis);

        V3D cross = axis.cross_prod(sampleDetVec);
        V3D normToSurface = axis.cross_prod(instrumentUp);
        if (normToSurface.scalar_prod(cross) < 0 )
        { 
          angle *= -1;
        }
        result += angle;
      }
      return result/static_cast<double>(m_detectors.size());
    }

    /// Computes the average position and returns the phi value
    double DetectorGroup::getPhi() const
    {
      V3D avgPos = this->getPos();
      double phi(0.0), dummy1(0.0), dummy2(0.0);
      avgPos.getSpherical(dummy1, dummy2, phi);
      return phi*M_PI/180.0;
    }

    /// Computes the average position and returns the phi value
    double DetectorGroup::getPhiOffset(const double& offset) const
    {
      double avgPos = getPhi();
      double phiOut = avgPos;
      if(avgPos  < 0)
      {
        phiOut = - (offset + avgPos);
      }
      else
      {
        phiOut = offset - avgPos;
      }
      return phiOut;
    }

    /** Return IDs for the detectors grouped
    * 
    *  @return vector of detector IDs
    */
    std::vector<detid_t> DetectorGroup::getDetectorIDs() const
    {
      std::vector<detid_t> result;
      result.reserve(m_detectors.size());
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        result.push_back( (*it).first );
      }
      return result;
    }

    /**
     * Return the list of detectors held within this group
     * @returns A vector of IDetector pointers
     */
    std::vector<IDetector_const_sptr> DetectorGroup::getDetectors() const
    {
      std::vector<IDetector_const_sptr> result;
      result.reserve(m_detectors.size());
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        result.push_back( (*it).second );
      }
      return result;
    }

    /** Gives the total solid angle subtended by a group of detectors by summing the
     *  contributions from the individual detectors.
     *  Any masked detector in the group is excluded from the sum.
     *  @param observer :: The point from which the detector is being viewed
     *  @return The solid angle in steradians
     *  @throw NullPointerException If geometrical form of any detector has not been provided in the instrument definition file
     */
    double DetectorGroup::solidAngle(const V3D& observer) const
    {
      double result = 0.0;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        IDetector_const_sptr det = (*it).second;
        if ( ! det->isMasked() ) result += det->solidAngle(observer);
      }
      return result;
    }

    /** Are ALL the detectors in this group masked?
     *  @return True if every one of the detectors in this group is masked, false otherwise.
     */
    bool DetectorGroup::isMasked() const
    {
      bool isMasked = true;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        if ( !(*it).second->isMasked() )
        {
          isMasked = false;
          break;
        }
      }
      return isMasked;
    }

    /** Return true if any detector in the group is parametrized.
     *
     */
    bool DetectorGroup::isParametrized() const
    {
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
        if( (*it).second->isParametrized()) return true;
      return false;

    }


    /** Indicates whether this is a monitor.
    *  Will return false if even one member of the group is not flagged as a monitor
    *  @return is detector group a monitor
    */
    bool DetectorGroup::isMonitor() const
    {
      // Would you ever want to group monitors?
      // For now, treat as NOT a monitor if even one isn't
      bool isMonitor = true;
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        if ( !(*it).second->isMonitor() ) isMonitor = false;
      }
      return isMonitor;

    }

    /** isValid() is true if the point is inside any of the detectors, i.e. one of the
    *  detectors has isValid() == true
    *  @param point :: this point is tested to see if it is one of the detectors
    *  @return if the point is in a detector it returns true else it returns false
    */
    bool DetectorGroup::isValid(const V3D& point) const
    {
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        if ( (*it).second->isValid(point) ) return true;
      }
      return false;
    }

    /** Does the point given lie on the surface of one of the detectors
    *  @param point :: the point that is tested to see if it is one of the detectors
    *  @return true if the point is on the side of a detector else it returns false
    */
    bool DetectorGroup::isOnSide(const V3D& point) const
    {
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        if ( (*it).second->isOnSide(point) ) return true;
      }
      return false;
    }

    /** tries to find a point that lies on or within the first detector in the storage
    * found in the storage map
    *  @param point :: if a point is found its coordinates will be stored in this varible
    *  @return 1 if point found, 0 otherwise
    */
    int  DetectorGroup::getPointInObject(V3D& point) const
    {
      DetCollection::const_iterator it;
      it = m_detectors.begin();
      if ( it == m_detectors.end() ) return 0;
      return ( *m_detectors.begin() ).second->getPointInObject(point);
    }

    /**
    * Get the names of the parameters for this component.
    * @param recursive :: If true, the parameters for all of the parent components are also included
    * @returns A set of strings giving the parameter names for this component
    */
    std::set<std::string> DetectorGroup::getParameterNames(bool recursive) const
    {
      (void) recursive; //Avoid compiler warning
      return std::set<std::string>();
    }

     /**
     * Get the names of the parameters for this component and it's parents.
     * @returns A map of strings giving the parameter names and the component they are from, warning this contains shared pointers keeping transient objects alive, do not keep longer than needed
     */
    std::map<std::string,ComponentID > DetectorGroup::getParameterNamesByComponent() const
    {
       return std::map<std::string,ComponentID >();
    }

    /**
    * Get a string representation of a parameter
    * @param pname :: The name of the parameter
    * @param recursive :: If true the search will walk up through the parent components
    * @returns A empty string as this is not a parameterized component
    */
    std::string DetectorGroup::getParameterAsString(const std::string& pname, bool recursive) const
    {
      (void) pname; //Avoid compiler warning
      (void) recursive; //Avoid compiler warning
      return "";
    }


    /**
    * Get the bounding box for this group of detectors. It is simply the sum of the bounding boxes of its constituents.
    * @param boundingBox :: [Out] The resulting bounding box is stored here.
    */
    void DetectorGroup::getBoundingBox(BoundingBox & boundingBox) const
    {
     // boundingBox = BoundingBox(); // this change may modify a lot of behaviour -> verify
      for( DetCollection::const_iterator cit = m_detectors.begin(); cit != m_detectors.end(); ++cit )
      {
        BoundingBox memberBox;
        if(!boundingBox.isAxisAligned()){
           // coordinate system
          const std::vector<V3D> *cs = &(boundingBox.getCoordSystem());
          memberBox.realign(cs);
        }
        IComponent_const_sptr det = cit->second;
        det->getBoundingBox(memberBox);
        boundingBox.grow(memberBox);
      }
    }
     /**
    * Returns a boolean indicating if the component has the named parameter
    * @param name :: The name of the parameter
    * @param recursive :: If true the parent components will also be searched (Default: true)
    * @returns A boolean indicating if the search was successful or not. Always false as this is not
    * parameterized
    */
    bool DetectorGroup::hasParameter(const std::string & name, bool recursive) const
    {
      (void) recursive; //Avoid compiler warning
      (void) name; //Avoid compiler warning
      return false;
    }
    /** Detectors group assumed to be non-parameterized */
    std::string DetectorGroup::getParameterType(const std::string & /*name*/, bool /*recursive = true*/) const
    {
        return std::string("");
    }

    /// Default implementation
    std::vector<double> DetectorGroup::getNumberParameter(const std::string&, bool) const
    {
      return std::vector<double>(0);
    }

    /// Default implementation
    std::vector<V3D> DetectorGroup::getPositionParameter(const std::string&, bool) const
    {
      return std::vector<V3D>(0);
    }

    /// Default implementation  
    std::vector<Quat> DetectorGroup::getRotationParameter(const std::string&, bool) const
    {
      return std::vector<Quat>(0);
    }

    /// Default implementation  
    std::vector<std::string> DetectorGroup::getStringParameter(const std::string&, bool) const
    {
      return std::vector<std::string>(0);
    }
   
    /// Default implementation  
    std::vector<int> DetectorGroup::getIntParameter(const std::string&, bool) const
    {
      return std::vector<int>(0);
    }

    /// Default implementation  
    std::vector<bool> DetectorGroup::getBoolParameter(const std::string&, bool) const
    {
      return std::vector<bool>(0);
    }

    /// 
    det_topology
    DetectorGroup::getTopology(V3D &center)const
    {
        if(group_topology==undef){
            this->calculateGroupTopology();
        }
        center = this->groupCentre;
        return group_topology;
    }
    /** the private function calculates the topology of the detector's group, namely if the detectors arranged into 
     *  a ring or into a rectangle. Uses assumption that a ring has hole inside, so geometrical centre of the shape does
     *  not belong to a ring but does belong to a rectangle
     */
    void 
    DetectorGroup::calculateGroupTopology()const
    {
        if(m_detectors.size()==1){
            group_topology = rect;
     
            return;
        }
        this->groupCentre = this->getPos();
        if (this->isValid(groupCentre)){
             group_topology = rect;
        }else{
            // the topology can still be rectangular, but randomisation errors or small gaps between 
            // detectors caused central point not to belong to detectors; need more accrurate estinations 
            // assuming that distance between detectors can not be bigger then detector's half size
            // get detector's size:
             IDetector_const_sptr spFirstDet = this->m_detectors.begin()->second;
            

             BoundingBox bbox;
             spFirstDet->getBoundingBox(bbox);
             V3D width = bbox.width();
                      
             // loop if any near point belongs to group;
             for(int i=0;i<6;i++){
                 int ic = int(i)/2;
                 int is = (i%2==0)?-1:1;
                 V3D cs = groupCentre;
                 cs[ic]    +=is*width[ic]/4;
                 if(this->isValid(cs)){  // if it is, finish and end
                       group_topology = rect; break;
                 }
             }
             // if not, consider this group to be a ring;
             if(this->group_topology == undef){
                    group_topology = cyl;
             }
        }
    }

    std::string DetectorGroup::getName() const
    {
      std::string result = "";
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        result += (*it).second->getName() + this->getNameSeparator();
      }
      return result;
    }

    std::string DetectorGroup::getFullName() const
    {
      std::string result = "";
      DetCollection::const_iterator it;
      for (it = m_detectors.begin(); it != m_detectors.end(); ++it)
      {
        result += (*it).second->getFullName() + this->getNameSeparator();
      }
      return result;
    }
} // namespace Geometry
} // namespace Mantid
