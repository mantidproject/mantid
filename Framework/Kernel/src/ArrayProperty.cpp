#include "MantidKernel/ArrayProperty.h"

// PropertyWithValue Definition
#include "MantidKernel/PropertyWithValue.tcc"
namespace Mantid {
namespace Kernel {
/// @cond

template class DLLExport ArrayProperty<int32_t>;
template class DLLExport ArrayProperty<uint32_t>;
template class DLLExport ArrayProperty<int64_t>;
template class DLLExport ArrayProperty<uint64_t>;
template class DLLExport ArrayProperty<float>;
template class DLLExport ArrayProperty<double>;
template class DLLExport ArrayProperty<std::string>;

template class DLLExport ArrayProperty<std::vector<int32_t>>;
template class DLLExport ArrayProperty<std::vector<uint32_t>>;
template class DLLExport ArrayProperty<std::vector<int64_t>>;
template class DLLExport ArrayProperty<std::vector<uint64_t>>;
template class DLLExport ArrayProperty<std::vector<float>>;
template class DLLExport ArrayProperty<std::vector<double>>;
template class DLLExport ArrayProperty<std::vector<std::string>>;

#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
template class DLLExport ArrayProperty<long>;
template class DLLExport ArrayProperty<unsigned long>;
template class DLLExport ArrayProperty<std::vector<long>>;
template class DLLExport ArrayProperty<std::vector<unsigned long>>;
#endif

/// @endcond
} // namespace Kernel
} // namespace Mantid
