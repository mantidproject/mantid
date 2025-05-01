// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Matrix.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyWithValue.hxx"
#include "MantidNexus/NeXusFile.hpp"

namespace Mantid::Kernel {

#define PROPERTYWITHVALUE_SAVEPROPERTY(type)                                                                           \
  template <> void PropertyWithValue<type>::saveProperty(::NeXus::File *file) {                                        \
    file->makeGroup(this->name(), "NXlog", true);                                                                      \
    file->writeData("value", m_value);                                                                                 \
    file->openData("value");                                                                                           \
    file->putAttr("units", this->units());                                                                             \
    file->closeData();                                                                                                 \
    file->closeGroup();                                                                                                \
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
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<int32_t>>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<std::string>>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::shared_ptr<PropertyManager>>;
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
// nexus does not support writeData for long type on mac, so we save as int64
template <> void PropertyWithValue<long>::saveProperty(::NeXus::File *file) {
  file->makeGroup(this->name(), "NXlog", true);
  file->writeData("value", static_cast<int64_t>(m_value));
  file->openData("value");
  file->putAttr("units", this->units());
  file->closeData();
  file->closeGroup();
}
template class MANTID_KERNEL_DLL PropertyWithValue<long>;
template class MANTID_KERNEL_DLL PropertyWithValue<unsigned long>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<long>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<unsigned long>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<long>>>;
#endif
#ifdef __linux__
template class MANTID_KERNEL_DLL PropertyWithValue<long long>;
template class MANTID_KERNEL_DLL PropertyWithValue<unsigned long long>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<long long>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<unsigned long long>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<long long>>>;
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

} // namespace Mantid::Kernel
