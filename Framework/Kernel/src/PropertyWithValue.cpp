// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MatrixProperty.h"
#include "MantidKernel/OptionalBool.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyWithValue.hxx"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidNexus/NexusFile.h"

namespace Mantid::Kernel {

/// @cond
// The headers declare these with MANTID_KERNEL_EXTERN_TEMPLATE, which already
// carries the export attribute, so gcc warns that the one here is ignored.
GNU_DIAG_OFF("attributes")
template class MANTID_KERNEL_DLL PropertyWithValue<float>;
template class MANTID_KERNEL_DLL PropertyWithValue<double>;
template class MANTID_KERNEL_DLL PropertyWithValue<int32_t>;
template class MANTID_KERNEL_DLL PropertyWithValue<uint32_t>;
template class MANTID_KERNEL_DLL PropertyWithValue<int64_t>;
template class MANTID_KERNEL_DLL PropertyWithValue<uint64_t>;
template class MANTID_KERNEL_DLL PropertyWithValue<uint16_t>;
template class MANTID_KERNEL_DLL PropertyWithValue<bool>;
template class MANTID_KERNEL_DLL PropertyWithValue<OptionalBool>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::string>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<float>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<double>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<int32_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint16_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint32_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<int64_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<uint64_t>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<bool>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<OptionalBool>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::string>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<int32_t>>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::vector<std::vector<std::string>>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<float>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<double>>;
template class MANTID_KERNEL_DLL PropertyWithValue<Matrix<int>>;
template class MANTID_KERNEL_DLL PropertyWithValue<std::shared_ptr<PropertyManager>>;
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
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
GNU_DIAG_ON("attributes")
/// @endcond

} // namespace Mantid::Kernel
