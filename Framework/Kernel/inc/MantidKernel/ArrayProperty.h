// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/Property.h"
#include "PropertyWithValue.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** Support for a property that holds an array of values.
    Implemented as a PropertyWithValue that holds a vector of the desired type.
    This class is really a convenience class to aid in the declaration of the
    property - there's no problem directly using a PropertyWithValue of vector
   type.

    @author Russell Taylor, Tessella Support Services plc
    @date 27/02/2008
 */
template <typename T> class DLLExport ArrayProperty : public PropertyWithValue<std::vector<T>> {
public:
  ArrayProperty(const std::string &name, std::vector<T> vec,
                const IValidator_sptr &validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, const IValidator_sptr &validator,
                const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, const unsigned int direction = Direction::Input);
  ArrayProperty(const std::string &name, const std::string &values,
                const IValidator_sptr &validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input);

  ArrayProperty(const ArrayProperty &);

  ArrayProperty<T> *clone() const override;

  // Unhide the base class assignment operator
  using PropertyWithValue<std::vector<T>>::operator=;

  // May want to add specialisation to the class later, e.g. setting just one
  // element of the vector

private:
  // This method is a workaround for the C4661 compiler warning in visual
  // studio. This allows the template declaration and definition to be separated
  // in different files. See stack overflow article for a more detailed
  // explanation:
  // https://stackoverflow.com/questions/44160467/warning-c4661no-suitable-definition-provided-for-explicit-template-instantiatio
  // https://stackoverflow.com/questions/33517902/how-to-export-a-class-derived-from-an-explicitly-instantiated-template-in-visual
  void visualStudioC4661Workaround();
};

template <> MANTID_KERNEL_DLL void ArrayProperty<int>::visualStudioC4661Workaround();

// 'extern template' declarations matching the explicit instantiations in ArrayProperty.cpp.
// Without these, every translation unit that declares a vector-valued property (e.g. via
// declareProperty<std::vector<T>>) implicitly re-instantiates ArrayProperty<T> locally -- and
// this header has a very wide fan-out across the codebase.
#ifndef ARRAYPROPERTY_PROVIDES_EXPLICIT_INSTANTIATIONS
// int32_t already gets an attributed member specialization (visualStudioC4661Workaround) just
// above, so the DLL macro is left off here too, to avoid the same "attributes ignored after
// type is already defined" warning ArrayProperty.cpp works around for its own instantiation.
extern template class ArrayProperty<int32_t>;
extern template class MANTID_KERNEL_DLL ArrayProperty<uint32_t>;
extern template class MANTID_KERNEL_DLL ArrayProperty<int64_t>;
extern template class MANTID_KERNEL_DLL ArrayProperty<uint64_t>;
extern template class MANTID_KERNEL_DLL ArrayProperty<bool>;
extern template class MANTID_KERNEL_DLL ArrayProperty<float>;
extern template class MANTID_KERNEL_DLL ArrayProperty<double>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::string>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<int32_t>>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<uint32_t>>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<int64_t>>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<uint64_t>>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<float>>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<double>>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<std::string>>;
#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
extern template class MANTID_KERNEL_DLL ArrayProperty<long>;
extern template class MANTID_KERNEL_DLL ArrayProperty<unsigned long>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<long>>;
extern template class MANTID_KERNEL_DLL ArrayProperty<std::vector<unsigned long>>;
#endif
#endif // ARRAYPROPERTY_PROVIDES_EXPLICIT_INSTANTIATIONS

} // namespace Kernel
} // namespace Mantid
