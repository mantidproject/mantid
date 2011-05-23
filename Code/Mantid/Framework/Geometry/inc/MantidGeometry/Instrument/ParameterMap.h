#ifndef PARAMETERMAP_H_ 
#define PARAMETERMAP_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Cache.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/ISpectraDetectorMap.h" //For specid_t
#include "MantidGeometry/Objects/BoundingBox.h"


#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

#include <vector>
#include <typeinfo>

namespace Mantid
{
namespace Geometry
{

  //---------------------------------------------------------------------------
  // Forward declarations
  //---------------------------------------------------------------------------
  class BoundingBox;
  class NearestNeighbours;

  /** @class ParameterMap ParameterMap.h

    ParameterMap class. Holds the parameters of modified (parametrized) instrument
    components. ParameterMap has a number of 'add' methods for adding parameters of
    different types. 

    @author Roman Tolchenov, Tessella Support Services plc
    @date 2/12/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
    
    This file is part of Mantid.
    
    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
    
    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport ParameterMap
  {
  public:
#ifndef HAS_UNORDERED_MAP_H
    /// Parameter map typedef
    typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> > pmap;
    /// Parameter map iterator typedef
    typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> >::iterator pmap_it;
    /// Parameter map iterator typedef
    typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> >::const_iterator pmap_cit;
#else
    /// Parameter map typedef
    typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> > pmap;
    /// Parameter map iterator typedef
    typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> >::iterator pmap_it;
    /// Parameter map iterator typedef
    typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> >::const_iterator pmap_cit;
#endif
    /// Default constructor
    ParameterMap();
    /// Constructor taking a pointer to a spectra map
    ParameterMap(const ISpectraDetectorMap *const);
    /// Returns true if the map is empty, false otherwise
    inline bool empty() const { return m_map.empty(); }
    /// Return the size of the map
    inline int size() const { return static_cast<int>(m_map.size()); }
    /// Clears the map
    inline void clear()
    {
      m_map.clear();
      clearCache();
    }

    /// Method for adding a parameter providing its value as a string
    void add(const std::string& type,const IComponent* comp,const std::string& name, 
             const std::string& value);

    /**
     * Method for adding a parameter providing its value of a particular type
     * @tparam T The concrete type
     * @param type :: A string denoting the type, e.g. double, string, fitting
     * @param comp :: A pointer to the component that this parameter is attached to
     * @param name :: The name of the parameter
     * @param value :: The parameter's value
     */
    template<class T>
    void add(const std::string& type,const IComponent* comp,const std::string& name, 
             const T& value)
    {
      PARALLEL_CRITICAL(parameter_add)
      {
        bool created(false);
        boost::shared_ptr<Parameter> param = retrieveParameter(created, type, comp, name);
        ParameterType<T> *paramT = dynamic_cast<ParameterType<T> *>(param.get());
        if (!paramT)
        {
          reportError("Error in adding parameter: incompatible types");
          throw std::runtime_error("Error in adding parameter: incompatible types");
        }
        paramT->setValue(value);
        if( created )
        {
          m_map.insert(std::make_pair(comp->getComponentID(),param));
        }
      }
    }
    /** @name Helper methods for adding and updating paramter types  */
    /// Create or adjust "pos" parameter for a component
    void addPositionCoordinate(const IComponent* comp,const std::string& name, const double value);
    /// Create or adjust "rot" parameter for a component
    void addRotationParam(const IComponent* comp,const std::string& name, const double deg);
    /// Adds a double value to the parameter map.
    void addDouble(const IComponent* comp,const std::string& name, const std::string& value);
    /// Adds a double value to the parameter map.
    void addDouble(const IComponent* comp,const std::string& name, double value);
    /// Adds an int value to the parameter map.
    void addInt(const IComponent* comp,const std::string& name, const std::string& value);
    /// Adds an int value to the parameter map.
    void addInt(const IComponent* comp,const std::string& name, int value);
    /// Adds a bool value to the parameter map.
    void addBool(const IComponent* comp,const std::string& name, const std::string& value);
    /// Adds a bool value to the parameter map.
    void addBool(const IComponent* comp,const std::string& name, bool value);
    /// Adds a std::string value to the parameter map.
    void addString(const IComponent* comp,const std::string& name, const std::string& value);
    /// Adds a V3D value to the parameter map.
    void addV3D(const IComponent* comp,const std::string& name, const std::string& value);
    /// @param value :: Parameter value as a V3D
    void addV3D(const IComponent* comp,const std::string& name, const V3D& value);
    /// Adds a Quat value to the parameter map.
    void addQuat(const IComponent* comp,const std::string& name, const Quat& value);
    //@}

    /// Does the named parameter exist for the given component for any type.
    bool contains(const IComponent* comp, const char*  name) const;
    /// Does the named parameter exist for the given component and type
    bool contains(const IComponent* comp, const std::string & name, const std::string & type = "") const;
    /// Get a parameter with a given name
    boost::shared_ptr<Parameter> get(const IComponent* comp, const char * name) const;
    /// Get a parameter with a given name and optional type
    boost::shared_ptr<Parameter> get(const IComponent* comp,const std::string& name,
				     const std::string & type = "")const;
    /// Use get() recursively to see if can find param in all parents of comp.
    boost::shared_ptr<Parameter> getRecursive(const IComponent* comp, const char * name) const;
    /// Use get() recursively to see if can find param in all parents of comp and given type
    boost::shared_ptr<Parameter> getRecursive(const IComponent* comp,const std::string& name, 
					      const std::string & type = "")const;

    /** Get the values of a given parameter of all the components that have the name: compName
     *  @tparam The parameter type
     *  @param compName :: The name of the component
     *  @param name :: The name of the parameter
     *  @return all component values from the given component name
     */
    template<class T>
    std::vector<T> getType(const std::string& compName,const std::string& name)const
    {
      std::vector<T> retval;

      pmap_cit it;
      for (it = m_map.begin(); it != m_map.end(); ++it)
      {
        if ( compName.compare(((const IComponent*)(*it).first)->getName()) == 0 )  
        {
          boost::shared_ptr<Parameter> param = get((const IComponent*)(*it).first,name);
          if (param)
            retval.push_back( param->value<T>() );
        }
      }
      return retval;
    }

    /// Return the value of a parameter as a string
    std::string getString(const IComponent* comp,const std::string& name);
    /// Returns a string parameter as vector's first element if exists and an empty vector if it doesn't
    std::vector<std::string> getString(const std::string& compName,const std::string& name) const 
    {
      return getType<std::string>(compName,name);
    }
    /**  
     * Returns a double parameter as vector's first element if exists and an empty vector if it doesn't
     * @param compName :: Component name
     * @param name :: Parameter name
     * @return a double parameter from component with the requested name
     */
    std::vector<double> getDouble(const std::string& compName,const std::string& name)const
    {
      return getType<double>(compName,name);
    }
    /**  
     * Returns a V3D parameter as vector's first element if exists and an empty vector if it doesn't
     * @param compName :: Component name
     * @param name :: Parameter name
     * @return a V3D parameter from component with the requested name
     */
    std::vector<V3D> getV3D(const std::string& compName,const std::string& name)const
    {
      return getType<V3D>(compName,name);
    }

    /// Returns a set with all parameter names for component
    std::set<std::string> names(const IComponent* comp) const;
    /// Returns a string with all component names, parameter names and values
    std::string asString()const;

    ///Clears the location and roatation caches
    void clearCache();
    ///Sets a cached location on the location cache
    void setCachedLocation(const IComponent* comp, const V3D& location) const;
    ///Attempts to retreive a location from the location cache
    bool getCachedLocation(const IComponent* comp, V3D& location) const;
    ///Sets a cached rotation on the rotation cache
    void setCachedRotation(const IComponent* comp, const Quat& rotation) const;
    ///Attempts to retreive a rotation from the rotation cache
    bool getCachedRotation(const IComponent* comp, Quat& rotation) const;
    ///Sets a cached bounding box
    void setCachedBoundingBox(const IComponent *comp, const BoundingBox & box) const;
    ///Attempts to retrieve a bounding box from the cache
    bool getCachedBoundingBox(const IComponent *comp, BoundingBox & box) const;
    
    /** @name Nearest neighbours */
    // MG
    // This is here for the moment because a detector doesn't have access to anything else.
    // Should this be on the instrument with a reference passed to the detectors?
    //@{
    /// Update the spectra map reference
    void resetSpectraMap(const ISpectraDetectorMap *const);
    /// Build and populate the NearestNeighbours object
    void buildNearestNeighbours(const IComponent *comp) const;
    /// Query the NearestNeighbours object for a detector
    std::map<specid_t, double> getNeighbours(const IDetector *comp, const double radius = 0.0) const;
  private:
    /// Requires a spectra map to give the correct neighbours
    const ISpectraDetectorMap *m_spectraMap;
    //@}

  private:
    ///Assignment operator
    ParameterMap& operator=(ParameterMap * rhs);
    /// Retrieve a parameter by either creating a new one of getting an existing one
    Parameter_sptr retrieveParameter(bool &created, const std::string & type, const IComponent* comp, 
				     const std::string & name);
    /// report an error
    void reportError(const std::string& str);

    /// internal parameter map instance
    pmap m_map;
    /// shared pointer to NearestNeighbours object
    mutable boost::shared_ptr<NearestNeighbours> m_nearestNeighbours;
    /// internal cache map instance for cached postition values
    mutable Kernel::Cache<const ComponentID,V3D > m_cacheLocMap;
    /// internal cache map instance for cached rotation values
    mutable Kernel::Cache<const ComponentID,Quat > m_cacheRotMap;
    ///internal cache map for cached bounding boxes
    mutable Kernel::Cache<const ComponentID,BoundingBox> m_boundingBoxMap;
    /// Static reference to the logger class
    static Kernel::Logger& g_log;
  };

  /// ParameterMap shared pointer typedef
  typedef boost::shared_ptr<ParameterMap> ParameterMap_sptr;
  /// ParameterMap constant shared pointer typedef
  typedef boost::shared_ptr<const ParameterMap> ParameterMap_const_sptr;

} // Namespace Geometry

} // Namespace Mantid

#endif /*PARAMETERMAP_H_*/
