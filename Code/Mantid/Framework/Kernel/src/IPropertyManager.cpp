//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
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
    int16_t IPropertyManager::getValue<int16_t>(const std::string &name) const
    {
        PropertyWithValue<int16_t> *prop = dynamic_cast<PropertyWithValue<int16_t>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected int16.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    uint16_t IPropertyManager::getValue<uint16_t>(const std::string &name) const
    {
        PropertyWithValue<uint16_t> *prop = dynamic_cast<PropertyWithValue<uint16_t>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected uint16.";
            throw std::runtime_error(message);
        }
    }


    template<> DLLExport
    int32_t IPropertyManager::getValue<int32_t>(const std::string &name) const
    {
        PropertyWithValue<int32_t> *prop = dynamic_cast<PropertyWithValue<int32_t>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected int32.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    uint32_t IPropertyManager::getValue<uint32_t>(const std::string &name) const
    {
        PropertyWithValue<uint32_t> *prop = dynamic_cast<PropertyWithValue<uint32_t>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected uint32.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    int64_t IPropertyManager::getValue<int64_t>(const std::string &name) const
    {
        PropertyWithValue<int64_t> *prop = dynamic_cast<PropertyWithValue<int64_t>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected int64.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    uint64_t IPropertyManager::getValue<uint64_t>(const std::string &name) const
    {
        PropertyWithValue<uint64_t> *prop = dynamic_cast<PropertyWithValue<uint64_t>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected uint64.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    unsigned long IPropertyManager::getValue<unsigned long>(const std::string &name) const
    {
        PropertyWithValue<unsigned long> *prop = 
	  dynamic_cast<PropertyWithValue<unsigned long>*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected unsigned long";
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
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected bool.";
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
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected double.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<int16_t> IPropertyManager::getValue<std::vector<int16_t> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<int16_t> > *prop = dynamic_cast<PropertyWithValue<std::vector<int16_t> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<int16>.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<uint16_t> IPropertyManager::getValue<std::vector<uint16_t> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<uint16_t> > *prop = dynamic_cast<PropertyWithValue<std::vector<uint16_t> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<uint16>.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<int32_t> IPropertyManager::getValue<std::vector<int32_t> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<int32_t> > *prop = dynamic_cast<PropertyWithValue<std::vector<int32_t> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<int32>.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<uint32_t> IPropertyManager::getValue<std::vector<uint32_t> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<uint32_t> > *prop = dynamic_cast<PropertyWithValue<std::vector<uint32_t> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<uint32>.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<int64_t> IPropertyManager::getValue<std::vector<int64_t> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<int64_t> > *prop = dynamic_cast<PropertyWithValue<std::vector<int64_t> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<int64>.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<uint64_t> IPropertyManager::getValue<std::vector<uint64_t> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<uint64_t> > *prop = dynamic_cast<PropertyWithValue<std::vector<uint64_t> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<uint64>.";
            throw std::runtime_error(message);
        }
    }

    template<> DLLExport
    std::vector<unsigned long>
    IPropertyManager::getValue<std::vector<unsigned long> >(const std::string &name) const
    {
        PropertyWithValue<std::vector<unsigned long> > *prop = 
	  dynamic_cast<PropertyWithValue<std::vector<unsigned long> >*>(getPointerToProperty(name));
        if (prop)
        {
            return *prop;
        }
        else
        {
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<unsigned long>";
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
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<double>.";
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
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected vector<string>.";
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
    IPropertyManager* IPropertyManager::setProperty<std::string>(const std::string &name, const std::string &value)
    {
        this->setPropertyValue(name, value);
        return this;
    }

    // Definitions for TypedValue cast operators
    // Have to come after getValue definitions above to keep MSVS2010 happy
    IPropertyManager::TypedValue::operator int16_t () { return pm.getValue<int16_t>(prop); }
    IPropertyManager::TypedValue::operator uint16_t () { return pm.getValue<uint16_t>(prop); }
    IPropertyManager::TypedValue::operator int32_t () { return pm.getValue<int32_t>(prop); }
    IPropertyManager::TypedValue::operator uint32_t () { return pm.getValue<uint32_t>(prop); }
    IPropertyManager::TypedValue::operator int64_t () { return pm.getValue<int64_t>(prop); }    
    IPropertyManager::TypedValue::operator uint64_t () { return pm.getValue<int64_t>(prop); }    
    // Intel 64-bit size_t   
    IPropertyManager::TypedValue::operator unsigned long () { return pm.getValue<unsigned long>(prop); }
    IPropertyManager::TypedValue::operator bool () { return pm.getValue<bool>(prop); }
    IPropertyManager::TypedValue::operator double () { return pm.getValue<double>(prop); }
    IPropertyManager::TypedValue::operator std::string () { return pm.getPropertyValue(prop); }
    IPropertyManager::TypedValue::operator Property* () { return pm.getPointerToProperty(prop); }
    /// @endcond

} // namespace Kernel
} // namespace Mantid
