//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/Exception.h"
#include <boost/tokenizer.hpp>
#include <algorithm>

namespace Mantid
{
  namespace Kernel
  {

    using std::string;

    // Get a reference to the logger
    Logger& PropertyManager::g_log = Logger::get("PropertyManager");

    //-----------------------------------------------------------------------------------------------
    /// Default constructor
    PropertyManager::PropertyManager() :
      m_properties(), m_orderedProperties()
    {
    }
    
    //-----------------------------------------------------------------------------------------------
    /// copy constructor
    /// @param other :: the PropertyManager to copy
    PropertyManager::PropertyManager(const PropertyManager& other) :
      m_properties(), m_orderedProperties(other.m_orderedProperties.size())
    {
      // We need to do a deep copy of the property pointers here
      for (unsigned int i = 0; i < m_orderedProperties.size(); ++i)
      {
        Property * p = other.m_orderedProperties[i]->clone();
        this->m_orderedProperties[i] = p;
        std::string key = p->name();
        std::transform(key.begin(), key.end(), key.begin(), toupper);
        this->m_properties[key] = p;
      }
    }

    //-----------------------------------------------------------------------------------------------
    /// Assignment operator - performs a deep copy
    /// @param other :: the PropertyManager to copy
    /// @return pointer to this
    PropertyManager& PropertyManager::operator=(const PropertyManager& other)
    {
      // We need to do a deep copy here
      if ( this != &other )
      {
        for ( PropertyMap::iterator it = m_properties.begin(); it != m_properties.end(); ++it )
        {
          delete it->second;
        }
        this->m_properties.clear();
        this->m_orderedProperties.resize(other.m_orderedProperties.size());
        for (unsigned int i = 0; i < m_orderedProperties.size(); ++i)
        {
          Property * p = other.m_orderedProperties[i]->clone();
          this->m_orderedProperties[i] = p;
          std::string key = p->name();
          std::transform(key.begin(), key.end(), key.begin(), toupper);
          this->m_properties[key] = p;
        }
      }
      return *this;
    }

    //-----------------------------------------------------------------------------------------------
    /// Virtual destructor
    PropertyManager::~PropertyManager()
    {
      for ( PropertyMap::iterator it = m_properties.begin(); it != m_properties.end(); ++it )
      {
        delete it->second;
      }
    }


    //-----------------------------------------------------------------------------------------------
    /**
     * Addition operator
     * @param rhs :: The object that is being added to this.
     * @returns A reference to the summed object
     */
    PropertyManager& PropertyManager::operator+=(const PropertyManager& rhs)
    {
      //Iterate through all properties on the RHS
      PropertyMap::const_iterator it;
      for (it = rhs.m_properties.begin(); it != rhs.m_properties.end(); it++)
      {
        //The name on the rhs
        string rhs_name = it->first;
        try
        {
          Property * lhs_prop = this->getPointerToProperty(rhs_name);
          //Use the property's += operator to add THAT. Isn't abstraction fun?!
          (*lhs_prop) += it->second;
        }
        catch (Exception::NotFoundError err)
        {
          //The property isnt on the lhs.
          //Let's copy it
          Property * copy = it->second->clone();
          //And we add a copy of that property to *this
          this->declareProperty(copy, "");
        }

        //(*it->second) +=
      }

      return *this;
    }


    //-----------------------------------------------------------------------------------------------
    /**
     * Filter out a run by time. Takes out any TimeSeriesProperty log entries outside of the given
     *  absolute time range.
     *
     * @param start :: Absolute start time. Any log entries at times >= to this time are kept.
     * @param stop :: Absolute stop time. Any log entries at times < than this time are kept.
     */
    void PropertyManager::filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop)
    {
      //Iterate through all properties
      PropertyMap::const_iterator it;
      for (it = this->m_properties.begin(); it != this->m_properties.end(); it++)
      {
        //Filter out the property
        Property * prop = it->second;
        prop->filterByTime(start, stop);
      }
    }


    //-----------------------------------------------------------------------------------------------
    /**
     * Split a run by time (splits the TimeSeriesProperties contained).
     *
     * Total proton charge will get re-integrated after filtering.
     *
     * @param splitter :: TimeSplitterType with the intervals and destinations.
     * @param outputs :: Vector of output runs.
     */
    void PropertyManager::splitByTime(TimeSplitterType& splitter, std::vector< PropertyManager * > outputs) const
    {
      size_t n = outputs.size();

      //Iterate through all properties
      PropertyMap::const_iterator it;
      for (it = this->m_properties.begin(); it != this->m_properties.end(); it++)
      {
        //Filter out the property
        Property * prop = it->second;

        //Make a vector of the output properties contained in the other property managers.
        //  NULL if it was not found.
        std::vector< Property *> output_properties;
        for (size_t i=0; i<n; i++)
        {
          if (outputs[i])
            output_properties.push_back( outputs[i]->getPointerToPropertyOrNull( prop->name() ) );
          else
            output_properties.push_back( NULL );
        }

        //Now the property does the splitting.
        prop->splitByTime(splitter, output_properties);

      }//for each property

    }


    //-----------------------------------------------------------------------------------------------
    /** Add a property to the list of managed properties
     *  @param p :: The property object to add
     *  @param doc :: A description of the property that may be displayed to users
     *  @throw Exception::ExistsError if a property with the given name already exists
     *  @throw std::invalid_argument  if the property declared has an empty name.
     */
    void PropertyManager::declareProperty( Property *p, const std::string &doc )
    {
      // Get the name of the property and don't permit empty names
      std::string key = p->name();
      if (key.empty())
      {
        delete p;
        throw std::invalid_argument("An empty property name is not permitted");
      }

      std::transform(key.begin(), key.end(), key.begin(), toupper);
      if ( m_properties.insert(PropertyMap::value_type(key, p)).second)
      {
        m_orderedProperties.push_back(p);
      }
      else
      {
        // Don't delete if this is actually the same property object!!!
        if ( m_properties.find(key)->second != p ) delete p;
        throw Exception::ExistsError( "Property with given name already exists", key );
      }
      p->setDocumentation(doc);
    }

    //-----------------------------------------------------------------------------------------------
    /** Set the ordered list of properties by one string of values.
     *  @param propertiesArray :: The list of property values
     *  @throw invalid_argument if error in parameters
     */
    // Care will certainly be required in the calling of this function or it could all go horribly wrong!
    void PropertyManager::setProperties( const std::string &propertiesArray )
    {
      // Split up comma-separated properties
      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

      boost::char_separator<char> sep(";");
      tokenizer propPairs(propertiesArray, sep);
      int index=0;
      // Iterate over the properties
      for (tokenizer::iterator it = propPairs.begin(); it != propPairs.end(); ++it)
      {
        boost::char_separator<char> sep2("=");
        tokenizer properties(*it,sep2);
        std::vector<std::string> property(properties.begin(), properties.end());
        // Call the appropriate setProperty method on the algorithm
        if ( property.size() == 2)
        {
          setPropertyValue(property[0],property[1]);
        }
        else if ( property.size() == 1)
        {
          // This is for a property with no value. Not clear that we will want such a thing.
          setPropertyOrdinal(index,property[0]);
        }
        // Throw if there's a problem with the string
        else
        {
          throw std::invalid_argument("Misformed properties string");
        }
        index++;
      }  
    }

    //-----------------------------------------------------------------------------------------------
    /** Set the value of a property by string
     *  N.B. bool properties must be set using 1/0 rather than true/false
     *  @param name :: The name of the property (case insensitive)
     *  @param value :: The value to assign to the property
     *  @throw Exception::NotFoundError if the named property is unknown
     *  @throw std::invalid_argument If the value is not valid for the property given
     */
    void PropertyManager::setPropertyValue( const std::string &name, const std::string &value )
    {
      Property *p = getPointerToProperty(name);   // throws NotFoundError if property not in vector
      std::string errorMsg = p->setValue(value);
      if ( !errorMsg.empty() ) 
      {
        errorMsg = "Invalid value for property " + p->name() + " (" +p->type() + ") \"" + value
          + "\": " + errorMsg;
        throw std::invalid_argument(errorMsg);
      }
    }

    //-----------------------------------------------------------------------------------------------
    /** Set the value of a property by an index
     *  N.B. bool properties must be set using 1/0 rather than true/false
     *  @param index :: The index of the property to assign
     *  @param value :: The value to assign to the property
     *  @throw std::runtime_error if the property index is too high
     *  @throw std::invalid_argument If the value is not valid for the property given
     */
    void PropertyManager::setPropertyOrdinal( const int& index, const std::string &value )
    {
      Property *p = getPointerToPropertyOrdinal(index);   // throws runtime_error if property not in vector
      std::string errorMsg = p->setValue(value);
      if ( !errorMsg.empty() ) 
      {
        errorMsg = "Invalid value for property " + p->name() + " (" +p->type() + ") \"" + value
          + "\" : " + errorMsg;
        throw std::invalid_argument(errorMsg);
      }
    }

    //-----------------------------------------------------------------------------------------------
    /** Checks whether the named property is already in the list of managed property.
     *  @param name :: The name of the property (case insensitive)
     *  @return True if the property is already stored
     */
    bool PropertyManager::existsProperty( const std::string& name ) const
    {
      try
      {
        getPointerToProperty(name);
        return true;
      }
      catch (Exception::NotFoundError&)
      {
        return false;
      }
    }

    //-----------------------------------------------------------------------------------------------
    /** Validates all the properties in the collection
     *  @return True if all properties have a valid value
     */
    bool PropertyManager::validateProperties() const
    {
      bool allValid = true;
      for ( PropertyMap::const_iterator it = m_properties.begin(); it != m_properties.end(); ++it )
      {
        //check for errors in each property
        std::string error = it->second->isValid();
        //"" means no error
        if ( !error.empty() )
        {
          g_log.error() << "Property \"" << it->first << "\" is not set to a valid value: \"" <<
            error << "\"." << std::endl;
          allValid=false;
        }
      }
      return allValid;
    }


    //-----------------------------------------------------------------------------------------------
    /** Get the value of a property as a string
     *  @param name :: The name of the property (case insensitive)
     *  @return The value of the named property
     *  @throw Exception::NotFoundError if the named property is unknown
     */
    std::string PropertyManager::getPropertyValue( const std::string &name ) const
    {
      Property *p = getPointerToProperty(name);   // throws NotFoundError if property not in vector
      return p->value();
    }


    //-----------------------------------------------------------------------------------------------
    /** Get a property by name
     *  @param name :: The name of the property (case insensitive)
     *  @return A pointer to the named property
     *  @throw Exception::NotFoundError if the named property is unknown
     */
    Property* PropertyManager::getPointerToProperty( const std::string &name ) const
    {
      std::string ucName = name;
      std::transform(ucName.begin(), ucName.end(), ucName.begin(), toupper);
      PropertyMap::const_iterator it = m_properties.find(ucName);
      if (it != m_properties.end())
      {
        return it->second;
      }
      throw Exception::NotFoundError("Unknown property", name);
    }

    //-----------------------------------------------------------------------------------------------
    /** Get a property by name
     *  @param name :: The name of the property (case insensitive)
     *  @return A pointer to the named property; NULL if not found
     */
    Property* PropertyManager::getPointerToPropertyOrNull( const std::string &name ) const
    {
      std::string ucName = name;
      std::transform(ucName.begin(), ucName.end(), ucName.begin(), toupper);
      PropertyMap::const_iterator it = m_properties.find(ucName);
      if (it != m_properties.end())
      {
        return it->second;
      }
      return NULL;
    }

    //-----------------------------------------------------------------------------------------------
    /** Get a property by an index
     *  @param index :: The name of the property (case insensitive)
     *  @return A pointer to the named property
     *  @throw std::runtime_error if the property index is too high
     */
    Property* PropertyManager::getPointerToPropertyOrdinal( const int& index) const
    {

      if ( index < static_cast<int>(m_orderedProperties.size()) )
      {
        return m_orderedProperties[index];
      }
      throw std::runtime_error("Property index too high");
    }

    //-----------------------------------------------------------------------------------------------
    /** Get the list of managed properties.
     *  The properties will be stored in the order that they were declared.
     *  @return A vector holding pointers to the list of properties
     */
    const std::vector< Property* >& PropertyManager::getProperties() const
    {
      return m_orderedProperties;
    }

    //-----------------------------------------------------------------------------------------------
    /** Get the value of a property. Allows you to assign directly to a variable of the property's type
     *  (if a supported type).
     *
     *  *** This method does NOT work for assigning to an existing std::string.
     *      In this case you have to use getPropertyValue() instead.
     *      Note that you can, though, construct a local string variable by writing,
     *      e.g. std::string s = getProperty("myProperty"). ***
     *
     *  @param name :: The name of the property
     *  @return The value of the property. Will be cast to the desired type (if a supported type).
     *  @throw std::runtime_error If an attempt is made to assign a property to a different type
     *  @throw Exception::NotFoundError If the property requested does not exist
     */
    PropertyManager::TypedValue PropertyManager::getProperty( const std::string &name ) const
    {
      return TypedValue(*this, name);
    }

    //-----------------------------------------------------------------------------------------------
    /** Removes the property from properties map.
     *  @param name ::  name of the property to be removed.
     */
    void PropertyManager::removeProperty(const std::string &name)
    {
      if(existsProperty(name))
      {
        //remove it
        Property* prop=getPointerToProperty(name);
        std::string key = name;
        std::transform(key.begin(), key.end(), key.begin(), toupper);
        m_properties.erase(key);
        std::vector<Property*>::iterator itr;
        itr=find(m_orderedProperties.begin(),m_orderedProperties.end(),prop);
        m_orderedProperties.erase(itr);
        delete prop;
      }
    }

  } // namespace Kernel
} // namespace Mantid
