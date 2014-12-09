#ifndef MANTID_KERNEL_PROPERTYMANAGEROWNER_H_
#define MANTID_KERNEL_PROPERTYMANAGEROWNER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <map>

#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
namespace Kernel
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class PropertyManager;

/** @class PropertyManagerOwner PropertyManagerOwner.h Kernel/PropertyManagerOwner.h

 Implementation of IPropertyManager which allows sharing the same set of Properties
 between several instances.

 @author Roman Tolchenov, Tessella plc
 @date 03/03/2009

 Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_KERNEL_DLL PropertyManagerOwner: virtual public IPropertyManager
{
public:
    PropertyManagerOwner();
    PropertyManagerOwner(const PropertyManagerOwner&);
    PropertyManagerOwner& operator=(const PropertyManagerOwner&);
    virtual ~PropertyManagerOwner(){}

    // Function to declare properties (i.e. store them)
    void declareProperty( Property *p, const std::string &doc="" );

    using IPropertyManager::declareProperty;

    // Sets all the declared properties from
    void setProperties(const std::string &propertiesArray);
    void setPropertyValue(const std::string &name, const std::string &value);
    void setPropertyOrdinal(const int &index, const std::string &value);

    /// Make m_properties point to the same PropertyManager as po.
    void copyPropertiesFrom(const PropertyManagerOwner& po)
    {
        *this = po;
    }

    bool existsProperty(const std::string &name) const;
    bool validateProperties() const;
    size_t propertyCount() const;

    std::string getPropertyValue(const std::string &name) const;
    const std::vector< Property*>& getProperties() const;

    /// Get the value of a property
    TypedValue getProperty(const std::string &name) const;
    /// Return the property manager serialized as a string.
    virtual std::string asString(bool withDefaultValues = false, char separator=',') const;
 
    /// Removes the property from management
    void removeProperty(const std::string &name, const bool delproperty = true);
    /// Clears all properties under management
    void clear();  
    /// Override this method to perform a custom action right after a property was set.
    /// The argument is the property name. Default - do nothing.
    virtual void afterPropertySet(const std::string&);

    virtual void filterByTime(const DateAndTime &/*start*/, const DateAndTime &/*stop*/){throw(std::runtime_error("Not yet implmented"));}
    virtual void splitByTime(std::vector<SplittingInterval>& /*splitter*/, std::vector< PropertyManager * >/* outputs*/) const{throw(std::runtime_error("Not yet implmented"));}
    virtual void filterByProperty(const TimeSeriesProperty<bool> & /*filte*/){throw(std::runtime_error("Not yet implmented"));}


public:

    Property* getPointerToProperty(const std::string &name) const;
    Property* getPointerToPropertyOrdinal(const int &index) const;

private:

    /// Shared pointer to the 'real' property manager
    boost::shared_ptr<PropertyManager> m_properties;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYMANAGEROWNER_H_*/
