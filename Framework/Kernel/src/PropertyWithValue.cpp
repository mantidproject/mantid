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
#define INSTANTIATE_WITH_EXPORT(Type)                                          \
  template class DLLExport PropertyWithValue<Type>;

// Explicit instantiations
INSTANTIATE_WITH_EXPORT(uint16_t)
INSTANTIATE_WITH_EXPORT(bool)
INSTANTIATE_WITH_EXPORT(OptionalBool)
/// @endcond

#define INSTANTIATE_WITH_EXPORT_VECTOR(Type)                                   \
  template class DLLExport PropertyWithValue<std::vector<Type>>;
INSTANTIATE_WITH_EXPORT_VECTOR(uint16_t)
INSTANTIATE_WITH_EXPORT_VECTOR(uint32_t)
INSTANTIATE_WITH_EXPORT_VECTOR(int64_t)
INSTANTIATE_WITH_EXPORT_VECTOR(uint64_t)
INSTANTIATE_WITH_EXPORT_VECTOR(bool)
INSTANTIATE_WITH_EXPORT_VECTOR(OptionalBool)
INSTANTIATE_WITH_EXPORT_VECTOR(std::string)

// The explicit template instantiations for some types does not have an export
// macro
// since this produces a warning on "gcc: warning: type attributes ignored after
// type is already define". We can remove the issue, by removing the visibility
// attribute
template class PropertyWithValue<double>;
template class PropertyWithValue<std::vector<double>>;

template class PropertyWithValue<int32_t>;
template class PropertyWithValue<std::vector<int32_t>>;

template class PropertyWithValue<uint32_t>;
template class PropertyWithValue<int64_t>;
template class PropertyWithValue<uint64_t>;
template class PropertyWithValue<std::string>;

} // namespace Kernel
} // namespace Mantid
