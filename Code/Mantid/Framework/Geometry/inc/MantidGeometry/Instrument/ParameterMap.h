#ifndef PARAMETERMAP_H_ 
#define PARAMETERMAP_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IDTypes.h" //For specid_t
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Cache.h"

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
#ifndef HAS_UNORDERED_MAP_H
    /// Parameter map iterator typedef
  typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> >::iterator component_map_it;
  typedef std::multimap<const ComponentID,boost::shared_ptr<Parameter> >::const_iterator component_map_cit;
#else
   /// Parameter map iterator typedef
   typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> >::iterator component_map_it;
   typedef std::tr1::unordered_multimap<const ComponentID,boost::shared_ptr<Parameter> >::const_iterator component_map_cit;
#endif

  class MANTID_GEOMETRY_DLL ParameterMap
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
    /// Returns true if the map is empty, false otherwise
    inline bool empty() const { return m_map.empty(); }
    /// Return the size of the map
    inline int size() const { return static_cast<int>(m_map.size()); }
    /// Return string to be used in the map
    static const std::string & pos();
    static const std::string & posx();
    static const std::string & posy();
    static const std::string & posz();
    static const std::string & rot();
    static const std::string & rotx();
    static const std::string & roty();
    static const std::string & rotz();
    static const std::string & pDouble(); // p prefix to avoid name clash
    static const std::string & pInt();
    static const std::string & pBool();
    static const std::string & pString();
    static const std::string & pV3D();
    static const std::string & pQuat();

    const std::string diff(const ParameterMap & rhs, const bool & firstDiffOnly = false) const;

    /// Inquality comparison operator
    bool operator!=(const ParameterMap & rhs) const;
    /// Equality comparison operator
    bool operator==(const ParameterMap & rhs) const;

    /// Clears the map
    inline void clear()
    {
      m_map.clear();
      clearPositionSensitiveCaches();
    }
    /// method swaps two parameter maps contents  each other. All caches contents is nullified (TO DO: it can be efficiently swapped too)
    void swap(ParameterMap &other)
    {
      m_map.swap(other.m_map);
      clearPositionSensitiveCaches();
    }
    /// Clear any parameters with the given name
    void clearParametersByName(const std::string & name);

    /// Clear any parameters with the given name for a specified component
    void clearParametersByName(const std::string & name,const IComponent* comp);

    /// Method for adding a parameter providing its value as a string
    void add(const std::string& type,const IComponent* comp,const std::string& name,
             const std::string& value);

    /**
     * Method for adding a parameter providing its value of a particular type.
     * If a parameter already exists then it is replaced with a new one of the
     * given type and value
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
      auto param = ParameterFactory::create(type,name);
      auto typedParam = boost::dynamic_pointer_cast<ParameterType<T>>(param);
      assert(typedParam); // If not true the factory has created the wrong type
      typedParam->setValue(value);
      this->add(comp, param);
    }
    /// Method for adding a parameter providing shared pointer to it. The class stores share pointer and increment ref count to it
    void add(const IComponent* comp, const boost::shared_ptr<Parameter> &param);

    /** @name Helper methods for adding and updating parameter types  */
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
    /// Adds a Kernel::V3D value to the parameter map.
    void addV3D(const IComponent* comp,const std::string& name, const std::string& value);
    /// @param value :: Parameter value as a Kernel::V3D
    void addV3D(const IComponent* comp,const std::string& name, const Kernel::V3D& value);
    /// Adds a Kernel::Quat value to the parameter map.
    void addQuat(const IComponent* comp,const std::string& name, const Kernel::Quat& value);
    //@}

    /// Does the named parameter exist for the given component and type (std::string version)
    bool contains(const IComponent* comp, const std::string & name, const std::string & type = "") const;
    /// Does the named parameter exist for the given component (c-string version)
    bool contains(const IComponent* comp, const char * name, const char * type = "") const;
    /// Does the given parameter & component combination exist
    bool contains(const IComponent* comp, const Parameter & parameter) const;
    /// Get a parameter with a given name and type (std::string version)
    boost::shared_ptr<Parameter> get(const IComponent* comp,const std::string& name,
                                     const std::string & type = "") const;
    /// Get a parameter with a given name and type (c-string version)
    boost::shared_ptr<Parameter> get(const IComponent* comp, const char *name, const char * type = "") const;
    /// Finds the parameter in the map via the parameter type.
    boost::shared_ptr<Parameter>  getByType(const IComponent* comp, const std::string& type) const;
    /// Use get() recursively to see if can find param in all parents of comp and given type (std::string version)
    boost::shared_ptr<Parameter> getRecursive(const IComponent* comp, const std::string& name,
                                              const std::string & type = "")const;
    /// Use get() recursively to see if can find param in all parents of comp and given type (const char type)
    boost::shared_ptr<Parameter> getRecursive(const IComponent* comp, const char * name,
                                              const char * type = "") const;
    /// Looks recursively upwards in the component tree for the first instance of a parameter with a specified type.
    boost::shared_ptr<Parameter> getRecursiveByType(const IComponent* comp, const std::string& type) const;

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
    std::string getString(const IComponent* comp,const std::string& name, bool recursive = false) const;
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
     * Returns a Kernel::V3D parameter as vector's first element if exists and an empty vector if it doesn't
     * @param compName :: Component name
     * @param name :: Parameter name
     * @return a Kernel::V3D parameter from component with the requested name
     */
    std::vector<Kernel::V3D> getV3D(const std::string& compName,const std::string& name)const
    {
      return getType<Kernel::V3D>(compName,name);
    }

    /// Returns a set with all parameter names for component
    std::set<std::string> names(const IComponent* comp) const;
    /// Returns a string with all component names, parameter names and values
    std::string asString()const;

    ///Clears the location, rotation & bounding box caches
    void clearPositionSensitiveCaches();
    ///Sets a cached location on the location cache
    void setCachedLocation(const IComponent* comp, const Kernel::V3D& location) const;
    ///Attempts to retrieve a location from the location cache
    bool getCachedLocation(const IComponent* comp, Kernel::V3D& location) const;
    ///Sets a cached rotation on the rotation cache
    void setCachedRotation(const IComponent* comp, const Kernel::Quat& rotation) const;
    ///Attempts to retrieve a rotation from the rotation cache
    bool getCachedRotation(const IComponent* comp, Kernel::Quat& rotation) const;
    ///Sets a cached bounding box
    void setCachedBoundingBox(const IComponent *comp, const BoundingBox & box) const;
    ///Attempts to retrieve a bounding box from the cache
    bool getCachedBoundingBox(const IComponent *comp, BoundingBox & box) const;
    /// Persist a representation of the Parameter map to the open Nexus file
    void saveNexus(::NeXus::File * file, const std::string & group) const;
    /// Copy pairs (oldComp->id,Parameter) to the m_map assigning the new newComp->id
    void copyFromParameterMap(const IComponent* oldComp,const IComponent* newComp, const ParameterMap *oldPMap);
    /// access iterators. begin;
    pmap_it begin(){return m_map.begin();}
    pmap_cit begin()const{return m_map.begin();}
    /// access iterators. end;
    pmap_it end(){return m_map.end();}
    pmap_cit end()const{return m_map.end();}

  private:
    ///Assignment operator
    ParameterMap& operator=(ParameterMap * rhs);
    /// internal function to get position of the parameter in the parameter map
    component_map_it positionOf(const IComponent* comp,const char *name, const char * type);
    ///const version of the internal function to get position of the parameter in the parameter map
    component_map_cit positionOf(const IComponent* comp,const char *name, const char * type) const;


    /// internal parameter map instance
    pmap m_map;
    /// internal cache map instance for cached position values
    mutable Kernel::Cache<const ComponentID, Kernel::V3D > m_cacheLocMap;
    /// internal cache map instance for cached rotation values
    mutable Kernel::Cache<const ComponentID, Kernel::Quat > m_cacheRotMap;
    ///internal cache map for cached bounding boxes
    mutable Kernel::Cache<const ComponentID,BoundingBox> m_boundingBoxMap;
  };

  /// ParameterMap shared pointer typedef
  typedef boost::shared_ptr<ParameterMap> ParameterMap_sptr;
  /// ParameterMap constant shared pointer typedef
  typedef boost::shared_ptr<const ParameterMap> ParameterMap_const_sptr;

} // Namespace Geometry

} // Namespace Mantid

#endif /*PARAMETERMAP_H_*/
