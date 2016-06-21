#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/PropertyHistory.h"

namespace Mantid {
namespace Kernel {

/** Constructor  for Maskedproperty class
 * @param name :: name of the property
 * @param defaultvalue :: defaultvalue of the property
 * @param validator :: property validator
 * @param direction :: Whether this is a Direction::Input, Direction::Output or
 * Direction::InOut (Input & Output) property
 */
template <typename TYPE>
MaskedProperty<TYPE>::MaskedProperty(const std::string &name, TYPE defaultvalue,
                                     IValidator_sptr validator,
                                     const unsigned int direction)
    : Kernel::PropertyWithValue<TYPE>(name, defaultvalue, validator, direction),
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
MaskedProperty<TYPE>::MaskedProperty(const std::string &name,
                                     const TYPE &defaultvalue,
                                     const unsigned int direction)
    : Kernel::PropertyWithValue<TYPE>(name, defaultvalue, direction),
      m_maskedValue("") {
  this->setRemember(false);
}

/**
 * Virtual copy
 */
template <typename TYPE>
MaskedProperty<TYPE> *MaskedProperty<TYPE>::clone() const {
  return new MaskedProperty<TYPE>(*this);
}

/**
 * @return A new PropertyHistory object with the value masked out
 */
template <typename TYPE>
const Kernel::PropertyHistory MaskedProperty<TYPE>::createHistory() const {
  return Kernel::PropertyHistory(this->name(), this->getMaskedValue(),
                                 this->type(), this->isDefault(),
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
template MANTID_KERNEL_DLL class Mantid::Kernel::MaskedProperty<std::string>;
///@endcond TEMPLATE

} // namespace API
} // namespace Mantid
