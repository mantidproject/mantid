#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Matrix.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

#include <NeXusFile.hpp>

namespace Mantid {
namespace Kernel {
// Forward declare
class PropertyManager;

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
template class MANTID_KERNEL_DLL PropertyWithValue<uint16_t>;
template class MANTID_KERNEL_DLL PropertyWithValue<bool>;
template class MANTID_KERNEL_DLL PropertyWithValue<OptionalBool>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<float>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint16_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint32_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<int64_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint64_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<bool>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<OptionalBool>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::string>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<float>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<double>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<int>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<std::vector<std::vector<int32_t>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<std::vector<std::vector<std::string>>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<IValidator>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<boost::shared_ptr<PropertyManager>>;
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
template class MANTID_KERNEL_DLL PropertyWithValue<long>;
template class MANTID_KERNEL_DLL PropertyWithValue<unsigned long>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<long>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<unsigned long>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<std::vector<std::vector<long>>>;
#endif
#ifdef __linux__
template class MANTID_KERNEL_DLL PropertyWithValue<long long>;
template class MANTID_KERNEL_DLL PropertyWithValue<unsigned long long>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<long long>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<std::vector<unsigned long long>>;
template class MANTID_KERNEL_DLL
    PropertyWithValue<std::vector<std::vector<long long>>>;
#endif
/// @endcond

// The explicit template instantiations for some types does not have an export
// macro
// since this produces a warning on "gcc: warning: type attributes ignored after
// type is already define". We can remove the issue, by removing the visibility
// attribute
template class PropertyWithValue<float>;
template class PropertyWithValue<double>;
template class PropertyWithValue<int32_t>;
template class PropertyWithValue<uint32_t>;
template class PropertyWithValue<int64_t>;
template class PropertyWithValue<uint64_t>;

template class PropertyWithValue<std::vector<double>>;
template class PropertyWithValue<std::vector<int32_t>>;

template class PropertyWithValue<std::string>;

} // namespace Kernel
} // namespace Mantid
