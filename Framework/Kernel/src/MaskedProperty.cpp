// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/PropertyHistory.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/PropertyWithValue.hxx"

namespace Mantid::Kernel {

/** Constructor  for Maskedproperty class
 * @param name :: name of the property
 * @param defaultvalue :: defaultvalue of the property
 * @param validator :: property validator
 * @param direction :: Whether this is a Direction::Input, Direction::Output or
 * Direction::InOut (Input & Output) property
 */
template <typename TYPE>
MaskedProperty<TYPE>::MaskedProperty(std::string name, TYPE defaultvalue, IValidator_sptr validator,
                                     const unsigned int direction)
    : Kernel::PropertyWithValue<TYPE>(std::move(name), std::move(defaultvalue), std::move(validator), direction),
      m_maskedValue("") {
  this->setRemember(false);
}

/** Constructor  for Maskedproperty class
 * @param name :: name of the property
 * @param defaultvalue :: defaultvalue of the property
 * @param direction :: Whether this is a Direction::Input, Direction::Output or
 * Direction::InOut (Input & Output) property
 */
template <typename TYPE>
MaskedProperty<TYPE>::MaskedProperty(std::string name, TYPE defaultvalue, const unsigned int direction)
    : Kernel::PropertyWithValue<TYPE>(std::move(name), std::move(defaultvalue), direction), m_maskedValue("") {
  this->setRemember(false);
}

/**
 * Virtual copy
 */
template <typename TYPE> MaskedProperty<TYPE> *MaskedProperty<TYPE>::clone() const {
  return new MaskedProperty<TYPE>(*this);
}

/**
 * @return A new PropertyHistory object with the value masked out
 */
template <typename TYPE> const Kernel::PropertyHistory MaskedProperty<TYPE>::createHistory() const {
  return Kernel::PropertyHistory(this->name(), this->getMaskedValue(), this->type(), this->isDefault(),
                                 Kernel::PropertyWithValue<TYPE>::direction());
}

/** This method returns the masked property value
 */
template <typename TYPE> TYPE MaskedProperty<TYPE>::getMaskedValue() const {
  doMasking();
  return m_maskedValue;
}

//---------------------------------------------------------------------------
// Private methods
//---------------------------------------------------------------------------

/**
 * This method creates a masked value for this property
 */
template <typename TYPE> void MaskedProperty<TYPE>::doMasking() const {
  TYPE value(this->value());
  m_maskedValue = std::string(value.size(), '*');
}

//---------------------------------------------------------------------------
// Template Instantiations
//---------------------------------------------------------------------------

///@cond TEMPLATE
template class MANTID_KERNEL_DLL Mantid::Kernel::MaskedProperty<std::string>;
///@endcond TEMPLATE

} // namespace Mantid::Kernel
