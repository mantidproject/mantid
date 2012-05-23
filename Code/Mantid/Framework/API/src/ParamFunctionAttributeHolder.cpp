//
// Includes
//
#include "MantidAPI/ParamFunctionAttributeHolder.h"

namespace Mantid
{
  namespace API
  {

    /**
     *
     * Default constructor.
     */
    ParamFunctionAttributeHolder::ParamFunctionAttributeHolder()
      : ParamFunction()
    {
    }

    /**
     * Returns the number of attributes associated with the function
     */
    size_t ParamFunctionAttributeHolder::nAttributes() const
    {
      return m_attrs.size();
    }

    /// Check if attribute named exists
    bool ParamFunctionAttributeHolder::hasAttribute(const std::string& name)const
    {
      return m_attrs.find(name) != m_attrs.end();
    }

    /// Returns a list of attribute names
    std::vector<std::string> ParamFunctionAttributeHolder::getAttributeNames() const
    {
      std::vector<std::string> names(nAttributes(),"");
      size_t index(0);
      for(auto iter = m_attrs.begin(); iter != m_attrs.end(); ++iter)
      {
        names[index] = iter->first;
        ++index;
      }
      return names;
    }

   /**
    * Return a value of attribute attName
    * @param name :: Returns the named attribute
    */
    API::IFunction::Attribute ParamFunctionAttributeHolder::getAttribute(const std::string& name) const
    {
      if(hasAttribute(name))
      {
        return m_attrs.at(name);
      }
      else
      {
        throw std::invalid_argument("ParamFunctionAttributeHolder::getAttribute - Unknown attribute '" + name + "'");
      }
    }

    /**
     *  Set a value to a named attribute. Can be overridden in the inheriting class, the default
     *  just stores the value
     *  @param name :: The name of the attribute
     *  @param value :: The value of the attribute
     */
    void ParamFunctionAttributeHolder::setAttribute(const std::string& name,
                                                    const API::IFunction::Attribute & value)
    {
      storeAttributeValue(name, value);
    }

    /**
     * Declares a single attribute
     * @param name :: The name of the attribute
     * @param defaultValue :: A default value
     */
    void ParamFunctionAttributeHolder::declareAttribute(const std::string & name,
                                                        const API::IFunction::Attribute & defaultValue)
    {
      m_attrs.insert(std::make_pair(name, defaultValue));
    }


    /// Initialize the function holder. Calls declareAttributes & declareParameters
    void ParamFunctionAttributeHolder::init()
    {
      declareAttributes();
      declareParameters();
    }

    /**
     *  Set a value to a named attribute
     *  @param name :: The name of the attribute
     *  @param value :: The value of the attribute
     */
    void ParamFunctionAttributeHolder::storeAttributeValue(const std::string& name, const API::IFunction::Attribute & value)
    {
      if(hasAttribute(name))
      {
        m_attrs[name] = value;
      }
      else
      {
        throw std::invalid_argument("ParamFunctionAttributeHolder::setAttribute - Unknown attribute '" + name + "'");
      }
    }

  }
}
