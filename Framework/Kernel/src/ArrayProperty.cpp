#include "MantidKernel/ArrayProperty.h"

// ArrayProperty Definition
#include "MantidKernel/ArrayProperty.tcc"
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

template DLLExport ArrayProperty<int>::ArrayProperty(
    const std::string &name, const std::vector<int> &vec,
    IValidator_sptr validator, const unsigned int direction);

template DLLExport
ArrayProperty<int>::ArrayProperty(const std::string &name,
                                  IValidator_sptr validator,
                                  const unsigned int direction);

template DLLExport
ArrayProperty<int>::ArrayProperty(const std::string &name,
                                  const unsigned int direction);

template DLLExport ArrayProperty<int>::ArrayProperty(
    const std::string &name, const std::string &values,
    IValidator_sptr validator, const unsigned int direction);

template DLLExport ArrayProperty<int> *ArrayProperty<int>::clone() const;

template DLLExport std::string ArrayProperty<int>::value() const;

template DLLExport std::string
ArrayProperty<int>::setValue(const std::string &value);
} // namespace Kernel
} // namespace Mantid
