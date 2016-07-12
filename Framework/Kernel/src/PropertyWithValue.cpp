#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {

namespace Kernel {

#define PROPERTYWITHVALUE_SAVEPROPERTY(type)                                   \
  template <>                                                                  \
  void PropertyWithValue<type>::saveProperty(::NeXus::File *file) {            \
    file->makeGroup(this->name(), "NXlog", 1);                                 \
    file->writeData("value", m_value);                                         \
    file->closeGroup();                                                        \
  }

PROPERTYWITHVALUE_SAVEPROPERTY(float)
PROPERTYWITHVALUE_SAVEPROPERTY(double)
PROPERTYWITHVALUE_SAVEPROPERTY(int32_t)
PROPERTYWITHVALUE_SAVEPROPERTY(uint32_t)
PROPERTYWITHVALUE_SAVEPROPERTY(int64_t)
PROPERTYWITHVALUE_SAVEPROPERTY(uint64_t)
PROPERTYWITHVALUE_SAVEPROPERTY(std::string)
PROPERTYWITHVALUE_SAVEPROPERTY(std::vector<double>)
PROPERTYWITHVALUE_SAVEPROPERTY(std::vector<int32_t>)

/// @cond
#define INSTANTIATE(Type)                                                      \
  template DLLExport class PropertyWithValue<Type>;                            \
  template DLLExport class PropertyWithValue<std::vector<Type>>;

// Explicit instantiations
INSTANTIATE(int32_t)
INSTANTIATE(int64_t)
INSTANTIATE(uint16_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(bool)
INSTANTIATE(OptionalBool)
INSTANTIATE(double)
INSTANTIATE(std::string)
/// @endcond

} // namespace Kernel
} // namespace Mantid
