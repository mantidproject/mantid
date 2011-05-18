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

/// Macro for declaring getValue more quickly.
#define IPROPERTYMANAGER_GETVALUE(type) \
    template<> DLLExport \
    type IPropertyManager::getValue<type>(const std::string &name) const \
    { \
        PropertyWithValue<type> *prop = dynamic_cast<PropertyWithValue<type>*>(getPointerToProperty(name)); \
        if (prop) \
        { \
            return *prop; \
        } \
        else \
        { \
            std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected type "#type; \
            throw std::runtime_error(message); \
        } \
    }

    IPROPERTYMANAGER_GETVALUE(int16_t);
    IPROPERTYMANAGER_GETVALUE(uint16_t);
    IPROPERTYMANAGER_GETVALUE(int32_t);
    IPROPERTYMANAGER_GETVALUE(uint32_t);
    IPROPERTYMANAGER_GETVALUE(int64_t);
    IPROPERTYMANAGER_GETVALUE(uint64_t);
    IPROPERTYMANAGER_GETVALUE(bool);
    IPROPERTYMANAGER_GETVALUE(double);
    IPROPERTYMANAGER_GETVALUE(std::vector<int16_t>);
    IPROPERTYMANAGER_GETVALUE(std::vector<uint16_t>);
    IPROPERTYMANAGER_GETVALUE(std::vector<int32_t>);
    IPROPERTYMANAGER_GETVALUE(std::vector<uint32_t>);
    IPROPERTYMANAGER_GETVALUE(std::vector<int64_t>);
    IPROPERTYMANAGER_GETVALUE(std::vector<uint64_t>);
    IPROPERTYMANAGER_GETVALUE(std::vector<double>);
    IPROPERTYMANAGER_GETVALUE(std::vector<std::string>);


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
    IPropertyManager::TypedValue::operator bool () { return pm.getValue<bool>(prop); }
    IPropertyManager::TypedValue::operator double () { return pm.getValue<double>(prop); }
    IPropertyManager::TypedValue::operator std::string () { return pm.getPropertyValue(prop); }
    IPropertyManager::TypedValue::operator Property* () { return pm.getPointerToProperty(prop); }
    /// @endcond

  #ifdef __INTEL_COMPILER

    IPROPERTYMANAGER_GETVALUE(unsigned long);
    IPROPERTYMANAGER_GETVALUE(std::vector<unsigned long>);

    // Intel 64-bit size_t   
    IPropertyManager::TypedValue::operator unsigned long () { return pm.getValue<unsigned long>(prop); }
  #endif


} // namespace Kernel
} // namespace Mantid
