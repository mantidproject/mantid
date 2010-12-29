//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Exception.h"
#include <algorithm>

namespace Mantid
{
namespace Kernel
{


    /// @cond

    // getValue template specialisations (there is no generic implementation)
    // Note that other implementations can be found in Workspace.cpp & Workspace1D/2D.cpp (to satisfy
    // package dependency rules).

    template<> DLLExport
    int IPropertyManager::getValue<int>(const std::string &name) const
    {
        PropertyWithValue<int> *prop = dynamic_cast<PropertyWithValue<int>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    long long IPropertyManager::getValue<long long>(const std::string &name) const
    {
        PropertyWithValue<long long> *prop = dynamic_cast<PropertyWithValue<long long>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type (long long)";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    bool IPropertyManager::getValue<bool>(const std::string &name) const
    {
        PropertyWithValue<bool> *prop = dynamic_cast<PropertyWithValue<bool>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    double IPropertyManager::getValue<double>(const std::string &name) const
    {
        PropertyWithValue<double> *prop = dynamic_cast<PropertyWithValue<double>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<int> IPropertyManager::getValue<std::vector<int> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<int> > *prop = dynamic_cast<PropertyWithValue<std::vector<int> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<double> IPropertyManager::getValue<std::vector<double> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<double> > *prop = dynamic_cast<PropertyWithValue<std::vector<double> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<std::string> IPropertyManager::getValue<std::vector<std::string> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<std::string> > *prop = dynamic_cast<PropertyWithValue<std::vector<std::string> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type";
            throw std::runtime_error(message);
        }
    }
	 template<> DLLExport
    std::vector<long long> IPropertyManager::getValue<std::vector<long long> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<long long> > *prop = dynamic_cast<PropertyWithValue<std::vector<long long> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type";
            throw std::runtime_error(message);
        }
    }

    template <> DLLExport
    const char* IPropertyManager::getValue<const char*>(const std::string &name) const
    {
        return getPropertyValue(name).c_str();
    }

    // This template implementation has been left in because although you can't assign to an existing string
    // via the getProperty() method, you can construct a local variable by saying,
    // e.g.: std::string s = getProperty("myProperty")
    template <> DLLExport
    std::string IPropertyManager::getValue<std::string>(const std::string &name) const
    {
        return getPropertyValue(name);
    }

    template <> DLLExport
    Property* IPropertyManager::getValue<Property*>(const std::string &name) const
    {
        return getPointerToProperty(name);
    }

    // If a string is given in the argument, we can be more flexible
    template <>
    void IPropertyManager::setProperty<std::string>(const std::string &name, const std::string &value)
    {
        this->setPropertyValue(name, value);
    }
	

    // Definitions for TypedValue cast operators
    // Have to come after getValue definitions above to keep MSVS2010 happy
    IPropertyManager::TypedValue::operator int () { return pm.getValue<int>(prop); }
    IPropertyManager::TypedValue::operator long long () { return pm.getValue<long long>(prop); }
    IPropertyManager::TypedValue::operator bool () { return pm.getValue<bool>(prop); }
    IPropertyManager::TypedValue::operator double () { return pm.getValue<double>(prop); }
    IPropertyManager::TypedValue::operator std::string () { return pm.getPropertyValue(prop); }
    IPropertyManager::TypedValue::operator Property* () { return pm.getPointerToProperty(prop); }

    /// @endcond

} // namespace Kernel
} // namespace Mantid
